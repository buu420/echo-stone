#pragma once
#include "mvci/beacon.hpp"
#include <functional>
namespace mvci {
struct DescriptionWaveApi {
 WaveOutputApi wave;
 decltype(&waveOutPause) pause=nullptr;
 decltype(&waveOutRestart) resume=nullptr;
};
class DescriptionWave {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 DescriptionWave();
 explicit DescriptionWave(DescriptionWaveApi);
 ~DescriptionWave();
 bool playing()const;
 bool healthy()const;
 bool play(const std::vector<unsigned char>& canonical_wave,const std::function<bool()>& current={});
 void pause();
 void resume();
 void stop();
};
}
