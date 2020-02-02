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
  FecEncode(const int32_t& data_pkg_num, const int32_t& redundant_pkg_num, const uint32_t& timeout = 1);
  ~FecEncode();
  ///return 1 means that fec encode is ok, and user need to call Output to get encoded data.
  int32_t Input(const char* input_data_pkg, int32_t length);
  ///调用者必须手动从出参中复制数据,并不能直接使用返回的指针
  int32_t Output(std::vector<char*>& data_pkgs, std::vector<int32_t>& data_pkgs_length);
  /**
   * this function is used to update inside timer of FecEncode
   * @note You should better call this function before and after you call @func Input or
   * call this function in a fixed interval
   * @param cur_millsec the current milliseconds
   * @return return 0 for everything is fine, return -1 for error, and return a positive
   * number means @func Input data package timeout, need to be flushed, and you should
   * call @func FlushUnEncodedData next
   */
  int32_t FecEncodeUpdateTime(const uint64_t& cur_millsec);
  ///output the unencoded data, note that you should copy the data from the pointers
  ///that this function return, and the pointers will be freed next time you call @Input
  int32_t FlushUnEncodedData(std::vector<char*>& data_pkgs, std::vector<int32_t>& data_pkg_length);
 private:
  void ResetDataPkgs();
 private:
  std::atomic_uint_least64_t inside_timer_;
  std::atomic_uint_least64_t newest_update_time_;
  std::atomic_int cur_data_pkgs_num_;
  std::atomic_int max_data_pkg_length_;
  std::vector<char *> data_pkgs_;
  std::vector<int32_t > data_pkgs_length_;
  std::mutex data_pkgs_mutex_;
  std::atomic_bool ready_for_fec_output_;
  int32_t data_pkg_num_;
  int32_t redundant_pkg_num_;
  uint16_t seq;
 private:
  const int32_t fec_encode_head_length_ = 11;
  const uint32_t unique_header_ = 0x12345678;
  const uint32_t timeout_time_ = 1;
};

#endif //LIBFEC_FEC_ENCODE_H
