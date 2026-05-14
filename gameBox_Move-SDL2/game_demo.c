#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600
#define MOVE_SPEED    5
#define FPS           60
#define FRAME_DELAY   (1000 / FPS)

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        printf("SDL init failed: %s\n", SDL_GetError());
        return -1;
    }
    printf("✅ SDL initialized successfully, version: %d.%d.%d\n", 
           SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);

    // Create window (force input focus)
    SDL_Window* window = SDL_CreateWindow(
        "Event-Driven Movement Test",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS
    );
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }
    printf("✅ Window created successfully\n");

    // Create renderer (software mode for maximum compatibility)
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    printf("Renderer created (software mode)\n");

    // Player initialization
    SDL_Rect player = {SCREEN_WIDTH/2 - 25, SCREEN_HEIGHT/2 - 25, 50, 50};
    bool isRunning = true;
    int logCounter = 0;

    // Pure event-driven movement state (core)
    bool moveUp = false, moveDown = false, moveLeft = false, moveRight = false;

    printf("\n🎮 Game started! Click the window and use WASD to move\n");
    printf("----------------------------------------\n");

    // Main game loop
    while (isRunning) {
        Uint32 frameStart = SDL_GetTicks();

        // --------------------------
        // Event handling (only input source)
        // --------------------------
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    isRunning = false;
                    break;

                // Window focus events
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                        printf("✅ Window gained focus\n");
                    } else if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                        printf("⚠️  Window lost focus\n");
                        // Stop all movement when focus is lost
                        moveUp = moveDown = moveLeft = moveRight = false;
                    }
                    break;

                // Key down event
                case SDL_KEYDOWN:
                    printf("🔘 Key pressed: %s (scancode: %d)\n", 
                           SDL_GetKeyName(event.key.keysym.sym), 
                           event.key.keysym.scancode);
                    fflush(stdout);
                    
                    // Set movement state
                    switch (event.key.keysym.scancode) {
                        case SDL_SCANCODE_W: moveUp = true; break;
                        case SDL_SCANCODE_S: moveDown = true; break;
                        case SDL_SCANCODE_A: moveLeft = true; break;
                        case SDL_SCANCODE_D: moveRight = true; break;
                        case SDL_SCANCODE_ESCAPE: isRunning = false; break;
                    }
                    break;

                // Key up event
                case SDL_KEYUP:
                    printf("Key released: %s\n", SDL_GetKeyName(event.key.keysym.sym));
                    fflush(stdout);
                    
                    // Clear movement state
                    switch (event.key.keysym.scancode) {
                        case SDL_SCANCODE_W: moveUp = false; break;
                        case SDL_SCANCODE_S: moveDown = false; break;
                        case SDL_SCANCODE_A: moveLeft = false; break;
                        case SDL_SCANCODE_D: moveRight = false; break;
                    }
                    break;
            }
        }

        // Movement logic (state-based)
        if (moveUp) player.y -= MOVE_SPEED;
        if (moveDown) player.y += MOVE_SPEED;
        if (moveLeft) player.x -= MOVE_SPEED;
        if (moveRight) player.x += MOVE_SPEED;

        // Boundary restriction
        player.x = (player.x < 0) ? 0 : player.x;
        player.x = (player.x + player.w > SCREEN_WIDTH) ? SCREEN_WIDTH - player.w : player.x;
        player.y = (player.y < 0) ? 0 : player.y;
        player.y = (player.y + player.h > SCREEN_HEIGHT) ? SCREEN_HEIGHT - player.h : player.y;

        // Logging (1 per second, no spam)
        logCounter++;
        if (logCounter >= FPS) {
            printf("📍 Player position: x=%d, y=%d | Movement: U=%d D=%d L=%d R=%d\n",
                   player.x, player.y, moveUp, moveDown, moveLeft, moveRight);
            fflush(stdout);
            logCounter = 0;
        }

        // Rendering
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Draw player (green square)
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer, &player);
        
        SDL_RenderPresent(renderer);

        // Frame rate control
        int frameTime = SDL_GetTicks() - frameStart;
        if (FRAME_DELAY > frameTime) {
            SDL_Delay(FRAME_DELAY - frameTime);
        }
    }

    // Cleanup resources
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    printf("\n✅ Game exited normally\n");
    return 0;
}