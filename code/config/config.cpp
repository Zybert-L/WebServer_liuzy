#include "config.h"
#include <c++/10/bits/algorithmfwd.h>

using namespace std;
Config::Config(const string& filename){
    Parse(filename);
}

void Config::Parse(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open config file: %s", filename.c_str());
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        // 去除首尾空白
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        // 跳过空行和注释
        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }

        // 解析 key=value
        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) {
            LOG_WARN("Invalid config line: %s", line.c_str());
            continue;
        }

        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);

        // 去除 key 和 value 的首尾空白
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        // 存储键值对
        config_[key] = value;
    }

    file.close();
    LOG_INFO("Config file %s loaded successfully", filename.c_str());
}

int Config::GetInt(const string& key,int defaultValue)const
{
    auto it = config_.find(key);
    if(it == config_.end()){
        LOG_WARN("Config key %s not found, using default: %d", key.c_str(), defaultValue);
        return defaultValue;
    }
    try{
        return stoi(it->second);
    }catch(...){
        LOG_WARN("Failed to parse %s as int, using default: %d", key.c_str(), defaultValue);
        return defaultValue;
    }
}

string Config::GetString(const string& key, string defaultValue) const {
    auto it = config_.find(key);
    if (it == config_.end()) {
        LOG_WARN("Config key %s not found, using default: %s", key.c_str(), defaultValue.c_str());
        return defaultValue;
    }
    return it->second;
}

bool Config::GetBool(const std::string& key, bool defaultValue) const {
    auto it = config_.find(key);
    if (it == config_.end()) {
        LOG_WARN("Config key %s not found, using default: %s", key.c_str(), defaultValue ? "true" : "false");
        return defaultValue;
    }
    std::string value = it->second;
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    if (value == "true" || value == "1" || value == "yes") {
        return true;
    }
    if (value == "false" || value == "0" || value == "no") {
        return false;
    }
    LOG_WARN("Failed to parse %s as bool, using default: %s", key.c_str(), defaultValue ? "true" : "false");
    return defaultValue;
}



