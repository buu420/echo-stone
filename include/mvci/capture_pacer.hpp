#pragma once
#include <cstdint>
namespace mvci {
// Single game-thread owner. Timestamps/gap use the same monotonic clock units.
class CapturePacer {
 std::uint64_t next_=0;
public:
 bool due(std::uint64_t now)const{return now>=next_;}
 void completed(std::uint64_t now,std::uint64_t gap){next_=gap>UINT64_MAX-now?UINT64_MAX:now+gap;}
};
}
