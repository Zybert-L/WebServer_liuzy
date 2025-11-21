#pragma once

#include <sys/stat.h>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/mman.h>    // mmap, munmap
#include <unordered_map>
#include "../Buffer/Buffer.h"
#include "../Log/Log.h"

using namespace std;

class http_response{
public:
    http_response();
    ~http_response();
    //初始化对象，设置资源目录、路径、连接状态和状态码
    void init(const string& src_dir,string& path,bool is_keep_alive,int code = -1);

    //生成完整的 HTTP 响应
    void make_response(Buffer& buff);

    //释放内存映射
    void unmap_file();

    //返回内存映射的文件内容指针
    char* file();

    //返回文件长度
    size_t file_len() const;

    //生成错误响应内容
    void error_content(Buffer& buff,string message);

    //返回当前状态码（内联函数）
    int code()const{return code_;}

private:
    void add_state_line(Buffer& buff);
    void add_header(Buffer& buff);
    void add_content(Buffer& buff);
    void error_html();
    string get_file_type();

private:
    int code_;                          //HTTP 状态码
    bool is_keep_alive_;                //是否保持连接
    string path_;                       //请求的资源路径
    string src_dir_;                    //资源根目录
    char* mm_file_;                     //内存映射的文件内容指针
    struct stat mm_file_stat_;          //文件状态结构体，存储文件大小等信息

    static const unordered_map<string,string>SUFFIX_TYPE;       //文件扩展名到 MIME 类型的映射
    static const unordered_map<int,string>CODE_STATUS;          //状态码到状态消息的映射。
    static const unordered_map<int,string>CODE_PATH;            //状态码到错误页面路径的映射
};