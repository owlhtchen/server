#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include <thread>
#include <vector>
#include <deque>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ThreadPool {
    public:
    unsigned int numThreads;
    std::vector<std::thread> threads;
    std::deque<std::function<void()>> tasks;
    std::mutex tasksMutex;
    std::condition_variable notEmpty;

    std::condition_variable oneMoreIdleWorker;
    std::atomic<unsigned int> numIdleWorkers;

    bool endPool;
    
    ThreadPool(): endPool(false), numThreads(std::thread::hardware_concurrency()) { 
        numIdleWorkers.store(0);
        createWorkers(numThreads);
    };
    ThreadPool(unsigned int numThreads): endPool(false), numThreads(numThreads) { 
        numIdleWorkers.store(0);
        createWorkers(numThreads);
    };
    ~ThreadPool() {
        waitAll();
        endPool = true;
        notEmpty.notify_all(); // in case any worker is blocked on notEmpty cond variable
        for(auto& t: threads) {
            t.join();
        }
    }

    void add(std::function<void()>&& task) {
        {
            std::lock_guard<std::mutex> lock(tasksMutex);
            tasks.emplace_back(std::move(task));
            // std::cerr << tasks.size() << " tasks in total\n";
            notEmpty.notify_all();
        }
    }

    void createWorkers(unsigned int numWorkers) {
        numWorkers = numWorkers > 0 ? numWorkers : 1;
        for(unsigned int i = 0; i < numWorkers; i++) {
            threads.emplace_back([&](){worker();}); 
            // threads.emplace_back(worker); // worker is a method, cannot be called independent of object
        }
    }

    void worker() {
        while(!endPool) {
            std::unique_lock<std::mutex> lk(tasksMutex);
            while(tasks.empty() && !endPool) {
                numIdleWorkers.fetch_add(1);
                oneMoreIdleWorker.notify_all();
                notEmpty.wait(lk);  
                numIdleWorkers.fetch_sub(1);
            }
            bool hasTask = false;
            if(!endPool) {
                auto current = std::move(tasks.front());
                tasks.pop_front();
                lk.unlock();
                current();
            } else {
                lk.unlock();
            }
        }
    }

    void waitAll() {
        std::unique_lock<std::mutex> lk(tasksMutex);
        while(!(tasks.empty() && numIdleWorkers.load() == numThreads)) {
            oneMoreIdleWorker.wait(lk);
        }
        lk.unlock();
    }
};

#endif