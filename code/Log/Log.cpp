#include "Log.h"

Log::Log(){
    line_count_ = 0;
    //is_open_ = true;
    is_aysnc_ = false;
    write_thread_ = nullptr;
    deque_ = nullptr;
    today_ = 0;
    fp_ = nullptr;
}

Log::~Log(){
    if(write_thread_&&write_thread_->joinable()){
        while(!deque_->empty()){
            deque_->flush();
        }
        deque_->close();
        write_thread_->join();//注意，个人觉得因为已经清空了队列，所以就没法写了
    }
    if(fp_){
        lock_guard<mutex>locker(mutex_);
        flush();
        fclose(fp_);
    }
}

void Log::init(int level = 1, const char* path,const char* suffix, int max_queue_capacity){
    is_open_ = true;
    level_ = level;
    if(max_queue_capacity > 0){
        is_aysnc_ = true;
        if(!deque_){
            unique_ptr<block_queue<string>>newdeque(new block_queue<string>);
            deque_ = move(newdeque);

            unique_ptr<thread> newthread(new thread (flush_log_thread));
            write_thread_ = move(newthread);
        }
    }else{
        is_aysnc_ = false;
    }

    line_count_ = 0;
    time_t timer = time(nullptr);
    struct tm *systime = localtime(&timer);
    struct tm t = * systime;
    path_ = path;
    suffix_ = suffix;
    char filename[LOG_NAME_LEN] = {0};
    snprintf(filename, LOG_NAME_LEN-1,"%s/%04d_%02d_%02d%s",path_,t.tm_year + 1900,t.tm_mon + 1,t.tm_mday,suffix_);
    today_ = t.tm_mday;

    {
        lock_guard<mutex>locker(mutex_);
        buffer_.clear_buff();//清空缓冲区，防止旧数据影响新日志。
        if(fp_){
            /*如果 fp_ 不是 nullptr，说明文件已经打开，则：
            flush();  强制刷新缓冲区，把 未写入磁盘的日志数据写入文件。
            fclose(fp_); 关闭当前日志文件，避免内存泄漏。*/
            flush();
            fclose(fp_);
        }
        fp_ = fopen(filename, "a");
        /*如果 fopen(fileName, "a") 失败（返回 nullptr）：
        可能是 文件所在目录 path_ 不存在，导致无法创建文件。
        调用 mkdir(path_, 0777); 创建目录，权限 0777 表示：
        所有用户（owner、group、others） 都有 读、写、执行 权限。
        重新尝试 fopen(fileName, "a"); 打开文件。*/
        if(fp_ == nullptr){
            mkdir(path_,0777);
            fp_ = fopen(filename, "a");
        }
        assert(fp_ != nullptr);
    }
}

void Log::append_log_level_title(int level){
    switch(level){
        case 0:{
            buffer_.append("[debug]: ",9);
            break;
        }
        case 1:{
            buffer_.append("[info] : ",9);
            break;
        }
        case 2:{
            buffer_.append("[warn] : ",9);
            break;
        }
        case 3:{
            buffer_.append("[error] : ",9);
            break;
        }
        default:{
            buffer_.append("[info] : ",9);
            break;
        }
    }
}

void Log::flush(){
    if(is_aysnc_){
        deque_->flush();
    }
    fflush(fp_);
}

Log* Log::get_instance(){
    static Log inst;
    return &inst;
}

void Log::flush_log_thread(){
    get_instance()->async_write();
}

void Log::async_write() {
    string str = "";
    while(deque_->pop(str)) {
        lock_guard<mutex> locker(mutex_);
        fputs(str.c_str(), fp_);
    }
}

void Log::write(int level,const char* format,...){
    struct timeval now ={0,0};
    gettimeofday(&now,nullptr);//gettimeofday 获取当前时间，并将结果存储在 now 中。
    time_t t_sec = now.tv_sec;
    struct tm* systime = localtime(&t_sec);
    struct tm t = *systime;
    /*localtime 将 time_t 类型的秒数转换为本地时间（struct tm 类型），
    它包含了年、月、日、小时、分钟、秒等信息。
    然后，将 sysTime 中的时间信息拷贝到 t 中，避免直接使用指针。*/
    va_list valist;
    if (today_ != t.tm_mday||(line_count_&&(line_count_ % MAX_LINES))){
        unique_lock<mutex>locker(mutex_);
        locker.unlock();
        char newfile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail,36,"%04d_%02d_%02d",t.tm_year+1900,t.tm_mon+1,t.tm_mday);
        
        if(today_!=t.tm_mday){
            snprintf(newfile,LOG_NAME_LEN-72,"%s/%s%s",path_,tail,suffix_);//suffix_ 是日志文件的后缀
            t.tm_mday =today_;
            line_count_ = 0;
        }else{
            snprintf(newfile,LOG_NAME_LEN-72,"%s/%s-%d%s",path_,tail,(line_count_/MAX_LINES),suffix_);
        }
        locker.lock();
        flush();
        fclose(fp_);
        fp_ = fopen(newfile,"a");
        assert(fp_!=nullptr);
    }

    {
        unique_lock<mutex>locker(mutex_);
        line_count_++;
        int n = snprintf(buffer_.get_write_peek(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                    t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
        buffer_.has_write(n);
        append_log_level_title(level);
        va_start(valist,format);
        int m =vsnprintf(buffer_.get_write_peek(),buffer_.write_able_bytes(),format,valist);
        va_end(valist);
        buffer_.has_write(m);
        buffer_.append("\n\0", 2);

        if(is_aysnc_ && deque_ && !deque_->full()){
            deque_->push_back(buffer_.read_all_to_str_andclear());
        }else{
            fputs(buffer_.get_read_peek(),fp_);
        }
        buffer_.clear_buff();
    }
}