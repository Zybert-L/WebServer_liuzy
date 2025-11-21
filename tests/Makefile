# 编译器设置
CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread

# 包含路径
INCLUDES = -I../code

# Buffer 源文件
BUFFER_SRC = ../code/Buffer/Buffer.cpp

# 测试文件
TEST_BUFFER = Buffer/buffer_test.cpp

# 可执行文件目标
all: test_buffer

test_buffer: $(TEST_BUFFER) $(BUFFER_SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o test_buffer $(TEST_BUFFER) $(BUFFER_SRC) -lgtest_main -lgtest -lpthread

run_tests: all
	./test_buffer

clean:
	rm -f test_buffer

.PHONY: all run_tests clean