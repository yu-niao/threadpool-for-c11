#include <iostream>
#include <thread>
#include <chrono>

#include "threadpool.h"

class MyTask : public Task
{
public:
    void run()
    {
        std::cout << "begin thread : " << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "end thread : " << std::this_thread::get_id() << std::endl;
    }
};

int main()
{
    ThreadPool pool;
    pool.start(4);

    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());
    pool.submitTask(std::make_shared<MyTask>());

    getchar();

    return 0;
}