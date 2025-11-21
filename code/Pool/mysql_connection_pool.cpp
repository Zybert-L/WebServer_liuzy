#include "mysql_connection_pool.h"

mysql_connection_pool* mysql_connection_pool::get_connection_pool() {
    static mysql_connection_pool pool;
    return &pool;
}

mysql_connection_pool::mysql_connection_pool(string ip_, unsigned short port_, string username_, string password_, string dbname_,int size_) {
    init_connections(ip_,port_,username_,password_,dbname_,size_);
    sem_init(&sem_id, 0, size_); // 初始化信号量为连接池大小
}

mysql_connection_pool::~mysql_connection_pool() {
    lock_guard<mutex> lock(queue_mutex);
    while (!queue_conn_pool.empty()) {
        mysql_connection* conn = queue_conn_pool.front();
        queue_conn_pool.pop(); // shared_ptr 自动释放连接
    }
    sem_destroy(&sem_id);
    mysql_library_end();
}

/*void mysql_connection_pool::init_connections(string ip_, unsigned short port_, string username_, string password_, string dbname_,int size_) {
    for (int i = 0; i < size_; ++i) {
        mysql_connection* conn = new mysql_connection();
        if (conn->connect(ip_, port_, username_, password_, dbname_)) {
            lock_guard<mutex> lock(queue_mutex);
            queue_conn_pool.push(conn);
        } else {
            delete conn;
        }
    }
}*/

// 方案1：在 init_connections 中初始化信号量
void mysql_connection_pool::init_connections(string ip_, unsigned short port_, 
                                           string username_, string password_, 
                                           string dbname_, int size_) {
    // 先销毁旧的信号量（如果存在）
    sem_destroy(&sem_id);
    
    // 初始化信号量
    sem_init(&sem_id, 0, 0);  // 先初始化为0
    
    // 创建连接
    for (int i = 0; i < size_; ++i) {
        mysql_connection* conn = new mysql_connection();
        if (conn->connect(ip_, port_, username_, password_, dbname_)) {
            lock_guard<mutex> lock(queue_mutex);
            queue_conn_pool.push(conn);
            sem_post(&sem_id);  // 每添加一个连接，信号量+1
        } else {
            delete conn;
            LOG_ERROR("Failed to create connection %d", i);
        }
    }
    
    // 保存连接池参数
    this->ip_ = ip_;
    this->port_ = port_;
    this->username_ = username_;
    this->password_ = password_;
    this->dbname_ = dbname_;
    this->size_ = size_;
}

unique_ptr<mysql_connection, function<void(mysql_connection*)>> mysql_connection_pool::get_connection() {
    unique_lock<mutex> lock(queue_mutex);  // 加锁保护队列访问

    // 等待信号量，确保有可用连接
    if (sem_wait(&sem_id) != 0) {
        // 日志记录错误（假设有日志宏 LOG_ERROR）
        //LOG_ERROR("Failed to wait for semaphore in get_connection");
        return unique_ptr<mysql_connection, function<void(mysql_connection*)>>(nullptr, nullptr);
    }

    if (queue_conn_pool.empty()) {
        sem_post(&sem_id);  // 恢复信号量
        //LOG_WARN("Connection pool is empty after sem_wait!");
        return unique_ptr<mysql_connection, function<void(mysql_connection*)>>(nullptr, nullptr);
    }

    // 从队列中取出一个连接
    mysql_connection* conn = queue_conn_pool.front();
    queue_conn_pool.pop();

    // 定义自定义删除器，将连接归还到池中
    auto deleter = [this](mysql_connection* c) {
        if (c) {
            unique_lock<mutex> guard(queue_mutex);  // 加锁保护队列
            queue_conn_pool.push(c);  // 将连接归还到队列
            sem_post(&sem_id);  // 增加信号量，通知其他线程
        }
    };

    // 返回绑定了裸指针和删除器的智能指针
    return unique_ptr<mysql_connection, function<void(mysql_connection*)>>(conn, deleter);
}