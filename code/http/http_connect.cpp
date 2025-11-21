#include "http_connect.h"

const char* http_connect::src_dir;
atomic<int>http_connect::user_count;
bool http_connect::is_et;

http_connect::http_connect(){
    fd_ = -1;
    addr_ = {0};
    is_close_ = true;
}

http_connect::~http_connect(){
    close_();
}

void http_connect::init(int fd,const sockaddr_in& addr){
    assert(fd>0);
    user_count++;
    addr_ = addr;
    fd_ = fd;
    write_buff_.clear_buff();
    read_buff_.clear_buff();
    is_close_ = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, get_ip(), get_port(), (int)user_count);
}

void http_connect::close_(){
    response_.unmap_file();
    if(is_close_ == false){
        is_close_ = true;
        user_count--;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_,get_ip(),get_port(),(int)user_count);
    }
}

int http_connect::get_fd()const{
    return fd_;
}

struct sockaddr_in http_connect::get_addr()const{
    return addr_;
}

const char* http_connect::get_ip()const{
    return inet_ntoa(addr_.sin_addr);
}

int http_connect::get_port()const{
    return addr_.sin_port;
}

ssize_t http_connect::read(int* save_errno){
    ssize_t len = -1;
    do{
        len = read_buff_.read_fd(fd_,save_errno);
        if(len<=0){
            LOG_DEBUG("read_fd returned %zd, errno: %d (%s)", len, *save_errno, strerror(*save_errno));
            break;
        }
    }while(is_et);
    return len;
}

bool http_connect::process(){
    request_.init();
    if(read_buff_.read_able_bytes() <= 0){
        LOG_DEBUG("No data in read buffer");
        return false;
    }else if(request_.parse(read_buff_)){
        LOG_DEBUG("%s", request_.path().c_str());
        response_.init(src_dir, request_.path(), request_.is_keep_alive(), 200);
    }else{
        response_.init(src_dir, request_.path(), false, 400);
    }

    response_.make_response(write_buff_);
    
    // 设置响应头数据
    iov_[0].iov_base = const_cast<char*>(write_buff_.get_read_peek());
    iov_[0].iov_len = write_buff_.read_able_bytes();
    iov_cnt_ = 1;

    // 设置文件数据
    if(response_.file_len() > 0 && response_.file()){
        iov_[1].iov_base = response_.file();
        iov_[1].iov_len = response_.file_len();
        iov_cnt_ = 2;
    }
    
    LOG_DEBUG("filesize:%d, iov_cnt:%d, to_write:%d", 
              response_.file_len(), iov_cnt_, to_write_bytes());
    return true;
}

ssize_t http_connect::write(int* save_errno){
    ssize_t len = -1;
    ssize_t total_written = 0;
    
    const size_t MAX_WRITE_PER_CALL = 65536; // 64KB
    
    do{
        // 计算本次要写入的字节数
        size_t to_write = to_write_bytes();
        if(to_write > MAX_WRITE_PER_CALL && !is_et) {
            // 在LT模式下，限制每次写入的数据量
            struct iovec temp_iov[2];
            int temp_cnt = 0;
            size_t remaining = MAX_WRITE_PER_CALL;
            
            if(iov_[0].iov_len > 0) {
                temp_iov[0] = iov_[0];
                if(temp_iov[0].iov_len > remaining) {
                    temp_iov[0].iov_len = remaining;
                    remaining = 0;
                } else {
                    remaining -= temp_iov[0].iov_len;
                }
                temp_cnt = 1;
            }
            
            if(remaining > 0 && iov_cnt_ > 1 && iov_[1].iov_len > 0) {
                temp_iov[1] = iov_[1];
                temp_iov[1].iov_len = (remaining < iov_[1].iov_len) ? remaining : iov_[1].iov_len;
                temp_cnt = 2;
            }
            
            len = writev(fd_, temp_iov, temp_cnt);
        } else {
            len = writev(fd_, iov_, iov_cnt_);
        }
        
        if(len <= 0) {
            *save_errno = errno;
            LOG_DEBUG("writev on fd[%d] returned %zd, errno: %d (%s)", 
                      fd_, len, *save_errno, strerror(*save_errno));
            
            if(*save_errno == EAGAIN || *save_errno == EWOULDBLOCK) {
                // 套接字缓冲区满了，返回已写入的字节数
                return total_written > 0 ? total_written : -1;
            }
            break;
        }
        
        total_written += len;
        
        // 更新iov数组
        if(iov_[0].iov_len + iov_[1].iov_len == 0){
            break;
        }
        else if(static_cast<size_t>(len) > iov_[0].iov_len){
            size_t written_from_file = len - iov_[0].iov_len;
            iov_[1].iov_base = (uint8_t*)iov_[1].iov_base + written_from_file;
            iov_[1].iov_len -= written_from_file;
            if(iov_[0].iov_len){
                write_buff_.has_read(iov_[0].iov_len);
                iov_[0].iov_len = 0;
            }
        }else{
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len;
            iov_[0].iov_len -= len; 
            write_buff_.has_read(len);
        }
        
        // 在LT模式下，写入一定量数据后返回，避免阻塞太久
        if(!is_et && total_written >= MAX_WRITE_PER_CALL) {
            break;
        }
        
    }while(is_et || to_write_bytes() > 0);
    
    return total_written > 0 ? total_written : len;
}
