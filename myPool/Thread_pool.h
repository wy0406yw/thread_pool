#ifndef MY_THREAD_POOL
#define MY_THREAD_POOL


#include<iostream>
#include<thread>
#include<atomic>
#include<cstdint>
#include<vector>
#include<queue>
#include<functional>
#include<future>

#define NORMAL_THS_NUM 4
#define ACTV_THS_NUM 0

class Thread_pool{

private:
    std::mutex mutex_lock;
    std::condition_variable cv;
	std::vector<std::thread> pool; //池
    std::queue<std::function<void()>> tasks_queue;//任务队列
    uint32_t active_pool_num = ACTV_THS_NUM;
    uint32_t capacity = NORMAL_THS_NUM;
    std::atomic<bool> stop;

protected:
    void start_task()
    {
        active_pool_num++;
    }

    void completed_task()
    {
        active_pool_num--;
    }
	
    //将任务封装成统一的签名 void(void);
    template<
        typename Fun,
        typename ...Args,
        typename Rtrn=typename std::result_of<Fun(Args...)>::type>
    auto make_task(
        Fun&& f,
        Args&& ...args
    ) -> std::packaged_task<Rtrn(void)>{

        auto task_fun = std::bind(
            std::forward<Fun>(f),
            std::forward<Args>(args)...
            );

        return std::packaged_task<Rtrn(void)>(task_fun);
    }

public:
	Thread_pool(uint32_t _capacity);
    ~Thread_pool();
    void _stop() 
    {
        stop.store(true);
    }
    void start()
    {
        stop.store(false);
    }
     
    
    template<
        typename Fun,
        typename ...Args,
        typename Rtrn=typename std::result_of<Fun(Args...)>::type>
    auto enqueue(
        Fun&& f,
        Args&& ...args
    ) -> std::future<Rtrn>
    {
        auto task = make_task(f,args...);
        auto future = task.get_future();
        auto task_ptr = std::make_shared<decltype(task)>(std::move(task));

        {
            std::lock_guard<std::mutex> lock_guard(mutex_lock);

            if(stop)
            {
                throw std::runtime_error("线程池已经停止提交任务");
            }

            auto pakload = [task_ptr]()->void{
                task_ptr->operator()(); //调用packaged_task中封装的函数对象void(void)
            };

            tasks_queue.emplace(pakload); //入队

        }

        cv.notify_one();

        return future;


    }
    

    

};

Thread_pool::Thread_pool(uint32_t _capacity) : 
stop(false), active_pool_num(0), capacity(_capacity)
{
	auto wait = [this] () -> void{
    //wait 是为了占位线程
    while(true)
    {
        std::function<void(void)> task;
        {
            std::unique_lock<std::mutex> unique_lock(mutex_lock);

            auto pred = [this] () -> bool {
                return stop || !tasks_queue.empty();
            };

            //如果状态stop，而且队列为空，要唤醒，结束这个wait, 所有线程都会结束
            cv.wait(unique_lock,pred);    //    ⬇-------|T
            //队列不为空，无论是否stop都要继续     ⬇
            if(stop && tasks_queue.empty()) return;

            task = std::move(tasks_queue.front());
            tasks_queue.pop();
            start_task();
        } //Here释放🔒

        task();

        {
            std::lock_guard<std::mutex> lock_guard(mutex_lock);
            completed_task();
        }
    }

    }; 

    for(uint64_t id = 0;id < capacity;id++)
    {
        pool.emplace_back(wait);
    }

}

inline Thread_pool::~Thread_pool()
{
    {
        std::lock_guard<std::mutex> lock_guard(mutex_lock);
        stop.store(true);
    }
    
    cv.notify_all();

    for(auto& thread:pool)
    {
        thread.join();
    }


}

#endif
