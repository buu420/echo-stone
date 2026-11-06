// Local read-only diagnostic. This executable is not the in-process reader.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include "mvci/unreal.hpp"
#include "mvci/selection.hpp"
#include "mvci/combat.hpp"
#include "mvci/collection.hpp"
#include "mvci/mission.hpp"
#include "mvci/story.hpp"
#include <iostream>
#include <string>
static std::string utf8(const std::wstring& s){if(s.empty())return {};int n=WideCharToMultiByte(CP_UTF8,0,s.data(),static_cast<int>(s.size()),nullptr,0,nullptr,nullptr);std::string v(n,'\0');WideCharToMultiByte(CP_UTF8,0,s.data(),static_cast<int>(s.size()),v.data(),n,nullptr,nullptr);return v;}
int wmain(int argc,wchar_t** argv){
 if(argc!=2){std::cerr<<"usage: mvci_probe PID\n";return 2;}DWORD pid=static_cast<DWORD>(std::stoul(argv[1]));
 HANDLE process=OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);if(!process){std::cerr<<"OpenProcess "<<GetLastError()<<'\n';return 3;}
 HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);MODULEENTRY32W module{};module.dwSize=sizeof(module);mvci::Address base=0;
 for(BOOL ok=Module32FirstW(snapshot,&module);ok;ok=Module32NextW(snapshot,&module))if(_wcsicmp(module.szExePath,L"D:\\SteamLibrary\\steamapps\\common\\MARVEL VS. CAPCOM INFINITE\\MVCI\\Binaries\\Win64\\MVCI.exe")==0){base=reinterpret_cast<mvci::Address>(module.modBaseAddr);break;}
 CloseHandle(snapshot);if(!base){CloseHandle(process);std::cerr<<"Exact MVCI target missing\n";return 4;}
 mvci::Memory memory;memory.reader=[process](mvci::Address a,void* out,std::size_t size){SIZE_T got=0;return ReadProcessMemory(process,reinterpret_cast<const void*>(a),out,size,&got)&&got==size;};
 mvci::Unreal unreal(memory,base);if(!unreal.prepare_capture(GetTickCount64())){CloseHandle(process);std::cerr<<"Native object discovery unavailable\n";return 5;}auto widgets=unreal.widgets();std::cout<<"widgets "<<widgets.size()<<'\n';for(const auto& w:widgets)if(unreal.shown(w.address))std::cout<<"shown "<<utf8(unreal.type(w.address))<<' '<<utf8(unreal.name(w.address,true))<<'\n';
 for(const auto& widget:widgets)if(unreal.is_a(widget.address,L"WD_Title_C")&&unreal.shown(widget.address)){
  const auto p=unreal.property(widget.address,L"TextBlock_213");const auto block=unreal.object_field(widget.address,L"TextBlock_213");
  std::cout<<"title prompt property="<<bool(p)<<" field="<<bool(block)<<" address="<<std::hex<<block.value_or(0)<<std::dec<<'\n';
  if(block&&*block){const auto text=unreal.text_field(*block,L"Text");std::cout<<"prompt object="<<bool(unreal.object(*block))<<" widget="<<unreal.is_a(*block,L"Widget")<<" shown="<<unreal.shown(*block)<<" text="<<bool(text)<<" value="<<utf8(text.value_or(L""))<<'\n';}
 }
 mvci::MenuCapture capture(unreal);auto menu=capture.sample(GetTickCount64());std::cout<<"valid "<<menu.valid<<" modal "<<menu.modal<<" blocked "<<menu.blocked<<"\nscreen: "<<utf8(menu.screen)<<"\ntext: "<<utf8(menu.label)<<"\nvalue: "<<utf8(menu.value)<<"\nhelp: "<<utf8(menu.help)<<'\n';
 if(!menu.valid&&!menu.blocked){mvci::CollectionCapture collection(unreal);menu=collection.sample(GetTickCount64());std::cout<<"collection valid="<<menu.valid<<" blocked="<<menu.blocked<<" screen="<<utf8(menu.screen)<<" label="<<utf8(menu.label)<<" value="<<utf8(menu.value)<<" enabled="<<menu.enabled<<" position="<<menu.position<<" count="<<menu.count<<" help="<<utf8(menu.help)<<'\n';}
 mvci::MissionCapture mission(unreal);
 if(!menu.modal){
  auto battle=mission.battle(!menu.valid&&!menu.blocked);
  if(battle.menu.valid||battle.menu.blocked){menu=std::move(battle.menu);std::cout<<"mission tutorial="<<utf8(menu.label)<<" help="<<utf8(menu.help)<<" blocked="<<menu.blocked<<'\n';}
  if(battle.objective.valid)std::cout<<"mission objective="<<utf8(battle.objective.text)<<'\n';
  if(battle.commands.valid)std::cout<<"mission commands="<<utf8(battle.commands.text)<<'\n';
 }
 if(!menu.valid&&!menu.blocked){menu=mission.sample(GetTickCount64());std::cout<<"mission valid="<<menu.valid<<" blocked="<<menu.blocked<<" screen="<<utf8(menu.screen)<<" label="<<utf8(menu.label)<<" value="<<utf8(menu.value)<<" help="<<utf8(menu.help)<<" rejection="<<utf8(mission.rejection())<<'\n';}
 mvci::StoryCapture story_capture(unreal);const auto story=story_capture.sample(GetTickCount64());
 if(!menu.valid&&!menu.blocked)menu=story.menu;
 const auto& movie=story.story.movie;std::cout<<"story valid="<<story.story.valid<<" movie="<<utf8(movie.id)<<" bound="<<movie.valid<<" playing="<<movie.playing<<" paused="<<movie.paused<<" sequence_seconds="<<movie.sequence_seconds.value_or(-1)<<" movie_seconds="<<movie.movie_seconds.value_or(-1)<<" frame="<<movie.frame.value_or(-1)<<" menu="<<utf8(story.menu.label)<<'\n';
 for(const auto& caption:story.story.captions)if(caption.valid)std::cout<<"story text="<<utf8(caption.text)<<'\n';if(story.story.subtitle.valid)std::cout<<"story repeat_only_subtitle="<<utf8(story.story.subtitle.text)<<'\n';
 mvci::SelectionCapture selection(unreal);bool selected=false;
 if(!menu.valid&&!menu.blocked)for(int team=0;team<2;++team){auto s=selection.sample(GetTickCount64(),team);selected|=s.valid;std::cout<<"selection player="<<team+1<<" valid="<<s.valid<<" label="<<utf8(s.label)<<" value="<<utf8(s.value)<<" help="<<utf8(s.help)<<" rejection="<<utf8(selection.rejection())<<'\n';}
 mvci::CombatCapture combat(unreal);const auto battle=combat.sample(GetTickCount64(),menu.valid||menu.blocked||selected);std::cout<<"combat valid="<<battle.valid<<" playable="<<battle.playable<<" cinematic="<<battle.cinematic<<" session="<<battle.session<<" diagnostic_distance="<<battle.normalized_distance.value_or(-1)<<'\n';
 for(int team=0;team<2;++team){const auto& f=battle.fighters[team];std::cout<<"fighter player="<<team+1<<" x="<<f.x.value_or(-100001)<<" health="<<f.health_percent.value_or(-1)<<" health_fighter="<<f.health_fighter.value_or(-1)<<" hyper="<<f.hyper_bars.value_or(-1)<<" infinity="<<f.infinity_fraction.value_or(-1)<<" storm="<<(f.storm_active?int(*f.storm_active):-1)<<'\n';}
 CloseHandle(process);return menu.valid||selected||battle.valid?0:5;
}
