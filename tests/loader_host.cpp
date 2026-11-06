#include <windows.h>
#include <cstdio>
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;
 const auto kernel=GetModuleHandleW(L"kernel32.dll");
 // Resolve APIs before loading the proxy. Calls through these saved addresses do
 // not enter the host's subsequently patched import table. This models a host
 // whose deferred IAT callbacks cannot be reached after protected startup.
 const auto module=reinterpret_cast<HMODULE(WINAPI*)(LPCWSTR)>(GetProcAddress(kernel,"GetModuleHandleW"));
 const auto procedure=reinterpret_cast<FARPROC(WINAPI*)(HMODULE,LPCSTR)>(GetProcAddress(kernel,"GetProcAddress"));
 const auto load=reinterpret_cast<HMODULE(WINAPI*)(LPCWSTR)>(GetProcAddress(kernel,"LoadLibraryW"));
 const auto sleep=reinterpret_cast<void(WINAPI*)(DWORD)>(GetProcAddress(kernel,"Sleep"));
 const auto owner=GetCurrentThreadId();if(!module||!procedure||!load||!sleep)return 3;
 if(!load(argv[1]))return 4;
 DWORD worker=0;
 for(int i=0;i<100&&!worker;++i){auto marker=module(L"MVCIContractMarker.asi");if(marker){auto query=reinterpret_cast<DWORD(*)()>(procedure(marker,"ContractWorker"));if(query)worker=query();}if(!worker)sleep(20);}
 const bool passed=worker&&worker!=owner;
 std::puts(passed?"pinned loader initialized an independent worker":"ASI worker was not initialized without a deferred IAT callback");return passed?0:1;
}
