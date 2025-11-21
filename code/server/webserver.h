#pragma once

#include <unordered_map>
#include <fcntl.h>       // fcntl()
#include <unistd.h>      // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../Log/Log.h"
#include "../timer/heap_timer.h"
#include "../Pool/mysql_connection.h"
#include "../Pool/mysql_connection_pool.h"
#include "../Pool/thread_pool.h"
#include "../http/http_connect.h"
#include "../config/config.h"
#include "../server/epoller.h"
class WebServer{
public:
    // 使用 Config 对象初始化服务器
    explicit WebServer(const Config& config);
    
    // 析构函数
    ~WebServer();
    
    // 启动服务器
    void Start();

private:
    // 初始化监听 socket
    bool InitSocket_(); 

    // 初始化事件模式（ET/LT）
    void InitEventMode_(int trigMode);

    // 添加新客户端连接
    void AddClient_(int fd, sockaddr_in addr);

    // 处理监听事件
    void DealListen_();
    
    // 处理写事件
    void DealWrite_(http_connect* client);
    
    // 处理读事件
    void DealRead_(http_connect* client);

    // 发送错误信息
    void SendError_(int fd, const char* info);
    
    // 延长客户端超时时间
    void ExtentTime_(http_connect* client);
    
    // 关闭客户端连接
    void CloseConn_(http_connect* client);

    // 处理读操作
    void OnRead_(http_connect* client);
    
    // 处理写操作
    void OnWrite_(http_connect* client);
    
    // 处理 HTTP 请求
    void OnProcess(http_connect* client);

    static const int MAX_FD = 65536; // 最大文件描述符数

    // 设置文件描述符为非阻塞
    static int SetFdNonblock(int fd);


    int port_;             // 服务器端口
    bool openLinger_;      // 是否启用优雅关闭
    int timeoutMS_;        // 超时时间（毫秒）
    bool isClose_;         // 服务器是否关闭
    int listenFd_;         // 监听文件描述符
    char* srcDir_;         // 资源目录路径

    uint32_t listenEvent_; // 监听事件
    uint32_t connEvent_;   // 连接事件

    std::unique_ptr<HeapTimer> timer_;      // 定时器
    std::unique_ptr<thread_pool> threadpool_; // 线程池
    std::unique_ptr<Epoller> epoller_;      // epoll 管理器
    std::unordered_map<int, http_connect> users_; // 客户端连接
};