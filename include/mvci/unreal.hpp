#pragma once
#include "mvci/model.hpp"
#include <cstring>
#include <array>
#include <map>
#include <vector>

namespace mvci {
using Address=std::uintptr_t;
// Use on authored TextBlocks whose native designer default is not game content.
bool is_umg_default_text(std::wstring_view);
class Memory {
public:
 using Reader=std::function<bool(Address,void*,std::size_t)>;
 Reader reader;
 bool bytes(Address a,void* out,std::size_t size)const;
 template<class T> bool read(Address a,T& value)const{return bytes(a,&value,sizeof(T));}
 template<class T> std::optional<T> get(Address a)const {T v{};if(!read(a,v))return {};return v;}
};
struct ObjectRef {Address address=0,cls=0;int index=0,serial=0;std::uint32_t flags=0;};
struct Property {Address address=0;int offset=0,size=0;std::wstring kind,inner;int stride=0;std::uint8_t bool_byte=0,bool_mask=0;};
struct ArrayView {Address data=0;int count=0,stride=0;};
struct DiscoveryStats {
 std::uint64_t full_passes=0,append_passes=0,slots_read=0,object_headers=0,class_walks=0,fallback_scans=0,hinted_objects=0,failures=0;
};
class Unreal {
 const Memory& memory_;
 Address base_;
 mutable std::map<int,std::wstring> names_;
 struct Discovered {ObjectRef ref;std::uint32_t classes=0;};
 std::map<int,Discovered> discovered_;
 Address discovery_table_=0;
 int discovery_count_=0;
 std::uint64_t discovery_full_at_=0;
 bool discovery_managed_=false,discovery_valid_=false;
 mutable DiscoveryStats discovery_stats_;
 std::array<Address,64> discovery_hints_{};
 std::size_t discovery_hint_next_=0,discovery_hint_count_=0;
 std::optional<std::uint32_t> discovery_class(const std::wstring&)const;
 std::vector<ObjectRef> discovered_instances(std::uint32_t)const;
public:
 Unreal(const Memory& memory,Address base):memory_(memory),base_(base){}
 const Memory& memory()const{return memory_;}
 Address base()const{return base_;}
 // Runtime capture calls this once before all adapters. No game code executes.
 bool prepare_capture(std::uint64_t now);
 DiscoveryStats discovery_stats()const;
 bool shared_discovery()const{return discovery_managed_;}
 void observe_event(Address object);
 std::optional<ObjectRef> object(Address)const;
 bool alive(const ObjectRef&)const;
 std::optional<Address> current_world()const;
 std::wstring name(Address,bool numbered=false)const;
 bool name_is(Address,const std::wstring&)const;
 std::wstring type(Address)const;
 bool is_a(Address,const std::wstring&)const;
 std::optional<Property> property(Address,const std::wstring&)const;
 std::optional<std::wstring> string(Address)const;
 std::optional<std::wstring> text(Address)const;
 std::optional<ArrayView> array(Address,const Property&,int limit=256)const;
 std::optional<Address> object_field(Address,const std::wstring&)const;
 std::optional<int> integer(Address,const std::wstring&)const;
 std::optional<double> number(Address,const std::wstring&)const;
 std::optional<bool> boolean(Address,const std::wstring&)const;
 std::optional<std::wstring> text_field(Address,const std::wstring&)const;
 std::vector<ObjectRef> widgets()const;
 std::vector<ObjectRef> menu_widgets()const;
 // Coarse shared-catalog prefilter only. Adapters still validate live class,
 // owner and rendered state; unavailable/mismatched catalog entries return null.
 std::optional<bool> discovered_class_match(const ObjectRef&,const std::wstring&)const;
 std::optional<int> object_count()const;
 std::vector<ObjectRef> instances(const std::wstring& class_name)const;
 bool shown(Address,const std::function<std::optional<int>(Address)>& effective_visibility={})const;
};
class MenuCapture {
 Unreal& unreal_;
 std::vector<ObjectRef> widgets_;
 std::uint64_t refreshed_=0;
 int object_count_=-1;
 std::function<std::optional<int>(Address)> visibility_;
 std::wstring hints(Address)const;
 std::optional<MenuSnapshot> dialog(const ObjectRef&)const;
 std::optional<MenuSnapshot> movie_pause(const ObjectRef&)const;
 std::optional<MenuSnapshot> menu(const ObjectRef&)const;
 std::optional<MenuSnapshot> main_menu(const ObjectRef&)const;
 std::optional<MenuSnapshot> title(const ObjectRef&)const;
public:
 explicit MenuCapture(Unreal& u,std::function<std::optional<int>(Address)> visibility={}):unreal_(u),visibility_(std::move(visibility)){}
 MenuSnapshot sample(std::uint64_t now);
};
// Rendered button-guide line. root owns a WD_BtnNavi field (or a ButtonNaviArray); with direct_btnnavi,
// root is itself a live, shown WD_BtnNavi_C (numbered fields such as WD_BtnNavi_9).
std::wstring menu_hints(const Unreal&,Address root,const std::function<std::optional<int>(Address)>& visibility={},bool direct_btnnavi=false);
std::wstring normalize_text(const std::wstring&);
}
