#pragma once
#include <iostream>
#include <mysql/mysql.h>
#include <string>
#include <vector>
#include "../Log/Log.h"

using namespace std;

class mysql_connection {
public:
    mysql_connection();
    ~mysql_connection();
    bool connect(string ip, unsigned short port, string user, string password, string dbname);
    bool update(string sql);
    bool query(string sql);  // 执行查询并存储结果
    vector<vector<string>> get_result();  // 获取查询结果
    void free_result();  // 释放结果集

private:
    MYSQL* connection_;
    MYSQL_RES* result_;  // 存储查询结果
    int m_close_log = 0;
};