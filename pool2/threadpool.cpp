#include <iostream>
#include <functional>
#include <thread>
#include <memory>

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

// 任务提交
void ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
    std::unique_lock<std::mutex> lock(mtx_);

    if (!notFull_.wait_for(lock, std::chrono::seconds(1),
        [&]()->bool { return taskQue_.size() < taskSizeShreshold_; }))
    {
        std::cerr << "the task queue is full, task submit failed." << std::endl;
        return;
    }

    taskQue_.emplace(sp);
    queSize_++;
    notEmpty_.notify_all();
}

// 开启线程池
void ThreadPool::start(int size)
{
    initThreadSize_ = size;

    for (int i = 0; i < initThreadSize_; i++)
    {
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this));
        threads_.emplace_back(std::move(ptr));
    }

    for (int i = 0; i < initThreadSize_; i++)
    {
        threads_[i]->start();
    }
}

// 任务函数
void ThreadPool::threadFunc()
{

    for (; ;)
    {
        std::shared_ptr<Task> task;
        {
            std::unique_lock<std::mutex> lock(mtx_);
            std::cout << std::this_thread::get_id() << "尝试获取任务: " << std::endl;

            notEmpty_.wait(lock, [&]()->bool {return taskQue_.size() > 0; });
            task = taskQue_.front();
            taskQue_.pop();
            queSize_--;
            std::cout << std::this_thread::get_id() << "获取任务成功: " << std::endl;

            if (queSize_ > 0)
                notEmpty_.notify_all();
            notFull_.notify_all();
        }

        if (task != nullptr)
        {
            task->run();
        }
    }
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