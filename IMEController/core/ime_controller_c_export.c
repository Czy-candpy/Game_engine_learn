#define IME_CONTROLLER_EXPORTS

#include "ime_controller_c.h"

#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllexport) IMEController* CreateIMEController(HWND hWnd) {
    return IMEController_Create(hWnd);
}

__declspec(dllexport) void DestroyIMEController(IMEController* controller) {
    IMEController_Destroy(controller);
}

__declspec(dllexport) void SetIMEControllerActive(IMEController* controller, bool active) {
    IMEController_SetActive(controller, active);
}

__declspec(dllexport) bool IsIMEControllerActive(IMEController* controller) {
    return IMEController_IsActive(controller);
}

__declspec(dllexport) void EnableIMEInputMode(IMEController* controller, bool enable) {
    IMEController_EnableInputMode(controller, enable);
}

__declspec(dllexport) bool IsIMEInputModeEnabled(IMEController* controller) {
    return IMEController_IsInputModeEnabled(controller);
}

__declspec(dllexport) bool ForceEnglishIME(IMEController* controller) {
    return IMEController_ForceEnglishIME(controller);
}

__declspec(dllexport) void UpdateIMEController(IMEController* controller) {
    IMEController_Update(controller);
}

__declspec(dllexport) IMEInputBox* CreateIMEInputBox(HWND parentWnd, int x, int y, int width, int height) {
    return IMEInputBox_Create(parentWnd, x, y, width, height);
}

__declspec(dllexport) void DestroyIMEInputBox(IMEInputBox* inputBox) {
    IMEInputBox_Destroy(inputBox);
}

__declspec(dllexport) void SetIMEInputBoxFocus(IMEController* controller, IMEInputBox* inputBox) {
    IMEInputBox_SetFocus(controller, inputBox);
}

__declspec(dllexport) void LostIMEInputBoxFocus(IMEController* controller, IMEInputBox* inputBox) {
    IMEInputBox_LostFocus(controller, inputBox);
}

__declspec(dllexport) void SetIMEInputBoxPosition(IMEInputBox* inputBox, int x, int y) {
    IMEInputBox_SetPosition(inputBox, x, y);
}

__declspec(dllexport) void UpdateIMEInputBoxCandidatePosition(IMEInputBox* inputBox) {
    IMEInputBox_UpdateCandidatePosition(inputBox);
}

#ifdef __cplusplus
}
#endif