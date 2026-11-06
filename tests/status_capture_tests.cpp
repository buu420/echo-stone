#include "mvci/status_capture.hpp"
#include <iostream>
#include <stdexcept>
using namespace mvci;
static void repeated_pending_reply_waits_for_the_new_native_capture(){
 StatusCaptureGate gate;Model model;Snapshot s;s.foreground=true;s.observed_ms=1000;s.story.valid=true;s.story.enemies_remaining=4;
 const auto first=gate.request(1000);s.hud_capture_ticket=first;if(gate.poll(first,1000)!=StatusCaptureResult::Ready)throw std::runtime_error("First status capture not ready");
 auto reply=model.update(s,1000,Request::ReadCombat);gate.filter(reply);if(!reply.speech||model.acknowledge(reply.speech->token,false,1000))throw std::runtime_error("Failed first F7 was not retained");
 const auto second=gate.request(1033);s.observed_ms=1033;reply=model.update(s,1033);gate.filter(reply);
 if(reply.speech)throw std::runtime_error("Pending older F7 bypassed a repeated request's native capture wait");
 if(gate.poll(first,1033)!=StatusCaptureResult::Waiting||gate.wanted()!=second)throw std::runtime_error("Old delivery cancelled the new demand");
 s.hud_capture_ticket=second;s.story.enemies_remaining=2;s.observed_ms=1066;if(gate.poll(second,1066)!=StatusCaptureResult::Ready)throw std::runtime_error("Repeated capture not ready");
 reply=model.update(s,1066,Request::ReadCombat);gate.filter(reply);
 if(!reply.speech||reply.speech->request!=Request::ReadCombat||reply.speech->text.find(L"Enemies remaining 2")==std::wstring::npos||reply.speech->text.find(L"remaining 4")!=std::wstring::npos)throw std::runtime_error("Repeated F7 did not use the new native HUD");
 if(!model.acknowledge(reply.speech->token,true,1066))throw std::runtime_error("Fresh status reply was not accepted");gate.cancel();if(gate.wanted())throw std::runtime_error("Delivered repeated request kept HUD reads active");
}
static void mission_repeat_waits_for_its_own_capture(){
 StatusCaptureGate gate;Model model;Snapshot snapshot;
 snapshot.foreground=true;snapshot.observed_ms=1000;snapshot.mission_hud=21;snapshot.story.valid=true;
 snapshot.story.subtitle={true,21,L"First set of commands"};
 auto previous=model.update(snapshot,1000,Request::ReadMenu);
 if(!previous.speech||model.acknowledge(previous.speech->token,false,1000))throw std::runtime_error("Expected pending F8 delivery");
 const auto wanted=gate.request(1020,Request::ReadMenu);
 snapshot.observed_ms=1020;auto retry=model.update(snapshot,1020);gate.filter(retry);
 if(retry.speech||gate.poll(wanted-1,1020)!=StatusCaptureResult::Waiting)throw std::runtime_error("F8 reused a retained command capture");
 snapshot.observed_ms=1033;snapshot.story.subtitle.text=L"Second set of commands";
 if(gate.poll(wanted,1033)!=StatusCaptureResult::Ready||gate.requested()!=Request::ReadMenu)throw std::runtime_error("F8 demand lost its request type");
 const auto fresh=model.update(snapshot,1033,gate.requested());
 if(!fresh.speech||fresh.speech->text!=L"Second set of commands")throw std::runtime_error("F8 ignored the new Mission command capture");
 model.acknowledge(fresh.speech->token,false,1033);
 snapshot.observed_ms=1066;snapshot.mission_command_view=99;
 const auto moved=model.update(snapshot,1066,Request::ReadMenu);
 if(!moved.speech||moved.speech->menu_key==fresh.speech->menu_key||
    speech_source_fingerprint(snapshot,moved.speech->menu_key)!=moved.speech->menu_key)throw std::runtime_error("F8 kept a previous pane's speech source");
}
int main(){try{
 repeated_pending_reply_waits_for_the_new_native_capture();
 mission_repeat_waits_for_its_own_capture();
 StatusCaptureGate gate;const auto first=gate.request(1000);if(!first||!gate.waiting())throw std::runtime_error("F7 did not arm a UI-thread request");
 if(gate.poll(0,1020)!=StatusCaptureResult::Waiting)throw std::runtime_error("F7 used an older capture");
 if(gate.poll(first,1033)!=StatusCaptureResult::Ready||gate.waiting()||gate.wanted()!=first)throw std::runtime_error("F7 did not retain demand through delivery");
 gate.cancel();if(gate.wanted()||gate.waiting())throw std::runtime_error("Accepted/cancelled F7 keeps expensive HUD reads enabled");
 const auto second=gate.request(2000);if(second<=first)throw std::runtime_error("Repeated F7 reused stale ticket");
 if(gate.poll(first,2249)!=StatusCaptureResult::Waiting||gate.poll(first,2250)!=StatusCaptureResult::Unavailable)throw std::runtime_error("Stopped capture did not produce a bounded unavailable response");
 gate.cancel();if(gate.poll(second,2300)!=StatusCaptureResult::Waiting)throw std::runtime_error("Focus loss resurrected cancelled F7");
 std::cout<<"On-demand F7 and F8 capture checks passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
