//
// Created by lwj on 2019/10/11.
//

#ifndef LIBFEC_FEC_ENCODE_H
#define LIBFEC_FEC_ENCODE_H

#include <string>
#include <atomic>
#include <cstdint>
#include <vector>
#include <mutex>
#include "rs.h"

class FecEncode{
 public:
  FecEncode(const int32_t& data_pkg_num, const int32_t& redundant_pkg_num);
  ~FecEncode();
  int32_t Input(const char* input_data_pkg, int32_t length);
  ///调用者必须手动从出参中复制数据,并不能直接使用返回的指针
  int32_t Output(std::vector<char*>& data_pkgs, std::vector<int32_t>& data_pkg_length_);
 private:
  void ResetDataPkgs();
 private:
  std::atomic_int cur_data_pkgs_num_;
  std::atomic_int max_data_pkg_length_;
  std::vector<char *> data_pkgs_;
  std::vector<int32_t > data_pkgs_length_;
  std::mutex data_pkgs_mutex_;
  std::atomic_bool ready_for_fec_output_;
  int32_t data_pkg_num_;
  int32_t redundant_pkg_num_;
  uint32_t seq;
  const int32_t fec_encode_head_length_ = 7;
};

#endif //LIBFEC_FEC_ENCODE_H
