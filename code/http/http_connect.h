#pragma once

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>
#include <stdlib.h>      // atoi()
#include <errno.h>      
#include "../Log/Log.h"
#include "../Buffer/Buffer.h"
#include "http_request.h"
#include "http_response.h"

using namespace std;

class http_connect{
public:
    http_connect();
    ~http_connect();
    //初始化连接，设置文件描述符和地址
    void init(int sockfd,const sockaddr_in& addr);
    //从套接字读取数据，返回读取字节数
    ssize_t read(int* save_errno);
    //向套接字写入数据，返回写入字节数
    ssize_t write(int* save_errno);
    //关闭连接
    void close_();
    //获取文件描述符
    int get_fd()const;
    //获取客户端端口号
    int get_port()const;
    //获取客户端IP地址
    const char* get_ip()const;
    //获取客户端地址结构
    sockaddr_in get_addr()const;
    //处理 HTTP 请求并生成响应
    bool process();
    //返回待写入的总字节数（iov_ 数组的总长度）
    int to_write_bytes(){return iov_[0].iov_len + iov_[1].iov_len; }
    //检查是否为长连接，调用 request_.IsKeepAlive()
    bool is_keep_alive()const{return request_.is_keep_alive();}

    void reset();

public:
    static bool is_et;                      //是否使用边缘触发模式（epoll ET 模式）
    static const char* src_dir;             //静态资源目录路径
    static atomic<int> user_count;          //原子计数器，记录当前连接数

private:
    int fd_;                                //套接字文件描述符
    struct sockaddr_in addr_;               //客户端地址
    bool is_close_;                         //连接是否关闭的标志
    int iov_cnt_;                           //数组中有效的元素数
    struct iovec iov_[2];                   //分散写缓冲区数组，固定大小为 2（响应头 + 文件内容）
    Buffer read_buff_;                      //读缓冲区，存储从套接字读取的数据
    Buffer write_buff_;                     //写缓冲区，存储待写入的数据
    http_request request_;                  //请求解析对象
    http_response response_;                //响应生成对象
};
