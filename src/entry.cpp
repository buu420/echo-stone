#include <windows.h>
#include <MinHook.h>
#include "mvci/beacon.hpp"
#include "mvci/combat.hpp"
#include "mvci/collection.hpp"
#include "mvci/mission.hpp"
#include "mvci/story.hpp"
#include "mvci/descriptions.hpp"
#include "mvci/status_capture.hpp"
#include "mvci/capture_pacer.hpp"
#include "mvci/controls.hpp"
#include "mvci/prism.hpp"
#include "mvci/target.hpp"
#include "mvci/selection.hpp"
#include "mvci/speech_gate.hpp"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>

namespace {
HMODULE self_module=nullptr;
std::atomic<bool> started{false},stopping{false};
using Event=void(__fastcall*)(void*,void*,void*);
std::wstring module_path(HMODULE module){std::wstring path(32768,L'\0');DWORD n=GetModuleFileNameW(module,path.data(),static_cast<DWORD>(path.size()));if(!n||n>=path.size())return {};path.resize(n);return path;}
std::string utf8(const std::wstring& s){if(s.empty())return {};int n=WideCharToMultiByte(CP_UTF8,0,s.data(),static_cast<int>(s.size()),nullptr,0,nullptr,nullptr);if(n<=0)return {};std::string out(static_cast<std::size_t>(n),'\0');WideCharToMultiByte(CP_UTF8,0,s.data(),static_cast<int>(s.size()),out.data(),n,nullptr,nullptr);return out;}
bool safe_read(mvci::Address a,void* out,std::size_t n){__try{std::memcpy(out,reinterpret_cast<const void*>(a),n);return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
int visibility(mvci::Address base,mvci::Address object){__try{return reinterpret_cast<unsigned char(__fastcall*)(mvci::Address)>(base+0x120f5e0)(object);}__except(EXCEPTION_EXECUTE_HANDLER){return -1;}}
bool focused(){DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);return pid==GetCurrentProcessId();}
BOOL CALLBACK find_window(HWND window,LPARAM param){DWORD pid=0;DWORD thread=GetWindowThreadProcessId(window,&pid);if(pid!=GetCurrentProcessId())return TRUE;wchar_t name[64]{};GetClassNameW(window,name,64);if(wcscmp(name,L"UnrealWindow")!=0)return TRUE;*reinterpret_cast<DWORD*>(param)=thread;return FALSE;}
DWORD game_window_thread(){DWORD thread=0;EnumWindows(find_window,reinterpret_cast<LPARAM>(&thread));return thread;}
struct State {
 mvci::Address base=0;
 mvci::Memory memory;
 mvci::Unreal unreal;
 mvci::MenuCapture menus;
 mvci::CombatCapture combat;
 mvci::SelectionCapture selection;
 mvci::CollectionCapture collection;
 mvci::MissionCapture mission;
 mvci::StoryCapture story;
 Event original=nullptr;
 std::atomic<DWORD> ui_thread{0};
 mvci::CapturePacer capture_pacer;
 std::atomic<std::uint64_t> captures{0},max_capture_us{0},capture_errors{0};
 std::atomic<std::uint64_t> capture_work_us{0};
 std::atomic<std::uint64_t> hud_requested{0};
 std::array<std::atomic<std::uint64_t>,7> capture_phase_us{};
 std::mutex mutex,log_mutex;
 mvci::Snapshot latest;
 std::array<mvci::MenuSnapshot,2> selection_menus;
 std::array<std::wstring,2> selection_rejections;
 mvci::DiscoveryStats latest_discovery;
 std::uint64_t revision=0;
 std::filesystem::path data_dir;
 explicit State(mvci::Address b,std::filesystem::path dir):base(b),unreal(memory,b),menus(unreal,[b](mvci::Address o)->std::optional<int>{int v=visibility(b,o);if(v<0||v>4)return {};return v;}),combat(unreal),selection(unreal,[b](mvci::Address o)->std::optional<int>{int v=visibility(b,o);if(v<0||v>4)return {};return v;}),collection(unreal,[b](mvci::Address o)->std::optional<int>{int v=visibility(b,o);if(v<0||v>4)return {};return v;}),mission(unreal,[b](mvci::Address o)->std::optional<int>{int v=visibility(b,o);if(v<0||v>4)return {};return v;}),story(unreal,[b](mvci::Address o)->std::optional<int>{int v=visibility(b,o);if(v<0||v>4)return {};return v;}),data_dir(std::move(dir)){memory.reader=safe_read;}
 void log(const std::string& line){std::lock_guard<std::mutex> lock(log_mutex);std::ofstream file(data_dir/L"reader.log",std::ios::app);if(file)file<<GetTickCount64()<<' '<<line<<'\n';}
};
std::atomic<State*> active{nullptr};
thread_local int event_depth=0;
thread_local bool in_capture=false;
std::uint64_t capture_clock(){return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
void __fastcall event_hook(void* object,void* function,void* parameters){
 State* state=active.load(std::memory_order_acquire);if(!state||!state->original)return;
 ++event_depth;
 try{state->original(object,function,parameters);}catch(...){--event_depth;throw;}
 --event_depth;
 if(in_capture||stopping.load()||GetCurrentThreadId()!=state->ui_thread.load())return;
 state->unreal.observe_event(reinterpret_cast<mvci::Address>(object));
 // Nested native events can construct a dialog in a reused registry slot.
 // Retain their address hints too; actual reads/capture still wait for the
 // outermost event to finish, and callbacks from capture are excluded above.
 if(event_depth)return;
 const auto begin=capture_clock();if(!state->capture_pacer.due(begin))return;in_capture=true;
 const auto now=GetTickCount64();auto stage_at=begin;
 const auto stage=[&](std::size_t index){const auto at=capture_clock();state->capture_phase_us[index].fetch_add(at-stage_at,std::memory_order_relaxed);stage_at=at;};
 try{
  mvci::Snapshot snapshot;const bool ready=state->unreal.prepare_capture(now);stage(0);
  if(ready)snapshot.menu=state->menus.sample(now);stage(1);
  if(ready&&!snapshot.menu.valid&&!snapshot.menu.blocked)snapshot.menu=state->collection.sample(now);stage(2);
  const auto hud_ticket=state->hud_requested.load(std::memory_order_acquire);
  mvci::MissionBattleSnapshot mission_battle;
  if(ready&&!snapshot.menu.modal){
   mission_battle=state->mission.battle(!snapshot.menu.valid&&!snapshot.menu.blocked,hud_ticket!=0);
   if(mission_battle.menu.valid||mission_battle.menu.blocked)snapshot.menu=std::move(mission_battle.menu);
  }
  if(ready&&!snapshot.menu.valid&&!snapshot.menu.blocked)snapshot.menu=state->mission.sample(now);stage(3);
  if(ready){auto story=state->story.sample(now,hud_ticket!=0);snapshot.story=std::move(story.story);if(!snapshot.menu.valid&&!snapshot.menu.blocked)snapshot.menu=std::move(story.menu);}
  if(mission_battle.hud_identity&&!snapshot.menu.valid&&!snapshot.menu.blocked&&!snapshot.story.loading.visible){
   snapshot.mission_hud=mission_battle.hud_identity;
   snapshot.mission_command_view=mission_battle.command_view;
   if(mission_battle.objective.valid){snapshot.story.valid=true;snapshot.story.captions[2]=std::move(mission_battle.objective);}
   if(mission_battle.commands.valid){snapshot.story.valid=true;snapshot.story.subtitle=std::move(mission_battle.commands);}
  }
  snapshot.hud_capture_ticket=hud_ticket;stage(4);
  std::array<mvci::MenuSnapshot,2> selections;
  std::array<std::wstring,2> rejections;
  if(ready&&!snapshot.menu.valid&&!snapshot.menu.blocked)for(int team=0;team<2;++team){selections[team]=state->selection.sample(now,team);rejections[team]=state->selection.rejection();}stage(5);
  if(ready)snapshot.combat=state->combat.sample(now,snapshot.menu.valid||snapshot.menu.blocked||selections[0].valid||selections[1].valid);stage(6);snapshot.observed_ms=now;
  {std::lock_guard<std::mutex> lock(state->mutex);state->latest=std::move(snapshot);state->selection_menus=std::move(selections);state->selection_rejections=std::move(rejections);state->latest_discovery=state->unreal.discovery_stats();++state->revision;}
 }catch(...){++state->capture_errors;std::lock_guard<std::mutex> lock(state->mutex);state->latest={};state->selection_menus={};state->selection_rejections={};++state->revision;}
 const auto end=capture_clock();state->capture_pacer.completed(end,33000);++state->captures;
 const auto elapsed=end-begin;state->capture_work_us.fetch_add(elapsed,std::memory_order_relaxed);auto previous=state->max_capture_us.load();while(elapsed>previous&&!state->max_capture_us.compare_exchange_weak(previous,elapsed)){}
 in_capture=false;
}
DWORD worker_impl(){
 const auto self_path=std::filesystem::path(module_path(self_module));const auto data_dir=self_path.parent_path()/L"MVCIAccess";
 std::error_code ec;std::filesystem::create_directories(data_dir,ec);
 auto* state=new State(reinterpret_cast<mvci::Address>(GetModuleHandleW(nullptr)),data_dir);
 state->log("Echo Stone 0.1.0 starting; shared incremental discovery and completion-paced capture; F5 player/F6 beacon/F7 status/F8 menu/F9 side/F10 descriptions; distance, side and descriptions default on; Story HUD read only on F7 demand");
 mvci::Prism prism;
 const auto prism_path=(data_dir/L"prism.dll").wstring();
 if(mvci::sha256_file(prism_path)!= "402cc717d01b8b6574adbcb7a1627d30c078061d6be2b9664958f661ea0aec75"){state->log("ERROR Prism dependency hash mismatch");return 1;}
 bool speech=prism.open(prism_path);if(speech)state->log("Prism backend "+prism.backend_name());
 if(mvci::sha256_file(module_path(GetModuleHandleW(nullptr)))!=mvci::supported_game_sha256){state->log("ERROR Unsupported executable; no hooks installed");if(speech)prism.speak(L"Echo Stone does not support this MVC Infinite build.");return 2;}
 // The eager compatibility loader only starts this worker. A real Unreal window
 // proves the protected bootstrap has reached the game; exact bytes are checked
 // again here, so no hook can race the earlier text-decoding phase.
 const auto deadline=GetTickCount64()+120000;DWORD initial_ui_thread=0;
 while(!stopping.load()&&GetTickCount64()<deadline){initial_ui_thread=game_window_thread();if(mvci::ready_for_hook(state->memory,state->base,initial_ui_thread))break;Sleep(20);}
 if(!mvci::ready_for_hook(state->memory,state->base,initial_ui_thread)){state->log("ERROR Game window or native UI code is not ready; no hooks installed");if(speech)prism.speak(L"Echo Stone could not attach to the game.");return 3;}
 HMODULE pinned=nullptr;GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(self_module),&pinned);
 state->ui_thread=initial_ui_thread;active.store(state,std::memory_order_release);
 const auto hook_init=MH_Initialize();
 if((hook_init!=MH_OK&&hook_init!=MH_ERROR_ALREADY_INITIALIZED)||MH_CreateHook(reinterpret_cast<void*>(state->base+mvci::process_event_rva),reinterpret_cast<void*>(&event_hook),reinterpret_cast<void**>(&state->original))!=MH_OK||MH_EnableHook(reinterpret_cast<void*>(state->base+mvci::process_event_rva))!=MH_OK){state->log("ERROR Cannot install the verified native UI hook");if(speech)prism.speak(L"Echo Stone could not attach to the game.");return 4;}
 state->log("Verified x64 ProcessEvent hook active; capture only after the outermost event on the game window thread");
 mvci::Model model;mvci::Controls controls;mvci::BeaconAudio audio;mvci::SpeechGate delivery;mvci::StatusCaptureGate status_capture;std::uint64_t audio_epoch=0;mvci::DescriptionAudio descriptions(data_dir.wstring(),[state,&audio_epoch]{std::lock_guard<std::mutex> lock(state->mutex);return state->revision==audio_epoch&&focused();});
 std::uint64_t retry_at=0,window_at=0,diagnostics_at=0;bool introduced=false,was_foreground=false;
 std::array<std::wstring,2> previous_rejections;
 std::uint64_t previous_capture_count=0;
 std::string previous_story_phase;
 std::string previous_loading_state;
 std::string previous_description;
 while(!stopping.load()){
  const auto now=GetTickCount64();
  if(now>=window_at){state->ui_thread=game_window_thread();window_at=now+500;}
  if(!speech&&now>=retry_at){speech=prism.open(prism_path);retry_at=now+2000;if(speech)state->log("Prism backend "+prism.backend_name());}
  const bool foreground=focused();if(was_foreground&&!foreground){prism.stop();delivery.reset();}was_foreground=foreground;std::array<bool,6> keys{};for(int i=0;i<6;++i)keys[i]=(GetAsyncKeyState(VK_F5+i)&0x8000)!=0;controls.observe(keys,foreground);
  mvci::Snapshot snapshot;mvci::DiscoveryStats discovery;std::array<std::wstring,2> rejections;std::uint64_t revision=0;{std::lock_guard<std::mutex> lock(state->mutex);snapshot=state->latest;if(!snapshot.menu.valid&&!snapshot.menu.blocked)snapshot.menu=state->selection_menus[model.player()];rejections=state->selection_rejections;discovery=state->latest_discovery;revision=state->revision;}snapshot.foreground=foreground;
  // Reasons are developer-only state/guard names, never narration. File IO stays
  // on the worker and only runs when a reason changes; it cannot block capture.
  for(int team=0;team<2;++team)if(rejections[team]!=previous_rejections[team]){previous_rejections[team]=rejections[team];if(!rejections[team].empty())state->log("SELECT unavailable requested_player="+std::to_string(team+1)+" reason="+utf8(rejections[team]));}
  const auto sampled_now=GetTickCount64();const bool fresh=foreground&&sampled_now>=snapshot.observed_ms&&sampled_now-snapshot.observed_ms<=250;
  audio_epoch=revision;descriptions.prepare(snapshot,sampled_now,model.descriptions_enabled());snapshot.story.captions[5]=descriptions.notice(snapshot,model.descriptions_enabled());
  if(fresh){
   const auto& movie=snapshot.story.movie;const auto phase=movie.valid?(movie.paused?"movie_paused":movie.playing?"movie_playing":"movie_prepared"):snapshot.story.captions[3].valid?"loading":snapshot.combat.playable?"combat_playable":snapshot.combat.valid?"combat_present":"other";
   const auto key=std::string(phase)+"|"+utf8(movie.id)+"|"+std::to_string(movie.context)+"|"+std::to_string(movie.presented);
   if(key!=previous_story_phase){
    previous_story_phase=key;
    state->log("STORY phase="+std::string(phase)+" movie="+utf8(movie.id)+
               " context="+std::to_string(movie.context)+" presented="+std::to_string(movie.presented)+
               " sequence_seconds="+std::to_string(movie.sequence_seconds.value_or(-1))+
               " movie_seconds="+std::to_string(movie.movie_seconds.value_or(-1))+
               " soundtrack_ms="+std::to_string(movie.soundtrack?movie.soundtrack->elapsed_ms:-1)+
               " raw_frame="+std::to_string(movie.frame.value_or(-1)));
   }
   const auto& loading=snapshot.story.loading;const auto loading_key=std::to_string(loading.known)+"|"+std::to_string(loading.visible)+"|"+std::to_string(loading.identity);
   if(loading_key!=previous_loading_state){previous_loading_state=loading_key;state->log("LOADING known="+std::to_string(loading.known)+" drawn="+std::to_string(loading.visible)+" owner="+std::to_string(loading.identity));}
  }
  if(delivery.retired_menu(mvci::speech_source_fingerprint(snapshot,delivery.source()),fresh))prism.stop();
  auto request=controls.next();
  const bool capture_mission=request==mvci::Request::ReadMenu&&snapshot.mission_hud&&!snapshot.menu.valid&&!snapshot.menu.blocked;
  const bool capture_requested=request==mvci::Request::ReadCombat||capture_mission;
  if(!foreground||(request!=mvci::Request::None&&!capture_requested)){
   status_capture.cancel();state->hud_requested.store(0,std::memory_order_release);
  }
  if(capture_requested){
   state->hud_requested.store(status_capture.request(GetTickCount64(),request),std::memory_order_release);
   request=mvci::Request::None;
  }
  if(status_capture.waiting()){
   const auto status=status_capture.poll(snapshot.hud_capture_ticket,GetTickCount64());
   if(status!=mvci::StatusCaptureResult::Waiting){
    request=status_capture.requested();
    if(status==mvci::StatusCaptureResult::Unavailable){
     if(request==mvci::Request::ReadCombat){snapshot.combat={};snapshot.story.enemies_remaining.reset();snapshot.story.time_seconds.reset();snapshot.story.infinite_time=false;}
     else if(snapshot.mission_hud)snapshot.story.subtitle={};
    }
   }
  }
  auto result=model.update(snapshot,GetTickCount64(),request);
  status_capture.filter(result);
  const bool narration_busy=status_capture.wanted()!=0||result.speech.has_value()||!delivery.allowed(mvci::Priority::Gauge,GetTickCount64(),prism.speaking());
  descriptions.tick(snapshot,GetTickCount64(),model.descriptions_enabled(),narration_busy);
  const auto description_state=descriptions.diagnostic();
  if(description_state!=previous_description){
   previous_description=description_state;
   const auto& movie=snapshot.story.movie;
   state->log("DESCRIPTION "+description_state+" movie="+utf8(movie.id)+
              " sequence_seconds="+std::to_string(movie.sequence_seconds.value_or(-1))+
              " movie_seconds="+std::to_string(movie.movie_seconds.value_or(-1))+
              " soundtrack_ms="+std::to_string(movie.soundtrack?movie.soundtrack->elapsed_ms:-1)+
              " raw_frame="+std::to_string(movie.frame.value_or(-1)));
  }
  if(now>=diagnostics_at){
   diagnostics_at=now+10000;const auto count=state->captures.load();const auto work=state->capture_work_us.exchange(0);
   const char* const stages[]{"discovery","menu","collection","mission","story","selection","combat"};std::string phases;
   for(std::size_t i=0;i<std::size(stages);++i){if(i)phases+=",";phases+=stages[i]+std::string(":")+std::to_string(state->capture_phase_us[i].exchange(0));}
   state->log("CAPTURE frames="+std::to_string(count)+" max_us="+std::to_string(state->max_capture_us.load())+" errors="+std::to_string(state->capture_errors.load())+" menu="+std::to_string(snapshot.menu.valid)+" combat="+std::to_string(snapshot.combat.valid)+" thread="+std::to_string(state->ui_thread.load())+
    " period_frames="+std::to_string(count-previous_capture_count)+" work_us="+std::to_string(work)+" phase_us="+phases+
    " discovery_full="+std::to_string(discovery.full_passes)+" discovery_append="+std::to_string(discovery.append_passes)+" discovery_slots="+std::to_string(discovery.slots_read)+" discovery_fallback="+std::to_string(discovery.fallback_scans)+" discovery_failures="+std::to_string(discovery.failures));previous_capture_count=count;
  }
  const auto audio_status=audio.Tick(result.beacon_interval_ms.has_value(),result.beacon_interval_ms.value_or(0),GetTickCount64());
  if(audio_status==mvci::BeaconAudioResult::Unavailable&&request==mvci::Request::ToggleBeacon)state->log("ERROR Beacon audio device unavailable");
  if(result.speech){
   const auto speaking=prism.speaking();const auto output_now=GetTickCount64();
   if(output_now<snapshot.observed_ms||output_now-snapshot.observed_ms>250){
    if(delivery.retired_menu(mvci::speech_source_fingerprint(snapshot,delivery.source()),false))prism.stop();
    // A slow backend query must not make a previously fresh native line stale.
    // Regenerate pending manual replies as unavailable/control-only without
    // reapplying F5/F6, and discard automatic native proposals.
    snapshot.foreground=focused();result=model.update(snapshot,output_now);audio.Tick(false,0,output_now);
    if(!result.speech){Sleep(20);continue;}
   }
   if(!delivery.allowed(result.speech->priority,GetTickCount64(),speaking)){model.acknowledge(result.speech->token,false,GetTickCount64());Sleep(20);continue;}
   bool current=true;{std::lock_guard<std::mutex> lock(state->mutex);current=state->revision==revision;}
   auto text=result.speech->text;if(!introduced)text=L"Echo Stone. Tracking Player "+std::to_wstring(model.player()+1)+L". "+text;
   const bool available=speech&&current&&focused();const auto submitted_at=GetTickCount64();
   const bool attempted=available&&model.submission_fresh(result.speech->token,submitted_at);
   const bool accepted=attempted&&prism.speak(text,true);const auto completed_at=GetTickCount64();
   if(accepted&&status_capture.wanted()!=0&&result.speech->request==status_capture.requested()){
    status_capture.cancel();state->hud_requested.store(0,std::memory_order_release);
   }
   const bool committed=model.acknowledge(result.speech->token,accepted,completed_at,submitted_at);
   if((attempted&&result.speech->priority==mvci::Priority::Story)||(accepted&&!committed))state->log("ACK accepted="+std::to_string(accepted)+" committed="+std::to_string(committed)+" submitted_capture_age_ms="+std::to_string(submitted_at>=snapshot.observed_ms?submitted_at-snapshot.observed_ms:0)+" backend_ms="+std::to_string(completed_at-submitted_at));
   if(accepted){introduced=true;delivery.accepted(result.speech->priority,text.size(),GetTickCount64(),result.speech->menu_key);state->log("SPEAK accepted=1 priority="+std::to_string(static_cast<int>(result.speech->priority))+" "+utf8(text));}
   else if(attempted){speech=false;retry_at=GetTickCount64()+2000;state->log("SPEAK accepted=0; retry only current state");}
  }
  Sleep(20);
 }
 audio.Tick(false,0,GetTickCount64());return 0;
}
DWORD WINAPI worker(void*){
 try{return worker_impl();}
 catch(const std::exception& error){if(auto* state=active.load())state->log(std::string("ERROR Reader worker stopped: ")+error.what());return 5;}
 catch(...){if(auto* state=active.load())state->log("ERROR Reader worker stopped");return 6;}
}
}
extern "C" __declspec(dllexport) void InitializeASI(){bool expected=false;if(started.compare_exchange_strong(expected,true)){HANDLE thread=CreateThread(nullptr,0,worker,nullptr,0,nullptr);if(thread)CloseHandle(thread);}}
BOOL APIENTRY DllMain(HMODULE module,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH)self_module=module;if(reason==DLL_PROCESS_DETACH)stopping=true;return TRUE;}
