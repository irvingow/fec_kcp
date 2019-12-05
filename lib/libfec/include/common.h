//
// Created by lwj on 2019/10/12.
//

#ifndef LIBFEC_COMMON_H
#define LIBFEC_COMMON_H
#include <cstdint>
#include <ctime>
#include <unistd.h>
#include <sys/time.h>

void write_u32(char * p,uint32_t l);

uint32_t read_u32(char * p);

int64_t getnowtime_ms();

void print_char_array_in_byte(char *buf) ;

#endif //LIBFEC_COMMON_H
