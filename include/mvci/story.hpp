#pragma once
#include "mvci/unreal.hpp"
#include <utility>
namespace mvci {
struct StoryCaptureResult {MenuSnapshot menu;StorySnapshot story;};
class StoryCapture {
 Unreal& unreal_;
 std::function<std::optional<int>(Address)> visibility_;
 std::wstring rejection_;
public:
 explicit StoryCapture(Unreal& unreal,std::function<std::optional<int>(Address)> visibility={}):unreal_(unreal),visibility_(std::move(visibility)){}
 StoryCaptureResult sample(std::uint64_t now,bool read_hud=true);
 const std::wstring& rejection()const{return rejection_;}
};
}
