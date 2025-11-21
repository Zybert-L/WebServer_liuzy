#include"Buffer.h"

Buffer::Buffer(int init_buff_size):buffer_(init_buff_size),read_pos(0),write_pos(0){}

size_t Buffer::read_able_bytes()const{
    return write_pos - read_pos;
}

size_t Buffer::write_able_bytes()const{
    return buffer_.size() - write_pos;
}

size_t Buffer::head_able_bytes()const{
    return read_pos;
}

const char* Buffer::get_read_peek()const{
    return begin_ptr() + read_pos;
}

char* Buffer::get_write_peek(){
    return begin_ptr() + write_pos;
}

const char* Buffer::get_write_peek_const()const{
    return begin_ptr() + write_pos;
}

void Buffer::enable_write_able(size_t len){
    if(write_able_bytes() < len){
        expand_space(len);
    }
    assert(write_able_bytes() >= len);
}

void Buffer::has_write(size_t len){
    write_pos += len;
}

void Buffer::has_read(size_t len){
    assert(len <= read_able_bytes());
    read_pos += len;
}

void Buffer::read_to_where(const char* end){
    assert(get_read_peek()<=end);
    has_read(end-get_read_peek());
}

void Buffer::clear_buff(){
    fill(buffer_.begin(),buffer_.end(),0);
    read_pos = 0;
    write_pos = 0;
}

string Buffer::read_all_to_str_andclear(){
    string str(get_read_peek(),read_able_bytes());
    clear_buff();
    return str;
}

void Buffer::append(const char* str,size_t len){
    assert(str);
    enable_write_able(len);
    copy(str,str+len,get_write_peek());
    has_write(len);
}

void Buffer::append(const Buffer& buff){
    append(buff.get_read_peek(), buff.read_able_bytes());
}

void Buffer::append(const void* data, size_t len){
    assert(data);
    append(static_cast<const char*>(data), len);
}

void Buffer::append(const std::string& str) {
    append(str.data(), str.length());
}

void Buffer::expand_space(size_t len){
    if(write_able_bytes()+head_able_bytes()<len){
        buffer_.resize(write_pos + len +1);
    }else{
        size_t read_able = read_able_bytes();
        copy(begin_ptr()+read_pos,begin_ptr()+write_pos,begin_ptr());
        read_pos = 0;
        write_pos = read_able;
    }
}

ssize_t Buffer::read_fd(int fd, int* saveErrno){
    char buff[65535];
    struct iovec iov[2];
    const size_t write_able = write_able_bytes();
    iov[0].iov_base = begin_ptr() + write_pos;
    iov[0].iov_len = write_able;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len =readv(fd,iov,2);
    if(len<0){
        *saveErrno = errno;
    }else if(static_cast<size_t>(len) <= write_able){
        write_pos += len;
    }else {
        write_pos = buffer_.size();
        append(buff,len-write_able);
    }
    return len;
}

ssize_t Buffer::write_fd(int fd, int* saveErrno){
    size_t readsize = read_able_bytes();
    ssize_t len = write(fd,get_read_peek(),readsize);
    if(len < 0){
        *saveErrno = errno;
        return len;
    }
    read_pos +=len;
    return len; 
}


char* Buffer::begin_ptr(){
    return buffer_.data();
}

const char* Buffer::begin_ptr()const{
    return buffer_.data();
}

