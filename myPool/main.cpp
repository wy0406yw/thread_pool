#include "Thread_pool.h"
#include<iostream>
#include<vector>
#include<future>

void f1(int x,float y)
{
    std::cout << "f1:" << x << "\t" << y << std::endl;
}

int f2(int x,float y)
{
    std::cout << "f2:" << x << "\t" << y << std::endl;
    return x + y;
}

float f3(int x,float y)
{
    std::cout << "f3:" << x << "\t" << y << std::endl;
    y = x / y;
    return y;
}

int main()
{
    //test
    Thread_pool test_pool(4);
    std::vector<std::future<int>> futures1; 
    std::vector<std::future<float>> futures2; 
    int a = 100;
    float b = 12.5;
    for(int i = 0;i < 30;i++)
    {
        if(i == 10) test_pool._stop();
        test_pool.enqueue(f1,a+i,b);
        futures1.emplace_back(test_pool.enqueue(f2,a+i,b));
        futures2.emplace_back(test_pool.enqueue(f3,a+i,b));
        if(i == 20) test_pool.start();
    }

    
    for(auto &f: futures1)
    {
        std::cout << f.get() << std::endl;
    }

    for(auto &f: futures2)
    {
        std::cout << f.get() << std::endl;
    }
    //多个线程执行时输出是不同步的，所以结果很乱
}