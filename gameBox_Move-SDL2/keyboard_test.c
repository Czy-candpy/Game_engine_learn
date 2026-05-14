#include <SDL2/SDL.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    // 只初始化事件系统，排除其他干扰
    if (SDL_Init(SDL_INIT_EVENTS) < 0) {
        printf("SDL events init failed: %s\n", SDL_GetError());
        return 1;
    }

    // 创建一个最小窗口（必须有窗口才能接收输入）
    SDL_Window* window = SDL_CreateWindow(
        "Keyboard Test",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        400, 300,
        SDL_WINDOW_SHOWN
    );

    printf("=== SDL Keyboard Test ===\n");
    printf("Click the window and press any key\n");
    printf("Press ESC to exit\n");
    printf("=========================\n");

    int running = 1;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = 0;
                    break;

                case SDL_KEYDOWN:
                    printf("KEY DOWN: sym=%d, scancode=%d, name=%s\n",
                           event.key.keysym.sym,
                           event.key.keysym.scancode,
                           SDL_GetKeyName(event.key.keysym.sym));
                    fflush(stdout);
                    
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        running = 0;
                    }
                    break;

                case SDL_KEYUP:
                    printf("KEY UP: sym=%d, name=%s\n",
                           event.key.keysym.sym,
                           SDL_GetKeyName(event.key.keysym.sym));
                    fflush(stdout);
                    break;
            }
        }

        // 降低CPU占用
        SDL_Delay(10);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}