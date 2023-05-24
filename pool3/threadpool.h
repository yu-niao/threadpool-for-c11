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


class Any
{
public:
    Any() = default;
    ~Any() = default;
    Any(const Any&) = delete;
    Any& operator=(const Any&) = delete;
    Any(Any&&) = default;
    Any& operator=(Any&&) = default;

    template<typename T>
    Any(T data)
        : base_(std::make_shared<Derive<T>>(data))
    {}

    template<typename T>
    T cast()
    {
        Derive<T>* dp = dynamic_cast<Derive<T>*>(base_.get());
        if (dp == nullptr)
        {
            throw "type is unmatch";
        }
        return dp->data_;
    }

private:
    class Base
    {
    public:
        virtual ~Base() = default;
    };

    template<typename T>
    class Derive : public Base
    {
    public:
        Derive(T data)
            : data_(data)
        {}

        T data_;
    };

private:
    std::shared_ptr<Base> base_;
};

class Semaphora
{
public:
    Semaphora(int limit = 0)
        : rscLimit_(limit)
    {}

    void wait()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        con_.wait(lock, [&]()-> bool {return rscLimit_ > 0; });
        rscLimit_--;
    }

    void post()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        rscLimit_++;
        con_.notify_all();
    }

private:
    int rscLimit_;
    std::mutex mtx_;
    std::condition_variable con_;
};

enum class PoolMode
{
    MODE_FIXED,     // 线程数量不变
    MODE_CACHED     // 线程数量可动态变化
};

class Result;

// 任务积基类
class Task
{
public:
    Task();
    ~Task() = default;

    void exec();

    void setResult(Result* res);

    // 任务函数设为纯虚函数，可接受任意类型任务函数
    virtual Any run() = 0;

private:
    Result* result_;
};

class Result
{
public:
    Result(std::shared_ptr<Task> sp, bool isValid = true)
        : task_(sp)
        , isValid_(isValid)
    {
        task_->setResult(this);
    }

    Any get();

    void setValue(Any any);

private:
    Any any_;
    Semaphora sem_;
    std::shared_ptr<Task> task_;
    std::atomic_bool isValid_;
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
    Result submitTask(std::shared_ptr<Task> sp);

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