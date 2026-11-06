#include "mvci/prism.hpp"
#include <windows.h>
#include <filesystem>
namespace mvci {
struct Prism::Impl {
 HMODULE module=nullptr;void* context=nullptr;void* backend=nullptr;DWORD owner=0;
 using Init=void*(__cdecl*)(const void*);using Best=void*(__cdecl*)(void*);
 using Output=int(__cdecl*)(void*,const char*,bool);using Stop=int(__cdecl*)(void*);
 using Free=void(__cdecl*)(void*);using Name=const char*(__cdecl*)(void*);
 using Speaking=int(__cdecl*)(void*,bool*);using Features=std::uint64_t(__cdecl*)(void*);
 Init init=nullptr;Best best=nullptr;Output output=nullptr;Stop stop=nullptr;Free release=nullptr,shutdown=nullptr;Name name=nullptr;
 Speaking speaking=nullptr;Features features=nullptr;
 template<class T>T symbol(const char* n){return reinterpret_cast<T>(GetProcAddress(module,n));}
 ~Impl(){
  if(owner&&owner!=GetCurrentThreadId())return;
  if(backend&&release)release(backend);if(context&&shutdown)shutdown(context);if(module)FreeLibrary(module);
 }
};
Prism::Prism():impl_(std::make_unique<Impl>()){}
Prism::~Prism()=default;
bool Prism::open(const std::wstring& path){
 auto& p=*impl_;if((p.owner&&p.owner!=GetCurrentThreadId())||!std::filesystem::path(path).is_absolute())return false;
 if(!p.module){
  p.module=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);if(!p.module)return false;
  p.init=p.symbol<Impl::Init>("prism_init");p.best=p.symbol<Impl::Best>("prism_registry_create_best");p.output=p.symbol<Impl::Output>("prism_backend_output");p.stop=p.symbol<Impl::Stop>("prism_backend_stop");p.release=p.symbol<Impl::Free>("prism_backend_free");p.shutdown=p.symbol<Impl::Free>("prism_shutdown");p.name=p.symbol<Impl::Name>("prism_backend_name");
  p.speaking=p.symbol<Impl::Speaking>("prism_backend_is_speaking");p.features=p.symbol<Impl::Features>("prism_backend_get_features");
 }
 if(!p.init||!p.best||!p.output||!p.stop||!p.release||!p.name||!p.shutdown)return false;
 p.owner=GetCurrentThreadId();if(!p.context)p.context=p.init(nullptr);if(!p.context)return false;if(!p.backend)p.backend=p.best(p.context);return p.backend!=nullptr;
}
bool Prism::speak(const std::wstring& text,bool interrupt){
 auto& p=*impl_;if(!p.backend||p.owner!=GetCurrentThreadId()||text.empty()||text.size()>16384)return false;
 int length=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,nullptr);if(length<=0)return false;
 std::string value(static_cast<std::size_t>(length),'\0');if(!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),value.data(),length,nullptr,nullptr))return false;
 const int status=p.output(p.backend,value.c_str(),interrupt);if(status!=0){p.release(p.backend);p.backend=nullptr;}return status==0;
}
bool Prism::stop(){auto& p=*impl_;return p.backend&&p.owner==GetCurrentThreadId()&&p.stop(p.backend)==0;}
std::optional<bool> Prism::speaking()const{
 auto& p=*impl_;if(!p.backend||p.owner!=GetCurrentThreadId()||!p.speaking||!p.features||(p.features(p.backend)&(1ull<<6))==0)return {};
 bool result=false;if(p.speaking(p.backend,&result)!=0)return {};return result;
}
std::string Prism::backend_name()const{auto& p=*impl_;if(!p.backend||!p.name||p.owner!=GetCurrentThreadId())return {};const char* name=p.name(p.backend);return name?name:"";}
}
