#include "http_response.h"

const unordered_map<string, string> http_response::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const unordered_map<int, string> http_response::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const unordered_map<int, string> http_response::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

http_response::http_response(){
    code_ = -1;
    path_ = src_dir_ = "";
    is_keep_alive_ = false;
    mm_file_ = nullptr;
    mm_file_stat_ = {0};
}

http_response::~http_response(){
    unmap_file();
}

void http_response::init(const string& src_dir, string& path, bool is_keep_alive, int code){
    assert(src_dir!="");
    if(mm_file_){
        unmap_file();
    }
    code_ = code;
    is_keep_alive_ = is_keep_alive;
    path_ = path;
    src_dir_ = src_dir;
    mm_file_ = nullptr;
    mm_file_stat_ = { 0 };
}

void http_response::make_response(Buffer& buff){
    //检查请求资源是否存在以及是否为目录
    if(stat((src_dir_ + path_).data(),&mm_file_stat_) < 0 || S_ISDIR(mm_file_stat_.st_mode)){
        code_ = 404;
    }
    //检查文件是否对“其他用户”（others）具有读取权限
    else if(!(mm_file_stat_.st_mode & S_IROTH)){
        code_ = 403;
    }
    else if(code_ == -1){
        code_ = 200;
    }
    error_html();
    add_state_line(buff);
    add_header(buff);
    add_content(buff);
}
/*void http_response::make_response(Buffer& buff){
    // 简化版本，用于调试
    
    // 检查文件
    string full_path = src_dir_ + path_;
    if(stat(full_path.data(), &mm_file_stat_) < 0 || S_ISDIR(mm_file_stat_.st_mode)){
        // 发送404响应
        buff.append("HTTP/1.1 404 Not Found\r\n");
        buff.append("Connection: close\r\n");
        buff.append("Content-type: text/html\r\n");
        
        string body = "<html><body><h1>404 Not Found</h1></body></html>";
        buff.append("Content-length: " + to_string(body.length()) + "\r\n");
        buff.append("\r\n");
        buff.append(body);
        return;
    }
    
    // 文件存在，准备发送
    
    // 1. 状态行 - 确保格式正确
    buff.append("HTTP/1.1 200 OK\r\n");
    
    // 2. 响应头
    buff.append("Connection: ");
    if(is_keep_alive_) {
        buff.append("keep-alive\r\n");
    } else {
        buff.append("close\r\n");
    }
    
    // 3. Content-Type
    buff.append("Content-type: " + get_file_type() + "\r\n");
    
    // 4. 打开文件并映射
    int srcfd = open(full_path.data(), O_RDONLY);
    if(srcfd < 0){
        LOG_ERROR("Cannot open file: %s", full_path.c_str());
        return;
    }
    
    void* mmret = mmap(0, mm_file_stat_.st_size, PROT_READ, MAP_PRIVATE, srcfd, 0);
    close(srcfd);
    
    if (mmret == MAP_FAILED) {
        LOG_ERROR("mmap failed for file: %s", full_path.c_str());
        return;
    }
    
    mm_file_ = (char*)mmret;
    
    // 5. Content-Length 和结束头部
    buff.append("Content-length: " + to_string(mm_file_stat_.st_size) + "\r\n");
    buff.append("\r\n");  // 空行结束头部
    
    LOG_DEBUG("Response headers sent, file size: %ld", (long)mm_file_stat_.st_size);
}*/

char* http_response::file(){
    return mm_file_;
}

size_t http_response::file_len()const{
    return mm_file_stat_.st_size;
}

void http_response::error_html(){
    if(CODE_PATH.count(code_)==1){
        path_ = CODE_PATH.find(code_)->second;
        //用于获取文件的元数据（例如大小、权限、类型等），并将结果存储在 mmFileStat_ 中
        stat((src_dir_+path_).data(),&mm_file_stat_);
    }
}

void http_response::add_state_line(Buffer& buff){
    string status;

    if(CODE_STATUS.count(code_)==1){
        status = CODE_STATUS.find(code_)->second;
    }else{
        code_ = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buff.append("HTTP/1.1 "+to_string(code_)+" "+status+"\r\n");
}

void http_response::add_header(Buffer& buff){
    buff.append("Connection: ");
    if(is_keep_alive_){
        buff.append("keep-alive\r\n");
        buff.append("Keep-Alive: max=6, timeout=120\r\n");
    }else{
        buff.append("close\r\n");
    }
    buff.append("Content-type: "+get_file_type()+"\r\n");
    LOG_DEBUG("Response Connection for fd[unknown]: %s", is_keep_alive_ ? "keep-alive" : "close");
}

/*void http_response::add_content(Buffer& buff){
    int srcfd = open((src_dir_ + path_).data(),O_RDONLY);
    if(srcfd<0){
        error_content(buff,"File Notfound!");
        return;
    }
    int* mmret=(int*)mmap(0,mm_file_stat_.st_size,PROT_READ,MAP_PRIVATE,srcfd,0);
    if (*mmret == -1) {
        error_content(buff, "File NotFound!");
        return;
    }
    mm_file_ = (char*)mmret;//这里是给外部使用的，在类中写了两个外部接口
    close(srcfd);
    buff.append("Content-length: " + to_string(mm_file_stat_.st_size)+"\r\n");
    buff.append("\r\n");
}*/

// 修复 http_response.cpp 中的 add_content 函数

void http_response::add_content(Buffer& buff){
    int srcfd = open((src_dir_ + path_).data(), O_RDONLY);
    if(srcfd < 0){
        error_content(buff, "File NotFound!");
        return;
    }
    
    // 修复1: 正确使用 mmap
    void* mmret = mmap(0, mm_file_stat_.st_size, PROT_READ, MAP_PRIVATE, srcfd, 0);
    close(srcfd);
    
    // 修复2: 正确检查 mmap 返回值
    if(mmret == MAP_FAILED) {  // 不是 *mmret == -1
        error_content(buff, "File NotFound!");
        return;
    }
    
    mm_file_ = (char*)mmret;
    
    // 添加 Content-length 头
    buff.append("Content-length: " + to_string(mm_file_stat_.st_size) + "\r\n");
    // 修复3: 添加空行结束 HTTP 头部（非常重要！）
    buff.append("\r\n");
}

void http_response::unmap_file(){
    if(mm_file_){
        munmap(mm_file_,mm_file_stat_.st_size);
        mm_file_ = nullptr;
    }
}

string http_response::get_file_type(){
    //找到文件路径中最后一个点（.）的位置，用于提取文件后缀。
    string::size_type idx = path_.find_last_of('.');
    //如果路径中没有 .（即没有后缀），返回默认 MIME 类型 text/plain
    if(idx == string::npos) {
        return "text/plain";
    }
    string suffix = path_.substr(idx);
    if(SUFFIX_TYPE.count(suffix)==1){
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

/*void http_response::error_content(Buffer& buff,string message){
    string body;
    string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(code_)==1){
        status = CODE_STATUS.find(code_)->second; 
    }else{
        status = "Bad Request";
    }

    body += to_string(code_)+" : "+status+"\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>Lzy_WebServer</em></body></html>";
    buff.append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
    //第一个 \r\n 用于结束当前头部行（例如 Content-Length 后的行）。
    //第二个 \r\n 用来 标记头部结束，并告诉服务器或客户端，接下来将是响应体的内容。
    buff.append(body);
}*/

// 同时修复 error_content 函数，确保格式正确
void http_response::error_content(Buffer& buff, string message){
    string body;
    string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(code_) == 1){
        status = CODE_STATUS.find(code_)->second; 
    }else{
        status = "Bad Request";
    }

    body += to_string(code_) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>Lzy_WebServer</em></body></html>";
    
    buff.append("Content-length: " + to_string(body.size()) + "\r\n");
    buff.append("\r\n");  // 空行结束头部
    buff.append(body);
}
