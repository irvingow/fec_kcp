//
// Created by lwj on 2019/12/1.
//

#ifndef FEC_KCP_COMMON_H
#define FEC_KCP_COMMON_H

#include <string>
#include <netinet/in.h>

typedef struct {
  sockaddr_in addr_;
  socklen_t slen_;
  int socket_fd_;
  bool isclient_;
} connection_info_t;

void print_char_array_in_byte(const char *buf) ;

int32_t AddEvent2Epoll(const int32_t &epoll_fd, const int32_t &fd, const uint32_t &events);

int new_listen_socket(const std::string &ip, const size_t &port, int &fd);

int new_connected_socket(const std::string &remote_ip, const size_t &remote_port, int &fd);

long get_milliseconds();

int32_t init(const std::string &remote_ip,
             const int32_t &remote_port,
             const std::string &local_listen_ip,
             const int32_t &local_listen_port,
             int32_t &epoll_fd,
             int32_t &local_listen_fd,
             int32_t &remote_connected_fd,
             int32_t &timerfd);

#endif //FEC_KCP_COMMON_H
