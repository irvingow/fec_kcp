####一个基于Kcp和Fec的流量转发软件

##### 受到kcptun的启发,自己实现的一个demo,不过性能上和kcptun相差不少,需要改进

##### usage:

> mkdir build
> cd build
> cmake ..
> make

##### 数据流向:

fec_kcp_client:client_tcp_package->kcp->fec_encode->udp->fec_decode->kcp->server

fec_kcp_server:server_tcp_package->kcp->fec_encode->udp->fec_decode->kcp->client
