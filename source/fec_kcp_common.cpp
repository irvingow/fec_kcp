//
// Created by lwj on 2019/12/1.
//

#include <sys/epoll.h>
#include <cstdint>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <glog/logging.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include "fec_kcp_common.h"

void print_char_array_in_byte(const char *buf) {
//    for (int32_t i = 0; i < strlen(buf); i++) {
    for (int32_t i = 0; i < 4; i++) {
        if (i > 0)
            printf(":");
        printf("%02X", buf[i]);
    }
    printf("\n");
}

int32_t AddEvent2Epoll(const int32_t &epoll_fd, const int32_t &fd, const uint32_t &events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    auto ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    if (ret != 0) {
        LOG(INFO) << "add fd" << fd << " to epoll_fd:" << epoll_fd << " failed, error:" << strerror(errno);
        return -1;
    }
    return 0;
}

int new_listen_socket(const std::string &ip, const size_t &port, int &fd) {
    fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);
    if (fd == -1) {
        LOG(ERROR) << "create new socket failed" << strerror(errno);
        return -1;
    }
    struct sockaddr_in local_addr = {0};
    socklen_t slen = sizeof(local_addr);
    local_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *) &local_addr, slen) == -1) {
        LOG(ERROR) << "socket bind error port:" << port
                   << " error:" << strerror(errno);
        return -1;
    }
    LOG(INFO) << "local socket listen_fd:" << fd;
}

int new_connected_socket(const std::string &remote_ip,
                         const size_t &remote_port, int &fd) {
    LOG(INFO) << "remote ip:" << remote_ip << " port:" << remote_port;
    struct sockaddr_in remote_addr_in = {0};
    socklen_t slen = sizeof(remote_addr_in);
    remote_addr_in.sin_addr.s_addr = inet_addr(remote_ip.c_str());
    remote_addr_in.sin_family = AF_INET;
    remote_addr_in.sin_port = htons(remote_port);

    fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);
    if (fd < 0) {
        LOG(ERROR) << "create new socket failed" << strerror(errno);
        return -1;
    }
    int ret = connect(fd, (struct sockaddr *) &remote_addr_in, slen);
    if (ret < 0) {
        ///实际上connect基本不会失败,因为内核只做一些基本的检查,比如参数是否合法,ip是否有效等等,
        ///之后内核就创建相应数据结构,并且返回,并没有真的去连接那个ip和port
        LOG(ERROR) << "failed to establish connection to remote, error:"
                   << strerror(errno);
        close(fd);
        return -1;
    }
    LOG(INFO) << "create new remote connection udp_fd:" << fd;
    return 0;
}

long get_milliseconds() {
    struct timeval tv = {0, 0};
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int32_t init(const std::string &remote_ip,
             const int32_t &remote_port,
             const std::string &local_listen_ip,
             const int32_t &local_listen_port,
             int32_t &epoll_fd,
             int32_t &local_listen_fd,
             int32_t &remote_connected_fd,
             int32_t &timerfd) {
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        LOG(ERROR) << "create epoll failed error:" << strerror(errno);
        return -1;
    }
    ///创建本地监听的local_listen_fd,同时将其加入epoll监听池中
    auto ret = new_listen_socket(local_listen_ip, local_listen_port, local_listen_fd);
    if (ret < 0) {
        LOG(ERROR) << "failed to new_listen_socket error:" << strerror(errno);
        close(epoll_fd);
        return -1;
    }
    ret = AddEvent2Epoll(epoll_fd, local_listen_fd, EPOLLIN);
    if (ret != 0) {
        close(local_listen_fd);
        close(epoll_fd);
        LOG(INFO) << "add local_udp_listen_fd to epoll failed, error:" << strerror(errno);
        return -1;
    }
    ///结束
    ///创建连接远端的remote_connected_fd,同时将其加入到epoll监听池中
    ret = new_connected_socket(remote_ip, remote_port, remote_connected_fd);
    if (ret != 0) {
        close(local_listen_fd);
        close(epoll_fd);
        LOG(ERROR) << "failed to create remote_connected_fd remote_ip:" << remote_ip << " port"
                   << remote_port;
        return -1;
    }
    ret = AddEvent2Epoll(epoll_fd, remote_connected_fd, EPOLLIN);
    if (ret != 0) {
        close(remote_connected_fd);
        close(local_listen_fd);
        close(epoll_fd);
        LOG(INFO) << "add local_udp_listen_fd failed, error:" << strerror(errno);
        return -1;
    }
    ///结束
    ///创建timer_fd,提醒间隔为10ms,将其加入到epoll监听池中;
    struct itimerspec temp = {0, 0, 0, 0};
    temp.it_value.tv_sec = 1;
    temp.it_value.tv_nsec = 0;
    temp.it_interval.tv_nsec = 50000000;
    timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (timerfd == -1) {
        LOG(ERROR) << "failed to call timerfd_create error:" << strerror(errno);
        return -1;
    }
    if (timerfd_settime(timerfd, TFD_TIMER_ABSTIME, &temp, nullptr) == -1) {
        LOG(ERROR) << "failed to call timerfd_settime error:" << strerror(errno);
        return -1;
    }
    ret = AddEvent2Epoll(epoll_fd, timerfd, EPOLLIN);
    if (ret != 0) {
        close(remote_connected_fd);
        close(local_listen_fd);
        close(timerfd);
        close(epoll_fd);
        LOG(INFO) << "add local_udp_listen_fd failed, error:" << strerror(errno);
        return -1;
    }
    return 0;
    ///结束
}
