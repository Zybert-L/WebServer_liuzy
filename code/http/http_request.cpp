
/*#include "http_request.h"

const unordered_set<string> http_request::DEFAULT_HTML{
    "/index", "/register", "/login",
     "/welcome", "/video", "/picture", };

const unordered_map<string, int> http_request::DEFAULT_HTML_TAG {
    {"/register.html", 0}, {"/login.html", 1},  };


void http_request::init(){
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool http_request::parse(Buffer &buff){
    const char CRLF[] = "\r\n";
    if(buff.read_able_bytes() <= 0){
        return false;
    }
    while(buff.read_able_bytes() && state_ != FINISH){
        const char* line_end = search(buff.get_read_peek(),buff.get_write_peek_const(),CRLF,CRLF+2);
        string line(buff.get_read_peek(),line_end);
        switch(state_){
            case REQUEST_LINE:{
                if(!parse_request_line(line)){
                    return false;
                }
                parse_path();
                break;
            }
            case HEADERS:{
                parse_header(line);
                if(buff.read_able_bytes()<=2){
                    state_ = FINISH;
                }
                break;
            }
            case BODY:{
                parse_body(line);
                break;
            }
            default:
                break;
        }
        if(line_end == buff.get_write_peek()){break;}
        buff.read_to_where(line_end + 2); // 移除已解析的数据（包括 `\r\n`）
    }
     // 调试头部解析结果
    for (const auto& header : header_) {
        LOG_DEBUG("Header: %s: %s", header.first.c_str(), header.second.c_str());
    }
    // 检查 Keep-Alive
    bool is_keep_alive_ = is_keep_alive();
    LOG_DEBUG("Request parsed, method: [%s], path: [%s], version: [%s], is_keep_alive: %d",
              method_.c_str(), path_.c_str(), version_.c_str(), is_keep_alive_);
    return true; 
}


string http_request::path() const{
    return path_;
}

string& http_request::path(){
    return path_;
}

string http_request::method() const {
    return method_;
}

string http_request::version() const {
    return version_;
}

string http_request::get_post(const string& key)const{
    assert(key != "");
    if(post_.count(key) == 1){
        return post_.find(key)->second;
    }
    return "";
}

string http_request::get_post(const char* key) const {
    assert(key != nullptr);
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

bool http_request::is_keep_alive() const{
    if(header_.count("Connection")==1){
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool http_request::parse_request_line(const string& line) {
    regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch sub_match;
    if (regex_match(line, sub_match, pattern)) {
        method_ = sub_match[1];
        path_ = sub_match[2];
        version_ = sub_match[3]; // 捕获 "1.1" 而不是 "/1.1"
        state_ = HEADERS;
        LOG_DEBUG("Parsed request line: method=%s, path=%s, version=%s",
                  method_.c_str(), path_.c_str(), version_.c_str());
        return true;
    }
    LOG_ERROR("request_line error: %s", line.c_str());
    return false;
}

/*void http_request::parse_header(const string& line){
    regex pattern("^([^:]*): ?(.*)$");
    smatch sub_match;
    if(regex_match(line, sub_match, pattern)){
        header_[sub_match[1]] = sub_match[2];
    }else{
        state_ = BODY;
    }
}

void http_request::parse_body(const string& line){
    body_ = line;
    parse_post();
    state_ = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

void http_request::parse_path(){
    if(path_ == "/"){
        path_ = "/index.html";
    }else{
        for(auto &item:DEFAULT_HTML){
            if(item == path_){
                path_ += ".html";
                break;
            }
        }
    }
}*/
/*
void http_request::parse_header(const string& line){
    if(line.empty()){
        // 空行，头部解析结束
        return;
    }
    
    regex pattern("^([^:]*): ?(.*)$");
    smatch sub_match;
    if(regex_match(line, sub_match, pattern)){
        string key = sub_match[1];
        string value = sub_match[2];
        // 去除前后空白
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        header_[key] = value;
    }
}

void http_request::parse_body(const string& line){
    body_ = line;
    parse_post();
    state_ = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

// 确保 path 解析正确
void http_request::parse_path(){
    if(path_ == "/"){
        path_ = "/index.html";
    } else {
        for(auto &item : DEFAULT_HTML){
            if(item == path_){
                path_ += ".html";
                break;
            }
        }
    }
}

void http_request::parse_from_url_encoded(){
    if(body_.size() == 0){
        return;
    }
    string key,value;
    int num = 0;
    int n = body_.size();
    int i = 0,j = 0;
    
    for(;i<n;i++){
        char ch = body_[i];
        switch (ch){
            case '=':{
                key = body_.substr(j,i-j);
                j = i + 1;
                break;
            }
            case '+':{
                body_[i] = ' ';
                break;
            }
            case '%':{
                num = ConverHex(body_[i+1])*16 + ConverHex(body_[i+2]);
                body_[i+2] = num % 10 +'0';
                body_[i+1] = num / 10 +'0';
                i +=2;
                break;
            }
            case '&':{
                value = body_.substr(j,i-j);
                j=j+1;
                post_[key] = value;
                LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
                break;
            }
            default:
                break;
        }
    }
    assert(j<=i);
    if(post_.count(key) == 0 && j<i){
        value = body_.substr(j,i-j);
        post_[key] = value;
    }
}*/

/*void http_request::parse_post(){
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded"){
        parse_from_url_encoded();
        if(DEFAULT_HTML_TAG.count(path_)){
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag:%d", tag);
            if( tag == 1 || tag == 0 ){
                bool is_login = (tag == 1);
                if(user_verify(post_["username"], post_["password"], is_login)){
                    path_ = "/welcome.html";
                }else{
                    path_ = "/error.html";
                }
            }
        }
    }
}*/

/*void http_request::parse_post(){
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded"){
        parse_from_url_encoded();
        if(DEFAULT_HTML_TAG.count(path_)){
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag:%d", tag);
            if( tag == 1 || tag == 0 ){
                bool is_login = (tag == 1);
                if(user_verify(post_["username"], post_["password"], is_login)){
                    path_ = "/welcome.html";
                } else {
                    path_ = "/error.html";
                }
            }
        }
    }
}

int http_request::ConverHex(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
    return ch;
}

bool http_request::user_verify(const string &name, const string &pwd, bool is_login){
    if (name == "" || pwd == "") { return false; }
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());

    mysql_connection_pool* pool = mysql_connection_pool::get_connection_pool();
    auto conn = pool->get_connection();
    if (!conn) {
        LOG_ERROR("Failed to get connection from pool");
        return false;
    }

    bool flag = false;
    char order[256]= {0};

    if(!is_login){flag = true;}

    snprintf(order,256,"SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);

    if(conn->query(order)){
        LOG_ERROR("Query failed");
        return false;
    }

    vector<vector<string>>result = conn->get_result();
    for(const auto&row :result){
        LOG_ERROR("MYSQL ROW: %s %s", row[0].c_str(), row[1].c_str());
        string password = row[1];
        if(is_login){
            if(pwd == password){
                flag = true;
            }
            else{
                flag = false;
                LOG_DEBUG("pwd error!");
            }
        }else{
            flag = false;
            LOG_DEBUG("user used!");
        }
    }

    if(!is_login && flag ==true){
        LOG_DEBUG("register!");
        bzero(order,256);
        snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG("%s", order);

        if(!conn->update(order)){
            LOG_DEBUG("Insert error!");
            flag = false;
        }else{
            flag = true;
        }
    }
    LOG_DEBUG("UserVerify success!!");
    return flag;
}*/


#include "http_request.h"

const unordered_set<string> http_request::DEFAULT_HTML{
    "/index", "/register", "/login",
     "/welcome", "/video", "/picture", };

const unordered_map<string, int> http_request::DEFAULT_HTML_TAG {
    {"/register.html", 0}, {"/login.html", 1},  };

void http_request::init(){
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool http_request::parse(Buffer &buff){
    const char CRLF[] = "\r\n";
    if(buff.read_able_bytes() <= 0){
        return false;
    }
    
    while(buff.read_able_bytes() && state_ != FINISH){
        if(state_ == BODY){
            // **关键修复**: 正确处理POST请求体
            auto it = header_.find("Content-Length");
            if(it != header_.end()){
                int content_length = std::stoi(it->second);
                LOG_DEBUG("Content-Length: %d, available: %zu", content_length, buff.read_able_bytes());
                
                if(buff.read_able_bytes() >= content_length){
                    // 读取指定长度的请求体
                    body_ = string(buff.get_read_peek(), content_length);
                    buff.has_read(content_length);
                    parse_post();
                    state_ = FINISH;
                    LOG_DEBUG("Body parsed: %s, len: %d", body_.c_str(), body_.size());
                } else {
                    // 数据还没有完全到达，等待更多数据
                    LOG_DEBUG("Waiting for more body data: need %d, have %zu", 
                              content_length, buff.read_able_bytes());
                    return false;
                }
            } else {
                // 没有Content-Length，可能是GET请求或者格式错误
                LOG_WARN("No Content-Length header for POST request");
                state_ = FINISH;
            }
        } else {
            // 处理请求行和头部
            const char* line_end = search(buff.get_read_peek(), buff.get_write_peek_const(), CRLF, CRLF+2);
            if(line_end == buff.get_write_peek_const()){
                // 没有找到完整的行，等待更多数据
                return false;
            }
            
            string line(buff.get_read_peek(), line_end);
            
            switch(state_){
                case REQUEST_LINE:{
                    if(!parse_request_line(line)){
                        return false;
                    }
                    parse_path();
                    break;
                }
                case HEADERS:{
                    if(line.empty()){
                        // **关键修复**: 空行表示头部结束
                        if(method_ == "POST"){
                            state_ = BODY;
                        } else {
                            state_ = FINISH;
                        }
                    } else {
                        parse_header(line);
                    }
                    break;
                }
                default:
                    break;
            }
            buff.read_to_where(line_end + 2); // 移除已解析的数据（包括 `\r\n`）
        }
    }
    
    // 调试头部解析结果
    for (const auto& header : header_) {
        LOG_DEBUG("Header: %s: %s", header.first.c_str(), header.second.c_str());
    }
    
    // 检查 Keep-Alive
    bool is_keep_alive_ = is_keep_alive();
    LOG_DEBUG("Request parsed, method: [%s], path: [%s], version: [%s], is_keep_alive: %d",
              method_.c_str(), path_.c_str(), version_.c_str(), is_keep_alive_);
    return true; 
}

string http_request::path() const{
    return path_;
}

string& http_request::path(){
    return path_;
}

string http_request::method() const {
    return method_;
}

string http_request::version() const {
    return version_;
}

string http_request::get_post(const string& key)const{
    assert(key != "");
    if(post_.count(key) == 1){
        return post_.find(key)->second;
    }
    return "";
}

string http_request::get_post(const char* key) const {
    assert(key != nullptr);
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

bool http_request::is_keep_alive() const{
    if(header_.count("Connection")==1){
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool http_request::parse_request_line(const string& line) {
    regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch sub_match;
    if (regex_match(line, sub_match, pattern)) {
        method_ = sub_match[1];
        path_ = sub_match[2];
        version_ = sub_match[3];
        state_ = HEADERS;
        LOG_DEBUG("Parsed request line: method=%s, path=%s, version=%s",
                  method_.c_str(), path_.c_str(), version_.c_str());
        return true;
    }
    LOG_ERROR("request_line error: %s", line.c_str());
    return false;
}

void http_request::parse_header(const string& line){
    if(line.empty()){
        // 空行，头部解析结束
        return;
    }
    
    regex pattern("^([^:]*): ?(.*)$");
    smatch sub_match;
    if(regex_match(line, sub_match, pattern)){
        string key = sub_match[1];
        string value = sub_match[2];
        // 去除前后空白
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        header_[key] = value;
    }
}

void http_request::parse_body(const string& line){
    body_ = line;
    parse_post();
    state_ = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

void http_request::parse_path(){
    if(path_ == "/"){
        path_ = "/index.html";
    } else {
        for(auto &item : DEFAULT_HTML){
            if(item == path_){
                path_ += ".html";
                break;
            }
        }
    }
}

// **关键修复**: 完全重写URL编码解析函数
void http_request::parse_from_url_encoded(){
    if(body_.size() == 0){
        LOG_DEBUG("Empty body for URL decoding");
        return;
    }
    
    LOG_DEBUG("Parsing URL encoded body: %s", body_.c_str());
    
    string key, value;
    int n = body_.size();
    int i = 0, j = 0;
    
    for(; i < n; i++){
        char ch = body_[i];
        switch (ch){
            case '=':{
                key = body_.substr(j, i - j);
                j = i + 1;  // 下一个值的起始位置
                break;
            }
            case '+':{
                body_[i] = ' ';  // 将 + 替换为空格
                break;
            }
            case '%':{
                if(i + 2 < n) {
                    // **修复**: 正确的URL解码
                    int num = ConverHex(body_[i+1]) * 16 + ConverHex(body_[i+2]);
                    body_[i] = static_cast<char>(num);
                    // 移除后面两个字符
                    body_.erase(i + 1, 2);
                    n -= 2;  // 更新字符串长度
                }
                break;
            }
            case '&':{
                value = body_.substr(j, i - j);
                j = i + 1;  // **关键修复**: 应该是 i + 1，不是 j + 1
                if(!key.empty()) {
                    post_[key] = value;
                    LOG_DEBUG("Parsed: %s = %s", key.c_str(), value.c_str());
                    key.clear();
                    value.clear();
                }
                break;
            }
            default:
                break;
        }
    }
    
    // **关键修复**: 处理最后一个键值对
    if(!key.empty() && j <= i){
        value = body_.substr(j, i - j);
        post_[key] = value;
        LOG_DEBUG("Parsed last: %s = %s", key.c_str(), value.c_str());
    }
    
    LOG_DEBUG("URL decoding completed, found %zu pairs", post_.size());
}

void http_request::parse_post(){
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded"){
        parse_from_url_encoded();
        if(DEFAULT_HTML_TAG.count(path_)){
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag:%d for path:%s", tag, path_.c_str());
            if( tag == 1 || tag == 0 ){
                bool is_login = (tag == 1);
                LOG_INFO("Attempting %s for user: %s", is_login ? "login" : "register", post_["username"].c_str());
                
                if(user_verify(post_["username"], post_["password"], is_login)){
                    path_ = "/welcome.html";
                    LOG_INFO("User %s successful", is_login ? "login" : "register");
                } else {
                    path_ = "/error.html";
                    LOG_WARN("User %s failed", is_login ? "login" : "register");
                }
            }
        } else {
            LOG_WARN("Path %s not found in DEFAULT_HTML_TAG", path_.c_str());
        }
    }
}

// **关键修复**: 修复十六进制转换函数
int http_request::ConverHex(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if(ch >= '0' && ch <= '9') return ch - '0';  // **修复**: 添加数字处理
    LOG_WARN("Invalid hex character: %c", ch);
    return 0;  // 默认返回0
}

// **关键修复**: 修复数据库查询逻辑
/*bool http_request::user_verify(const string &name, const string &pwd, bool is_login){
    if (name == "" || pwd == "") { 
        LOG_WARN("Empty username or password");
        return false; 
    }
    
    LOG_INFO("Verify name:%s pwd:%s, is_login:%d", name.c_str(), pwd.c_str(), is_login);

    mysql_connection_pool* pool = mysql_connection_pool::get_connection_pool();
    auto conn = pool->get_connection();
    if (!conn) {
        LOG_ERROR("Failed to get connection from pool");
        return false;
    }

    bool flag = false;
    char order[256]= {0};

    if(!is_login){flag = true;}  // 注册时默认为true

    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("Executing query: %s", order);

    // **关键修复**: 修正查询返回值判断
    if(!conn->query(order)){  // query成功返回true，失败返回false
        LOG_ERROR("Database query failed: %s", order);
        return false;
    }

    vector<vector<string>> result = conn->get_result();
    LOG_DEBUG("Query returned %zu rows", result.size());
    
    for(const auto& row : result){
        LOG_DEBUG("Found user in database: %s", row[0].c_str());
        string password = row[1];
        if(is_login){
            // 登录逻辑：检查密码
            if(pwd == password){
                flag = true;
                LOG_DEBUG("Login password correct");
            } else {
                flag = false;
                LOG_DEBUG("Login password incorrect");
            }
        } else {
            // 注册逻辑：用户已存在
            flag = false;
            LOG_DEBUG("Registration failed - user already exists");
        }
    }

    // 注册新用户
    if(!is_login && flag == true){
        LOG_DEBUG("Proceeding with user registration");
        bzero(order, 256);
        snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG("Executing insert: %s", order);

        // **关键修复**: 修正更新返回值判断
        if(!conn->update(order)){  // update成功返回true，失败返回false
            LOG_ERROR("Database insert failed: %s", order);
            flag = false;
        } else {
            LOG_INFO("User registered successfully");
            flag = true;
        }
    }
    
    LOG_DEBUG("UserVerify result: %s", flag ? "success" : "failed");
    return flag;
}*/

// 改进的 user_verify 函数，增加详细的调试日志
bool http_request::user_verify(const string &name, const string &pwd, bool is_login){
    if (name == "" || pwd == "") { 
        LOG_WARN("Empty username or password");
        return false; 
    }
    
    LOG_INFO("=== Starting user verification ===");
    LOG_INFO("User: %s, Password: %s, Is_login: %d", name.c_str(), pwd.c_str(), is_login);

    // **调试**: 获取数据库连接池
    LOG_DEBUG("Getting connection pool...");
    mysql_connection_pool* pool = mysql_connection_pool::get_connection_pool();
    if(!pool) {
        LOG_ERROR("Failed to get connection pool instance");
        return false;
    }
    LOG_DEBUG("✓ Connection pool obtained");

    // **调试**: 获取数据库连接
    LOG_DEBUG("Getting database connection...");
    auto conn = pool->get_connection();
    if (!conn) {
        LOG_ERROR("Failed to get connection from pool");
        return false;
    }
    LOG_DEBUG("✓ Database connection obtained");

    bool flag = false;
    char order[256]= {0};

    if(!is_login){
        flag = true;  // 注册时默认为true（假设用户不存在）
        LOG_DEBUG("Registration mode: flag set to true initially");
    } else {
        LOG_DEBUG("Login mode: flag set to false initially");
    }

    // **调试**: 构建查询语句
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("Executing query: [%s]", order);

    // **调试**: 执行查询
    bool query_result = conn->query(order);
    LOG_DEBUG("Query execution result: %s", query_result ? "SUCCESS" : "FAILED");
    
    if(!query_result) {
        LOG_ERROR("Database query failed: %s", order);
        return false;
    }

    // **调试**: 获取查询结果
    LOG_DEBUG("Getting query results...");
    vector<vector<string>> result = conn->get_result();
    LOG_DEBUG("Query returned %zu rows", result.size());
    
    // **调试**: 处理查询结果
    if(result.empty()) {
        LOG_DEBUG("No user found in database");
        if(is_login) {
            // 登录时用户不存在
            flag = false;
            LOG_DEBUG("Login failed - user does not exist");
        } else {
            // 注册时用户不存在，保持flag=true
            LOG_DEBUG("Registration can proceed - user does not exist");
        }
    } else {
        // 找到用户
        LOG_DEBUG("User found in database");
        for(const auto& row : result){
            if(row.size() >= 2) {
                LOG_DEBUG("Database user: %s, stored password: %s", row[0].c_str(), row[1].c_str());
                string stored_password = row[1];
                
                if(is_login){
                    // 登录逻辑：检查密码
                    if(pwd == stored_password){
                        flag = true;
                        LOG_DEBUG("✓ Login successful - password matches");
                    } else {
                        flag = false;
                        LOG_DEBUG("✗ Login failed - password does not match");
                        LOG_DEBUG("Input password: [%s]", pwd.c_str());
                        LOG_DEBUG("Stored password: [%s]", stored_password.c_str());
                    }
                } else {
                    // 注册逻辑：用户已存在
                    flag = false;
                    LOG_DEBUG("✗ Registration failed - user already exists");
                }
            } else {
                LOG_ERROR("Invalid database row format");
            }
        }
    }

    // **调试**: 注册新用户
    if(!is_login && flag == true){
        LOG_DEBUG("=== Proceeding with user registration ===");
        bzero(order, 256);
        snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG("Executing insert: [%s]", order);

        bool update_result = conn->update(order);
        LOG_DEBUG("Insert execution result: %s", update_result ? "SUCCESS" : "FAILED");
        
        if(!update_result) {
            LOG_ERROR("Database insert failed: %s", order);
            flag = false;
        } else {
            LOG_INFO("✓ User registered successfully in database");
            flag = true;
        }
    }
    
    LOG_INFO("=== User verification completed ===");
    LOG_INFO("Final result: %s", flag ? "SUCCESS" : "FAILED");
    return flag;
}






