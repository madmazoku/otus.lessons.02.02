#include <iostream>
#include <spdlog/spdlog.h>
#include <boost/lexical_cast.hpp>

#include <chrono>

#include "../bin/version.h"

#define _USE_MATH_DEFINES
#include <cmath> 
#include <math.h>

#include <SDL.h>

struct Pixel24 {
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char a;
};

const int window_size = 640;

int main(int argc, const char** argv)
{
    auto console = spdlog::stdout_logger_st("console");
    auto wellcome = std::string("Application started, \"") + argv[0] + "\" version: " + boost::lexical_cast<std::string>(version());
    console->info(wellcome);

    if (SDL_Init(SDL_INIT_VIDEO) != 0){
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("Hello World!", 100, 100, window_size, window_size, SDL_WINDOW_SHOWN);
    if (win == nullptr){
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Surface *scr = SDL_GetWindowSurface(win);

    SDL_Surface *img = SDL_CreateRGBSurface(0, window_size, window_size, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
    SDL_LockSurface(img);

    auto start = std::chrono::system_clock::now();
    auto last = start;

    SDL_Rect rect = { 0, 0, window_size, window_size };

    int count = 0;
    int last_count = count;
    int offset = 0;
    int offset_step = 1;
    SDL_Event event;
    while(1)
    {
        ++count;
        offset += offset_step;
        for(int y = 0; y < img->h; ++y) {
            for(int x = 0; x < img->w; ++x) {
                unsigned char* pixel = (unsigned char*)(scr->pixels) + x * 4 + y * img->pitch;
                Pixel24* px24 = (Pixel24*)pixel;
                px24->r = (unsigned char)((sin((offset * 3+x+y) * M_PI/window_size) + 1) * 127);
                px24->g = (unsigned char)((sin((offset * 5+x+y*2) * M_PI/window_size) + 1) * 127);
                px24->b = (unsigned char)((sin((offset * 7+x*2+y) * M_PI/window_size) + 1) * 127);
                // px24->r = (offset+x+y) & 0xff;
                // px24->g = (offset+x+y*2) & 0xff;
                // px24->b = (offset+x*2+y) & 0xff;
            }
        }

        SDL_UnlockSurface(img);

        SDL_BlitSurface(img, &rect, scr, &rect);

        SDL_UpdateWindowSurface(win);

        /* Any of these event types will end the program */
        if(SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT
             || event.type == SDL_KEYDOWN
             || event.type == SDL_KEYUP)
                break;
        }

        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> full_elapsed = end-start;
        std::chrono::duration<double> last_elapsed = end-last;
        if(last_elapsed.count() >= 1 || (offset % 10) == 0) {
            double fps = ((count - last_count) / last_elapsed.count());
            offset_step = 50 / fps + 1;
            std::cout << "sec: " << full_elapsed.count() << " fps: " << fps << " step: " << offset_step << std::endl;
            last = end;
            last_count = count;
        }
    }    

    SDL_DestroyWindow(win);
    SDL_Quit();

    console->info("Application finished successfully.");
    return 0;
}

