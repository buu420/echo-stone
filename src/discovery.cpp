#include "mvci/unreal.hpp"
#include <algorithm>
#include <array>
#include <set>
namespace mvci {
namespace {
struct Registry {Address data;int capacity,count;};
struct Item {Address object;std::uint32_t flags;int cluster,serial,padding;};
struct Header {Address vtable;std::uint32_t flags;int index;Address cls;int name,number;Address outer;};
static_assert(sizeof(Registry)==16&&sizeof(Item)==24&&sizeof(Header)==40);
constexpr Address objects_rva=0x35d36f0;
constexpr std::uint64_t refresh_ms=500;
constexpr int block_slots=256;
// Every discovery family used by the native adapters. Other diagnostic/future
// queries retain the original uncached path and increment fallback_scans.
const wchar_t* const classes[]{L"UserWidget",L"GF_Collection_C",L"GF_Mission_C",L"WD_KeyConfMain_C",L"WD_KeyboardConfMain_C",L"WD_CtrlSelectMain_C",L"BP_CharaSelectSimple_C",L"BP_StageSelectMain_C",L"UIMenuItemDataAsset",L"BP_HUD_Root_C",L"WD_WindowBase_C",L"WD_MenuBase_C",L"WD_MainMenu_C",L"WD_Title_C",L"DemoSystem",L"MoviePlayBase",L"BP_SequencerMovieEnv_C",L"GF_FightStory_C",L"WD_PauseMenu_Story_C",L"WD_Loading_C",L"WD_CommandList_C",L"GF_Thanks_C",
 L"GF_FightMission_C",L"GF_Lobby_C",L"GF_Ranking_C",L"GF_Replay_C",L"GF_Title_C"};
static_assert(std::size(classes)<=32,"Discovery family mask overflow");
bool pointer(Address a){return a>=0x10000&&a<0x800000000000;}
bool valid(const Registry& r){return r.count>=0&&r.count<=r.capacity&&r.capacity>=0&&r.capacity<=0x400000&&(!r.count||pointer(r.data));}
}
std::optional<std::uint32_t> Unreal::discovery_class(const std::wstring& target)const{
 for(std::size_t i=0;i<std::size(classes);++i)if(target==classes[i])return static_cast<std::uint32_t>(1u<<i);return {};
}
std::vector<ObjectRef> Unreal::discovered_instances(std::uint32_t mask)const{
 std::vector<ObjectRef> out;if(!discovery_valid_)return out;
 for(const auto& entry:discovered_)if(entry.second.classes&mask)out.push_back(entry.second.ref);return out;
}
std::vector<ObjectRef> Unreal::menu_widgets()const{
 if(!discovery_managed_)return widgets();
 std::uint32_t mask=0;
 for(const auto target:{L"WD_WindowBase_C",L"WD_MenuBase_C",L"WD_MainMenu_C",L"WD_Title_C"})if(const auto bit=discovery_class(target))mask|=*bit;
 // One ordered pass preserves identity/order and includes overlapping families once.
 return discovered_instances(mask);
}
std::optional<bool> Unreal::discovered_class_match(const ObjectRef& ref,const std::wstring& family)const{
 if(!discovery_managed_||!discovery_valid_)return {};const auto bit=discovery_class(family);if(!bit)return {};
 const auto found=discovered_.find(ref.index);if(found==discovered_.end())return {};const auto& tracked=found->second;
 if(tracked.ref.address!=ref.address||tracked.ref.cls!=ref.cls||tracked.ref.serial!=ref.serial)return {};return (tracked.classes&*bit)!=0;
}
DiscoveryStats Unreal::discovery_stats()const{return discovery_stats_;}
void Unreal::observe_event(Address address){
 // Called on the game thread only. Collect addresses without dereferencing,
 // allocating, invoking game code or retaining objects. Validation is deferred
 // to the next capture; repeated calls on the same owner need only one hint.
 if(!pointer(address))return;
 if(discovery_hint_count_&&discovery_hints_[(discovery_hint_next_+discovery_hints_.size()-1)%discovery_hints_.size()]==address)return;
 discovery_hints_[discovery_hint_next_]=address;discovery_hint_next_=(discovery_hint_next_+1)%discovery_hints_.size();discovery_hint_count_=std::min(discovery_hint_count_+1,discovery_hints_.size());
}
bool Unreal::prepare_capture(std::uint64_t now){
 discovery_managed_=true;
 const auto invalidate=[this]{discovered_.clear();discovery_valid_=false;discovery_table_=0;discovery_count_=0;++discovery_stats_.failures;return false;};
 Registry registry{};if(!memory_.read(base_+objects_rva+0x10,registry)||!valid(registry))return invalidate();
 const bool full=!discovery_valid_||registry.data!=discovery_table_||registry.count<discovery_count_||now<discovery_full_at_||now-discovery_full_at_>=refresh_ms;
 std::map<Address,std::uint32_t> class_masks;
 const auto classify=[&](Address cls)->std::optional<std::uint32_t>{
  const auto found=class_masks.find(cls);if(found!=class_masks.end())return found->second;
  ++discovery_stats_.class_walks;std::uint32_t mask=0;Address current=cls;
  std::array<Address,64> seen{};std::size_t depth=0;
  // Cache the result for this starting class only. A truncated deep walk must
  // not poison a later shallower query for one of its ancestors.
  while(current&&depth<seen.size()&&std::find(seen.begin(),seen.begin()+depth,current)==seen.begin()+depth){
   seen[depth++]=current;if(!object(current))return {};const auto own=name(current);if(own.empty())return {};
   for(std::size_t i=0;i<std::size(classes);++i)if(own==classes[i])mask|=static_cast<std::uint32_t>(1u<<i);
   const auto parent=memory_.get<Address>(current+0x30);if(!parent)return {};current=*parent;
  }class_masks.emplace(cls,mask);return mask;
 };
 const auto record=[&](int index,const Item& item){
  if(!item.object||(item.flags&0x30000000))return true;
  Header h{};++discovery_stats_.object_headers;
  // The registry item was already read in the shared block. Validate the
  // header once here; adapters still revalidate its registry serial/identity.
  if(!memory_.read(item.object,h))return false;
  if(!pointer(h.vtable)||!pointer(h.cls)||h.index!=index||(h.flags&0x18030))return true;
  const auto mask=classify(h.cls);if(!mask)return false;
  if(*mask)discovered_[index]=Discovered{ObjectRef{item.object,h.cls,index,item.serial,h.flags},*mask};return true;
 };
 const auto scan=[&](int first,int last){
  std::array<Item,block_slots> items{};
  for(int at=first;at<last;){const int count=std::min(block_slots,last-at);
   if(!memory_.bytes(registry.data+static_cast<Address>(at)*sizeof(Item),items.data(),static_cast<std::size_t>(count)*sizeof(Item)))return false;
   discovery_stats_.slots_read+=static_cast<std::uint64_t>(count);
   for(int i=0;i<count;++i)if(!record(at+i,items[static_cast<std::size_t>(i)]))return false;at+=count;
  }return true;
 };
 if(full){
  discovered_.clear();++discovery_stats_.full_passes;
  if(!scan(0,registry.count))return invalidate();discovery_full_at_=now;
 }else if(registry.count>discovery_count_){
  ++discovery_stats_.append_passes;if(!scan(discovery_count_,registry.count))return invalidate();
 }
 if(!full){
  // Reused tracked slots must be recognized immediately, even when NumElements
  // is unchanged. Unknown/previously empty slots are covered by the periodic pass.
  std::vector<int> retired;
  for(const auto& entry:discovered_){const auto& old=entry.second.ref;const auto current=object(old.address);
   if(!current||current->cls!=old.cls||current->index!=old.index||current->serial!=old.serial||(current->flags&0x30))retired.push_back(entry.first);
  }
  for(const auto index:retired){discovered_.erase(index);Item item{};
   if(!memory_.read(registry.data+static_cast<Address>(index)*sizeof(Item),item))return invalidate();++discovery_stats_.slots_read;if(!record(index,item))return invalidate();
  }
 }
 // A Blueprint owner may be allocated into an old empty/asset slot without
 // increasing NumElements. Observe its real ProcessEvent as a discovery hint,
 // then require the same identity/class/visibility/owner checks as all objects.
 std::set<Address> hints;
 for(std::size_t i=0;i<discovery_hint_count_;++i)hints.insert(discovery_hints_[i]);
 discovery_hint_count_=0;discovery_hint_next_=0;
 for(const auto address:hints){++discovery_stats_.hinted_objects;
  Header h{};++discovery_stats_.object_headers;if(!memory_.read(address,h))return invalidate();
  if(!pointer(h.vtable)||!pointer(h.cls)||h.index<0||h.index>=registry.count||(h.flags&0x18030))continue;
  Item item{};if(!memory_.read(registry.data+static_cast<Address>(h.index)*sizeof(Item),item))return invalidate();
  if(item.object!=address||(item.flags&0x30000000))continue;
  const auto mask=classify(h.cls);if(!mask)return invalidate();
  if(*mask)discovered_[h.index]=Discovered{ObjectRef{address,h.cls,h.index,item.serial,h.flags},*mask};else discovered_.erase(h.index);
 }
 Registry after{};if(!memory_.read(base_+objects_rva+0x10,after)||!valid(after)||after.data!=registry.data||after.count<registry.count)return invalidate();
 // Appends on the async loader during this pass remain pending for the next
 // capture; never jump the frontier to indices that were not actually read.
 discovery_table_=registry.data;discovery_count_=registry.count;discovery_valid_=true;return true;
}
}
