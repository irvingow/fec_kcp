#include "ikcp.h"
#include "fec_encode.h"
#include "fec_decode.h"
#include "fec_kcp_common.h"
#include "fec_manager.h"
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <glog/logging.h>
#include <sys/epoll.h>
#include <memory>

const std::string LOCAL_IP = "0.0.0.0";
const int32_t LOCAL_PORT = 9999;

int udpout(const char *buf, int len, ikcpcb *kcp, void *user) {
    auto fec_encoder_manager = static_cast<FecEncodeManager *> (user);
    return fec_encoder_manager->Input(buf, len);
}

void run(int32_t epoll_fd,
         int32_t local_listen_fd,
         int32_t remote_connected_fd,
         int32_t kcp_update_timer_fd
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
    sp_conn->socket_fd_ = remote_connected_fd;
    sp_conn->isclient_ = true;
    std::shared_ptr<FecEncode> sp_fec_encode(new FecEncode(2, 1));
    FecEncodeManager fec_encode_manager(sp_conn, sp_fec_encode);
    ikcpcb *kcp = ikcp_create(0x11111111, (void *) &fec_encode_manager);
    kcp->output = udpout;
    ///结束
    sockaddr_in local_client;
    socklen_t slen = sizeof(local_client);
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
                    LOG(INFO) << "recv data from remote server";
                    ///获得从server端的数据
                    bzero(recv_buf, sizeof(recv_buf));
                    recv_len = recv(remote_connected_fd, recv_buf, sizeof(recv_buf), 0);
                    if (recv_len < 0) {
                        LOG(ERROR) << "failed to recv data from remote server error:%s" << strerror(errno);
                        continue;
                    }
                    auto decode_ret = fec_decoder.Input(recv_buf, recv_len);
                    if (decode_ret == 1) {
                        ///说明数据已经被全部解码完成,需要发送给kcp处理
                        std::vector<char *> data_pkgs;
                        std::vector<int32_t> data_pkgs_length;
                        decode_ret = fec_decoder.Output(data_pkgs, data_pkgs_length);
                        if (decode_ret < 0) {
                            LOG(ERROR) << "failed to get decoded data from fec_decoder";
                            continue;
                        }
                        LOG(INFO) << "fec decode success data_pks size:" << data_pkgs.size();
                        for (size_t i = 0; i < data_pkgs.size(); ++i) {
                            auto temp_ret = ikcp_input(kcp, data_pkgs[i], data_pkgs_length[i]);
                            LOG(INFO) << "kcp_input:" << data_pkgs[i] << " size:" << data_pkgs_length[i];
                            if (temp_ret < 0) {
                                LOG(WARNING) << "ikcp_input error:" << temp_ret;
                                continue;
                            }
                        }
                    }
                } else if (events[index].data.fd == local_listen_fd) {
                    ///接收到来自本地监听端口的数据
                    bzero(recv_buf, sizeof(recv_buf));
                    recv_len =
                        recvfrom(local_listen_fd, recv_buf, sizeof(recv_buf), 0, (sockaddr *) &local_client, &slen);
                    if (recv_len < 0) {
                        LOG(ERROR) << "failed to recv data from local client error:%s" << strerror(errno);
                        continue;
                    }
                    LOG(INFO) << "recv data from client:" << recv_buf;
                    auto temp_ret = ikcp_send(kcp, recv_buf, recv_len);
                    if (temp_ret < 0) {
                        LOG(WARNING) << "ikcp_send error";
                        continue;
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
                        }
                        ///需要将从远端获得的数据再回送给client
                        sendto(local_listen_fd, recv_buf, temp_ret, 0, (sockaddr *) &local_client, slen);
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
    int epoll_fd = -1, local_listen_fd = -1, remote_connected_fd = -1, kcp_update_timer_fd = -1;
    auto ret =
        init(remote_ip,
             remote_port,
             LOCAL_IP,
             LOCAL_PORT,
             epoll_fd,
             local_listen_fd,
             remote_connected_fd,
             kcp_update_timer_fd
             );
    if (ret < 0) {
        LOG(ERROR) << "failed to call init";
        return -1;
    }
    run(epoll_fd, local_listen_fd, remote_connected_fd, kcp_update_timer_fd);
    return 0;
}