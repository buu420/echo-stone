#pragma once
#include "mvci/model.hpp"
#include <cstdint>
namespace mvci {
enum class StatusCaptureResult {Waiting,Ready,Unavailable};
class StatusCaptureGate {
 std::uint64_t serial_=0,wanted_=0,started_=0;
 bool waiting_=false;
 Request requested_=Request::None;
public:
 std::uint64_t request(std::uint64_t now,Request requested=Request::ReadCombat){wanted_=++serial_;started_=now;waiting_=true;requested_=requested;return wanted_;}
 StatusCaptureResult poll(std::uint64_t captured,std::uint64_t now){
  if(!waiting_)return StatusCaptureResult::Waiting;
  if(captured==wanted_){waiting_=false;return StatusCaptureResult::Ready;}
  if(now<started_||now-started_>=250){waiting_=false;return StatusCaptureResult::Unavailable;}
  return StatusCaptureResult::Waiting;
 }
 void cancel(){wanted_=0;waiting_=false;requested_=Request::None;}
 bool waiting()const{return waiting_;}
 std::uint64_t wanted()const{return wanted_;}
 Request requested()const{return requested_;}
 void filter(Result& result)const{
  // Model can still hold an older manual reading while a new native ticket waits.
  if(waiting_&&result.speech&&(result.speech->request==Request::ReadCombat||result.speech->request==Request::ReadMenu))result.speech.reset();
 }
};
}
