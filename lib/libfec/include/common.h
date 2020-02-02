//
// Created by lwj on 2019/10/12.
//

#ifndef LIBFEC_COMMON_H
#define LIBFEC_COMMON_H
#include <cstdint>
#include <ctime>
#include <unistd.h>
#include <sys/time.h>

void write_u32(char *p, uint32_t l);

void write_u32_r(char *p, uint32_t l);

uint32_t read_u32(const char *p);

uint32_t read_u32_r(const char *p);

void write_u16(char *p, uint16_t l);

uint16_t read_u16(const char *p);

int64_t getnowtime_ms();

void print_char_array_in_byte(const char *buf);

#endif //LIBFEC_COMMON_H
