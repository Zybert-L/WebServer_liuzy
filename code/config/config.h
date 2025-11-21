#pragma once

#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include "../Log/Log.h"

class Config{
public:
    explicit Config(const std::string& filename);

    int GetInt(const std::string& key, int defaultValue)const;

    std::string GetString(const std::string& key, std::string defaultValue)const;

    bool GetBool(const std::string& key, bool defaultValue) const;
private:
    std::unordered_map<std::string, std::string> config_;

    void Parse(const std::string& filename);
};