#include "ime_controller_c.h"
#include <imm.h>
#include <stdio.h>

#pragma comment(lib, "imm32.lib")

typedef struct {
    bool imeEnabled;
    DWORD lastSwitchTime;
    HIMC originalIMC;
} IMEState;

IMEController* IMEController_Create(HWND hWnd) {
    if (hWnd == NULL) {
        return NULL;
    }
    
    IMEController* controller = (IMEController*)malloc(sizeof(IMEController) + sizeof(IMEState));
    if (controller == NULL) {
        return NULL;
    }
    
    controller->windowHandle = hWnd;
    controller->isActive = true;
    controller->isInputMode = false;
    controller->currentIMC = NULL;
    controller->focusedInputBox = NULL;
    
    IMEState* state = (IMEState*)(controller + 1);
    state->imeEnabled = true;
    state->lastSwitchTime = 0;
    state->originalIMC = ImmGetContext(hWnd);
    
    printf("[IME] Controller created for window: %p\n", hWnd);
    return controller;
}

void IMEController_Destroy(IMEController* controller) {
    if (controller == NULL) {
        return;
    }
    
    IMEState* state = (IMEState*)(controller + 1);
    
    if (state->originalIMC != NULL) {
        ImmAssociateContext(controller->windowHandle, state->originalIMC);
    }
    
    if (controller->currentIMC != NULL) {
        ImmReleaseContext(controller->windowHandle, controller->currentIMC);
    }
    
    free(controller);
    printf("[IME] Controller destroyed\n");
}

void IMEController_SetActive(IMEController* controller, bool active) {
    if (controller == NULL) {
        return;
    }
    controller->isActive = active;
    printf("[IME] Active state changed to: %s\n", active ? "true" : "false");
}

bool IMEController_IsActive(IMEController* controller) {
    return (controller != NULL) ? controller->isActive : false;
}

void IMEController_EnableInputMode(IMEController* controller, bool enable) {
    if (controller == NULL) {
        return;
    }
    
    DWORD currentTime = GetTickCount();
    IMEState* state = (IMEState*)(controller + 1);
    
    if (currentTime - state->lastSwitchTime < 500) {
        return;
    }
    
    controller->isInputMode = enable;
    state->lastSwitchTime = currentTime;
    
    if (enable) {
        IMEController_EnableIME(controller);
    } else {
        IMEController_DisableIME(controller);
    }
    
    printf("[IME] Input mode changed to: %s\n", enable ? "true" : "false");
}

bool IMEController_IsInputModeEnabled(IMEController* controller) {
    return (controller != NULL) ? controller->isInputMode : false;
}

bool IMEController_DisableIME(IMEController* controller) {
    if (controller == NULL || controller->windowHandle == NULL) {
        return false;
    }
    
    IMEState* state = (IMEState*)(controller + 1);
    
    if (!state->imeEnabled) {
        return true;
    }
    
    DWORD currentTime = GetTickCount();
    if (currentTime - state->lastSwitchTime < 500) {
        return false;
    }
    
    HIMC hIMC = ImmGetContext(controller->windowHandle);
    if (hIMC != NULL) {
        ImmSetCompositionString(hIMC, SCS_SETSTR, NULL, 0, NULL, 0);
        ImmReleaseContext(controller->windowHandle, hIMC);
    }
    
    HIMC oldIMC = ImmAssociateContext(controller->windowHandle, NULL);
    if (oldIMC != NULL) {
        state->originalIMC = oldIMC;
    }
    
    state->imeEnabled = false;
    state->lastSwitchTime = currentTime;
    
    printf("[IME] IME disabled\n");
    return true;
}

bool IMEController_EnableIME(IMEController* controller) {
    if (controller == NULL || controller->windowHandle == NULL) {
        return false;
    }
    
    IMEState* state = (IMEState*)(controller + 1);
    
    if (state->imeEnabled) {
        return true;
    }
    
    DWORD currentTime = GetTickCount();
    if (currentTime - state->lastSwitchTime < 500) {
        return false;
    }
    
    if (state->originalIMC != NULL) {
        ImmAssociateContext(controller->windowHandle, state->originalIMC);
    } else {
        HIMC hIMC = ImmGetContext(controller->windowHandle);
        ImmAssociateContext(controller->windowHandle, hIMC);
        ImmReleaseContext(controller->windowHandle, hIMC);
    }
    
    state->imeEnabled = true;
    state->lastSwitchTime = currentTime;
    
    printf("[IME] IME enabled\n");
    return true;
}

bool IMEController_ForceEnglishIME(IMEController* controller) {
    return IMEController_DisableIME(controller);
}

void IMEController_Update(IMEController* controller) {
    if (controller == NULL || !controller->isActive) {
        return;
    }
    
    HWND foregroundWnd = GetForegroundWindow();
    if (foregroundWnd != controller->windowHandle) {
        return;
    }
    
    if (controller->isInputMode) {
        return;
    }
    
    IMEController_DisableIME(controller);
}

IMEInputBox* IMEInputBox_Create(HWND parentWnd, int x, int y, int width, int height) {
    IMEInputBox* inputBox = (IMEInputBox*)malloc(sizeof(IMEInputBox));
    if (inputBox == NULL) {
        return NULL;
    }
    
    inputBox->handle = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        x, y, width, height,
        parentWnd,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (inputBox->handle == NULL) {
        free(inputBox);
        return NULL;
    }
    
    inputBox->imc = ImmGetContext(inputBox->handle);
    inputBox->position.left = x;
    inputBox->position.top = y;
    inputBox->position.right = x + width;
    inputBox->position.bottom = y + height;
    inputBox->isFocused = false;
    
    return inputBox;
}

void IMEInputBox_Destroy(IMEInputBox* inputBox) {
    if (inputBox == NULL) {
        return;
    }
    
    if (inputBox->imc != NULL) {
        ImmReleaseContext(inputBox->handle, inputBox->imc);
    }
    
    if (inputBox->handle != NULL) {
        DestroyWindow(inputBox->handle);
    }
    
    free(inputBox);
}

void IMEInputBox_SetFocus(IMEController* controller, IMEInputBox* inputBox) {
    if (inputBox == NULL) {
        return;
    }
    
    SetFocus(inputBox->handle);
    inputBox->isFocused = true;
    
    if (controller != NULL) {
        controller->focusedInputBox = inputBox->handle;
    }
}

void IMEInputBox_LostFocus(IMEController* controller, IMEInputBox* inputBox) {
    if (inputBox == NULL) {
        return;
    }
    
    inputBox->isFocused = false;
    
    if (controller != NULL && controller->focusedInputBox == inputBox->handle) {
        controller->focusedInputBox = NULL;
    }
}

void IMEInputBox_SetPosition(IMEInputBox* inputBox, int x, int y) {
    if (inputBox == NULL) {
        return;
    }
    
    int width = inputBox->position.right - inputBox->position.left;
    int height = inputBox->position.bottom - inputBox->position.top;
    
    SetWindowPos(inputBox->handle, NULL, x, y, width, height, SWP_NOZORDER);
    
    inputBox->position.left = x;
    inputBox->position.top = y;
    inputBox->position.right = x + width;
    inputBox->position.bottom = y + height;
}

void IMEInputBox_UpdateCandidatePosition(IMEInputBox* inputBox) {
    if (inputBox == NULL || inputBox->handle == NULL) {
        return;
    }
    
    HIMC hIMC = ImmGetContext(inputBox->handle);
    if (hIMC != NULL) {
        COMPOSITIONFORM cf;
        cf.dwStyle = CFS_POINT;
        cf.ptCurrentPos.x = inputBox->position.left;
        cf.ptCurrentPos.y = inputBox->position.bottom + 5;
        
        ImmSetCompositionWindow(hIMC, &cf);
        ImmReleaseContext(inputBox->handle, hIMC);
    }
}