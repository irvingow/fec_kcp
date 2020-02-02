//
// Created by lwj on 2019/12/13.
//

#include "fec_manager.h"
#include "fec_kcp_common.h"
#include <vector>
#include <unistd.h>

FecEncodeManager::FecEncodeManager(std::shared_ptr<connection_info_t> sp_conn,
                                   std::shared_ptr<FecEncode> sp_fec_encoder)
    : sp_conn_(std::move(sp_conn)),
      sp_fec_encoder_(std::move(sp_fec_encoder)) {}

int32_t FecEncodeManager::Input(const char *data, const int32_t &length) {
    std::lock_guard<std::mutex> lck(fec_manager_mutex_);
    auto ret = sp_fec_encoder_->Input(data, length);
    if (ret < 0)
        return -1;
    if (ret == 1) {
        std::vector<char *> data_pkgs;
        std::vector<int32_t> data_pkgs_length;
        ret = sp_fec_encoder_->Output(data_pkgs, data_pkgs_length);
        if (ret < 0) {
            return -2;
        }
        const int size = data_pkgs.size();
        if (size != data_pkgs_length.size())
            return -3;
        for (int i = 0; i < size; ++i) {
            ret = send_data(data_pkgs[i], data_pkgs_length[i]);
            if (ret < 0) {
                return -4;
            }
        }
    }
}

int32_t FecEncodeManager::send_data(const char *data, const int32_t &length) {
    if (sp_conn_->isclient_) {
        auto ret = send(sp_conn_->socket_fd_, data, length, 0);
        return ret;
    } else {
        auto ret = sendto(sp_conn_->socket_fd_, data, length, 0, (sockaddr *) &(sp_conn_->addr_), sp_conn_->slen_);
        return ret;
    }
}

int32_t FecEncodeManager::FlushUnEncodedData() {
    std::vector<char *> data_pkgs;
    std::vector<int32_t> data_pkgs_length;
    sp_fec_encoder_->FlushUnEncodedData(data_pkgs, data_pkgs_length);
    const int size = data_pkgs.size();
    if (size != data_pkgs_length.size())
        return -1;
    for (int i = 0; i < size; ++i) {
        auto ret = send_data(data_pkgs[i], data_pkgs_length[i]);
        if (ret < 0) {
            return -2;
        }
    }
    return 0;
}



















