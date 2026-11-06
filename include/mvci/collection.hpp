#pragma once
#include "mvci/unreal.hpp"

namespace mvci {
// Collection mode (/Game/UI/Collectons): overview, movie/story, Database, artwork, BGM and voice lists
// (research/collection/report.md). The input owner is GF_Collection_C.CurrentWidget, descending through
// the top's ChildOpen child. Only rendered, visibility-proven text and drawn NEW/greyed indicators are
// reported; anything unproven yields an invalid snapshot. Modal dialogs keep priority (caller).
class CollectionCapture {
 struct Cache {std::vector<ObjectRef> objects;std::uint64_t at=0;int count=-1;bool filled=false;};
 Unreal& unreal_;
 std::function<std::optional<int>(Address)> visibility_;
 Cache flows_;
 int count_=-1; // Registry count for this sample (Unreal::object_count).
 template<class T> std::optional<T> get(Address a)const{return unreal_.memory().get<T>(a);}
 const std::vector<ObjectRef>& cached(Cache&,const wchar_t* cls,std::uint64_t now)const;
 bool clean(Address)const;
 bool exact(Address,const wchar_t* cls)const;
 bool derives(Address,const wchar_t* cls)const;
 bool in_world(Address actor,Address world)const;
 bool active(Address owner)const;
 bool shown(Address widget)const;
 std::optional<Address> child(Address owner,const wchar_t* field)const;
 std::optional<std::wstring> rendered(Address owner,const wchar_t* field)const;
 std::optional<bool> disabled(Address block)const;
 bool badge(Address part,const wchar_t* field)const;
 std::wstring help(Address owner,Address top)const;
 std::optional<MenuSnapshot> list(Address owner,Address top,const std::wstring& caption)const;
 std::optional<MenuSnapshot> artwork(Address top,const std::wstring& caption)const;
 std::optional<MenuSnapshot> viewer(Address owner,Address top,const std::wstring& caption)const;
public:
 explicit CollectionCapture(Unreal& u,std::function<std::optional<int>(Address)> visibility={}):unreal_(u),visibility_(std::move(visibility)){}
 MenuSnapshot sample(std::uint64_t now);
};
}
