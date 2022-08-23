#ifndef DYNAMIC_THREADPOOL_DYNAMICTHREADPOOL_H
#define DYNAMIC_THREADPOOL_DYNAMICTHREADPOOL_H
#include <thread>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <algorithm>
class threadpool{

private:
    std::atomic<bool>stop{false};
    std::condition_variable cv_task;
    std::mutex m_task;
    int max_num=0;
//    std::atomic<int> now_run_workers{0};
    int now_run_workers{0};
    using Task = std::function<void()>;
    std::queue<Task>queue;
    int tolerance=10;
    std::vector<std::thread> pool;
public:
    threadpool(size_t t):max_num(t)
    {
//        for(int i=0;i<t;i++)
//            pool.emplace_back(&threadpool::schedual,this);
    }
    ~threadpool()
    {
//        for(int i=0;i<pool.size();i++)
//            pool[i].detach();
        {
            std::lock_guard<std::mutex>lock{this->m_task};
            stop.store(true);
        }
        cv_task.notify_all();
    }

    template<class F,class... Args>
    auto commit(F&& f,Args&&... args)->std::future<decltype(f(args...))>
    {
        if(stop.load())
        {
            throw  std::runtime_error("thread stop!!!!");
        }
        using Restype = decltype(f(args...));
        auto task = std::make_shared<std::packaged_task<Restype()>>(std::bind(std::forward<F>(f),std::forward<Args>(args)...));
        {

            std::lock_guard<std::mutex>lock{this->m_task};
            queue.emplace([task](){
                (*task)();
            });
        }

        if(now_run_workers==0||(now_run_workers<max_num&&queue.size()>=tolerance))
            AddThread();
        cv_task.notify_one();
        return task->get_future();
    }
private:

    Task get_one_task()
    {
        {
            std::unique_lock<std::mutex> lock{this->m_task};
            this->cv_task.wait(lock,[this](){return !this->queue.empty();});
//            std::lock_guard<std::mutex> lock(this->m_task);
            auto task = std::move(this->queue.front());
            queue.pop();
            return task;
        }
    }
    void schedual()
    {

        while(!queue.empty())
        {
            if (stop) {
                break;   // 退出线程循环
            }
            try {
//                std::unique_lock<std::mutex> lock(this->m_task);
               if( Task task = get_one_task())
                task();

            }catch (std::exception& e){
                std::cout<<"running task, with exception..."<<e.what()<<std::endl;
                return;
            }
        }

    }
    void AddThread()
    {
        ++now_run_workers;
        std::thread ttr([this](){
            schedual();
            --now_run_workers;
        });
        ttr.detach();

    }
};

#endif //DYNAMIC_THREADPOOL_DYNAMICTHREADPOOL_H
