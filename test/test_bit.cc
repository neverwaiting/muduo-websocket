#include <iostream>
#include <inttypes.h>
#include <string.h>

struct frame
{
	uint8_t fin: 1;
	uint8_t rsv1: 1;
	uint8_t rsv2: 1;
	uint8_t rsv3: 1;
	uint8_t opcode: 4;

	uint8_t mask: 1;
	uint8_t playloadLen: 7;
};

union frameUnion
{
	frame f;
	uint8_t uint8f;
};

int main()
{
	frameUnion fu;
	fu.uint8f = 15;
	std::cout << (int)fu.f.fin << std::endl;
	std::cout << (int)fu.f.rsv1 << std::endl;
	std::cout << (int)fu.f.rsv2 << std::endl;
	std::cout << (int)fu.f.rsv3 << std::endl;
	std::cout << (int)fu.f.opcode << std::endl;
	std::cout << (int)fu.f.mask << std::endl;
	std::cout << (int)fu.f.playloadLen << std::endl;

	uint8_t fff = 15;
	frame fr;
	memcpy(&fr, &fff, 1);
	std::cout << std::endl;
	std::cout << (int)fr.fin << std::endl;
	std::cout << (int)fr.rsv1 << std::endl; std::cout << (int)fr.rsv2 << std::endl;
	std::cout << (int)fr.rsv3 << std::endl;
	std::cout << (int)fr.opcode << std::endl;
	std::cout << (int)fr.mask << std::endl;
	std::cout << (int)fr.playloadLen << std::endl;

	unsigned int a = 0x12345678;
	std::cout << std::hex << "0x" << a << std::endl;
	std::cout << std::hex << "0x" << (*(int*)(char*)&a) << std::endl;
	std::cout << std::hex << "0x" << (*(int*)((char*)&a + 1)) << std::endl;
	std::cout << std::hex << "0x" << (*(int*)((char*)&a + 2)) << std::endl;
	std::cout << std::hex << "0x" << (*(int*)((char*)&a + 3)) << std::endl;
}
