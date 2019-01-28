
#ifndef UTILS_H
#define UTILS_H
#include <stdint.h>
struct bridge_ipinfo {
    uint32_t ip; //本地序
    uint32_t prefix; //子网掩码前缀 16 24等
};
struct bridge_ipinfo get_brip(const char *br);
void new_containerip(char *ipaddr, int len);

#endif

