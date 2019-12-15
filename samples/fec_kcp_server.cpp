#include "ikcp.h"
#include "fec_encode.h"
#include "fec_decode.h"
#include <sys/types.h>
#include <unistd.h>
#include <glog/logging.h>
#include <sys/epoll.h>
#include <fec_manager.h>
#include "fec_kcp_common.h"

const std::string LOCAL_IP = "0.0.0.0";
const int32_t LOCAL_PORT = 5555;

int udpout(const char *buf, int len, ikcpcb *kcp, void *user) {
    auto fec_encode_manger = static_cast<FecEncodeManager *>(user);
    return fec_encode_manger->Input(buf, len);
}

void run(int32_t epoll_fd,
         int local_listen_fd,
         int remote_connected_fd,
         int kcp_update_timer_fd
) {
    char recv_buf[2048] = {0};
    int recv_len = 0;
    const int max_events = 64;
    struct epoll_event events[max_events];
    ///创建encoder和decoder
    FecDecode fec_decoder(10000);
    ///结束
    ///创建kcp相关结构
    std::shared_ptr<connection_info_t> sp_conn(new connection_info_t);
    sp_conn->socket_fd_ = local_listen_fd;
    sp_conn->isclient_ = false;
    std::shared_ptr<FecEncode> sp_fec_encode(new FecEncode(2, 1));
    FecEncodeManager fec_encode_manager(sp_conn, sp_fec_encode);
    ikcpcb *kcp = ikcp_create(0x11111111, (void *) &fec_encode_manager);
    kcp->output = udpout;
    ///结束
    while (true) {
        int nfds = epoll_wait(epoll_fd, events, max_events, -1);
        if (nfds < 0) {
            if (errno == EINTR) {
                LOG(WARNING) << "epoll interrupted by signal continue";
                continue;
            } else {
                LOG(ERROR) << "epoll_wait return " << nfds
                           << " error:" << strerror(errno);
                break;
            }
        }
        for (size_t index = 0; index < nfds; ++index) {
            if (events[index].events & EPOLLIN) {
                if (events[index].data.fd == remote_connected_fd) {
                    ///获得从server端的数据,需要回送给kcp来处理
                    bzero(recv_buf, sizeof(recv_buf));
                    recv_len = recv(remote_connected_fd, recv_buf, sizeof(recv_buf), 0);
                    if (recv_len < 0) {
                        LOG(ERROR) << "failed to recv data from remote server error:%s" << strerror(errno);
                        continue;
                    }
                    LOG(INFO) << "recv data from remote server:" << recv_buf;
                    auto temp_ret = ikcp_send(kcp, recv_buf, recv_len);
                    if (temp_ret < 0) {
                        LOG(WARNING) << "ikcp_send error";
                        continue;
                    }
                } else if (events[index].data.fd == local_listen_fd) {
                    LOG(INFO) << "recv data from kcp client";
                    ///接收到来自kcp client的数据
                    bzero(recv_buf, sizeof(recv_buf));
                    sockaddr_in addr;
                    socklen_t slen = sizeof(addr);
                    recv_len = recvfrom(local_listen_fd, recv_buf, sizeof(recv_buf), 0, (sockaddr *) &addr, &slen);
                    sp_conn->addr_ = addr;
                    sp_conn->slen_ = slen;
                    if (recv_len < 0) {
                        LOG(ERROR) << "failed to recv data from local client error:%s" << strerror(errno);
                        continue;
                    }
                    auto decode_ret = fec_decoder.Input(recv_buf, recv_len);
                    if (decode_ret == 1) {
                        ///说明数据已经被全部解码完成,需要被传递给kcp来处理
                        std::vector<char *> data_pkgs;
                        std::vector<int32_t> data_pkgs_length;
                        decode_ret = fec_decoder.Output(data_pkgs, data_pkgs_length);
                        if (decode_ret < 0) {
                            LOG(ERROR) << "failed to get decoded data from fec_decoder";
                            continue;
                        }
                        LOG(INFO) << "fec decode success data_pks size:" << data_pkgs.size();
                        for (size_t i = 0; i < data_pkgs.size(); ++i) {
                            print_char_array_in_byte(data_pkgs[i]);
                            auto temp_ret = ikcp_input(kcp, data_pkgs[i], data_pkgs_length[i]);
                            if (temp_ret < 0) {
                                LOG(WARNING) << "ikcp_input error ret:" << temp_ret;
                                continue;
                            }
                        }
                    }
                } else if (events[index].data.fd == kcp_update_timer_fd) {
                    ///50ms的定时器被触发
                    ikcp_update(kcp, get_milliseconds());
                    int temp_ret = 1;
                    while (temp_ret > 0) {
                        bzero(recv_buf, sizeof(recv_buf));
                        temp_ret = ikcp_recv(kcp, recv_buf, sizeof(recv_buf));
                        if (temp_ret > 0) {
                            LOG(INFO) << "recv data from remote:" << recv_buf;
                        } else {
                            break;
                        }
                        auto ret = send(remote_connected_fd, recv_buf, temp_ret, 0);
                        if (ret < 0) {
                            LOG(ERROR) << "failed to send data to remote server";
                            break;
                        }
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    google::InitGoogleLogging("INFO");
    FLAGS_logtostderr = true;
    if (argc < 3) {
        LOG(ERROR) << "usage:" << argv[0] << " remote_ip remote_port";
        return -1;
    }
    const std::string remote_ip(argv[1]);
    auto remote_port = static_cast<int32_t > (strtol(argv[2], nullptr, 10));
    if (remote_port < 0) {
        LOG(ERROR) << "invalid remote_port, port should be in 0-65535";
        return -1;
    }
    int epoll_fd = -1, local_listen_fd = -1, remote_connected_fd = -1, kcp_update_timerfd = -1;
    auto ret =
        init(remote_ip,
             remote_port,
             LOCAL_IP,
             LOCAL_PORT,
             epoll_fd,
             local_listen_fd,
             remote_connected_fd,
             kcp_update_timerfd);
    if (ret < 0) {
        LOG(ERROR) << "failed to call init";
        return -1;
    }
    run(epoll_fd, local_listen_fd, remote_connected_fd, kcp_update_timerfd);
    return 0;
}