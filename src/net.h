#ifndef NET_H
#define NET_H

#include <arpa/inet.h>
#include <endian.h>

namespace net {

template<class T> T ntoh(T) = delete;
uint64_t ntoh(uint64_t v);
uint32_t ntoh(uint32_t v);
uint16_t ntoh(uint16_t v);
uint8_t ntoh(uint8_t v);
int8_t ntoh(int8_t v);
char ntoh(char v);

template<class T> T hton(T) = delete;
uint64_t hton(uint64_t v);
uint32_t hton(uint32_t v);
uint16_t hton(uint16_t v);
uint8_t hton(uint8_t v);
int8_t hton(int8_t v);
char hton(char v);

} // namespace net

#endif

