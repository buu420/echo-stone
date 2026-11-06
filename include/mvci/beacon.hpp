#pragma once
#include <windows.h>
#include <mmeapi.h>
#include <cstdint>
#include <memory>
#include <vector>
namespace mvci {
std::vector<std::int16_t> MakeBeaconPulse();
struct WaveOutputApi {
 decltype(&waveOutOpen) open=nullptr;
 decltype(&waveOutPrepareHeader) prepare=nullptr;
 decltype(&waveOutWrite) write=nullptr;
 decltype(&waveOutReset) reset=nullptr;
 decltype(&waveOutUnprepareHeader) unprepare=nullptr;
 decltype(&waveOutClose) close=nullptr;
};
enum class BeaconAudioResult {Idle,Played,Unavailable};
class BeaconAudio {
 struct Impl;
 std::unique_ptr<Impl> impl_;
public:
 BeaconAudio();
 explicit BeaconAudio(WaveOutputApi);
 ~BeaconAudio();
 BeaconAudio(const BeaconAudio&)=delete;
 BeaconAudio& operator=(const BeaconAudio&)=delete;
 BeaconAudioResult Tick(bool enabled,std::uint32_t interval_ms,std::uint64_t now);
};
}
