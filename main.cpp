
// open a handle to the target process
// allocate memory
// locate loadlibrarya using kernel32 as the base
// create a thread to loadlibrarya using the dll path as an arg

#include <iostream>
#include <Windows.h>

char target_window_name[_MAX_PATH] = "Tutorial-i386";

int main(){

	bool result{};

	// Get the full path of the DLL to inject.
	char full_path[_MAX_PATH]{};
	result = GetFullPathNameA("haxy.dll", _MAX_PATH, full_path, NULL);
	if(result == NULL){
		std::cout << "Failed to get the full file path of the dll." << '\n';
		return 1;
	}

	// Check if the DLL at the target path actually exists.
	FILE* dllExists;
	fopen_s(&dllExists, full_path, "r");
	if(dllExists == NULL){
		std::cout << "Could not find the DLL to inject." << '\n';
		return 1;
	}

	// Get a handle to the window of the target.
	HWND windowHandle = FindWindowA(NULL, target_window_name);
	if(windowHandle == NULL){
		std::cout << "Failed to get window handle. Error Code: " << GetLastError() << '\n';
		return 1;
	}

	// Get the process PID.
	DWORD PID{};
	GetWindowThreadProcessId(windowHandle, &PID);

	if(PID == NULL){
		std::cout << "Failed to get window PID. Error Code: " << GetLastError() << '\n';
		return 1;
	}

	// Open a handle to the target process.
	HANDLE pHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);
	if(pHandle == NULL){
		std::cout << "Failed get a handle to the process. Error Code: " << GetLastError() << '\n';
		return 1;
	}

	// Allocate memory in the target process.
	LPVOID memory = VirtualAllocEx(pHandle, NULL, _MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if(memory == NULL){
		std::cout << "Failed to allocate memory in the target process. Error Code: " << GetLastError() << '\n';
		return 1;
	}

	// Write the DLL path into the allocated memory in the target process.
	result = WriteProcessMemory(pHandle, memory, full_path, strlen(full_path), NULL);
	if(result == NULL){
		std::cout << "Failed to write process memory at the target process. Error Code: " << GetLastError() << '\n';
		return 1;
	}

	HMODULE kernelHandle = GetModuleHandleA("KERNEL32");
	if(kernelHandle == NULL){
		std::cout << "Failed to get a handle to the kernel." << '\n';
		return 1;
	}

	// Get a pointer to LoadLibraryA.
	LPVOID LoadLibrary_addr = GetProcAddress(kernelHandle, "LoadLibraryA");
	if(LoadLibrary_addr == NULL){
		std::cout << "Failed to get address to LoadLibraryA." << '\n';
		return 1;
	}

	// Create a thread at LoadLibraryA and pass in the DLL path at the allocated memory as the arg.
	HANDLE LLAThread = CreateRemoteThread(pHandle, NULL, NULL, (LPTHREAD_START_ROUTINE)LoadLibrary_addr, memory, NULL, NULL);
	if(LLAThread == NULL){
		std::cout << "Failed to create remote thread at LoadLibraryA." << '\n';
		return 1;
	}

	WaitForSingleObject(LLAThread, INFINITE);

	VirtualFreeEx(pHandle, memory, 0, MEM_RELEASE);
	std::cout << CloseHandle(LLAThread) << '\n';
	std::cout << CloseHandle(pHandle) << '\n';

	std::cout << "Injection should be successful." << '\n';

	Sleep(1000);

	return 0;
}