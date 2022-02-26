#ifndef NET_UTILS_H
#define NET_UTILS_H

#include <stdint.h>
#include <endian.h>

inline bool isLitleEndian() {
	return BYTE_ORDER == LITTLE_ENDIAN;
}

inline uint64_t bswap64(uint64_t data) {
	return ((data >> 56)
			| ((data & 0x00ff000000000000) >> 40)
			| ((data & 0x0000ff0000000000) >> 24)
			| ((data & 0x000000ff00000000) >> 8)
			| ((data & 0x00000000ff000000) << 8)
			| ((data & 0x0000000000ff0000) << 24)
			| ((data & 0x000000000000ff00) << 40)
			| (data << 56));
}
inline uint32_t bswap32(uint32_t data) {
	return ((data >> 24)
			| ((data & 0x00ff0000) >> 8)
			| ((data & 0x0000ff00) << 8)
			| ((data << 24)));
}
inline uint16_t bswap16(uint16_t data) {
	return ((data >> 8) | (data << 8));
}

inline uint16_t hton16(uint16_t h) {
	if (isLitleEndian())
		return bswap16(h);
	return h;
}
inline uint32_t hton32(uint32_t h) {
	if (isLitleEndian())
		return bswap32(h);
	return h;
}
inline uint64_t hton64(uint64_t h) {
	if (isLitleEndian())
		return bswap64(h);
	return h;
}

inline uint16_t ntoh16(uint16_t n) {
	return hton16(n);
}
inline uint32_t ntoh32(uint32_t n) {
	return hton32(n);
}
inline uint64_t ntoh64(uint64_t n) {
	return hton64(n);
}

#endif
