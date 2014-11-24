#include "net.h"

namespace net {

uint64_t ntoh(uint64_t v) { return be64toh(v); }
uint32_t ntoh(uint32_t v) { return ntohl(v); }
uint16_t ntoh(uint16_t v) { return ntohs(v); }
uint8_t ntoh(uint8_t v) { return v; }
int8_t ntoh(int8_t v) { return v; }
char ntoh(char v) { return v; }

uint64_t hton(uint64_t v) { return htobe64(v); }
uint32_t hton(uint32_t v) { return htonl(v); }
uint16_t hton(uint16_t v) { return htons(v); }
uint8_t hton(uint8_t v) { return v; }
int8_t hton(int8_t v) { return v; }
char hton(char v) { return v; }

} // namespace net

