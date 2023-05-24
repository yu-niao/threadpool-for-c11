#include <iostream>
#include <thread>
#include <chrono>

#include "threadpool.h"

class MyTask : public Task
{
public:
    MyTask(int begin, int end)
        : begin_(begin)
        , end_(end)
    {}

    Any run()
    {
        int num = 0;
        for (int i = begin_; i <= end_; i++)
        {
            num += i;
        }

        return num;
    }

private:
    int begin_;
    int end_;
};

int main()
{
    ThreadPool pool;
    pool.start(4);

    Result res1 = pool.submitTask(std::make_shared<MyTask>(100, 200));
    Result res2 = pool.submitTask(std::make_shared<MyTask>(201, 300));
    Result res3 = pool.submitTask(std::make_shared<MyTask>(301, 400));

    int num1 = res1.get().cast<int>();
    int num2 = res2.get().cast<int>();
    int num3 = res3.get().cast<int>();

    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << num1 << " " << num2 << " " << num3 << std::endl;
    std::cout << num1 + num2 + num3 << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(5));

    return 0;
}