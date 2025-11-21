#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include <deque>
#include <mutex>
#include <condition_variable>
#include <sys/time.h>

using namespace std;

template <class T>
class block_queue{
public:
    //构造函数
    explicit block_queue(size_t MaxCapacity = 1000);

    //析构函数
    ~block_queue();

    //清空
    void clear();

    //是否为空
    bool empty();

    //是否满了
    bool full();

    //关闭阻塞队列
    void close();

    //求队列的大小
    size_t size();

    //队列的容量
    size_t capacity();

    //最前面元素
    T front();

    //末尾元素
    T back();

    //从尾部插入元素
    void push_back(const T& item);

    //从头部插入元素
    void push_front(const T& item);
    
    //弹出元素
    bool pop (T &item);

    //弹出元素，并带有超时时间
    bool pop(T &item, int timeout);

    //唤醒一个等待的消费者线程
    void flush();
private:
    deque<T> deque_;                                //双端队列
    size_t capacity_;                               //容量
    mutex mutex_;                                   //互斥锁
    condition_variable consumer_cond_var;           //消费者条件变量
    condition_variable producer_cond_var;           //生产者条件变量
    bool is_close;                                  //是否关闭
};

template<class T>
block_queue<T>::block_queue(size_t MaxCapacity):capacity_(MaxCapacity){
    assert(MaxCapacity > 0);
    is_close = false;
}
template<class T>
block_queue<T>::~block_queue(){
    close();
}

template<class T>
void block_queue<T>::clear(){
    lock_guard<mutex> lock(mutex_);
    deque_.clear();
}

template<class T>
bool block_queue<T>::empty(){
    lock_guard<mutex> lock(mutex_);
    return deque_.empty();
}

template<class T>
bool block_queue<T>::full(){
    lock_guard<mutex> lock(mutex_);
    return deque_.size()>=capacity_;
}

template<class T>
void block_queue<T>::close(){
    {
        lock_guard<mutex> lock(mutex_);
        deque_.clear();
        is_close = true;
    }
    producer_cond_var.notify_all();
    consumer_cond_var.notify_all();
}

template<class T>
size_t block_queue<T>::size(){
    lock_guard<mutex> lock(mutex_);
    return deque_.size();
}

template<class T>
size_t block_queue<T>::capacity(){
    lock_guard<mutex> lock(mutex_);
    return capacity_;
}

template<class T>
T block_queue<T>::front(){
    lock_guard<mutex> lock(mutex_);
    return deque_.front();
}

template<class T>
T block_queue<T>::back(){
    lock_guard<mutex> lock(mutex_);
    return deque_.back();
}

template<class T>
void block_queue<T>::push_back(const T&item){
    unique_lock<mutex> lock(mutex_);
    while(deque_.size() >= capacity_) {
        producer_cond_var.wait(lock);
    }
    deque_.push_back(item);
    consumer_cond_var.notify_one();
}

template<class T>
void block_queue<T>::push_front(const T&item){
    unique_lock<mutex> lock(mutex_);
    while(deque_.size() >= capacity_) {
        producer_cond_var.wait(lock);
    }
    deque_.push_front(item);
    consumer_cond_var.notify_one();
}

//避免返回值拷贝，提高性能（特别是 T 是复杂对象时）。
template<class T>
bool block_queue<T>::pop(T &item) {
    unique_lock<mutex> lock(mutex_);
    while(deque_.empty()){
        consumer_cond_var.wait(lock);
        if(is_close){
            return false;
        }
    }
    item = deque_.front();
    deque_.pop_front();
    consumer_cond_var.notify_one();
    return true;
}

//避免返回值拷贝，提高性能（特别是 T 是复杂对象时）
template<class T>
bool block_queue<T>::pop(T &item,int timeout) {
    unique_lock<mutex> lock(mutex_);
    while(deque_.empty()){
        if(consumer_cond_var.wait_for(lock, std::chrono::seconds(timeout)) 
        == std::cv_status::timeout){
            return false;
        }
        if(is_close){
            return false;
        }
    }
    item = deque_.front();
    deque_.pop_front();
    consumer_cond_var.notify_one();
    return true;
}

template<class T>
void block_queue<T>::flush() {
    consumer_cond_var.notify_one();
};

#endif