//
// Created by lwj on 2019/10/11.
//

#ifndef LIBFEC_RANDOM_GENERATOR_H
#define LIBFEC_RANDOM_GENERATOR_H

#include <noncopyable.h>
#include <cstdint>
///boost单例模式实现
class RandomNumberGenerator : public noncopyable {
 public:
  static RandomNumberGenerator *GetInstance();
  int32_t GetRandomNumber32(uint32_t &random_number);
  int32_t GetRandomNumberU32(uint32_t &random_number);
  int32_t GetRandomNumber16(uint16_t &random_number);
  int32_t GetRandomNumberU16(uint16_t &random_number);

 protected:
  struct Obj_Creator {
    Obj_Creator();
  };
  static Obj_Creator obj_creator_;
  RandomNumberGenerator();
  ~RandomNumberGenerator();
  int32_t random_number_fd_;
};

#endif //LIBFEC_RANDOM_GENERATOR_H
