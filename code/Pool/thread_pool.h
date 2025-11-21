#pragma once
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <cassert>
#include <thread>
using namespace std;

class thread_pool{
public:
    explicit thread_pool(size_t thread_count = 8):pool_(make_shared<pool>())
    {
        assert (thread_count > 0);
        for(size_t i = 0 ; i < thread_count ; i++){
            thread([pool = pool_]{
                unique_lock<mutex>locker(pool->mutex_);
                while(true){
                    if(!pool->task.empty()){
                        auto task = move(pool->task.front());
                        pool->task.pop();
                        locker.unlock();
                        task();
                        locker.lock();
                    }else if(pool->is_close)break;
                    else{
                        pool->cv.wait(locker);
                    }
                }
            }).detach();
        }
    }
    thread_pool() = default;
    thread_pool(thread_pool&&) = default;
    ~thread_pool(){
        if(static_cast<bool>(pool_)){
            lock_guard<mutex>locker(pool_->mutex_);
            pool_->is_close = true;
        }
        pool_->cv.notify_all();
    }

    template<class T>
    void add_task(T&& task){
        {
            lock_guard<mutex> locker(pool_->mutex_);
            pool_->task.emplace(forward<T>(task));
        }
        pool_->cv.notify_one();
    }


private:
    struct pool{
        mutex mutex_;
        condition_variable cv;
        bool is_close;
        queue<function<void()>>task;
    };
    shared_ptr<pool>pool_;
};