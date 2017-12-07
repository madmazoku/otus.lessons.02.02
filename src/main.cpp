#include <iostream>
#include <spdlog/spdlog.h>
#include <boost/lexical_cast.hpp>

#include "../bin/version.h"

#include <SDL.h>

struct Pixel24 {
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char a;
};

int main(int argc, const char** argv)
{
    auto console = spdlog::stdout_logger_st("console");
    auto wellcome = std::string("Application started, \"") + argv[0] + "\" version: " + boost::lexical_cast<std::string>(version());
    console->info(wellcome);

    if (SDL_Init(SDL_INIT_VIDEO) != 0){
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("Hello World!", 100, 100, 640, 480, SDL_WINDOW_SHOWN);
    if (win == nullptr){
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Surface *scr = SDL_GetWindowSurface(win);

    SDL_Surface *img = SDL_CreateRGBSurface(0, 640, 480, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
    SDL_LockSurface(img);

    int offset = 0;
    SDL_Event event;
    while(1)
    {
        ++offset;
        for(int y = 0; y < img->h; ++y) {
            for(int x = 0; x < img->w; ++x) {
                unsigned char* pixel = (unsigned char*)(scr->pixels) + x * 4 + y * img->pitch;
                Pixel24* px24 = (Pixel24*)pixel;
                px24->r = (offset+x+y) & 0xff;
                px24->g = (offset+x+y*2) & 0xff;
                px24->b = (offset+x*2+y) & 0xff;
            }
        }

        SDL_UnlockSurface(img);

        SDL_Rect rect = { 0, 0, 640, 480 };
        SDL_BlitSurface(img, &rect, scr, &rect);

        SDL_UpdateWindowSurface(win);

        /* Any of these event types will end the program */
        if(SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT
             || event.type == SDL_KEYDOWN
             || event.type == SDL_KEYUP)
                break;
        }
    }    

    SDL_DestroyWindow(win);
    SDL_Quit();

    console->info("Application finished successfully.");
    return 0;
}

