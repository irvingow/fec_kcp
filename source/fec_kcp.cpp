//
// Created by lwj on 2019/11/28.
//

#include <functional>
#include "fec_kcp.h"
#include <cstdlib>

FecKcp::FecKcp(const int32_t &data_pkg_num,
               const int32_t &redundant_pkg_num,
               void *user_cb_data,
               FecKcpSendFunc fec_kcp_send_func)
    : user_cb_data_(user_cb_data),
      fec_kcp_send_func_(fec_kcp_send_func),
      sp_fec_encode_(new FecEncode(data_pkg_num, redundant_pkg_num)),
      sp_fec_decode_(new FecDecode(10000)) {
    kcp_ = ikcp_create(0x11112222, this);
    kcp_->output = &FecKcp::KcpSendFunc;
}

int32_t FecKcp::UserDataSend(const char *buf, int32_t length) {
    return ikcp_send(kcp_, buf, length);
}

int32_t FecKcp::DataInput(const char *buf, int32_t length) {
    ///we need to decode before we send the data to kcp
    auto len = sp_fec_decode_->Input(buf, length);
    int ret = 0;
    while (len > 0) {
        ///说明数据已经被全部解码完成,需要被传递给kcp来处理
        char *recv_buf = (char *) malloc(len + 1);
        if (recv_buf == nullptr)
            return -2;
        bzero(recv_buf, len + 1);
        ret = sp_fec_decode_->Output(recv_buf, len);
        if (ret < 0) {
            free(recv_buf);
            break;
        }
        auto temp_ret = ikcp_input(kcp_, recv_buf, len);
        len = ret;
        if (temp_ret < 0) {
            ret = temp_ret;
            free(recv_buf);
            continue;
        }
        free(recv_buf);
    }
    return ret;
}

int32_t FecKcp::DataOutput(char *buf, int32_t length) {
    return ikcp_recv(kcp_, buf, length);
}

void FecKcp::FecKcpUpdate(const int64_t &milliseconds) {
    ikcp_update(kcp_, milliseconds);
}

int32_t FecKcp::FecEncodeInput(const char *buf, int32_t length) {
    return sp_fec_encode_->Input(buf, length);
}

int32_t FecKcp::FecEncodeOutput(std::vector<char *> &data_pkgs, std::vector<int32_t> &data_pkgs_length) {
    return sp_fec_encode_->Output(data_pkgs, data_pkgs_length);
}

int32_t FecKcp::KcpSendFunc(const char *buf, int length, ikcpcb *kcp, void *user) {
    auto fec_kcp = reinterpret_cast<FecKcp *>(user);
    auto ret = fec_kcp->FecEncodeInput(buf, length);
    if (ret < 0)
        return -1;
    if (ret == 1) {
        std::vector<char *> data_pkgs;
        std::vector<int32_t> data_pkgs_length;
        ret = fec_kcp->FecEncodeOutput(data_pkgs, data_pkgs_length);
        if (ret < 0) {
            return -2;
        }
        const int size = data_pkgs.size();
        if (size != data_pkgs_length.size())
            return -3;
        for (int i = 0; i < size; ++i) {
            fec_kcp->fec_kcp_send_func_(data_pkgs[i], data_pkgs_length[i], fec_kcp->user_cb_data_);
            if (ret < 0) {
                return -4;
            }
        }
    }
    return 0;
}




















