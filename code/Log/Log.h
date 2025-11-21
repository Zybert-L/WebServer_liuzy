#pragma once

#include<mutex>
#include<string>
#include<thread>
#include<memory>
#include <sys/stat.h>  
#include <stdarg.h>
#include"../Buffer/Buffer.h"
#include"block_queue.h"

using namespace std;

class Log{
public:
    void init(int level,const char* path="./log",const char* suffix=".log",int max_queue_capacity =1024);

    static Log* get_instance();

    static void flush_log_thread();

    void write(int level,const char* format,...);

    void flush();

    int get_level(){
        lock_guard<mutex> locker(mutex_);
        return level_;
    }

    void set_level(int level){
        lock_guard<mutex> locker(mutex_);
        level_ = level;
    }

    bool is_open(){return is_open_;}

private:

    Log();

    void append_log_level_title(int level);

    virtual ~Log();

    void async_write();

private:

    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 50000;

    const char* path_;
    const char* suffix_;

    //int MAX_LINES;

    int line_count_;
    int today_;

    bool is_open_;

    Buffer buffer_;
    int level_;
    bool is_aysnc_;

    FILE *fp_;
    unique_ptr<block_queue<string>>deque_;
    unique_ptr<thread> write_thread_;
    mutex mutex_;
};

#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::get_instance();\
        if (log->is_open() && log->get_level() <= level) {\
            log->write(level, format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);