#pragma once
#include "mvci/unreal.hpp"

namespace mvci {
// Character, stone, stage, battle-setting and controller-assignment selection screens
// (research/selection/report.md, ghidra/VERIFIED.md). Only rendered, visibility-proven text is
// reported; anything unproven yields an invalid snapshot rather than a guessed label.
// The caller gives modal dialogs and generic menus (MenuCapture) priority over this capture.
class SelectionCapture {
 struct Cache {std::vector<ObjectRef> objects;std::uint64_t at=0;int count=-1;bool filled=false;};
 struct Member {int offset=0,size=0;std::wstring kind;};
 struct Root {std::optional<Address> address;bool ambiguous=false;};
 Unreal& unreal_;
 std::function<std::optional<int>(Address)> visibility_;
 Cache chara_,stage_,ctrl_,gem_items_,pad_conf_,keyboard_conf_;
 int count_=-1; // Registry count for this sample (Unreal::object_count).
 mutable std::wstring rejection_;
 template<class T> std::optional<T> get(Address a)const{return unreal_.memory().get<T>(a);}
 const std::vector<ObjectRef>& cached(Cache&,const wchar_t* cls,std::uint64_t now)const;
 Root unique(const std::vector<ObjectRef>&,const std::function<bool(Address)>& valid)const;
 bool clean(Address)const;
 bool in_world(Address actor,Address world)const;
 bool shown_actor(Address actor,Address world)const;
 bool active(Address owner,bool prestate)const;
 bool preview_ready(Address screen)const;
 bool tick_hidden(Address owner)const;
 std::wstring tick_desc(Address owner)const;
 std::nullopt_t reject(std::wstring reason)const{rejection_=std::move(reason);return std::nullopt;}
 bool component_shown(Address component,Address owner)const;
 bool widget_shown(Address widget)const;
 bool locally_shown(Address widget,Address root)const;
 std::optional<std::wstring> fname(Address at)const;
 std::optional<Member> member(Address script_struct,const wchar_t* name)const;
 std::optional<Address> struct_of(Address property,bool array)const;
 std::optional<Address> brush_resource(Address image)const;
 std::optional<Address> parameter(Address material,const wchar_t* array,const wchar_t* name,const wchar_t* kind)const;
 std::wstring glyph(Address image)const;
 std::optional<std::wstring> line_text(Address box)const;
 std::optional<int> portrait(Address image)const;
 std::optional<Address> child(Address owner,const wchar_t* component,const wchar_t* cls,Address world)const;
 std::optional<Address> element(Address owner,const wchar_t* field,int index,int limit=64)const;
 std::optional<int> int_element(Address owner,const wchar_t* field,int index,int limit=64)const;
 std::optional<int> count(Address owner,const wchar_t* field,const wchar_t* inner,int stride,int limit=64)const;
 std::optional<std::wstring> rendered(Address owner,const wchar_t* field)const;
 std::optional<std::wstring> rendered_local(Address owner,const wchar_t* field,Address root)const;
 std::optional<std::wstring> chara_name(int player_type)const;
 std::optional<std::wstring> stone_name(int index,std::uint64_t now);
 std::optional<MenuSnapshot> grid(Address root,Address parent,Address world,int team)const;
 std::optional<MenuSnapshot> costume(Address root,Address menu,int team)const;
 std::optional<MenuSnapshot> stone(Address root,Address gem,Address world,int team,std::uint64_t now);
 std::optional<MenuSnapshot> team_preset(Address root,Address window,int team)const;
 std::optional<MenuSnapshot> battle(Address root,Address world,int team)const;
 std::optional<MenuSnapshot> handicap(Address con,int team)const;
 std::optional<MenuSnapshot> key_form(Address form,bool keyboard)const;
 std::optional<MenuSnapshot> key_config(Address root,bool keyboard,int team)const;
 std::optional<MenuSnapshot> chara(Address root,Address world,int team,std::uint64_t now);
 std::optional<MenuSnapshot> stage(Address root,Address world)const;
 std::optional<MenuSnapshot> controller(Address root)const;
public:
 explicit SelectionCapture(Unreal& u,std::function<std::optional<int>(Address)> visibility={}):unreal_(u),visibility_(std::move(visibility)){}
 // team: 0 = Player 1 (left), 1 = Player 2 (right); the side tracked with F5.
 // Stage select and controller assignment are read independently of team.
 MenuSnapshot sample(std::uint64_t now,int team);
 // Developer diagnostic (read-only, never spoken): the first guard that left the last sample() without a
 // character-select snapshot, or empty after a valid one. Guard names and lifecycle states only; no IDs.
 const std::wstring& rejection()const{return rejection_;}
};
}
