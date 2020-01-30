//
// Created by lwj on 2019/11/28.
//

#ifndef FEC_KCP_FEC_KCP_H
#define FEC_KCP_FEC_KCP_H
#include <string>
#include <memory>
#include "ikcp.h"
#include "fec_manager.h"
#include "fec_encode.h"
#include "fec_decode.h"

class FecKcp{
 public:
  typedef int (*FecKcpSendFunc)(const char*, int, void *);
  FecKcp(const int32_t& data_pkg_num, const int32_t& redundant_pkg_num, void *user_cb_data, FecKcpSendFunc fec_kcp_send_func);
  int32_t UserDataSend(const char* buf, int32_t length);
  int32_t DataInput(const char* buf, int32_t length);
  int32_t DataOutput(char* buf, int32_t length);
  void FecKcpUpdate(const int64_t& milliseconds);
  int32_t FecEncodeInput(const char* buf, int32_t length);
  int32_t FecEncodeOutput(std::vector<char*>& data_pkgs, std::vector<int32_t>& data_pkgs_length);
  void *user_cb_data_;
  FecKcpSendFunc fec_kcp_send_func_;
 private:
  static int32_t KcpSendFunc(const char* buf, int length, ikcpcb* kcp, void *user);
  ikcpcb *kcp_;
  std::shared_ptr<FecEncode> sp_fec_encode_;
  std::shared_ptr<FecDecode> sp_fec_decode_;
};

#endif //FEC_KCP_FEC_KCP_H
