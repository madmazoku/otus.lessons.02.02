#include <iostream>
#include <spdlog/spdlog.h>
#include <boost/lexical_cast.hpp>

#include <chrono>

#include "../bin/version.h"

#define _USE_MATH_DEFINES
#include <cmath> 
#include <math.h>
#include <random>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <list>

#include <SDL.h>

#include "runner.h"

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */

struct Pixel32 {
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char a;
};

#pragma pack(pop)   /* restore original alignment from stack */

const int window_width = 1024;
const int window_height = 512;
const int window_size = window_width < window_height ? window_width : window_height;
const double x_limit = double(window_width) / window_size;
const double y_limit = double(window_height) / window_size;

template<typename T> T sqr(const T& x) { return x * x; }

template double sqr<double>(const double&);

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> urd(0.0, 1.0);


struct XY {
    double x;
    double y;

    XY() : x(0), y(0) {}
    XY(const double &xy_) : x(xy_), y(xy_) {}
    XY(const double &x_, const double &y_) : x(x_), y(y_) {}
    XY(const struct XY &xy_) : x(xy_.x), y(xy_.y) {}
    ~XY() {}

    void rand(const struct XY &offset, const struct XY &amplitude) 
    { 
        x = urd(gen) * amplitude.x + offset.x;
        y = urd(gen) * amplitude.y + offset.y;
    }
    void rand_angle(const double &amplitude)
    {
        double a = urd(gen) * 2*M_PI;
        double l = amplitude;
        x = l * sin(a);
        y = l * cos(a);
    }

    double dist2(const struct XY &xy_) const
    {
        return sqr(x - xy_.x) + sqr(y - xy_.y);
    }

    double rad2() const
    {
        return sqr(x) + sqr(y);
    }

    double dist(const struct XY &xy_) const
    {
        return sqrt(sqr(x - xy_.x) + sqr(y - xy_.y));
    }

    double rad() const
    {
        return sqrt(sqr(x) + sqr(y));
    }

    XY mul(const double &d) const
    {
        return XY(x*d, y*d);
    }
};

std::ostream& operator<<(std::ostream &os, const struct XY &xy_) {
    return os << '{' << xy_.x << ", "  << xy_.y << '}';
}

std::ostream& operator<<(std::ostream &os, struct SDL_Surface* s) {
    if(s == nullptr)
        return os << "{ null }";
    else
        return os << "{ size: {" << s->w << ", "  << s->h << "}, pitch: " << s->pitch << ", bytesPerPixel: " << int(s->format->BytesPerPixel) << '}';
}

double sgn(const double &x)
{
    return x < 0.0 ? -1.0 : x > 0.0 ? 1.0 : 0.0;
}

double cap(const double& x)
{
    return x < 0.0 ? 0.0 : x > 1.0 ? 1.0 : x;
}

double inv_sqr(const double &x)
{
    return fabs(x) < 1e-8 ? 1e8 : 1/sqr(x);
}

double inv(const double &x)
{
    return fabs(x) < 1e-4 ? 1e4 : 1/x;
}

double edge_force(const double &p, const double &l)
{
    return (inv(p+0.5)-inv(l-p+0.5));
}

void update_xy_pos_by_velovity(struct XY &pos, struct XY &vel, double time_step)
{
    double ax = (edge_force(pos.x, x_limit) - vel.x*0.5 +sgn(vel.x)*fabs(vel.y)*0.1);
    double ay = (edge_force(pos.y, y_limit) - vel.y*0.5 +sgn(vel.x)*fabs(vel.y)*0.1);
    vel.x += ax*time_step;
    vel.y += ay*time_step;
    if(vel.rad() < (x_limit+y_limit)*0.01)
        vel.rand_angle((x_limit+y_limit)*0.4);

    pos.x += vel.x * time_step;
    pos.y += vel.y * time_step;
    if(pos.x > x_limit) {
        pos.x = x_limit;
        vel.x = -vel.x;
    }
    if(pos.x < 0.0) {
        pos.x = 0.0;
        vel.x = -vel.x;
    }
    if(pos.y > y_limit) {
        pos.y = y_limit;
        vel.y = -vel.y;
    }
    if(pos.y < 0.0) {
        pos.y = 0.0;
        vel.y = -vel.y;
    }
}

unsigned char to_lum(const struct XY& pos, const struct XY& p, const struct XY& v) {
    double d2 = pos.dist2(p);
    double r2 = v.rad2();
    return d2 < r2 ? (unsigned char)(((r2 - d2) / r2) * 0xff) : 0x00;

}

void fill_pixels(SDL_Surface* img, int from, int to, const struct XY &pr, const struct XY &pg, const struct XY &pb)
{
    for(int y = from; y < to && y < img->h; ++y) {
        for(int x = 0; x < img->w; ++x) {
            XY pos(double(x) / window_size, double(y) / window_size);
            unsigned char* pixel = (unsigned char*)(img->pixels) + x * 4 + y * img->pitch;

            double dr = pos.dist2(pr);
            double dg = pos.dist2(pg);
            double db = pos.dist2(pb);
            double d = (inv(dr)+inv(dg)+inv(db))/3.0;
            double dp = inv_sqr(10-d*0.5) + d*0.001;
            double dpr = inv(dr*20);
            double dpg = inv(dg*20);
            double dpb = inv(db*20);

            unsigned short lumr = (unsigned char)((0.0 + cap(dpr * dp)) * 0xff);
            unsigned short lumg = (unsigned char)((0.0 + cap(dpg * dp)) * 0xff);
            unsigned short lumb = (unsigned char)((0.0 + cap(dpb * dp)) * 0xff);

            *((Uint32*)pixel) = SDL_MapRGBA(img->format, lumr, lumg, lumb, 0x00);
        }
    }
}

// #define USE_RUNNER

int main(int argc, const char** argv)
{
    auto console = spdlog::stdout_logger_st("console");
    auto wellcome = std::string("Application started, \"") + argv[0] + "\" version: " + boost::lexical_cast<std::string>(version());
    console->info(wellcome);

    if (SDL_Init(SDL_INIT_VIDEO) != 0){
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("Hellow World!", 100, 100, window_width, window_height, SDL_WINDOW_SHOWN);
    if (win == nullptr){
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Surface *scr = SDL_GetWindowSurface(win);
    SDL_Surface *img = SDL_CreateRGBSurface(0, scr->w, scr->h, 32, 0, 0, 0, 0);

    std::cout << "Screen: " << scr << std::endl;
    std::cout << "Image: " << img << std::endl;

    auto start = std::chrono::system_clock::now();
    auto last = start;

    int count = 0;
    int last_count = count;
    double time_step = 0.0;

    SDL_Event event;

    XY pr, pb, pg;
    XY vr, vb, vg;

    XY off(x_limit*0.5,y_limit*0.5);
    XY amp(x_limit*0.4,y_limit*0.4);

    pr.rand(off, amp);
    pg.rand(off, amp);
    pb.rand(off, amp);

    vr.rand_angle(100.0 / window_size);
    vg.rand_angle(100.0 / window_size);
    vb.rand_angle(100.0 / window_size);


#ifdef USE_RUNNER
    Runner r;

    int h_off = (img->h / (std::thread::hardware_concurrency()+1)) + 1;
    int h = 0;

    while(h < img->h) {
        r.add([&,h](){fill_pixels(img, h, h+h_off, pr, pg, pb);});
        h += h_off;
    }

    r.start();
#endif

    bool run = true;
    while(run)
    {
        ++count;

        SDL_LockSurface(img);

#ifdef USE_RUNNER
        r.run();
#else
        int h_off = (img->h / (std::thread::hardware_concurrency()+1)) + 1;
        int h = 0;

        std::list<std::thread> threads;
        while(h < img->h) {
            threads.push_back(std::thread([&,h](){fill_pixels(img, h, h+h_off, pr, pg, pb);}));
            h += h_off;
        }
        for(auto &t : threads)
            t.join();
#endif

        SDL_UnlockSurface(img);

        SDL_BlitSurface(img, nullptr, scr, nullptr);

        SDL_UpdateWindowSurface(win);

        while(SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT
             || event.type == SDL_KEYDOWN
             || event.type == SDL_KEYUP) {
                run = false;
#ifdef USE_RUNNER
                r.stop();
#endif
            }
        }

        update_xy_pos_by_velovity(pr, vr, time_step);
        update_xy_pos_by_velovity(pg, vg, time_step);
        update_xy_pos_by_velovity(pb, vb, time_step);

        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> full_elapsed = end-start;
        std::chrono::duration<double> last_elapsed = end-last;
        if(!run || last_elapsed.count() >= 1) {
            int frames = count - last_count;
            double fps = ((double)frames) / last_elapsed.count();

            SDL_SetWindowTitle(win, ("Hello World! FPS: " + boost::lexical_cast<std::string>(fps)).c_str());

            time_step = frames > 0 ? 1 / (last_elapsed.count() * frames) : 0;

            last = end;
            last_count = count;
        }
    }

#ifdef USE_RUNNER
    r.join();
#endif

    SDL_FreeSurface(img);
    SDL_FreeSurface(scr);

    SDL_DestroyWindow(win);
    SDL_Quit();

    console->info("Application finished successfully.");
    return 0;
}

