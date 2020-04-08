//
// Created by lwj on 2019/12/13.
//

#ifndef FEC_KCP_FEC_MANAGER_H
#define FEC_KCP_FEC_MANAGER_H

#include "fec_kcp_common.h"
#include "fec_encode.h"
#include "fec_decode.h"
#include <memory>
#include <thread>

class FecEncodeManager{
 public:
  FecEncodeManager(std::shared_ptr<connection_info_t> sp_conn, std::shared_ptr<FecEncode> sp_fec_encoder);
  int32_t Input(const char* data, const int32_t& length);
  int32_t FlushUnEncodedData();
 private:
  int32_t send_data(const char* data, const int32_t& length);
 private:
  std::shared_ptr<connection_info_t> sp_conn_;
  std::shared_ptr<FecEncode> sp_fec_encoder_;
};

#endif //FEC_KCP_FEC_MANAGER_H
