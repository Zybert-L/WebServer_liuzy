#include <gtest/gtest.h>
#include "../../code/Buffer/Buffer.h"  // 改为这个路径

// 测试Buffer基本功能
TEST(BufferTest, BasicFunctionality) {
    Buffer buffer(1024);
    
    // 测试初始状态
    EXPECT_EQ(buffer.read_able_bytes(), 0);
    EXPECT_GT(buffer.write_able_bytes(), 0);
    
    // 测试写入数据
    std::string test_data = "Hello World";
    buffer.append(test_data);
    EXPECT_EQ(buffer.read_able_bytes(), test_data.length());
    
    // 测试读取数据
    std::string result = buffer.read_all_to_str_andclear();
    EXPECT_EQ(result, test_data);
    EXPECT_EQ(buffer.read_able_bytes(), 0);
}

// 测试Buffer扩容
TEST(BufferTest, Expansion) {
    Buffer buffer(10);  // 很小的缓冲区
    
    std::string large_data(100, 'A');  // 100个'A'
    buffer.append(large_data);
    
    EXPECT_EQ(buffer.read_able_bytes(), large_data.length());
}

// 额外测试：检查其他方法
TEST(BufferTest, AdditionalMethods) {
    Buffer buffer(1024);
    
    // 测试head_able_bytes
    EXPECT_EQ(buffer.head_able_bytes(), 0);  // 初始时head应该为0
    
    // 测试append不同类型
    const char* test_cstr = "Test";
    buffer.append(test_cstr, 4);
    EXPECT_EQ(buffer.read_able_bytes(), 4);
    
    // 测试清空
    buffer.clear_buff();
    EXPECT_EQ(buffer.read_able_bytes(), 0);
}