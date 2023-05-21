#include <iostream>
#include <thread>
#include <chrono>

#include "threadpool.h"

int main()
{
    ThreadPool pool;
    pool.start(6);
    std::this_thread::sleep_for(std::chrono::seconds(5));

    return 0;
}