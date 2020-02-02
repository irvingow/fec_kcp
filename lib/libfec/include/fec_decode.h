//
// Created by lwj on 2019/10/11.
//

#ifndef LIBFEC_FEC_DECODE_H
#define LIBFEC_FEC_DECODE_H

#include <cstdint>
#include <vector>
#include <cstring>
#include <atomic>
#include <mutex>
#include <map>
#include <memory>
#include "timeout_map.h"

typedef struct{
  std::atomic_bool ready_for_output;
  char *data = nullptr;
  uint32_t actural_len;
  uint32_t max_len;
  uint32_t last_process_seq;
  uint32_t last_process_index;
  int32_t LargerMem(uint32_t expected_len);
} FecDecodeOutputDataUnit;

class FecDecode {
 public:
  explicit FecDecode(const int32_t &timeout_ms);
  ~FecDecode();
  /**
   * @param input_data_pkg 指向输入数据的指针
   * @param length 输入数据的长度
   * @return 如果返回值大于等于0,说明正常返回,返回值为0代表正常接收,返回值为1代表可以进行fec解码,
   * 调用Output函数可以获得解码之后的数据,如果小于0代表发生错误,一般因为输入数据包内容有问题
   * return 0 means data package has been correctly received but not prepared
   * for output, return positive number means the length of the prepared data
   * package, you should call @func Output with a buffer of length @return
   * return -2 means input_data_pkg is a wrong data package
   */
  int32_t Input(const char *input_data_pkg, int32_t length);

  /**
   * handle the issue when fec_encode send some unencoded data
   * @param input_data_pkg pointer to the start position of the data package
   * @param length the length of the input data package
   * @return -2 means input_data_pkg is a wrong data package
   * normally return a positive number which means the length of next prepared
   * data package, and @func Output is ready for output, you should call @func
   * Output with a buffer of length @return
   */
  int32_t DealUnEncodeData(const char *input_data_pkg, int32_t length);
  /**
   * @param data_pkgs 输出数据组的指针
   * @param length 对应每个输出数据的长度
   * @return
   * 如果小于0代表发生错误,一般是因为没有足够的包来进行fec解码,所以无可返回的数据
   * return 0 means everything is well, but no more prepared data for output
   * return a positive value means the length of next prepared data package
   * and you should call @func Output again with a buffer of length @return
   */
  int32_t Output(char *recv_buf, int32_t length);

  /**
   * 清除过期的数据,如果一些数据长时间没有获得足够的包来进行fec解码,就变成无用的数据了,可以调用
   * 这个函数来清除这些无用的数据
   */
  void ClearTimeoutDatas();
 private:
  void RemoveSeqRespondData(const uint32_t& seq);
  int32_t SearchForNextReadySeq(const int32_t& least_len);
 private:
  std::mutex seq_mutex_;///这个锁的范围比较大,保护下面6个数据结构
  std::map<uint32_t, int32_t> seq2data_pkgs_num_;
  std::map<uint32_t, int32_t> seq2redundant_data_pkgs_num_;
  std::map<uint32_t, int32_t> seq2max_data_pkg_length_;
  std::shared_ptr<TimeOutMap> Sptr2TimeoutMap_;
  std::map<uint32_t, std::vector<char *>> seq2data_pkgs_;
  std::map<uint32_t, std::vector<int32_t>> seq2data_pkgs_length_;
  std::map<uint32_t , int32_t > seq2cur_recv_data_pkg_num_;
  std::map<uint32_t , bool> seq2ready_for_output_;
  std::atomic_int ready_seqs_nums_;
 private:
  std::mutex output_unit_mutex_;
  FecDecodeOutputDataUnit output_unit_;
  const int32_t fec_encode_head_length_ = 11;
  const uint32_t unique_header_ = 0x12345678;
};

#endif //LIBFEC_FEC_DECODE_H
