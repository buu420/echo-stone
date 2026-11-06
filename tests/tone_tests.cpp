#include "mvci/beacon.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
int main(){auto pcm=mvci::MakeBeaconPulse();if(pcm.size()!=7056){std::cerr<<"80 ms stereo pulse missing\n";return 1;}int crossings=0,peak=0;for(std::size_t i=0;i<3528;++i){int value=pcm[i*2];peak=std::max(peak,std::abs(value));if(std::abs(value-static_cast<int>(pcm[i*2+1]))>1)return 2;if(i&&pcm[(i-1)*2]<=0&&value>0)++crossings;}if(crossings!=64||peak<8800||peak>9052||pcm.front()!=0||std::abs(pcm.back())>50){std::cerr<<"carrier/envelope mismatch "<<crossings<<' '<<peak<<'\n';return 3;}std::cout<<"800 Hz / 80 ms stereo envelope verified\n";return 0;}
