//
// Created by lwj on 2019/10/16.
//
#include "common.h"
#include <iostream>
#include <cstring>

void write_u32(char * p,uint32_t l)
{
    *(unsigned char*)(p + 3) = (unsigned char)((l >>  0) & 0xff);
    *(unsigned char*)(p + 2) = (unsigned char)((l >>  8) & 0xff);
    *(unsigned char*)(p + 1) = (unsigned char)((l >> 16) & 0xff);
    *(unsigned char*)(p + 0) = (unsigned char)((l >> 24) & 0xff);
}
void write_u32_r(char * p,uint32_t l)
{
    *(unsigned char*)(p + 0) = (unsigned char)((l >>  0) & 0xff);
    *(unsigned char*)(p + 1) = (unsigned char)((l >>  8) & 0xff);
    *(unsigned char*)(p + 2) = (unsigned char)((l >> 16) & 0xff);
    *(unsigned char*)(p + 3) = (unsigned char)((l >> 24) & 0xff);
}
uint32_t read_u32(const char * p)
{
    uint32_t res;
    res = *(const unsigned char*)(p + 0);
    res = *(const unsigned char*)(p + 1) + (res << 8);
    res = *(const unsigned char*)(p + 2) + (res << 8);
    res = *(const unsigned char*)(p + 3) + (res << 8);
    return res;
}
uint32_t read_u32_r(const char * p)
{
    uint32_t res;
    res = *(const unsigned char*)(p + 3);
    res = *(const unsigned char*)(p + 2) + (res << 8);
    res = *(const unsigned char*)(p + 1) + (res << 8);
    res = *(const unsigned char*)(p + 0) + (res << 8);
    return res;
}
void write_u16(char * p,uint16_t l)
{
    *(unsigned char*)(p + 1) = (unsigned char)((l >> 0) & 0xff);
    *(unsigned char*)(p + 0) = (unsigned char)((l >> 8) & 0xff);
}
uint16_t read_u16(const char * p)
{
    uint16_t res;
    res = *(const unsigned char*)(p + 0);
    res = *(const unsigned char*)(p + 1) + (res << 8);
    return res;
}

int64_t getnowtime_ms(){
    struct timeval tv = {0};
    gettimeofday(&tv, nullptr);
    return 1000 * tv.tv_sec + tv.tv_usec / 1000;
}

void print_char_array_in_byte(const char *buf) {
    const int32_t size = sizeof(buf);
    for (int32_t i = 0; i < strlen(buf); i++) {
        if (i > 0)
            printf(":");
        printf("%02X", buf[i]);
    }
    printf("\n");
}