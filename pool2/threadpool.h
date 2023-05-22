#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <iostream>
#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>

enum class PoolMode
{
    MODE_FIXED,     // 线程数量不变
    MODE_CACHED     // 线程数量可动态变化
};

// 任务积基类
class Task
{
public:
    // 任务函数设为纯虚函数，可接受任意类型任务函数
    virtual void run() = 0;

private:
};

// 线程类
class Thread
{
public:

    using threadFunc = std::function<void()>;
public:
    Thread(threadFunc func);

    ~Thread();

    void start();

private:
    threadFunc func_;
};

// 线程池类
class ThreadPool
{
public:
    ThreadPool();

    ~ThreadPool();

    // 设置任务数量的最大阈值
    void setShreshold(int maxsize);

    // 设置线程池模式
    void setMode(PoolMode mode);

    // 开启线程池
    void start(int size = 4);

    // 任务提交
    void submitTask(std::shared_ptr<Task> sp);

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    // 任务函数
    void threadFunc();

private:
    std::vector<std::unique_ptr<Thread>> threads_;  // 线程列表
    int initThreadSize_;    // 线程初始数量
    std::queue<std::shared_ptr<Task>> taskQue_;     // 任务队列
    std::atomic_int queSize_;   // 任务队列的大小
    int taskSizeShreshold_;     // 任务队列数量的最大阈值

    std::mutex mtx_;
    std::condition_variable notFull_;
    std::condition_variable notEmpty_;

    PoolMode mode_; // 线程池的模式
};

#endif