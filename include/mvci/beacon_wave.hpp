#pragma once
#include <array>
#include <cmath>
#include <cstdint>

namespace mvci {
constexpr unsigned beacon_sample_rate=44100;
constexpr size_t beacon_frames=3528;
using BeaconWave=std::array<int16_t,beacon_frames*2>;

// Cyber Sleuth PathBeacon renderer, independently traced in the installed
// beta24 DLL:800Hz,80ms,10% attack/release,0.4 equal-power gain,PCM scale32000.
// Keep center pan; proximity changes pulse timing, not the familiar sound.
inline BeaconWave beacon_wave() {
 BeaconWave result{};
 const float theta=3.141590118408203f*.25f;
 const float left=std::cos(theta)*.4f,right=std::sin(theta)*.4f;
 for(size_t i=0;i<beacon_frames;++i) {
  const float progress=static_cast<float>(i)/static_cast<float>(beacon_frames);
  float value=std::sin(static_cast<float>(i)/44100.f*5026.5439453125f);
  if(progress<.1f)value*=progress/.1f;
  else if(progress>.9f)value*=(1.f-progress)/.1f;
  result[i*2]=static_cast<int16_t>(value*left*32000.f);
  result[i*2+1]=static_cast<int16_t>(value*right*32000.f);
 }
 return result;
}
}
