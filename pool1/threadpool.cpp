#include <iostream>
#include <functional>
#include <thread>

#include "threadpool.h"

const int TASK_MAX_SIZE = 1024;

// ------线程池成员函数-----------------

ThreadPool::ThreadPool()
    : initThreadSize_(0)
    , queSize_(0)
    , taskSizeShreshold_(TASK_MAX_SIZE)
    , mode_(PoolMode::MODE_FIXED)
{}

ThreadPool::~ThreadPool()
{}

// 设置任务数量的最大阈值
void ThreadPool::setShreshold(int maxsize)
{
    taskSizeShreshold_ = maxsize;
}

// 设置线程池模式
void ThreadPool::setMode(PoolMode mode)
{
    mode_ = mode;
}

// 开启线程池
void ThreadPool::start(int size)
{
    initThreadSize_ = size;

    for (int i = 0; i < initThreadSize_; i++)
    {
        threads_.emplace_back(new Thread(std::bind(&ThreadPool::threadFunc, this)));
    }

    for (int i = 0; i < initThreadSize_; i++)
    {
        threads_[i]->start();
    }
}

// 任务函数
void ThreadPool::threadFunc()
{
    std::cout << std::this_thread::get_id() << "begin!!" << std::endl;
    std::cout << std::this_thread::get_id() << "end!!" << std::endl;
}


// ------------线程成员函数----------------

Thread::Thread(threadFunc func) : func_(func)
{}

Thread::~Thread()
{}

void Thread::start()
{
    std::thread t(func_);
    t.detach();
}