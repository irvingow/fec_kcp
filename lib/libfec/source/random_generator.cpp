//
// Created by lwj on 2019/10/11.
//

#include "random_generator.h"
#include <fcntl.h>
#include <glog/logging.h>

RandomNumberGenerator *RandomNumberGenerator::GetInstance() {
    static RandomNumberGenerator instance;
    return &instance;
}

int32_t RandomNumberGenerator::GetRandomNumber32(uint32_t &random_number) {
    int size = read(random_number_fd_, &random_number, sizeof(random_number));
    if (size != sizeof(random_number)) {
        LOG(ERROR) << "get random number failed error:" << strerror(errno);
        return -1;
    }
    return 0;
}

int32_t RandomNumberGenerator::GetRandomNumberU32(uint32_t &random_number) {
    int32_t ret = GetRandomNumber32(random_number);
    while (ret == 0 && random_number == 0) {
        ret = GetRandomNumber32(random_number);
    }
    return ret;
}

int32_t RandomNumberGenerator::GetRandomNumber16(uint16_t & random_number) {
    int size = read(random_number_fd_, &random_number, sizeof(random_number));
    if (size != sizeof(random_number)) {
        LOG(ERROR) << "get random number failed error:" << strerror(errno);
        return -1;
    }
    return 0;
}

int32_t RandomNumberGenerator::GetRandomNumberU16(uint16_t & random_number) {
    int32_t ret = GetRandomNumber16(random_number);
    while (ret == 0 && random_number == 0) {
        ret = GetRandomNumber16(random_number);
    }
    return ret;
}

RandomNumberGenerator::Obj_Creator::Obj_Creator() {
    RandomNumberGenerator::GetInstance();
}

RandomNumberGenerator::RandomNumberGenerator() {
    random_number_fd_ = open("/dev/urandom", O_RDONLY | O_NONBLOCK);
    if (random_number_fd_ == -1) {
        LOG(ERROR) << "failed to open /dev/urandom error" << strerror(errno);
        return;
    }
}

RandomNumberGenerator::~RandomNumberGenerator() { close(random_number_fd_); }