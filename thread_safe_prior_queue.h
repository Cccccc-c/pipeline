/*std::priority_queue是C++标准库提供的一种容器适配器。它提供了一种以队列方式存储元素的方法，其中每个元素都有一个相关联的优先级。优先级队列确保具有最高优先级的元素始终位于队列前端。
std::priority_queue实现为堆数据结构，这允许有效地插入和检索最大元素。该容器支持以下操作：
    push：将元素插入到优先级队列中
    pop：从优先级队列中删除最高优先级的元素
    top：返回优先级队列中具有最高优先级的元素的引用
    empty：如果优先级队列为空，则返回true
    size：返回优先级队列中元素的数量
默认情况下，std::priority_queue是一个最大堆，这意味着具有最大值的元素具有最高优先级。但是，可以在创建优先级队列对象时指定自定义比较函数来创建最小堆。

std::condition_variable 是一个同步原语，用于实现线程间的协调。它允许一个或多个线程等待另一个线程通知它们继续执行
当cv_.notify_one()执行后
cv_.wait会启动判断lamda是不返回true，返回true往下执行，返回false继续等待唤醒

这种队列可以适用于多个生产者多个消费者
*/

#ifndef THREAD_SAFE_PRIOR_QUEUE_H_
#define THREAD_SAFE_PRIOR_QUEUE_H_

#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <condition_variable>

template <typename T, typename OP>
class ThreadSafePriorQueue
{
    public:
    ThreadSafePriorQueue(const int max_size) { max_size_ = max_size; }
    ThreadSafePriorQueue() { max_size_=0; }
    ThreadSafePriorQueue& operator=(const ThreadSafePriorQueue& other) = delete;
    ThreadSafePriorQueue(const ThreadSafePriorQueue& other) = delete;

    ~ThreadSafePriorQueue() { BreakAllWait(); }

    void Enqueue(const T& element) 
    {
        while (true && max_size_ != 0) 
        {
            if (queue_.size() >= max_size_) 
            {
                std::this_thread::sleep_for(std::chrono::microseconds(10000));
            }
            else 
            {
                break;
            }
        }
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.emplace(element);
        cv_.notify_one();
    }

    bool Dequeue(T* element) 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) 
        {
            return false;
        }
        *element = std::move(queue_.front());
        queue_.pop();
        return true;
    }
    
    const T& Top()
    {
        return queue_.top();
    }

    bool WaitDequeue(T* element) 
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() { return break_all_wait_ || !queue_.empty(); });
        if (break_all_wait_) 
        {
            return false;
        }
        *element = std::move(queue_.top());
        queue_.pop();
        return true;
    }
    
    bool WaitDequeue2(T* element, int dstIndex) 
    { 
        dstIndex_=dstIndex;
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() 
        { 
            return break_all_wait_ || (!queue_.empty() && queue_.top().first == dstIndex_);
        });
        if (break_all_wait_) 
        {
            return false;
        }
        *element = std::move(queue_.top());
        queue_.pop();
        return true;
    }

    typename std::queue<T>::size_type Size() 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    bool Empty()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    void BreakAllWait() 
    {
        break_all_wait_ = true;
        cv_.notify_all();
    }

private:
    volatile bool break_all_wait_ = false;
    std::mutex mutex_;
    std::priority_queue<T, std::vector<T>, OP> queue_;
    std::condition_variable cv_;
    volatile int dstIndex_;
    volatile int max_size_;
};
#endif  // THREAD_SAFE_PRIOR_QUEUE_H_
