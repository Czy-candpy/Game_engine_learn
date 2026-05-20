#pragma once

#define UNICODE
#define _UNICODE

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef IME_CONTROLLER_EXPORTS
#define IME_API __declspec(dllexport)
#else
#define IME_API __declspec(dllimport)
#endif

#include <windows.h>

typedef struct {
    HWND windowHandle;
    bool isActive;
    bool isInputMode;
    HIMC currentIMC;
    HWND focusedInputBox;
} IMEController;

typedef struct {
    HWND handle;
    HIMC imc;
    RECT position;
    bool isFocused;
} IMEInputBox;

IME_API IMEController* IMEController_Create(HWND hWnd);
IME_API void IMEController_Destroy(IMEController* controller);
IME_API void IMEController_SetActive(IMEController* controller, bool active);
IME_API bool IMEController_IsActive(IMEController* controller);
IME_API void IMEController_EnableInputMode(IMEController* controller, bool enable);
IME_API bool IMEController_IsInputModeEnabled(IMEController* controller);
IME_API bool IMEController_DisableIME(IMEController* controller);
IME_API bool IMEController_EnableIME(IMEController* controller);
IME_API bool IMEController_ForceEnglishIME(IMEController* controller);
IME_API void IMEController_Update(IMEController* controller);

IME_API IMEInputBox* IMEInputBox_Create(HWND parentWnd, int x, int y, int width, int height);
IME_API void IMEInputBox_Destroy(IMEInputBox* inputBox);
IME_API void IMEInputBox_SetFocus(IMEController* controller, IMEInputBox* inputBox);
IME_API void IMEInputBox_LostFocus(IMEController* controller, IMEInputBox* inputBox);
IME_API void IMEInputBox_SetPosition(IMEInputBox* inputBox, int x, int y);
IME_API void IMEInputBox_UpdateCandidatePosition(IMEInputBox* inputBox);

#ifdef __cplusplus
}
#endif