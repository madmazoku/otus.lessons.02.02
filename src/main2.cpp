#include <iostream>
#include <boost/lexical_cast.hpp>

#include <chrono>

#include "runner.h"

int main(int argc, const char** argv)
{
    std::list<std::thread> threads;

    std::cout << "thread main started" << std::endl;

    Runner r;

    unsigned int n = 0;
    unsigned int c[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    while(n < 10) {
        r.add([&,n](){
            std::stringstream ss;
            ss << "thread " << n << " count " << c[n] << std::endl;
            std::cout << ss.str();
            ++c[n];
        });
        ++n;
    }

    r.start();
    for(int i = 0; i < 10; ++i) {
        r.run();
    }
    r.stop();
    r.join();

    std::cout << "thread main finished" << std::endl;

    return 0;
}

