#pragma once
// Private copy of the selection_tests.cpp fixture (hand-encoded UE4.15 layouts) for collection_tests.
#include "mvci/unreal.hpp"
#include <algorithm>
#include <cstring>
#include <map>
#include <stdexcept>
#include <vector>
namespace mvci_test {
using namespace mvci;
// Independent hand-encoded layouts. Native offsets come from research/selection/report.md and
// ghidra/VERIFIED.md (GEngine 0x36bc348 +0x5d8 viewport +0x80 world; Level+0xb0 OwningWorld; component
// owner +0xb0; ChildActorComponent.ChildActor +0x308; g_GameSetting 0x348cb48 table +0x28/num +0x30;
// UWidget Slot +0x30, slot Parent +0x28, Visibility +0x91; TextBlock.Text +0x140; UserWidget viewport +0x208/+0x210).
// Blueprint field offsets are fixture choices resolved through reflection, as in the game.
struct Fixture {
 static constexpr Address base=0x140000000,heap=0x20000000;
 static constexpr int capacity=30000;
 std::map<Address,std::vector<unsigned char>> blocks;
 Address next=heap,entries=0,names=0,chunk=0;int objects=0,name_count=0,next_offset=0x400;
 std::map<std::wstring,int> name_ids;std::map<std::wstring,Address> kinds,cls;std::map<Address,Address> panel_of,tree_of;
 Memory memory;Unreal unreal;Address world=0,level=0,engine=0,gs=0,table=0;
 Fixture():unreal(memory,base){
  memory.reader=[this](Address a,void* out,std::size_t n){auto i=blocks.upper_bound(a);if(i==blocks.begin())return false;--i;if(a<i->first||a-i->first>i->second.size()||n>i->second.size()-(a-i->first))return false;std::memcpy(out,i->second.data()+a-i->first,n);return true;};
  blocks[base+0x35cafd0].resize(8);blocks[base+0x35d36f0].resize(0x20);blocks[base+0x36bc348].resize(8);blocks[base+0x348cb48].resize(8);
  names=alloc(0x408);chunk=alloc(16384*8);entries=alloc(capacity*24);
  put(base+0x35cafd0,names);put(names,chunk);put(base+0x35d36f0+0x10,entries);put<int>(base+0x35d36f0+0x18,capacity);put<int>(base+0x35d36f0+0x1c,0);
  auto widget=klass(L"Widget");prop_at(widget,L"Visibility",L"EnumProperty",0x91,1);prop_at(widget,L"Slot",L"ObjectProperty",0x30,8);
  klass(L"UserWidget",widget);auto tb=klass(L"TextBlock",widget);prop_at(tb,L"Text",L"TextProperty",0x140,24);klass(L"UserNameTextBlock",tb);klass(L"CanvasPanel",widget);
  auto actor=klass(L"Actor");prop(actor,L"bHidden",L"BoolProperty",1);
  auto scene=klass(L"SceneComponent");prop(scene,L"bVisible",L"BoolProperty",1);prop(scene,L"bHiddenInGame",L"BoolProperty",1);
  klass(L"StaticMeshComponent",scene);auto wc=klass(L"WidgetComponent",scene);prop(wc,L"Widget",L"ObjectProperty",8);klass(L"ChildActorComponent",scene);
  auto tick=klass(L"BP_TickStateObj_C");prop(tick,L"State",L"ByteProperty",1);prop(tick,L"PreState",L"BoolProperty",1);prop(tick,L"ParentObj",L"ObjectProperty",8);
  auto ui=klass(L"BP_UIBase_C",actor);prop(ui,L"TickStateObj",L"ObjectProperty",8);klass(L"BP_UIMenuBase_C",ui);
  auto wp=klass(L"WD_UIParent_C",cls[L"UserWidget"]);prop(wp,L"TickStateObj",L"ObjectProperty",8);klass(L"WD_UIPart_C",cls[L"UserWidget"]);
  klass(L"WD_MMBase_C",klass(L"WD_MenuBase_C",wp));
  world=object(L"World",klass(L"World"));level=object(L"PersistentLevel",klass(L"Level"),0x2000,world);put(level+0xb0,world);
  engine=object(L"GameEngine",kind(L"Object"));put(base+0x36bc348,engine);auto viewport=object(L"GameViewport",kind(L"Object"));put(engine+0x5d8,viewport);put(viewport+0x80,world);
  auto asset=klass(L"DuckCharaAttrDataAsset");prop(asset,L"CharaID",L"ByteProperty",1);prop(asset,L"CharaName",L"TextProperty",24);
  gs=object(L"GameSetting",klass(L"GameSetting"));table=alloc(64*8);put(gs+0x28,table);put<int>(gs+0x30,64);put<int>(gs+0x34,64);put(base+0x348cb48,gs);
 }
 Address alloc(std::size_t n){Address a=next;next+=(n+0x1f)&~std::size_t(0x1f);blocks[a].resize(n);return a;}
 template<class T> T raw(Address a){T v{};if(!memory.reader(a,&v,sizeof(v)))throw std::runtime_error("fixture read");return v;}
 template<class T> void put(Address a,T v){auto i=blocks.upper_bound(a);if(i==blocks.begin())throw std::runtime_error("fixture range");--i;if(a-i->first+sizeof(T)>i->second.size())throw std::runtime_error("fixture bounds");std::memcpy(i->second.data()+a-i->first,&v,sizeof(v));}
 int name_id(const std::wstring& s){auto i=name_ids.find(s);if(i!=name_ids.end())return i->second;int n=name_count++;auto a=alloc(0x10+2*(s.size()+1));put<std::uint32_t>(a,1);for(std::size_t j=0;j<s.size();++j)put<std::uint16_t>(a+0x10+j*2,static_cast<std::uint16_t>(s[j]));put(chunk+static_cast<Address>(n)*8,a);name_ids[s]=n;return n;}
 Address object(const std::wstring& name,Address c=0,std::size_t size=0x2000,Address outer=0){if(objects>=capacity)throw std::runtime_error("fixture capacity");auto a=alloc(size);int index=objects++;put(a,base+0x1000);put(entries+index*24,a);put<int>(entries+index*24+0x10,100+index);put<int>(a+0xc,index);put(a+0x10,c);put<int>(a+0x18,name_id(name));put(a+0x20,outer);put<int>(base+0x35d36f0+0x1c,objects);return a;}
 Address kind(const std::wstring& name){if(kinds.count(name))return kinds[name];if(name==L"Class"){auto a=object(name,0,0x100);put(a+0x10,a);return kinds[name]=a;}return kinds[name]=object(name,kind(L"Class"),0x100);}
 Address klass(const std::wstring& name,Address parent=0){auto a=object(name,kind(L"Class"),0x100);put(a+0x30,parent);put<int>(a+0x40,0x2000);return cls[name]=a;}
 Address prop_at(Address c,const std::wstring& name,const std::wstring& kind_name,int offset,int size){auto p=object(name,kind(kind_name),0x100);put<int>(p+0x50,offset);put<int>(p+0x34,size);auto old=raw<Address>(c+0x38);put(p+0x28,old);put(c+0x38,p);if(kind_name==L"BoolProperty")put<std::uint8_t>(p+0x7b,0xff);return p;}
 Address prop(Address c,const std::wstring& name,const std::wstring& kind_name,int size){auto p=prop_at(c,name,kind_name,next_offset,size);next_offset+=0x20;if(next_offset>0x2000)throw std::runtime_error("fixture offsets");return p;}
 Address arr(Address c,const std::wstring& name,const std::wstring& inner_kind,int stride){auto p=prop(c,name,L"ArrayProperty",16);auto inner=object(L"Inner",kind(inner_kind),0x100);put<int>(inner+0x34,stride);put(p+0x78,inner);return p;}
 int off(Address o,const std::wstring& field){auto p=unreal.property(o,field);if(!p)throw std::runtime_error("fixture field");return p->offset;}
 template<class T> void set(Address o,const std::wstring& field,T v){put(o+static_cast<Address>(off(o,field)),v);}
 void flag(Address o,const std::wstring& field,bool v){put<std::uint8_t>(o+static_cast<Address>(off(o,field)),v?1:0);}
 void str(Address at,const std::wstring& value){auto data=alloc((value.size()+1)*2);for(std::size_t j=0;j<value.size();++j)put<std::uint16_t>(data+j*2,static_cast<std::uint16_t>(value[j]));put(at,data);put<int>(at+8,static_cast<int>(value.size()+1));put<int>(at+12,static_cast<int>(value.size()+1));}
 void text(Address at,const std::wstring& value){auto td=alloc(0x80),vt=alloc(24),view=alloc(16);put(vt+8,base+0x1876760);put(td,vt);put(td+8,view);str(view,value);put(at,td);}
 template<class T> void items(Address o,const std::wstring& field,const std::vector<T>& v){auto data=alloc(std::max<std::size_t>(1,v.size())*sizeof(T));for(std::size_t i=0;i<v.size();++i)put(data+i*sizeof(T),v[i]);auto at=o+static_cast<Address>(off(o,field));put(at,data);put<int>(at+8,static_cast<int>(v.size()));put<int>(at+12,static_cast<int>(v.size()));}
 void texts(Address o,const std::wstring& field,const std::vector<std::wstring>& v){auto data=alloc(std::max<std::size_t>(1,v.size())*24);for(std::size_t i=0;i<v.size();++i)text(data+i*24,v[i]);auto at=o+static_cast<Address>(off(o,field));put(at,data);put<int>(at+8,static_cast<int>(v.size()));put<int>(at+12,static_cast<int>(v.size()));}
 void field_text(Address o,const std::wstring& field,const std::wstring& v){text(o+static_cast<Address>(off(o,field)),v);}
 // Actors live in the persistent level of the current world.
 Address actor(Address c,const std::wstring& name){return object(name,c,0x2000,level);}
 Address tick(Address owner,int state=2,bool pre=false){auto t=object(L"Tick",cls[L"BP_TickStateObj_C"]);set(owner,L"TickStateObj",t);set(t,L"ParentObj",owner);set<std::uint8_t>(t,L"State",static_cast<std::uint8_t>(state));flag(t,L"PreState",pre);return t;}
 Address comp(const std::wstring& c,const std::wstring& name,Address owner,bool visible=true){auto x=object(name,cls[c],0x2000,owner);put(x+0xb0,owner);flag(x,L"bVisible",visible);flag(x,L"bHiddenInGame",false);return x;}
 Address attach(Address owner,const std::wstring& field,Address child){auto c=comp(L"ChildActorComponent",field,owner);put(c+0x308,child);set(owner,field,c);return c;}
 Address user(Address c,const std::wstring& name,bool viewport){auto u=object(name,c);auto tree=object(L"WidgetTree",kind(L"WidgetTree"),0x100,u);auto panel=object(L"CanvasPanel_0",cls[L"CanvasPanel"],0x200,tree);panel_of[u]=panel;tree_of[u]=tree;
  if(viewport){auto shared=alloc(16);put<int>(shared+8,1);put(u+0x208,heap);put(u+0x210,shared);}return u;}
 void slot(Address child,Address parent_user){auto s=object(L"CanvasPanelSlot",kind(L"CanvasPanelSlot"),0x100);put(s+0x28,panel_of.at(parent_user));put(child+0x30,s);put(child+0x20,tree_of.at(parent_user));}
 Address nested(Address parent,const std::wstring& field,Address c,const std::wstring& name){auto u=user(c,name,false);slot(u,parent);set(parent,field,u);return u;}
 Address block(Address u,const std::wstring& field,const std::wstring& value,const std::wstring& c=L"TextBlock"){auto b=object(field,cls[c]);slot(b,u);text(b+0x140,value);set(u,field,b);return b;}
 Address panel(Address u,const std::wstring& field){auto p=object(field,cls[L"CanvasPanel"],0x200);slot(p,u);set(u,field,p);return p;}
 void name(int id,const std::wstring& value){auto a=object(L"CharacterAttributeAsset",cls[L"DuckCharaAttrDataAsset"]);set<std::uint8_t>(a,L"CharaID",static_cast<std::uint8_t>(id));field_text(a,L"CharaName",value);put(table+static_cast<Address>(id)*8,a);}
};
}
