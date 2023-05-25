#include <iostream>
#include <functional>
#include <thread>
#include <memory>

#include "threadpool.h"

const int TASK_MAX_SIZE = 1024;
const int THREAD_MAX_SIZE = 10;
const int THREAD_MAX_WAIT_TIME = 10;

// -------- 线程池成员函数 -----------------

ThreadPool::ThreadPool()
    : initThreadSize_(0)
    , queSize_(0)
    , taskSizeShreshold_(TASK_MAX_SIZE)
    , mode_(PoolMode::MODE_FIXED)
    , isPoolRunning_(false)
    , currentThreadNum_(0)
    , spareThreadNum_(0)
    , threadMaxSizeShreshold_(THREAD_MAX_SIZE)
{}

ThreadPool::~ThreadPool()
{
    isPoolRunning_ = false;

    // 等待线程池里面所有的线程返回  有两种状态：阻塞 & 正在执行任务中
    std::unique_lock<std::mutex> lock(mtx_);
    notEmpty_.notify_all();
    notExit_.wait(lock, [&]()->bool {return threads_.size() == 0; });
}

// 设置任务数量的最大阈值
void ThreadPool::setShreshold(int maxsize)
{
    if (getPoolStatus())
        return;
    taskSizeShreshold_ = maxsize;
}

// 设置线程池模式
void ThreadPool::setMode(PoolMode mode)
{
    if (getPoolStatus())
        return;
    mode_ = mode;
}

// 设置 cached 模式下线程数量上限阈值
void ThreadPool::setThreadSizeShrehold(int size)
{
    if (getPoolStatus())
        return;
    if (mode_ == PoolMode::MODE_CACHED)
    {
        threadMaxSizeShreshold_ = size;
    }
}

// 获得当前线程池的状态
bool ThreadPool::getPoolStatus() const
{
    return isPoolRunning_;
}

// 任务提交
Result ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
    std::unique_lock<std::mutex> lock(mtx_);

    if (!notFull_.wait_for(lock, std::chrono::seconds(1),
        [&]()->bool { return taskQue_.size() < taskSizeShreshold_; }))
    {
        std::cerr << "the task queue is full, task submit failed." << std::endl;
        return Result(sp, false);
    }

    taskQue_.emplace(sp);
    queSize_++;
    notEmpty_.notify_all();

    if (mode_ == PoolMode::MODE_CACHED
        && queSize_ > spareThreadNum_
        && currentThreadNum_ < threadMaxSizeShreshold_)
    {
        std::cout << "<<< create a new thread..." << std::endl;

        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
        int threadId = ptr->getId();
        threads_.emplace(threadId, std::move(ptr));

        threads_[threadId]->start();

        currentThreadNum_++;
        spareThreadNum_++;
    }

    return Result(sp);
}

// 开启线程池
void ThreadPool::start(int size)
{
    isPoolRunning_ = true;

    initThreadSize_ = size;
    currentThreadNum_ = size;

    for (int i = 0; i < initThreadSize_; i++)
    {
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
        int threadId = ptr->getId();
        threads_.emplace(threadId, std::move(ptr));
    }

    for (int i = 0; i < initThreadSize_; i++)
    {
        threads_[i]->start();
        spareThreadNum_++;
    }
}

// 任务函数
void ThreadPool::threadFunc(int threadId)
{
    auto lastTime = std::chrono::high_resolution_clock().now();
    for (; ;)
    {
        std::shared_ptr<Task> task;
        {
            std::unique_lock<std::mutex> lock(mtx_);
            std::cout << std::this_thread::get_id() << "尝试获取任务: " << std::endl;

            while (queSize_ == 0)
            {
                if (!isPoolRunning_)
                {
                        threads_.erase(threadId);
                        std::cout << "threadid:" << std::this_thread::get_id() << " exit!"
                            << std::endl;
                        notExit_.notify_all();
                        return; // 线程函数结束，线程结束
                }

                if (mode_ == PoolMode::MODE_CACHED)
                {
                    if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1)))
                    {
                        auto now = std::chrono::high_resolution_clock().now();
                        auto timeDuration = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
                        if (timeDuration.count() > THREAD_MAX_WAIT_TIME)
                        {
                            threads_.erase(threadId);
                            currentThreadNum_--;
                            spareThreadNum_--;

                            std::cout << "thread: " << std::this_thread::get_id()
                                << " exit! " << std::endl;
                            return;
                        }
                    }
                }
                else
                {
                    notEmpty_.wait(lock, [&]()->bool {return taskQue_.size() > 0; });
                }
            }
            spareThreadNum_--;

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
            task->exec();
        }

        spareThreadNum_++;
    }
}


// ------------ 线程成员函数 ----------------

int Thread::generateId_ = 0;

Thread::Thread(threadFunc func)
    : func_(func)
    , threadId_(generateId_++)
{}

Thread::~Thread()
{}

void Thread::start()
{
    std::thread t(func_, threadId_);
    t.detach();
}

int Thread::getId() const
{
    return threadId_;
}

// ---------- task 成员函数 -------------
Task::Task()
    : result_(nullptr)
{}

void Task::exec()
{
    if (result_ != nullptr)
    {
        result_->setValue(run()); // 这里发生多态调用
    }
}

void Task::setResult(Result* res)
{
    result_ = res;
}
 
// ---------- Result成员函数 ------------

Any Result::get()
{
    if (!isValid_)
    {
        return " ";
    }
    sem_.wait();
    return std::move(any_);
}

void Result::setValue(Any any)
{
    this->any_ = std::move(any);
    sem_.post();
}