#pragma once
#include "mvci/localization.hpp"
#include "mvci/unreal.hpp"
#include <algorithm>
#include <cmath>
#include <set>
namespace mvci::story_detail {
inline bool clean(const Unreal& u,Address a){const auto o=u.object(a);return o&&!(o->flags&0x30);}
inline bool readable(const std::wstring& s){
 if(s.empty()||s.rfind(L"msg_",0)==0)return false;
 const wchar_t* const placeholders[]{L"Text Block",L"Text Blockg",L"CHARACTER NAME",L"//////",L"///////"};
 return std::none_of(std::begin(placeholders),std::end(placeholders),[&](const wchar_t* p){return s==p;});
}
// Reuses the proven Missions rendered-widget fences without changing that reader.
struct Frame {
 Unreal& u;const std::function<std::optional<int>(Address)>& visibility;
 std::vector<std::function<bool()>> checks;std::vector<std::pair<Address,bool>> shown_values;std::vector<std::function<bool()>> bindings;
 bool retain(Address a){const auto o=u.object(a);if(!o||(o->flags&0x30))return false;checks.push_back([this,ref=*o]{return u.alive(ref);});return true;}
 template<class T> std::optional<T> get(Address a){auto v=u.memory().get<T>(a);checks.push_back([this,a,v]{return u.memory().get<T>(a)==v;});return v;}
 std::optional<Address> field(Address a,const wchar_t* name){
  auto v=u.object_field(a,name);const std::wstring key=name;checks.push_back([this,a,key,v]{return u.object_field(a,key)==v;});
  if(v&&*v&&!retain(*v))return {};return v;
 }
 std::optional<int> integer(Address a,const wchar_t* name){auto v=u.integer(a,name);const std::wstring key=name;checks.push_back([this,a,key,v]{return u.integer(a,key)==v;});return v;}
 std::optional<bool> boolean(Address a,const wchar_t* name){auto v=u.boolean(a,name);const std::wstring key=name;checks.push_back([this,a,key,v]{return u.boolean(a,key)==v;});return v;}
 bool shown(Address a){const bool v=a&&clean(u,a)&&u.shown(a,visibility);shown_values.emplace_back(a,v);return v;}
 // Follow the actual widget slot/tree chain back to the declared owner.
 bool within(Address a,Address owner){
  std::set<Address> seen;
  while(a&&seen.size()<64&&seen.insert(a).second){
   if(a==owner)return true;if(!retain(a)||!u.is_a(a,L"Widget"))return false;
   const auto slot=get<Address>(a+0x30);if(!slot)return false;
   if(*slot){if(!retain(*slot))return false;const auto parent=get<Address>(*slot+0x28);if(!parent)return false;if(*parent){a=*parent;continue;}}
   const auto outer=get<Address>(a+0x20);if(!outer||!*outer||!retain(*outer))return false;
   if(u.type(*outer)==L"WidgetTree"){const auto parent=get<Address>(*outer+0x20);if(!parent)return false;a=*parent;}
   else if(u.is_a(*outer,L"UserWidget"))a=*outer;
   else return false;
  }return false;
 }
 std::optional<std::wstring> text(Address owner,const wchar_t* name){
  const auto block=field(owner,name);if(!block||!*block||!u.is_a(*block,L"TextBlock")||!within(*block,owner)||!shown(*block))return {};
  return display(*block);
 }
 std::optional<std::wstring> display(Address block){auto v=u.text_field(block,L"Text");checks.push_back([this,block,v]{return u.text_field(block,L"Text")==v;});return v;}
 std::optional<Property> member(const Property& parent,const wchar_t* structure_name,const wchar_t* name){
  if(parent.kind!=L"StructProperty"||!retain(parent.address))return {};
  const auto structure=get<Address>(parent.address+0x78);if(!structure||!*structure||!retain(*structure)||!u.is_a(*structure,L"ScriptStruct")||!u.name_is(*structure,structure_name))return {};
  auto child=get<Address>(*structure+0x38);std::set<Address> seen;
  while(child&&*child&&seen.size()<128&&seen.insert(*child).second){
   if(!retain(*child))return {};
   if(u.name_is(*child,name)){const auto offset=get<int>(*child+0x50),size=get<int>(*child+0x34);if(!offset||!size||*offset<0||*size<=0||*size>parent.size||*offset>parent.size-*size)return {};Property p;p.address=*child;p.offset=parent.offset+*offset;p.size=*size;p.kind=u.type(*child);return p;}
   child=get<Address>(*child+0x28);
  }return {};
 }
 std::optional<float> text_alpha(Address block){
  // The Story fades animate FSlateColor.SpecifiedColor, not the enum Visibility.
  const auto color=u.property(block,L"ColorAndOpacity");if(!color||color->size<16||color->size>128||get<int>(color->address+0x50)!=std::optional<int>(color->offset)||get<int>(color->address+0x34)!=std::optional<int>(color->size))return {};
  const auto rule=member(*color,L"SlateColor",L"ColorUseRule"),specified=member(*color,L"SlateColor",L"SpecifiedColor");
  if(!rule||rule->size!=1||(rule->kind!=L"ByteProperty"&&rule->kind!=L"EnumProperty")||get<std::uint8_t>(block+static_cast<Address>(rule->offset))!=std::optional<std::uint8_t>(std::uint8_t{0})||!specified)return {};
  const auto alpha=member(*specified,L"LinearColor",L"A");if(!alpha||alpha->kind!=L"FloatProperty"||alpha->size!=4)return {};
  const auto value=get<float>(block+static_cast<Address>(alpha->offset));return value&&std::isfinite(*value)&&*value>=0&&*value<=1?value:std::nullopt;
 }
 std::optional<ArrayView> array(Address owner,const wchar_t* name,int limit,const wchar_t* inner=L"ObjectProperty",int stride=8){
  const auto p=u.property(owner,name);if(!p||p->inner!=inner||p->stride!=stride)return {};
  auto v=u.array(owner,*p,limit);checks.push_back([this,owner,prop=*p,v,limit]{auto n=u.array(owner,prop,limit);return bool(n)==bool(v)&&(!v||(n->data==v->data&&n->count==v->count&&n->stride==v->stride));});return v;
 }
 bool active(Address owner){
  const auto t=field(owner,L"TickStateObj");const auto ref=t&&*t?u.object(*t):std::nullopt;
  return ref&&u.name_is(ref->cls,L"BP_TickStateObj_C")&&field(*t,L"ParentObj")==std::optional<Address>(owner)&&integer(*t,L"State")==std::optional<int>(2);
 }
 std::wstring hints(Address guide,bool direct){
  const auto text=menu_hints(u,guide,visibility,direct);bindings.push_back([this,guide,direct,text]{return menu_hints(u,guide,visibility,direct)==text;});return text;
 }
 std::optional<Address> image_resource(Address image){
  const auto brush=u.property(image,L"Brush");if(!brush||brush->kind!=L"StructProperty")return {};
  const auto structure=get<Address>(brush->address+0x78);if(!structure||!*structure||!retain(*structure))return {};
  auto member=get<Address>(*structure+0x38);std::set<Address> seen;std::optional<Address> resource;
  while(member&&*member&&seen.size()<128&&seen.insert(*member).second){
   if(!retain(*member))return {};
   if(u.name_is(*member,L"ResourceObject")){
    const auto offset=get<int>(*member+0x50),size=get<int>(*member+0x34);
    if(u.type(*member)!=L"ObjectProperty"||!offset||!size||*size!=8||*offset<0||*offset>brush->size-8)return {};
    resource=get<Address>(image+static_cast<Address>(brush->offset+*offset));break;
   }member=get<Address>(*member+0x28);
  }
  if(!resource||!*resource||!retain(*resource))return {};return resource;
 }
 std::wstring glyph(Address image){
  const auto unknown=tr(L"(button icon)");const auto resource=image_resource(image);if(!resource)return unknown;
  Address material=*resource;
  if(u.is_a(material,L"MaterialInstanceDynamic")){const auto parent=field(material,L"Parent");if(!parent||!*parent)return unknown;material=*parent;}
  if(!u.is_a(material,L"MaterialInstance"))return unknown;
  const auto name=u.name(material,true);const std::wstring prefix=L"MI_Btn_";if(name.rfind(prefix,0)!=0)return unknown;
  const auto suffix=name.substr(prefix.size());const wchar_t* const known[]{L"A",L"B",L"X",L"Y",L"LB",L"RB",L"LT",L"RT",L"LS",L"RS"};
  for(const auto k:known)if(suffix==k)return suffix;
  // The live tutorial contains these PC/Steam materials. Describe their
  // directional/stick meaning without inventing a physical keyboard binding.
  if(suffix==L"Dir"||suffix==L"Sti"){
   const auto package=get<Address>(material+0x20);
   if(package&&*package&&retain(*package))for(const auto platform:{L"PC",L"Steam"}){
    const auto path=std::wstring(L"/Game/UI/BaseAsset/Platform/")+platform+L"/"+name;
    if(u.name_is(*package,path))return suffix==L"Dir"?tr(L"directional controls"):tr(L"analog stick");
   }
  }return unknown;
 }
 std::optional<std::wstring> line(Address box,Address popup){
  if(!u.is_a(box,L"HorizontalBox")||!within(box,popup))return {};
  const auto slots=array(box,L"Slots",64);if(!slots)return {};std::wstring out;
  for(int i=0;i<slots->count;++i){
   const auto slot=get<Address>(slots->data+static_cast<Address>(i)*8);if(!slot||!*slot||!retain(*slot))return {};
   const auto content=field(*slot,L"Content");if(!content||!*content||get<Address>(*slot+0x28)!=std::optional<Address>(box)||get<Address>(*content+0x30)!=slot||!within(*content,box))return {};
   if(!shown(*content))continue;std::wstring piece;
   if(u.is_a(*content,L"TextBlock")){const auto t=display(*content);if(!t||(!t->empty()&&!readable(*t)))return {};piece=*t;}
   else if(u.is_a(*content,L"Image"))piece=glyph(*content);
   else if(u.is_a(*content,L"Spacer"))continue;
   else return {};
   if(!piece.empty()){if(!out.empty())out+=L" ";out+=piece;}if(out.size()>8192)return {};
  }return out;
 }
 bool finish(){
  for(const auto& binding:bindings)if(!binding())return false;
  for(const auto& v:shown_values)if((v.first&&clean(u,v.first)&&u.shown(v.first,visibility))!=v.second)return false;
  return std::all_of(checks.begin(),checks.end(),[](const std::function<bool()>& check){return check();});
 }
};
// Read a rendered move glyph from the Command List atlas shared by Mission HUD rows.
std::optional<std::wstring> command_glyph(Frame&,Address image);
}
