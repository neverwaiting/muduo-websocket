#include "../WebsocketContext.h"
#include <iostream>

using namespace muduo::net;
using namespace muduo::net::websocket;

std::string getOpcodeName(uint8_t opcode)
{
	std::string res;
	if (opcode == frame::opcode::BINARY)
	{
		res = "BINARY";
	}
	else if (opcode == frame::opcode::TEXT)
	{
		res = "TEXT";
	}
	else if (opcode == frame::opcode::CLOSE)
	{
		res = "CLOSE";
	}
	else if (opcode == frame::opcode::CONTINUATION)
	{
		res = "CONTINUATION";
	}
	else if (opcode == frame::opcode::PING)
	{
		res = "PING";
	}
	else if (opcode == frame::opcode::PONG)
	{
		res = "PONG";
	}
	else
	{
		res = "Unknow";
	}
	return res;
}

int main()
{
	//Buffer buff;
	//uint8_t raw[7] = {0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f };
	//uint8_t raw[11] = {0x81, 0x85, 0x37, 0xfa, 0x21, 0x3d, 0x7f, 0x9f, 0x4d, 0x51, 0x58};
	//uint8_t raw[7] = {0x89, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f};
	//uint8_t raw[11] = {0x8a, 0x85, 0x37, 0xfa, 0x21, 0x3d, 0x7f, 0x9f, 0x4d, 0x51, 0x58};
	//buff.append(raw, 11);

	WebsocketContext context;
	
	Buffer outBuffer;
	WebsocketData resData;
	resData.setFin();
	resData.setOpcode(frame::opcode::PING);

	WebsocketContext::encode(&outBuffer, resData);
	std::cout << resData.payloadLen() << std::endl;
	std::cout << (int)resData.extendedPayloadLength() << std::endl;

	assert(context.decode(&outBuffer));

	WebsocketData& data = context.requestData();
	std::cout << "byte1: " << std::hex << "0x" << (int)*((const uint8_t*)data.basicHeaderByte1()) << std::endl;
	std::cout << "byte2: " << std::hex << "0x" << (int)*((const uint8_t*)data.basicHeaderByte1()) << std::endl;
	std::cout << "FIN: " << (data.fin() ? "true" : "false") << std::endl;
	std::cout << "mask: " << (data.mask() ? "true" : "false") << std::endl;
	std::cout << "opcode: " << getOpcodeName(data.opcode()) << std::endl;
	std::cout << "payload len: " << (int)data.payloadLen() << std::endl;
	std::cout << "payload extended len size: " << (int)data.extendedPayloadLength() << std::endl;
	std::cout << "payload: " << std::string(data.payload().peek(), data.payload().readableBytes()) << std::endl;
}
