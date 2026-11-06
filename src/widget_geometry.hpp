#pragma once
#include "story_read.hpp"
#include <initializer_list>

namespace mvci::widget_geometry {
using story_detail::Frame;
struct Member {Address property=0;int offset=0,size=0;std::wstring kind;};
inline std::optional<Member> member(Frame& f,Address structure,const wchar_t* name){
 // UStruct.Children +0x38, UField.Next +0x28, UProperty Offset_Internal +0x50 / ElementSize +0x34.
 auto field=f.get<Address>(structure+0x38);std::set<Address> seen;
 while(field&&*field&&seen.size()<128&&seen.insert(*field).second){
  if(!f.retain(*field))return {};
  if(f.u.name_is(*field,name)){const auto offset=f.get<int>(*field+0x50),size=f.get<int>(*field+0x34);if(!offset||!size||*offset<0||*size<=0)return {};return Member{*field,*offset,*size,f.u.type(*field)};}
  field=f.get<Address>(*field+0x28);
 }return {};
}
inline std::optional<Address> struct_of(Frame& f,Address property){
 // UStructProperty.Struct +0x78.
 const auto s=f.get<Address>(property+0x78);if(!s||!*s||!f.retain(*s))return {};return s;
}
// Address of a reflected float/byte reached through a StructProperty member path, bounded by the outer property size.
inline std::optional<Address> path(Frame& f,Address object,const wchar_t* property,std::initializer_list<const wchar_t*> names,const wchar_t* leaf_kind,int leaf_size){
 const auto p=f.u.property(object,property);if(!p||p->kind!=L"StructProperty")return {};auto s=struct_of(f,p->address);if(!s)return {};int offset=0;std::size_t i=0;
 for(const auto name:names){
  const auto m=member(f,*s,name);if(!m||m->offset>p->size-m->size-offset)return {};offset+=m->offset;
  if(++i==names.size()){if(m->size!=leaf_size||(m->kind!=leaf_kind&&!(std::wstring(leaf_kind)==L"ByteProperty"&&m->kind==L"EnumProperty")))return {};return object+static_cast<Address>(p->offset+offset);}
  if(m->kind!=L"StructProperty")return {};s=struct_of(f,m->property);if(!s)return {};
 }return {};
}
inline std::optional<float> number(Frame& f,Address object,const wchar_t* property,std::initializer_list<const wchar_t*> names){
 const auto a=path(f,object,property,names,L"FloatProperty",4);if(!a)return {};const auto v=f.get<float>(*a);if(!v||!std::isfinite(*v))return {};return v;
}
inline bool near(const std::optional<float>& v,float want,float slack=.01f){return v&&std::fabs(*v-want)<=slack;}
struct Box {float left,top,right,bottom,min_x,min_y,max_x,max_y,align_x,align_y;};
inline std::optional<Box> anchor(Frame& f,Address slot){
 if(!f.u.is_a(slot,L"CanvasPanelSlot"))return {};
 const auto o=[&](const wchar_t* n){return number(f,slot,L"LayoutData",{L"Offsets",n});};
 const auto a=[&](const wchar_t* m,const wchar_t* n){return number(f,slot,L"LayoutData",{L"Anchors",m,n});};
 const auto g=[&](const wchar_t* n){return number(f,slot,L"LayoutData",{L"Alignment",n});};
 const std::optional<float> v[]{o(L"Left"),o(L"Top"),o(L"Right"),o(L"Bottom"),a(L"Minimum",L"X"),a(L"Minimum",L"Y"),a(L"Maximum",L"X"),a(L"Maximum",L"Y"),g(L"X"),g(L"Y")};
 for(const auto& x:v)if(!x)return {};
 return Box{*v[0],*v[1],*v[2],*v[3],*v[4],*v[5],*v[6],*v[7],*v[8],*v[9]};
}
// Slot owning `child` in `parent`, with exact Slot/Parent/Content backlinks.
inline std::optional<Address> slot_of(Frame& f,Address child,Address parent){
 const auto s=f.get<Address>(child+0x30);if(!s||!*s||!f.retain(*s))return {};
 if(f.get<Address>(*s+0x28)!=std::optional<Address>(parent)||f.field(*s,L"Content")!=std::optional<Address>(child))return {};return s;
}
}
