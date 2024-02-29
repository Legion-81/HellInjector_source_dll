#include <iostream>
#include <Windows.h>
#include <Psapi.h>
#include <cstring>

using namespace std;

int main()
{
    wchar_t injectorPath[MAX_PATH];
    GetModuleFileNameW(NULL, injectorPath, MAX_PATH);

    // Remove the injector executable name from the path
    wchar_t* lastBackslash = wcsrchr(injectorPath, L'\\');
    if (lastBackslash != NULL)
        *(lastBackslash + 1) = L'\0';

    // Concatenate the injector path with the DLL filename
    wchar_t dllName[] = L"Helldll.dll";
    wchar_t dllPath[MAX_PATH];
    wcscpy_s(dllPath, injectorPath);
    wcscat_s(dllPath, dllName);

    // Use dllPath as the new DllPath in your injection code
    LPCWSTR DllPath = dllPath;

    // Find the process ID using the executable name
    wchar_t exeName[] = L"IdleWizard.exe";
    DWORD processes[1024];
    DWORD needed;
    if (!EnumProcesses(processes, sizeof(processes), &needed))
    {
        cout << "Failed to enumerate processes." << endl;
        return 1;
    }

    int processCount = needed / sizeof(DWORD);

    DWORD procID = 0;
    for (int i = 0; i < processCount; i++)
    {
        if (processes[i] != 0)
        {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processes[i]);
            if (hProcess)
            {
                wchar_t processName[MAX_PATH];
                if (GetModuleBaseNameW(hProcess, NULL, processName, sizeof(processName) / sizeof(wchar_t)))
                {
                    if (wcscmp(processName, exeName) == 0)
                    {
                        procID = processes[i];
                        break;
                    }
                }
            }
            CloseHandle(hProcess);
        }
    }

    if (procID == 0)
    {
        cout << "Process not found." << endl;
        return 1;
    }

    HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);

    // Allocate memory for the dllpath in the target process, length of the path string + null terminator
    LPVOID pDllPath = VirtualAllocEx(handle, 0, (wcslen(DllPath) + 1) * sizeof(wchar_t), MEM_COMMIT, PAGE_READWRITE);

    // Write the path to the address of the memory we just allocated in the target process
    WriteProcessMemory(handle, pDllPath, (LPVOID)DllPath, (wcslen(DllPath) + 1) * sizeof(wchar_t), 0);

    // Create a Remote Thread in the target process which calls LoadLibraryW as our dllpath as an argument -> program loads our dll
    HANDLE hLoadThread = CreateRemoteThread(handle, 0, 0,
        (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"Kernel32.dll"), "LoadLibraryW"), pDllPath, 0, 0);

    WaitForSingleObject(hLoadThread, INFINITE);

    cout << "Dll path allocated at: " << hex << pDllPath << endl;
    cin.get();

    VirtualFreeEx(handle, pDllPath, (wcslen(DllPath) + 1) * sizeof(wchar_t), MEM_RELEASE);

    return 0;
}