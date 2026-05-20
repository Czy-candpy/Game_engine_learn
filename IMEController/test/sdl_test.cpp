#define SDL_MAIN_HANDLED
#define UNICODE
#define _UNICODE
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <windows.h>
#include <imm.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "../core/ime_controller_c.h"
#ifdef __cplusplus
}
#endif

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define PLAYER_SIZE 50
#define PLAYER_SPEED 5
#define INTERACT_RADIUS 40

typedef struct {
    int x, y;
    int size;
    int speed;
} Player;

typedef struct {
    int x, y;
    int radius;
    bool active;
} InteractPoint;

typedef struct {
    bool visible;
    char inputText[256];
    SDL_Rect inputRect;
    SDL_Rect enterBtn;
    SDL_Rect delBtn;
    SDL_Rect closeBtn;
    bool ignoreNextInput;
    DWORD openTime;
} DialogBox;

typedef struct {
    wchar_t composition[256];
    wchar_t candidates[10][256];
    int candidateCount;
    int candidateSelection;
    POINT position;
    bool visible;
} IMECandidateWindow;

Player player = {SCREEN_WIDTH/2 - PLAYER_SIZE/2, SCREEN_HEIGHT/2 - PLAYER_SIZE/2, PLAYER_SIZE, PLAYER_SPEED};
InteractPoint interactPoint = {SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 150, INTERACT_RADIUS, true};
DialogBox dialog = {false, "", {0}, {0}, {0}, {0}, false, 0};
IMECandidateWindow imeCandidate = {L"", {{0}}, 0, 0, {0, 0}, false};
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
HDC hdc = NULL;
HFONT font = NULL;
IMEController* imeController = NULL;
DWORD lastUpdateTime = 0;
HWND gameHwnd = NULL;

bool InitSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    
    window = SDL_CreateWindow("IME Controller Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);
    hdc = GetDC(wmInfo.info.win.window);
    gameHwnd = wmInfo.info.win.window;
    
    font = CreateFont(24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GB2312_CHARSET,
                      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                      DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei");
    if (!font) {
        font = CreateFont(24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                          DEFAULT_PITCH | FF_SWISS, L"Arial");
    }
    
    return true;
}

void CloseSDL() {
    if (imeController != NULL) {
        IMEController_Destroy(imeController);
    }
    if (font != NULL) {
        DeleteObject(font);
    }
    if (hdc != NULL) {
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(window, &wmInfo);
        ReleaseDC(wmInfo.info.win.window, hdc);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void HandleInput() {
    const Uint8* keystate = SDL_GetKeyboardState(NULL);
    
    if (!dialog.visible) {
        if (keystate[SDL_SCANCODE_W] || keystate[SDL_SCANCODE_UP]) {
            player.y = (player.y - player.speed < 0) ? 0 : player.y - player.speed;
        }
        if (keystate[SDL_SCANCODE_S] || keystate[SDL_SCANCODE_DOWN]) {
            player.y = (player.y + player.speed > SCREEN_HEIGHT - player.size) ? SCREEN_HEIGHT - player.size : player.y + player.speed;
        }
        if (keystate[SDL_SCANCODE_A] || keystate[SDL_SCANCODE_LEFT]) {
            player.x = (player.x - player.speed < 0) ? 0 : player.x - player.speed;
        }
        if (keystate[SDL_SCANCODE_D] || keystate[SDL_SCANCODE_RIGHT]) {
            player.x = (player.x + player.speed > SCREEN_WIDTH - player.size) ? SCREEN_WIDTH - player.size : player.x + player.speed;
        }
    }
}

bool CheckInteract() {
    if (!interactPoint.active) return false;
    
    int playerCenterX = player.x + player.size / 2;
    int playerCenterY = player.y + player.size / 2;
    
    int dx = playerCenterX - interactPoint.x;
    int dy = playerCenterY - interactPoint.y;
    int distance = dx*dx + dy*dy;
    int radiusSq = (player.size/2 + interactPoint.radius) * (player.size/2 + interactPoint.radius);
    
    return distance <= radiusSq;
}

void DrawText(const char* text, int x, int y, SDL_Color color) {
    if (!font || !renderer) return;
    
    SDL_Surface* surface = SDL_CreateRGBSurface(0, 400, 40, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    if (!surface) return;
    
    HDC surfaceDC = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, surface->w, surface->h);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(surfaceDC, hBitmap);
    
    HFONT oldFont = (HFONT)SelectObject(surfaceDC, font);
    SetTextColor(surfaceDC, RGB(color.r, color.g, color.b));
    SetBkMode(surfaceDC, TRANSPARENT);
    
    int len = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    wchar_t* wText = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wText, len);
    
    RECT rect = {0, 0, surface->w, surface->h};
    DrawTextW(surfaceDC, wText, -1, &rect, DT_LEFT | DT_TOP);
    
    free(wText);
    SelectObject(surfaceDC, oldFont);
    
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = surface->w;
    bmi.bmiHeader.biHeight = -surface->h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    GetDIBits(surfaceDC, hBitmap, 0, surface->h, surface->pixels, &bmi, DIB_RGB_COLORS);
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rectDest = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rectDest);
    
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
    SelectObject(surfaceDC, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(surfaceDC);
}

void DrawTextCentered(const char* text, SDL_Rect rect, SDL_Color color) {
    if (!font || !renderer) return;
    
    int textWidth = 0, textHeight = 0;
    
    HDC tempDC = CreateCompatibleDC(hdc);
    HFONT oldFont = (HFONT)SelectObject(tempDC, font);
    
    int len = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    wchar_t* wText = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wText, len);
    
    SIZE size;
    GetTextExtentPoint32W(tempDC, wText, wcslen(wText), &size);
    textWidth = size.cx;
    textHeight = size.cy;
    
    free(wText);
    SelectObject(tempDC, oldFont);
    DeleteDC(tempDC);
    
    int textX = rect.x + (rect.w - textWidth) / 2;
    int textY = rect.y + (rect.h - textHeight) / 2;
    
    DrawText(text, textX, textY, color);
}

void DrawPlayer() {
    SDL_Rect playerRect = {player.x, player.y, player.size, player.size};
    SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);
    SDL_RenderFillRect(renderer, &playerRect);
    
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &playerRect);
}

void DrawInteractPoint() {
    if (!interactPoint.active) return;
    
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    
    for (int i = interactPoint.radius - 3; i <= interactPoint.radius; i++) {
        for (int angle = 0; angle < 360; angle += 2) {
            float rad = angle * 3.14159f / 180.0f;
            int x = interactPoint.x + cos(rad) * i;
            int y = interactPoint.y + sin(rad) * i;
            SDL_RenderDrawPoint(renderer, x, y);
        }
    }
    
    DrawTextCentered("E", (SDL_Rect){interactPoint.x - 20, interactPoint.y - 20, 40, 40}, (SDL_Color){255, 255, 255, 255});
}

void DrawIMECandidateWindow() {
    if (!imeCandidate.visible || imeCandidate.candidateCount == 0) return;
    
    int windowX = imeCandidate.position.x;
    int windowY = imeCandidate.position.y - 30;
    int windowWidth = 200;
    int windowHeight = 30 + imeCandidate.candidateCount * 25;
    
    SDL_Rect bgRect = {windowX, windowY, windowWidth, windowHeight};
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 245);
    SDL_RenderFillRect(renderer, &bgRect);
    SDL_SetRenderDrawColor(renderer, 0, 200, 255, 255);
    SDL_RenderDrawRect(renderer, &bgRect);
    
    if (wcslen(imeCandidate.composition) > 0) {
        char compStr[512];
        WideCharToMultiByte(CP_UTF8, 0, imeCandidate.composition, -1, compStr, 512, NULL, NULL);
        DrawText(compStr, windowX + 5, windowY + 5, (SDL_Color){255, 255, 100, 255});
    }
    
    for (int i = 0; i < imeCandidate.candidateCount && i < 10; i++) {
        char candStr[512];
        WideCharToMultiByte(CP_UTF8, 0, imeCandidate.candidates[i], -1, candStr, 512, NULL, NULL);
        
        if (i == imeCandidate.candidateSelection) {
            SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);
            SDL_Rect selRect = {windowX + 5, windowY + 28 + i * 25, windowWidth - 10, 22};
            SDL_RenderFillRect(renderer, &selRect);
            
            DrawText(candStr, windowX + 30, windowY + 30 + i * 25, (SDL_Color){255, 255, 255, 255});
        } else {
            char displayStr[520];
            snprintf(displayStr, sizeof(displayStr), "%d. %s", i + 1, candStr);
            DrawText(displayStr, windowX + 5, windowY + 30 + i * 25, (SDL_Color){200, 200, 200, 255});
        }
    }
}

void DrawDialog() {
    if (!dialog.visible) return;
    
    int dialogX = SCREEN_WIDTH / 2 - 150;
    int dialogY = SCREEN_HEIGHT / 2 - 100;
    
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_Rect dialogRect = {dialogX, dialogY, 300, 200};
    SDL_RenderFillRect(renderer, &dialogRect);
    
    SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);
    SDL_RenderDrawRect(renderer, &dialogRect);
    
    DrawTextCentered("互动对话框", (SDL_Rect){dialogX + 10, dialogY + 10, 280, 30}, (SDL_Color){255, 255, 255, 255});
    
    dialog.inputRect = (SDL_Rect){dialogX + 10, dialogY + 50, 280, 40};
    SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
    SDL_RenderFillRect(renderer, &dialog.inputRect);
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderDrawRect(renderer, &dialog.inputRect);
    
    const char* displayText = (strlen(dialog.inputText) > 0) ? dialog.inputText : "请输入文字...";
    DrawText(displayText, dialogX + 20, dialogY + 58, (SDL_Color){255, 255, 255, 255});
    
    dialog.enterBtn = (SDL_Rect){dialogX + 20, dialogY + 110, 80, 35};
    SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);
    SDL_RenderFillRect(renderer, &dialog.enterBtn);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &dialog.enterBtn);
    DrawTextCentered("确定", dialog.enterBtn, (SDL_Color){255, 255, 255, 255});
    
    dialog.delBtn = (SDL_Rect){dialogX + 110, dialogY + 110, 80, 35};
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_RenderFillRect(renderer, &dialog.delBtn);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &dialog.delBtn);
    DrawTextCentered("删除", dialog.delBtn, (SDL_Color){255, 255, 255, 255});
    
    dialog.closeBtn = (SDL_Rect){dialogX + 200, dialogY + 110, 80, 35};
    SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
    SDL_RenderFillRect(renderer, &dialog.closeBtn);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &dialog.closeBtn);
    DrawTextCentered("关闭", dialog.closeBtn, (SDL_Color){255, 255, 255, 255});
    
    DrawIMECandidateWindow();
}

bool PointInRect(int x, int y, SDL_Rect rect) {
    return (x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h);
}

void HandleDialogClick(int mouseX, int mouseY) {
    if (PointInRect(mouseX, mouseY, dialog.enterBtn)) {
        printf("确定按钮点击! 输入内容: %s\n", dialog.inputText);
        if (strlen(dialog.inputText) > 0) {
            interactPoint.active = false;
            dialog.visible = false;
            player.x = SCREEN_WIDTH / 2 - PLAYER_SIZE / 2;
            player.y = SCREEN_HEIGHT / 2 - PLAYER_SIZE / 2;
            memset(dialog.inputText, 0, sizeof(dialog.inputText));
            IMEController_EnableInputMode(imeController, false);
        }
    } else if (PointInRect(mouseX, mouseY, dialog.delBtn)) {
        memset(dialog.inputText, 0, sizeof(dialog.inputText));
        printf("删除按钮点击!\n");
    } else if (PointInRect(mouseX, mouseY, dialog.closeBtn)) {
        dialog.visible = false;
        player.x = (player.x < SCREEN_WIDTH / 2) ? player.x - 200 : player.x + 200;
        player.x = (player.x < 0) ? 0 : (player.x > SCREEN_WIDTH - player.size) ? SCREEN_WIDTH - player.size : player.x;
        IMEController_EnableInputMode(imeController, false);
        printf("关闭按钮点击!\n");
    }
}

void EnableIMEForDialog(bool enable) {
    if (imeController != NULL) {
        if (enable) {
            IMEController_EnableIME(imeController);
            IMEController_EnableInputMode(imeController, true);
            printf("[IME] 对话框打开，启用IME\n");
        } else {
            IMEController_EnableInputMode(imeController, false);
            IMEController_DisableIME(imeController);
            printf("[IME] 对话框关闭，禁用IME\n");
        }
    }
}

void UpdateIMECandidateInfo() {
    if (!gameHwnd || !dialog.visible) {
        imeCandidate.visible = false;
        return;
    }
    
    HIMC hIMC = ImmGetContext(gameHwnd);
    if (hIMC == NULL) {
        imeCandidate.visible = false;
        return;
    }
    
    DWORD compLen = ImmGetCompositionString(hIMC, GCS_COMPSTR, NULL, 0);
    if (compLen > 0 && compLen < sizeof(imeCandidate.composition)) {
        ImmGetCompositionString(hIMC, GCS_COMPSTR, imeCandidate.composition, compLen);
        imeCandidate.composition[compLen / sizeof(wchar_t)] = L'\0';
        imeCandidate.visible = true;
    } else {
        imeCandidate.composition[0] = L'\0';
    }
    
    DWORD candSize = ImmGetCandidateList(hIMC, 0, NULL, 0);
    if (candSize > 0) {
        LPCANDIDATELIST candList = (LPCANDIDATELIST)malloc(candSize);
        if (ImmGetCandidateList(hIMC, 0, candList, candSize) > 0) {
            imeCandidate.candidateCount = candList->dwCount;
            imeCandidate.candidateSelection = candList->dwSelection;
            
            for (DWORD i = 0; i < candList->dwCount && i < 10; i++) {
                LPTSTR candStr = (LPTSTR)((DWORD_PTR)candList + candList->dwOffset[i]);
                wcscpy_s(imeCandidate.candidates[i], 256, candStr);
            }
            
            imeCandidate.visible = true;
        }
        free(candList);
    } else if (compLen == 0) {
        imeCandidate.visible = false;
    }
    
    ImmReleaseContext(gameHwnd, hIMC);
    
    imeCandidate.position.x = dialog.inputRect.x;
    imeCandidate.position.y = dialog.inputRect.y;
}

LRESULT WINAPI IMEWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);
    
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    
    if (!InitSDL()) {
        return -1;
    }
    
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);
    HWND hwnd = wmInfo.info.win.window;
    imeController = IMEController_Create(hwnd);
    if (imeController == NULL) {
        printf("[IME] IME控制器初始化失败\n");
    } else {
        printf("[IME] IME控制器初始化成功\n");
        IMEController_SetActive(imeController, true);
        IMEController_DisableIME(imeController);
    }
    
    bool running = true;
    SDL_Event e;
    
    printf("游戏启动! 使用WASD移动，靠近红色圆圈按E键互动。\n");
    
    while (running) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                running = false;
            } else if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                    printf("[IME] 窗口获得焦点\n");
                    if (!dialog.visible) {
                        IMEController_EnableInputMode(imeController, false);
                        IMEController_DisableIME(imeController);
                    }
                } else if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                    printf("[IME] 窗口失去焦点\n");
                    IMEController_EnableIME(imeController);
                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (dialog.visible) {
                    int mouseX = e.button.x;
                    int mouseY = e.button.y;
                    HandleDialogClick(mouseX, mouseY);
                }
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_e && CheckInteract() && !dialog.visible) {
                    dialog.visible = true;
                    dialog.ignoreNextInput = true;
                    dialog.openTime = GetTickCount();
                    EnableIMEForDialog(true);
                    printf("触发互动!\n");
                } else if (e.key.keysym.sym == SDLK_ESCAPE && dialog.visible) {
                    dialog.visible = false;
                    EnableIMEForDialog(false);
                    printf("ESC关闭对话框\n");
                } else if (dialog.visible) {
                    if (e.key.keysym.sym == SDLK_BACKSPACE && strlen(dialog.inputText) > 0) {
                        dialog.inputText[strlen(dialog.inputText) - 1] = '\0';
                        printf("退格: %s\n", dialog.inputText);
                    }
                }
            } else if (e.type == SDL_TEXTINPUT && dialog.visible) {
                if (dialog.ignoreNextInput) {
                    dialog.ignoreNextInput = false;
                    printf("忽略首次输入: %s\n", e.text.text);
                    continue;
                }
                
                if (GetTickCount() - dialog.openTime < 100) {
                    printf("忽略快速输入: %s\n", e.text.text);
                    continue;
                }
                
                strncat(dialog.inputText, e.text.text, 255 - strlen(dialog.inputText));
                printf("输入: %s\n", dialog.inputText);
            }
        }
        
        HandleInput();
        
        if (dialog.visible) {
            UpdateIMECandidateInfo();
        }
        
        DWORD currentTime = GetTickCount();
        if (imeController != NULL && !dialog.visible && currentTime - lastUpdateTime > 500) {
            IMEController_Update(imeController);
            lastUpdateTime = currentTime;
        }
        
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        
        DrawInteractPoint();
        DrawPlayer();
        DrawDialog();
        
        SDL_RenderPresent(renderer);
        
        SDL_Delay(16);
    }
    
    CloseSDL();
    return 0;
}