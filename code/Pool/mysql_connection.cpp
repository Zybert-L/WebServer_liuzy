#include "mysql_connection.h"

mysql_connection::mysql_connection() {
    connection_ = mysql_init(nullptr);
    if (!connection_) {
        throw std::runtime_error("mysql_init failed");
    }
    result_ = nullptr;
}

mysql_connection::~mysql_connection() {
    if (result_) {
        mysql_free_result(result_);
        result_ = nullptr;
    }
    if (connection_) {
        mysql_close(connection_);
    }
}

bool mysql_connection::connect(string ip, unsigned short port, string user, string password, string dbname) {
    MYSQL* p = mysql_real_connect(connection_, ip.c_str(), user.c_str(),
                                  password.c_str(), dbname.c_str(), port, nullptr, 0);
    if (!p) {
        LOG_ERROR("MySQL connect failed");
        return false;
    }
    return true;
}

bool mysql_connection::update(string sql) {
    if (!connection_) {
        LOG_ERROR("MySQL connection is null");
        return false;
    }
    if (mysql_query(connection_, sql.c_str())) {
        LOG_ERROR("MySQL update Error: %s", mysql_error(connection_));
        return false;
    }
    return true;
}

/*bool mysql_connection::query(string sql) {
    if (!connection_) {
        LOG_ERROR("MySQL connection is null");
        return false;
    }
    if (result_) {  // 释放上一次的结果
        mysql_free_result(result_);
        result_ = nullptr;
    }
    if (mysql_query(connection_, sql.c_str())) {
        LOG_ERROR("MySQL query Error: %s", mysql_error(connection_));
        return false;
    }
    result_ = mysql_store_result(connection_);
    if (!result_) {
        LOG_ERROR("MySQL store result Error: %s", mysql_error(connection_));
        return false;
    }
    return true;
}*/

bool mysql_connection::query(string sql) {
    if (!connection_) {
        LOG_ERROR("MySQL connection is null");
        return false;  // 失败返回 false
    }
    
    if (result_) {  // 释放上一次的结果
        mysql_free_result(result_);
        result_ = nullptr;
    }
    
    if (mysql_query(connection_, sql.c_str())) {
        LOG_ERROR("MySQL query Error: %s", mysql_error(connection_));
        return false;  // 查询失败返回 false
    }
    
    result_ = mysql_store_result(connection_);
    if (!result_) {
        LOG_ERROR("MySQL store result Error: %s", mysql_error(connection_));
        return false;  // 存储结果失败返回 false
    }
    
    return true;  // 成功返回 true
}

vector<vector<string>> mysql_connection::get_result() {
    vector<vector<string>> rows;
    if (!result_) {
        return rows;  // 返回空结果
    }
    while (MYSQL_ROW row = mysql_fetch_row(result_)) {
        vector<string> row_data;
        unsigned int num_fields = mysql_num_fields(result_);
        for (unsigned int i = 0; i < num_fields; ++i) {
            row_data.push_back(row[i] ? row[i] : "NULL");
        }
        rows.push_back(row_data);
    }
    return rows;
}

void mysql_connection::free_result() {
    if (result_) {
        mysql_free_result(result_);
        result_ = nullptr;
    }
}