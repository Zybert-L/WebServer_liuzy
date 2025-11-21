#include "webserver.h"
#include <netinet/tcp.h>  // for TCP_NODELAY

WebServer::WebServer(const Config& config):
    port_(config.GetInt("port",10001)),
    openLinger_(config.GetBool("opt_linger",false)),
    timeoutMS_(config.GetInt("timeout_ms",30000)),
    isClose_(false),
    timer_(new HeapTimer),
    threadpool_(new thread_pool(config.GetInt("thread_num",8))),
    epoller_(new Epoller)
{
    // 设置资源目录
    srcDir_ = getcwd(nullptr,256);
    assert(srcDir_);
    strncat(srcDir_,"/resources/",16);
    http_connect::user_count = 0;
    http_connect::src_dir = srcDir_;

    // 初始化数据库连接池
    mysql_connection_pool::get_connection_pool()->init_connections("localhost",
                                                                   config.GetInt("sql_port",3306),
                                                                   config.GetString("sql_user","cpp11"),
                                                                   config.GetString("sql_pwd","LzyMysql123!"),
                                                                   config.GetString("db_name","lzydb"),
                                                                   config.GetInt("conn_pool_num",8));
    
    // 初始化事件模式
    InitEventMode_(config.GetInt("trigger_mode",0));

    // 初始化监听socket
    if(!InitSocket_()){
        isClose_ = true;
    }

    // 初始化日志系统
    if(config.GetBool("open_log",true)){
        //Log::get_instance()->init(config.GetInt("log_level",0)),"./log",".log",config.GetInt("log_queue_size",1024);
        Log::get_instance()->init(config.GetInt("log_level",0),"./log",".log",config.GetInt("log_queue_size",1024));
        if(isClose_){
            LOG_ERROR("========== Server init error!==========");
        }else{
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Port:%d, OpenLinger: %s", port_, openLinger_ ? "true" : "false");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                    (listenEvent_ & EPOLLET ? "ET" : "LT"),
                    (connEvent_ & EPOLLET ? "ET" : "LT"));
            LOG_INFO("LogSys level: %d", config.GetInt("log_level", 1));
            LOG_INFO("srcDir: %s", http_connect::src_dir);
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d",
                     config.GetInt("conn_pool_num", 8),
                     config.GetInt("thread_num", 8));
        }
    }
}

WebServer::~WebServer() {
    close(listenFd_);
    isClose_ = true;
    free(srcDir_);
}

void WebServer::InitEventMode_(int trigMode){
    listenEvent_ = EPOLLRDHUP;
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMode)
    {
    case 0:
        break;
    case 1:
        connEvent_ |= EPOLLET;
        break;
    case 2:
        listenEvent_ |=EPOLLET;
        break;
    case 3:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    default:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    }
    http_connect::is_et = (connEvent_ & EPOLLET);
}

void WebServer::Start(){
    int timeMS = -1;
    if(!isClose_){ LOG_INFO("========== Server start =========="); }
    while(!isClose_){
        if(timeoutMS_>0){
            timeMS = timer_->GetNextTick();
        }
        int eventCnt = epoller_->Wait(timeMS);
        for(int i = 0;i<eventCnt;i++){
            int fd = epoller_->GetEventFd(i);
            uint32_t events = epoller_->GetEvents(i);
            ///////////////////////////16.45
            LOG_DEBUG("Event on fd[%d]: %x (IN:%d, OUT:%d, RDHUP:%d, HUP:%d, ERR:%d)",
              fd, events, events & EPOLLIN, events & EPOLLOUT,
              events & EPOLLRDHUP, events & EPOLLHUP, events & EPOLLERR);
            if(fd == listenFd_){
                DealListen_();
            }else if(events&(EPOLLRDHUP|EPOLLHUP|EPOLLERR)){
                assert(users_.count(fd)>0);
                CloseConn_(&users_[fd]);
            }else if(events & EPOLLIN) {
                assert(users_.count(fd) > 0);
                DealRead_(&users_[fd]);
            }else if(events & EPOLLOUT) {
                assert(users_.count(fd) > 0);
                DealWrite_(&users_[fd]);
            } else {
                LOG_ERROR("Unexpected event on fd[%d]: %x", fd, events);
            }
        }
    }
}

void WebServer::SendError_(int fd, const char* info){
    assert(fd > 0);
    int ret = send (fd, info, strlen(info),0);
    if(ret < 0) {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}

void WebServer::CloseConn_(http_connect * client){
    assert(client);
    LOG_INFO("Client[%d] quit!", client->get_fd());
    epoller_->DelFd(client->get_fd());
    client->close_();
}

/*void WebServer::AddClient_(int fd,sockaddr_in addr){
    assert(fd > 0);
    users_[fd].init(fd,addr);
    if(timeoutMS_>0){
       timer_->Add(fd, timeoutMS_, std::bind(&WebServer::CloseConn_, this, &users_[fd]));
    }
    epoller_->AddFd(fd,EPOLLIN|connEvent_);
    SetFdNonblock(fd);
    LOG_INFO("Client[%d] in!", users_[fd].get_fd());
}*/

// 在 webserver.cpp 的 AddClient_ 函数中添加套接字缓冲区设置

void WebServer::AddClient_(int fd, sockaddr_in addr){
    assert(fd > 0);
    
    // 增加套接字发送缓冲区大小
    int send_buffer_size = 262144; // 256KB
    if(setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &send_buffer_size, sizeof(send_buffer_size)) < 0) {
        LOG_WARN("Failed to set send buffer size for fd[%d]", fd);
    }
    
    // 增加套接字接收缓冲区大小
    int recv_buffer_size = 262144; // 256KB
    if(setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &recv_buffer_size, sizeof(recv_buffer_size)) < 0) {
        LOG_WARN("Failed to set recv buffer size for fd[%d]", fd);
    }
    
    // 启用TCP_NODELAY，减少延迟
    int flag = 1;
    if(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) < 0) {
        LOG_WARN("Failed to set TCP_NODELAY for fd[%d]", fd);
    }
    
    users_[fd].init(fd, addr);
    if(timeoutMS_ > 0){
       timer_->Add(fd, timeoutMS_, std::bind(&WebServer::CloseConn_, this, &users_[fd]));
    }
    epoller_->AddFd(fd, EPOLLIN | connEvent_);
    SetFdNonblock(fd);
    LOG_INFO("Client[%d] in!", users_[fd].get_fd());
}

void WebServer::DealListen_(){
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do{
        int fd = accept(listenFd_,(struct sockaddr*)&addr,&len);
        if(fd <= 0) {
            return;
        }else if(http_connect::user_count>=MAX_FD){
            SendError_(fd,"Server busy!");
            LOG_WARN("Clients is full!");
            return;
        }
        AddClient_(fd,addr);
    }while(listenEvent_ & EPOLLIN);
}

void WebServer::DealRead_(http_connect * client){
    assert(client);
    ExtentTime_(client);
    threadpool_->add_task(std::bind(&WebServer::OnRead_,this,client));
}

void WebServer::DealWrite_(http_connect* client) {
    assert(client);
    ExtentTime_(client);
    threadpool_->add_task(std::bind(&WebServer::OnWrite_, this, client));
}

void WebServer::ExtentTime_(http_connect * client){
    assert(client);
    if(timeoutMS_>0){
        timer_->Adjust(client->get_fd(),timeoutMS_);
    }
}

void WebServer::OnRead_(http_connect * client){
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);
    if(ret<=0 && readErrno!=EAGAIN){
        LOG_DEBUG("OnRead_ failed, ret: %d, errno: %d (%s)", ret, readErrno, strerror(readErrno));
        CloseConn_(client);
        return;
    }
    OnProcess(client);
}

// 在 webserver.cpp 中修改 OnWrite_ 函数

/*void WebServer::OnWrite_(http_connect* client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    LOG_DEBUG("OnWrite_ fd[%d], ret: %d, errno: %d (%s)", 
              client->get_fd(), ret, writeErrno, strerror(writeErrno));
    
    if(client->to_write_bytes() == 0) {
        // 数据发送完毕
        if(client->is_keep_alive()) {
            OnProcess(client);
            return;
        }
    } 
    else if(ret < 0) {
        if(writeErrno == EAGAIN || writeErrno == EWOULDBLOCK) {
            // 缓冲区满了，继续监听写事件
            epoller_->ModFd(client->get_fd(), connEvent_ | EPOLLOUT);
            LOG_DEBUG("Write would block, continue monitoring EPOLLOUT for fd[%d]", client->get_fd());
            return;  // 不要关闭连接！
        }
        // 其他错误才关闭连接
        LOG_ERROR("Write error on fd[%d], closing connection", client->get_fd());
    }
    else {
        // 部分数据发送成功，继续监听写事件
        epoller_->ModFd(client->get_fd(), connEvent_ | EPOLLOUT);
        return;
    }
    
    CloseConn_(client);
}*/

void WebServer::OnWrite_(http_connect* client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    LOG_DEBUG("OnWrite_ fd[%d], ret: %d, errno: %d (%s)", 
              client->get_fd(), ret, writeErrno, strerror(writeErrno));
    
    if(client->to_write_bytes() == 0) {
        // 数据发送完毕
        if(client->is_keep_alive()) {
            // 重要：先调用 OnProcess 重新初始化，这会释放之前的mmap
            OnProcess(client);
            return;
        }
        // 不是keep-alive，关闭连接
        CloseConn_(client);
    } 
    else if(ret < 0) {
        if(writeErrno == EAGAIN || writeErrno == EWOULDBLOCK) {
            // 缓冲区满了，继续监听写事件
            epoller_->ModFd(client->get_fd(), connEvent_ | EPOLLOUT);
            LOG_DEBUG("Write would block, continue monitoring EPOLLOUT for fd[%d]", client->get_fd());
            return;
        }
        // 其他错误才关闭连接
        LOG_ERROR("Write error on fd[%d], closing connection", client->get_fd());
        CloseConn_(client);
    }
    else {
        // 部分数据发送成功，继续监听写事件
        epoller_->ModFd(client->get_fd(), connEvent_ | EPOLLOUT);
        return;
    }
}


bool WebServer::InitSocket_(){
    int ret;
    struct sockaddr_in addr;
    if(port_ > 65535 || port_ < 1024) {
        LOG_ERROR("Port:%d error!", port_);
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    struct linger optLinger = {0};
    if(openLinger_){
         /* 优雅关闭: 直到所剩数据发送完毕或超时 */
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }
    listenFd_ = socket(AF_INET,SOCK_STREAM,0);
    if(listenFd_ < 0) {
        LOG_ERROR("Create socket error!", port_);
        return false;
    }
    //调用 setsockopt，设置套接字的 SO_LINGER 选项，应用 optLinger 配置（优雅关闭）。
    ret = setsockopt(listenFd_,SOL_SOCKET,SO_LINGER,&optLinger,sizeof(optLinger));
    if(ret < 0){
        close(listenFd_);
        LOG_ERROR("Init linger error!", port_);
        return false;
    }

    int optval = 1;
    /* 端口复用 */
    /* 只有最后一个套接字会正常接收数据。 */
    ret = setsockopt(listenFd_,SOL_SOCKET,SO_REUSEADDR,(const void*)&optval,sizeof(int));
    if(ret == -1) {
        LOG_ERROR("set socket setsockopt error !");
        close(listenFd_);
        return false;
    }

    ret = bind(listenFd_,(struct sockaddr*)&addr,sizeof(addr));
    if(ret < 0) {
        LOG_ERROR("Bind Port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    ret = listen(listenFd_, 6);
    if(ret < 0) {
        LOG_ERROR("Listen port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    ret = epoller_->AddFd(listenFd_,listenEvent_|EPOLLIN);
    if(ret == 0) {
        LOG_ERROR("Add listen error!");
        close(listenFd_);
        return false;
    }
    SetFdNonblock(listenFd_);
    LOG_INFO("Server port:%d", port_);
    return true;
}

int WebServer::SetFdNonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}

void WebServer::OnProcess(http_connect* client) {
    if(client->process()) {
        epoller_->ModFd(client->get_fd(), connEvent_ | EPOLLOUT);
    } else {
        epoller_->ModFd(client->get_fd(), connEvent_ | EPOLLIN);
    }
}