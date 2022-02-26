#include "WebsocketContext.h"

namespace muduo
{
namespace net
{
namespace websocket
{

bool WebsocketContext::decode(Buffer* buff)
{
	while (state_ != READY && state_ != FATAL)
	{
		// 处理最开始2字节数据
		if (state_ == BASIC_HEADER)
		{
			size_t leftLen = buff->readableBytes();
			if (leftLen < frame::BASIC_HEADER_LENGTH) break;

			reqData_.setBasicHeader(*buff->peek(), *(buff->peek() + 1));

			// 对于没有能力处理的opcode，直接断开该连接
			if (!frame::opcode::canHandleOpcode(reqData_.opcode()))
			{
				state_ = FATAL;
				break;
			}

			buff->retrieve(frame::BASIC_HEADER_LENGTH);
			reqData_.setPayloadLen(reqData_.payloadSize());
			bool masked = reqData_.mask();
			if (reqData_.payloadSize() == 0)
			{
				state_ = (masked ? GET_MASK : READY);
			}
			else
			{
				if (reqData_.payloadSize() <= frame::PAYLOAD_SIZE_BASIC)
				{
					state_ = (masked ? GET_MASK : APPLICATION);
				}
				else
				{
					if (reqData_.payloadSize() == frame::PAYLOAD_SIZE_16BIT)
					{
						reqData_.setExtendedPayloadLength(frame::EXTENDED_LENGTH_16BIT);
					}
					else
					{
						reqData_.setExtendedPayloadLength(frame::EXTENDED_LENGTH_64BIT);
					}
					state_ = EXTENDED_HEADER_LENGTH;
				}
			}
		}
		else if (state_ == EXTENDED_HEADER_LENGTH)
		{
			size_t leftLen = buff->readableBytes();
			if (leftLen < reqData_.extendedPayloadLength()) break;
			
			if (reqData_.extendedPayloadLength() == frame::EXTENDED_LENGTH_16BIT)
			{
				reqData_.setPayloadLen(
						ntoh16(*(reinterpret_cast<const uint16_t*>(buff->peek()))));
			}
			else
			{
				uint64_t len = ntoh64(*(reinterpret_cast<const uint64_t*>(buff->peek())));
				// 判断长度是否超出最大限制
				if (len > frame::MAX_PAYLOAD_LENGTH)
				{
					state_ = FATAL;
					break;
				}
				reqData_.setPayloadLen(len);
			}
			
			buff->retrieve(reqData_.extendedPayloadLength());
			state_ = (reqData_.mask() ? GET_MASK : APPLICATION);
		}
		else if (state_ == GET_MASK)
		{
			size_t leftLen = buff->readableBytes();
			if (leftLen < frame::MASK_BYTES) break;

			reqData_.setMaskKey(reinterpret_cast<const uint8_t*>(buff->peek()));

			buff->retrieve(frame::MASK_BYTES);
			state_ = (reqData_.payloadSize() == 0 ? READY : APPLICATION);
		}
		else if (state_ == APPLICATION)
		{
			size_t leftLen = buff->readableBytes();
			if (leftLen < reqData_.payloadLen()) break;

			reqData_.handlePayload(buff->peek(), reqData_.payloadLen());

			buff->retrieve(reqData_.payloadLen());
			state_ = READY;
		}
	}

	return state_ != FATAL;
}

void WebsocketContext::encode(Buffer* buff, const WebsocketData& data)
{
	buff->append(data.basicHeaderByte1(), 1);
	buff->append(data.basicHeaderByte2(), 1);

	buff->ensureWritableBytes(data.extendedPayloadLength());
	data.getExtendedPayloadLengthInfo(buff->beginWrite());
	buff->hasWritten(data.extendedPayloadLength());

	if (data.mask())
	{
		buff->ensureWritableBytes(frame::MASK_BYTES);
		data.getMaskKey(buff->beginWrite());	
		buff->hasWritten(frame::MASK_BYTES);
	}

	if (data.payloadSize() > 0)
	{
		buff->ensureWritableBytes(data.payloadLen());
		data.getPayload(buff->beginWrite());
		buff->hasWritten(data.payloadLen());
	}
}

void WebsocketContext::getFrame(Buffer* buff, uint8_t opcode, bool mask, Buffer* payload)
{
	WebsocketData data;
	data.setFin();
	data.setOpcode(opcode);
	
	if (mask)
	{
		data.setMask();
		uint32_t maskKey = rand();
		data.setMaskKey(maskKey);
	}

	if (payload && payload->readableBytes() > 0)
		data.setPayload(payload);

	encode(buff, data);
	//printRawData(buff);
}

void WebsocketContext::getPingFrame(Buffer* buff)
{
	getFrame(buff, frame::opcode::PING);
}

void WebsocketContext::getPongFrame(Buffer* buff)
{
	getFrame(buff, frame::opcode::PONG, true);
}

} // namespace websocket
} // namespace net
} // muduo
