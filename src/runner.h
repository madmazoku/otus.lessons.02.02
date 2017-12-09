#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <list>


    void log(const char* f, const char* m) {
        std::stringstream ss;
        ss << std::this_thread::get_id() << " [" << f << "]: " << m << std::endl;
        std::cout << ss.str();
    }

class Runner {
private:
    std::list<std::thread> threads;

    bool execute;
    bool process;
    int threads_active;
    std::mutex thread_loop_mutex;
    std::condition_variable thread_loop_cv;

public:
    Runner() : execute(false), process(false), threads_active(0) {};

    void add(std::function<void (void)> lambda) {
        threads.push_back(std::thread([&,lambda](){

            {
                std::unique_lock<std::mutex> lock_thread_loop(thread_loop_mutex);
                thread_loop_cv.wait(lock_thread_loop, [&](){ return execute; });
                if(--threads_active == 0) {
                    thread_loop_cv.notify_all();
                }
            }

            while(1) {
                {
                    std::unique_lock<std::mutex> lock_thread_loop(thread_loop_mutex);
                    thread_loop_cv.wait(lock_thread_loop, [&](){ return !execute || process; });
                    if(!execute) {
                        break;
                    }
                }

                lambda();

                {
                    std::unique_lock<std::mutex> lock_thread_loop(thread_loop_mutex);
                    if(--threads_active == 0) {
                        thread_loop_cv.notify_all();
                    }
                    thread_loop_cv.wait(lock_thread_loop, [&](){ return !process; });
                    if(--threads_active == 0) {
                        thread_loop_cv.notify_all();
                    }
                }

            }
        }));
    }

    void start() {
        std::unique_lock<std::mutex> lock_thread_loop(thread_loop_mutex);
        execute = true;
        threads_active += threads.size();
        thread_loop_cv.notify_all();
        thread_loop_cv.wait(lock_thread_loop, [&](){ return threads_active == 0; });
    }

    void run() {
        std::unique_lock<std::mutex> lock_thread_loop(thread_loop_mutex);
        process = true;
        threads_active += threads.size();
        thread_loop_cv.notify_all();
        thread_loop_cv.wait(lock_thread_loop, [&](){ return threads_active == 0; });
        process = false;
        threads_active += threads.size();
        thread_loop_cv.notify_all();
        thread_loop_cv.wait(lock_thread_loop, [&](){ return threads_active == 0; });
    }

    void stop() {
        {
            std::unique_lock<std::mutex> lock_thread_loop(thread_loop_mutex);
            execute = false;
            thread_loop_cv.notify_all();
        }
    }

    void join() {
        for(auto &t : threads) 
            t.join();

        threads.clear();
    }
};