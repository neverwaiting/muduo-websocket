#ifndef MUDUO_NET_WEBSOCKET_CONTEXT_H
#define MUDUO_NET_WEBSOCKET_CONTEXT_H

#include <inttypes.h>
#include <random>
#include <muduo/net/Buffer.h>
#include "netEndian.h"
#include "base64.h"
#include "sha1.h"
#include <iostream>

namespace muduo
{
namespace net
{
namespace websocket
{
namespace frame
{
	static const uint8_t FIN = 0x80;
	static const uint8_t RSV1 = 0x40;
	static const uint8_t RSV2 = 0x20;
	static const uint8_t RSV3 = 0x10;
	static const uint8_t OPCODE = 0x0F;
	static const uint8_t MASK = 0x80;
	static const uint8_t PAYLOAD_LEN = 0x7F;

	namespace opcode
	{

	// 4 位opcode
	// continuation, text, binary, 3-7 non-control frames, connection close, ping, pong, reserved frames
	static const uint8_t CONTINUATION = 0x0;
	static const uint8_t TEXT = 0x1;
	static const uint8_t BINARY = 0x2;
	static const uint8_t CLOSE = 0x8;
	static const uint8_t PING = 0x9;
	static const uint8_t PONG = 0xA;

	// 扩展opcode项
	inline bool reserved(uint8_t opcode)
	{
		return (opcode >= 0x3 && opcode <= 0x7) || (opcode >= 0xB && opcode <= 0xF);
	}
	// 是否为控制帧
	inline bool control(uint8_t opcode)
	{
		return opcode >= CLOSE;
	}

	// 能够处理的opcode, 暂时不考虑预留项和消息分片
	inline bool canHandleOpcode(uint8_t opcode)
	{
		return opcode != CONTINUATION && !reserved(opcode);
	}

	} // namespace opcode

	// Minimum length of a WebSocket frame header.
	static const unsigned int BASIC_HEADER_LENGTH = 2;

	static const unsigned int EXTENDED_LENGTH_0BIT = 0;
	static const unsigned int EXTENDED_LENGTH_16BIT = 2;
	static const unsigned int EXTENDED_LENGTH_64BIT = 8;

	static const uint8_t PAYLOAD_SIZE_BASIC = 0x7D;// 125
	static const uint8_t PAYLOAD_SIZE_16BIT = 0x7E;// 126
	static const uint8_t PAYLOAD_SIZE_64BIT = 0x7F;// 127

	static const unsigned int MAX_PAYLOAD_LENGTH = (2 << 21); // 2M字节

	static const uint8_t MASK_BYTES = 4;

} // namespace frame

static char const handshakeGuid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

inline uint32_t uint32RandomNumber()
{
	static std::default_random_engine e;
	static std::uniform_int_distribution<uint32_t> u;
	return u(e);
}

union uint16_converter
{
	uint16_t i;
	uint8_t c[2];
};
union uint32_converter
{
	uint32_t i;
	uint8_t c[4];
};
union uint64_converter
{
	uint64_t i;
	uint8_t c[8];
};

class WebsocketData
{
public:
	void setBasicHeader(uint8_t byte1, uint8_t byte2)
	{
		basic_header_byte1_ = byte1;
		basic_header_byte2_ = byte2;
	}
	const char* basicHeaderByte1() const
	{
		return reinterpret_cast<const char*>(&basic_header_byte1_);
	}
	const char* basicHeaderByte2() const
	{
		return reinterpret_cast<const char*>(&basic_header_byte2_);
	}

	void setOpcode(uint8_t opcode)
	{
		uint8_t tmp = basic_header_byte1_ & frame::opcode::CONTINUATION;
		basic_header_byte1_ |= tmp;
		basic_header_byte1_ |= opcode;
	}
	uint8_t opcode() const
	{
		return (basic_header_byte1_ & frame::OPCODE);
	}

	void setFin()
	{
		basic_header_byte1_ |= frame::FIN;
	}
	bool fin() const
	{
		return (basic_header_byte1_ & frame::FIN);
	}

	void setMask()
	{
		basic_header_byte2_ |= frame::MASK;
	}
	bool mask() const
	{
		return (basic_header_byte2_ & frame::MASK);
	}

	// 记录payload长度信息所占的字节数 2字节或者8字节
	void setExtendedPayloadLength(uint8_t len)
	{
		extendedPayloadLength_ = len;
	}
	uint8_t extendedPayloadLength() const
	{
		return extendedPayloadLength_;
	}
	void getExtendedPayloadLengthInfo(char* out) const
	{
		if (extendedPayloadLength_ == frame::EXTENDED_LENGTH_16BIT)
		{
			uint16_converter len;
			len.i = hton16(static_cast<uint16_t>(payloadLen_));
			std::copy(len.c, len.c + 2, out);
		}
		else if (extendedPayloadLength_ == frame::EXTENDED_LENGTH_64BIT)
		{
			uint64_converter len;
			len.i = hton64(static_cast<uint64_t>(payloadLen_));
			std::copy(len.c, len.c + 8, out);
		}
	}

	// 7 位payload len 所表示的信息
	uint8_t payloadSize() const
	{
		return (basic_header_byte2_ & frame::PAYLOAD_LEN);
	}
	void setPayloadSize(uint8_t size)
	{
		basic_header_byte2_ |= size;
	}

	// 真实的负载长度
	void setPayloadLen(unsigned int len)
	{
		payloadLen_ = len;
	}
	unsigned payloadLen() const
	{
		return payloadLen_;
	}

	void setMaskKey(const uint8_t* begin)
	{
		setMask();
		std::copy(begin, begin + frame::MASK_BYTES, maskKey_.c);
	}
	void setMaskKey(uint32_t key)
	{
		setMask();
		maskKey_.i = key;
	}
	void getMaskKey(char* out) const
	{
		std::copy(maskKey_.c, maskKey_.c + 4, out);
	}

	void handlePayload(const char* begin, size_t len)
	{
		payload_.ensureWritableBytes(len);
		
		if (mask())
		{
			for (size_t i = 0; i < len; ++i)
			{
				payload_.beginWrite()[i] = (*(begin + i)) ^ maskKey_.c[i % 4];
			}
		}
		else
		{
			for (size_t i = 0; i < len; ++i)
			{
				payload_.beginWrite()[i] = *(begin + i);
			}
		}
		payload_.hasWritten(len);
	}

	Buffer& payload() 
	{
		return payload_;
	}
	void getPayload(char* out) const
	{
		if (mask())
		{
			for (size_t i = 0; i < payloadLen_; ++i)
			{
				out[i] = (*(payload_.peek() + i)) ^ maskKey_.c[i % 4];
			}
		}
		else
		{
			for (size_t i = 0; i < payloadLen_; ++i)
			{
				out[i] = *(payload_.peek() + i);
			}
		}
	}

	void setPayload(const char* data, size_t len)
	{
		assert(len <= frame::MAX_PAYLOAD_LENGTH);
		payloadLen_ = static_cast<unsigned>(len);
		uint8_t size = 0;
		if (len > 0xFFFF)
		{
			extendedPayloadLength_ = frame::EXTENDED_LENGTH_64BIT;
			size = frame::PAYLOAD_SIZE_64BIT;
		}
		else if (len > frame::PAYLOAD_SIZE_BASIC)
		{
			extendedPayloadLength_ = frame::EXTENDED_LENGTH_16BIT;
			size = frame::PAYLOAD_SIZE_16BIT;
		}
		else
		{
			size = static_cast<uint8_t>(len);
		}
		setPayloadSize(size);
		payload_.append(data, len);
	}
	void setPayload(const std::string& data)
	{
		setPayload(&*data.begin(), data.size());
	}
	void setPayload(Buffer* buff)
	{
		setPayload(buff->peek(), buff->readableBytes());
	}

	void reset()
	{
		basic_header_byte1_ = basic_header_byte2_ = 0;
		extendedPayloadLength_ = frame::EXTENDED_LENGTH_0BIT;
		payloadLen_ = 0;
		payload_.retrieveAll();
	}

private:
	uint8_t basic_header_byte1_ = 0;
	uint8_t basic_header_byte2_ = 0;

	uint8_t extendedPayloadLength_ = frame::EXTENDED_LENGTH_0BIT;
	unsigned int payloadLen_ = 0;
	uint32_converter maskKey_;
	Buffer payload_;
};

// debug info
static void printRawData(Buffer* buff)
{
	for (size_t i = 0; i < buff->readableBytes(); ++i)
	{
		std::cout << std::hex << "0x" << (int)*((const uint8_t*)(buff->peek() + i)) << " ";
	}
	std::cout << std::endl;
}

class WebsocketContext
{
public:
	enum ParserState
	{
		BASIC_HEADER,
		EXTENDED_HEADER_LENGTH,
		GET_MASK,
		APPLICATION,
		READY,
		FATAL
	};

	bool decode(Buffer* buff);
	static void encode(Buffer* buff, const WebsocketData& data);

	static void getFrame(Buffer* buff, uint8_t opcode, bool mask = false, Buffer* payload = nullptr);
	static void getPingFrame(Buffer* buff);
	static void getPongFrame(Buffer* buff);

	bool ready()
	{
		return state_ == READY;
	}

	WebsocketData& requestData()
	{
		return reqData_;
	}

	void reset()
	{
		reqData_.reset();
		state_ = BASIC_HEADER;
	}

private:
	WebsocketData reqData_;
	ParserState state_ = BASIC_HEADER;
};

static std::string getSecWebsocketKey()
{
	uint32_converter conv;
	unsigned char raw_key[16];
	for (int i = 0; i < 4; ++i)
	{
		conv.i = uint32RandomNumber();
		std::copy(conv.c, conv.c + 4, &raw_key[i * 4]);
	}
	return base64_encode(raw_key, 16);
}

static std::string getSecWebsocketAccept(const std::string& key)
{
	std::string res(key);
	res.append(websocket::handshakeGuid);
	unsigned char sha1Key[20];
	websocket::sha1::calc(res.c_str(), res.size(), sha1Key);
	res = websocket::base64_encode(sha1Key, 20);
	return res;
}

static bool validateWebsocketAccept(const std::string& key, const std::string& accept)
{
	return getSecWebsocketAccept(key) == accept;
}

} // namespace websocket
} // namespace net
} // muduo

#endif
