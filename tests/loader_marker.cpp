#include <windows.h>
#include <atomic>
namespace {std::atomic<DWORD> worker_id{0};std::atomic<bool> started{false};
DWORD WINAPI work(void*){auto library=LoadLibraryExW(L"version.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);if(library){FreeLibrary(library);worker_id=GetCurrentThreadId();}return 0;}}
extern "C" __declspec(dllexport) DWORD ContractWorker(){return worker_id.load();}
extern "C" __declspec(dllexport) void InitializeASI(){bool expected=false;if(started.compare_exchange_strong(expected,true)){auto t=CreateThread(nullptr,0,work,nullptr,0,nullptr);if(t)CloseHandle(t);}}
BOOL APIENTRY DllMain(HMODULE,DWORD,LPVOID){return TRUE;}
