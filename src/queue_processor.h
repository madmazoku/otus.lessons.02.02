#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <list>
#include <queue>

class QueueProcessor {
private:
    std::list<std::thread> threads;
    std::queue<std::function<void (void)> > tasks;
    std::queue<std::function<void (void)> > finished_tasks;
    unsigned int active_count;
    bool terminate;

    std::mutex thread_mutex;
    std::condition_variable thread_cv;

public:
    QueueProcessor() : active_count(0), terminate(false) {};

    void start(const unsigned int &thread_count = std::thread::hardware_concurrency()) {
        while(threads.empty() || threads.size() < thread_count) {
            threads.push_back(std::thread([&](){
                while(true) {
                    std::unique_lock<std::mutex> lock_thread(thread_mutex);
                    thread_cv.wait(lock_thread, [&](){ return !tasks.empty() || terminate; });
                    if(terminate)
                        break;
                    auto lambda = tasks.front();
                    tasks.pop();
                    ++active_count;
                    lock_thread.unlock();

                    lambda();

                    lock_thread.lock();
                    finished_tasks.push(lambda);
                    --active_count;
                    if(active_count == 0 && tasks.empty()) {
                        thread_cv.notify_all();
                    }
                }
            }));
        }
    }

    void add(std::function<void (void)> lambda) {
        std::lock_guard<std::mutex> lock_thread(thread_mutex);
        tasks.push(lambda);
    }

    void rewind() {
        std::lock_guard<std::mutex> lock_thread(thread_mutex);
        while(!finished_tasks.empty()) {
            auto lambda = finished_tasks.front();
            finished_tasks.pop();
            tasks.push(lambda);
        }
    }

    void wait() {
        std::unique_lock<std::mutex> lock_thread(thread_mutex);
        while(!finished_tasks.empty())
            finished_tasks.pop();
        thread_cv.notify_all();
        thread_cv.wait(lock_thread, [&](){ return tasks.empty() && active_count == 0; });
    }

    void stop() {
        std::lock_guard<std::mutex> lock_thread(thread_mutex);
        terminate = true;
        thread_cv.notify_all();
    }

    void clear() {
        std::lock_guard<std::mutex> lock_thread(thread_mutex);
        while(!tasks.empty())
            tasks.pop();
        while(!finished_tasks.empty())
            finished_tasks.pop();
    }

    void join() {
        for(auto &t : threads) 
            t.join();

        threads.clear();
    }
};