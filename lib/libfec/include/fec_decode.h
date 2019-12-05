//
// Created by lwj on 2019/10/11.
//

#ifndef LIBFEC_FEC_DECODE_H
#define LIBFEC_FEC_DECODE_H

#include <cstdint>
#include <vector>
#include <atomic>
#include <mutex>
#include <map>
#include <memory>
#include "timeout_map.h"
class FecDecode {
 public:
  explicit FecDecode(const int32_t &timeout_ms);
  /**
   * @param input_data_pkg 指向输入数据的指针
   * @param length 输入数据的长度
   * @return 如果返回值大于等于0,说明正常返回,返回值为0代表正常接收,返回值为1代表可以进行fec解码,
   * 调用Output函数可以获得解码之后的数据,如果小于0代表发生错误,一般因为输入数据包内容有问题
   */
  int32_t Input(char *input_data_pkg, int32_t length);
  /**
   * @param data_pkgs 输出数据组的指针
   * @param length 对应每个输出数据的长度
   * @return 如果返回值大于等于0,说明正常返回,返回值代表剩余的已经解码成功的数据包组的数量
   * 如果小于0代表发生错误,一般是因为没有足够的包来进行fec解码,所以无可返回的数据
   */
  int32_t Output(std::vector<char *> &data_pkgs, std::vector<int32_t> &length);
  /**
   * 清除过期的数据,如果一些数据长时间没有获得足够的包来进行fec解码,就变成无用的数据了,可以调用
   * 这个函数来清除这些无用的数据
   */
  void ClearTimeoutDatas();
 private:
  void RemoveSeqRespondData(const uint32_t& seq);
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
  const int32_t fec_encode_head_length_ = 7;
};

#endif //LIBFEC_FEC_DECODE_H
