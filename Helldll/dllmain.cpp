#include <Windows.h>
#include <vector>
#include <CommCtrl.h>

HWND hwndCheckbox;

BOOL IsCheckboxChecked()
{
    return SendMessage(hwndCheckbox, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        // Create the checkbox
        hwndCheckbox = CreateWindowEx(
            0,
            WC_BUTTON,
            L"Add 100",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            10, 10, 150, 25,
            hwnd,
            NULL,
            NULL,
            NULL
        );
        break;
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            if (IsCheckboxChecked())
            {
                // Get the module base address
                const wchar_t* moduleName = L"mono-2.0-bdwgc.dll";
                HMODULE moduleHandle = GetModuleHandle(moduleName);
                if (moduleHandle == NULL)
                {
                    MessageBox(0, L"Failed to get module handle.", L"Error", MB_ICONERROR);
                    break;
                }
                uintptr_t moduleBase = reinterpret_cast<uintptr_t>(moduleHandle);

                // Calculate the final address using the base address and offsets
                uintptr_t baseAddress = 0x009F1318;
                std::vector<uintptr_t> offsets = { 0xCC8, 0x600, 0x79C, 0x0, 0x1E8, 0x10, 0x20 };

                uintptr_t finalAddress = moduleBase + baseAddress;
                for (size_t i = 0; i < offsets.size(); i++)
                {
                    finalAddress = *(uintptr_t*)finalAddress;
                    finalAddress += offsets[i];
                }

                // Read the current value
                double value = 0.0;
                ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(finalAddress), &value, sizeof(value), nullptr);

                // Add 100 to the value
                value += 100.0;

                // Write the updated value to the final address
                WriteProcessMemory(GetCurrentProcess(), reinterpret_cast<LPVOID>(finalAddress), &value, sizeof(value), nullptr);
            }
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        // Create the window class
        const wchar_t CLASS_NAME[] = L"SampleWindowClass";

        WNDCLASS wc = { 0 };
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hModule;
        wc.lpszClassName = CLASS_NAME;

        RegisterClass(&wc);

        // Create the window
        HWND hwnd = CreateWindowEx(
            0,
            CLASS_NAME,
            L"Sample Window",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 300, 200,
            NULL,
            NULL,
            hModule,
            NULL
        );

        if (hwnd == NULL)
        {
            MessageBox(0, L"Failed to create window.", L"Error", MB_ICONERROR);
            return FALSE;
        }

        ShowWindow(hwnd, SW_SHOW);

        // Run the message loop
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return TRUE;
}