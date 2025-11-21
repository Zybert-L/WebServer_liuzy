#pragma once

#include <cstring>   //perror
#include <iostream>
#include <unistd.h>  // write
#include <sys/uio.h> //readv
#include <vector> //readv
#include <atomic>
#include <assert.h>

using namespace std;

class Buffer{
public:
    Buffer(int init_buff_size = 1024);
    ~Buffer() = default;

    //大小查询
    size_t read_able_bytes() const;             //计算当前可读数据的大小。
    size_t write_able_bytes() const;            //计算当前可写数据的大小。
    size_t head_able_bytes() const;             //计算缓冲区头部的空闲空间

    //数据访问
    const char* get_read_peek()const;           //获取当前可读数据的起始位置
    char* get_write_peek();                     //获取当前可写数据的起始位置
    const char* get_write_peek_const()const;    //

    //缓冲区管理
    void enable_write_able(size_t len);         //确保缓冲区有足够的空间容纳指定长度的数据。
    void has_write(size_t len);                 //标记指定长度的数据已被写入，更新write_pos。
    void has_read(size_t len);                  //已经读了多少，更新read_pos
    void read_to_where(const char* end);        //读数据直到指定的指针位置
    void clear_buff();                          //读完缓冲区中的所有数据
    string read_all_to_str_andclear();          //读完所有数据并返回为 std::string。

    //数据追加
    void append(const string& str);             //追加字符串。
    void append(const char* str,size_t len);    //追加指定长度的字符数组。
    void append(const void* str,size_t len);    //追加任意类型的数据。
    void append(const Buffer& buff);            //追加另一个 Buffer 对象的内容
    
    //文件描述符IO
    ssize_t read_fd(int fd, int* Errno);        //从文件描述符读取数据到缓冲区  
    ssize_t write_fd(int fd, int* Errno);       //将缓冲区数据写入文件描述符
private:
    char* begin_ptr();                          //返回缓冲区起始指针（有 const 和非 const 两种版本）。
    const char* begin_ptr()const;
    void expand_space(size_t len);              //扩容
private:
    atomic<size_t>write_pos;                    // 写到哪里的位置
    atomic<size_t>read_pos;                     // 读到哪里的位置
    vector<char>buffer_;                        // 作为底层存储，可以动态调整大小。
};
