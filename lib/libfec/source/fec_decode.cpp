//
// Created by lwj on 2019/10/11.
//
#include "fec_decode.h"
#include "common.h"
#include <algorithm>
#include <cstring>
#include "rs.h"

FecDecode::FecDecode(const int32_t &timeout_ms) : Sptr2TimeoutMap_(new TimeOutMap(timeout_ms)),
                                                  ready_seqs_nums_(0) {}

int32_t FecDecode::Input(char *input_data_pkg, int32_t length) {
    if (length <= fec_encode_head_length_ || input_data_pkg == nullptr)
        return -1;
    uint32_t seq = read_u32(input_data_pkg);
    auto data_pkg_num = static_cast<int32_t>(input_data_pkg[4]);
    auto redundant_pkg_num = static_cast<int32_t>(input_data_pkg[5]);
    auto index = static_cast<int32_t>(input_data_pkg[6]);
    if (index == 0 || index > (data_pkg_num + redundant_pkg_num))
        return -1;
    ///由于索引是从1开始的,所以这里需要做一个减法操作
    index--;
    length -= fec_encode_head_length_;
    ///下面这种情况说明实际上对应seq的所有数据已经解码完成同时已经被输出过了,所以直接返回0就好
    if (seq2data_pkgs_.count(seq))
        if (!seq2ready_for_output_[seq] && seq2cur_recv_data_pkg_num_[seq] >= seq2data_pkgs_num_[seq])
            return 0;
    if (seq2ready_for_output_[seq])
        return 1;
    char *data = (char *) malloc((length + 1));
    bzero(data, (length + 1));
    memcpy(data, input_data_pkg + fec_encode_head_length_, length+1);
    std::lock_guard<std::mutex> lck(seq_mutex_);
    seq2data_pkgs_num_[seq] = data_pkg_num;
    seq2redundant_data_pkgs_num_[seq] = redundant_pkg_num;
    seq2max_data_pkg_length_[seq] = std::max(seq2max_data_pkg_length_[seq], length);
    Sptr2TimeoutMap_->Add(seq, getnowtime_ms());
    if (seq2data_pkgs_[seq].empty()) {
        seq2data_pkgs_[seq].resize(data_pkg_num + redundant_pkg_num);
        seq2data_pkgs_length_[seq].resize(data_pkg_num + redundant_pkg_num);
    }
    ///防止有重复的包出现
    if (seq2data_pkgs_[seq][index] == nullptr)
        seq2data_pkgs_[seq][index] = data;
    else {
        ///这里直接释放数据然后返回就好
        free(data);
        return 0;
    }
    seq2data_pkgs_length_[seq][index] = length;
    ++seq2cur_recv_data_pkg_num_[seq];
    if (seq2cur_recv_data_pkg_num_[seq] >= seq2data_pkgs_num_[seq]) {
        ///说明可以进行解码操作了
        char **wait_decode_data = nullptr;
        wait_decode_data = (char **) malloc((data_pkg_num +
            redundant_pkg_num) * sizeof(char *));
        for (int32_t i = 0; i < data_pkg_num + redundant_pkg_num; ++i) {
            wait_decode_data[i] = seq2data_pkgs_[seq][i];
        }
        int32_t ret = rs_decode2(data_pkg_num, data_pkg_num + redundant_pkg_num, wait_decode_data,
                                 seq2max_data_pkg_length_[seq]);
        if (ret < 0)
            return -1;
        for (int32_t i = 0; i < data_pkg_num + redundant_pkg_num; ++i) {
            seq2data_pkgs_[seq][i] = wait_decode_data[i];
            if (seq2data_pkgs_length_[seq][i] == 0)
                seq2data_pkgs_length_[seq][i] = seq2max_data_pkg_length_[seq];
        }
        seq2data_pkgs_[seq].erase(std::begin(seq2data_pkgs_[seq]) + data_pkg_num,
                                  std::end(seq2data_pkgs_[seq]));
        seq2data_pkgs_length_[seq].erase(std::begin(seq2data_pkgs_length_[seq]) + data_pkg_num,
                                         std::end(seq2data_pkgs_length_[seq]));
        ///解码完成
        free(wait_decode_data);
        seq2ready_for_output_[seq] = true;
        ready_seqs_nums_++;
        return 1;
    }
    if (seq2data_pkgs_.size() > 50)
        ClearTimeoutDatas();
    return 0;
}

int32_t FecDecode::Output(std::vector<char *> &data_pkgs, std::vector<int32_t> &length) {
    if (ready_seqs_nums_ == 0) {
        ClearTimeoutDatas();
        return -1;
    }
    std::lock_guard<std::mutex> lck(seq_mutex_);
    for (auto iter = seq2ready_for_output_.begin(); iter != seq2ready_for_output_.end(); ++iter) {
        if (iter->second) {
            data_pkgs = seq2data_pkgs_[iter->first];
            length = seq2data_pkgs_length_[iter->first];
            --ready_seqs_nums_;
            ///不需要管这个iter->first对应的包,在ClearTimeoutDatas会清理这些包以及相关的数据
            ///但是需要下面这句话,否则在ClearTimeoutDatas里面不会清除对应数据
            seq2ready_for_output_[iter->first] = false;
//            RemoveSeqRespondData(iter->first);
            return ready_seqs_nums_;
        }
    }
    return -1;
}

void FecDecode::ClearTimeoutDatas() {
    auto timeout_seqs = Sptr2TimeoutMap_->GetTimeOutElements(getnowtime_ms());
    for (const auto &timeout_seq : timeout_seqs) {
        if (!seq2ready_for_output_[timeout_seq])
            RemoveSeqRespondData(timeout_seq);
    }
}

void FecDecode::RemoveSeqRespondData(const uint32_t &seq) {
    for (char *pkg : seq2data_pkgs_[seq]) {
        free(pkg);
        pkg = nullptr;
    }
    seq2data_pkgs_num_.erase(seq);
    seq2redundant_data_pkgs_num_.erase(seq);
    seq2max_data_pkg_length_.erase(seq);
    Sptr2TimeoutMap_->Remove(seq);
    seq2data_pkgs_.erase(seq);
    seq2data_pkgs_length_.erase(seq);
    seq2cur_recv_data_pkg_num_.erase(seq);
    seq2ready_for_output_.erase(seq);
}







