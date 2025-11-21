#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <regex>
#include <errno.h>     
#include <mysql/mysql.h>  //mysql

#include "../Buffer/Buffer.h"
#include "../Log/Log.h"
#include "../Pool/mysql_connection_pool.h"

using namespace std;

class http_request{
public:
    enum PARSE_STATE{
        REQUEST_LINE,           //解析请求行
        HEADERS,                //解析头部
        BODY,                   //解析正文
        FINISH,                 //解析完成
    };

    enum HTTP_CODE{
        NO_REQUEST = 0,         //没有请求数据
        GET_REQUEST,            //成功的 GET 请求
        BAD_REQUEST,            //请求格式错误
        NO_RESOURSE,            //资源不存在
        FORBIDDENT_REQUEST,     //禁止访问
        FILE_REQUEST,           //文件请求
        INTERNAL_ERROR,         //服务器内部错误
        CLOSED_CONNECTION,      //连接关闭
    };

    http_request(){init();}     //默认构造函数，调用Init初始化
    ~http_request() =default;   

    void init();                //初始化函数
    bool parse(Buffer &buff);   //解析HTTP请求

    //获取请求信息的访问器
    string path() const;        //获取路径（只读）
    string& path();             //获取路径（可修改）
    string method() const;      //获取请求方法
    string version() const;     //获取HTTP版本
    string get_post(const string& key)const;     //获取POST数据（string键）
    string get_post(const char* key) const;      // 获取POST数据（C字符串键）

    bool is_keep_alive() const;                  //检查是否为长连接

    
private:
    bool parse_request_line(const string& line);     // 解析请求行
    void parse_header(const string& line);           // 解析头部
    void parse_body(const string& line);             // 解析正文
    void parse_path();                               // 处理请求路径
    void parse_post();                               // 解析POST数据
    void parse_from_url_encoded();                   // 解码URL编码表单数据

    // 静态工具方法
    static bool user_verify(const string& name, const string& pwd, bool isLogin); // 用户验证
    static int ConverHex(char ch);                   // 十六进制字符转整数

    // 数据成员
    PARSE_STATE state_;                              // 当前解析状态
    string method_;                                  // 请求方法
    string path_;                                    // 请求路径
    string version_;                                 // HTTP版本
    string body_;                                    // 请求正文
    unordered_map<string, string> header_;           // 头部键值对
    unordered_map<string, string> post_;             // POST数据键值对

    // 静态常量成员
    static const unordered_set<string> DEFAULT_HTML;            // 默认HTML路径集合
    static const unordered_map<string, int> DEFAULT_HTML_TAG;   // HTML路径标记映射   
};