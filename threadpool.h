#include <iostream>
#include <iostream>
#include <functional>
#include <thread>
#include <condition_variable>
#include <future>
#include <atomic>
#include <vector>
#include <queue>
////using namespace std;
////struct ListNode{
////    std::thread action;
////    shared_ptr<ListNode> next;
//////    shared_ptr<ListNode> pre;
////};
////void PushTask()
////{
////    shared_ptr<ListNode> head = make_shared<ListNode>();
////}
////
////void del()
////{
////    shared_ptr<ListNode> head;
////    while(head)
////    {
////        head->action.detach();
////        head = head->next;
////    }
////}
//
//
//// 命名空间
//
//
//
//
// C++ program to illustrate the
// lvalue and rvalue
#include <iostream>
#include <unistd.h>

using namespace std;

// Driver Code


class threadpool
{
private:
    mutex m_task;
    condition_variable cv_task;
    using Task = function<void()>;
    atomic<bool>stop;
    queue<Task>queue;
    atomic<int> now_run_num{0};
    int max_thread_num  =0;
    int tolerance = 10;
public:
    threadpool(size_t t):max_thread_num(t)
    {
        if(max_thread_num>thread::hardware_concurrency())
            max_thread_num = thread::hardware_concurrency();
    }

    template<class F,class ...Args>
            auto commit(F&&f,Args&&...args)-> future<decltype(f(args...))> {
        if(stop.load()){    // stop == true ??
            throw std::runtime_error("task executor have closed commit.");
        }
        atomic<bool> add_thread{false};
        using Restype = decltype(f(args...));
        auto task = make_shared<packaged_task<Restype()>>(bind(forward<F>(f), forward<Args>(args)...));
        {
            unique_lock<mutex> lock{m_task};
            queue.template emplace([task]() {
                (*task)();
            });
            if(now_run_num==0||(queue.size()>tolerance))
            {
                add_thread.store(true);
            }
        }
        if(add_thread)
        {
            AddThread();
        }
        cv_task.notify_all();
        future<Restype> future = task->get_future();
        return future;
    }
    void shutdown(){
        this->stop.store(true);
    }

    // 重启任务提交
    void restart(){
        this->stop.store(false);
    }


    ~threadpool(){

    }
private:
    Task get_one_task()
    {
        unique_lock<mutex>lock{m_task};
        cv_task.wait(lock,[this](){return !queue.empty();});
        Task now_task = queue.front();
        queue.pop();
        return now_task;
    }
    void schedual()
    {   //while(true)
            while (Task task = get_one_task()) {
                task();
            }
    }
    void AddThread()
    {
        now_run_num++;
        thread new_thread([this]() {
            while (true)
            {
                if (stop.load()) {
                    break;
                }
                schedual();
            }
        }); 
        now_run_num--;
        new_thread.detach();
    }
};



