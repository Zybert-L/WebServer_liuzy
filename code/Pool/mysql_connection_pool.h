#pragma once
#include <mysql/mysql.h>
#include <queue>
#include "mysql_connection.h"
#include <memory>
#include <semaphore.h>
#include <mutex>
#include <functional>

using namespace std;

class mysql_connection_pool {
public:
    static mysql_connection_pool* get_connection_pool();
    std::unique_ptr<mysql_connection, std::function<void(mysql_connection*)>> get_connection();

    void init_connections(string ip_, unsigned short port_, string username_, string password_, string dbname_,int size_);

private:

    mysql_connection_pool() = default;
    mysql_connection_pool(string ip_, unsigned short port_, string username_, string password_, string dbname_,int size_);
    ~mysql_connection_pool();

private:
    string ip_;
    unsigned short port_;
    string username_;
    string password_;
    string dbname_;
    int size_; // 固定连接池大小

    queue<mysql_connection*> queue_conn_pool; // 连接队列
    mutex queue_mutex;                                   // 保护队列的互斥锁
    sem_t sem_id;                                        // 信号量，控制可用连接数
};


/*
mysql -u cpp11 -p       登录
USE lzydb;              先选择数据库
SHOW TABLES;            查看所有表
DESC user;              查看表的结构
SELECT * FROM user;     查询表里的数据
DELETE FROM user;       删除表里所有的数据
TRUNCATE TABLE user;    删除所有记录并重建表结构，自增计数器重置为 1
*/

/*  string ip_ = "localhost";
    unsigned short port_ = 3306;
    string username_ = "cpp11";
    string password_ = "LzyMysql123!";
    string dbname_ = "lzydb";
    int size_ = 20; // 固定连接池大小*/

