#include "mvci/selection.hpp"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <limits>
using namespace mvci;
static int checks=0;
#define CHECK(x) do{++checks;if(!(x))throw std::runtime_error(std::string(__func__)+":"+std::to_string(__LINE__)+" " #x);}while(false)
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

// Character select level: root, 3D grid, four screens, input owners. Team 0 is on ULTRON, team 1 waits.
struct Chara {
 Fixture& f;Address root=0,sp=0,menu_cls=0;Address tiles[3]{},screen[2][2]{},gem[2]{},menu[2]{};
 explicit Chara(Fixture& fx):f(fx){
  auto simple=f.klass(L"BP_CharaSelectSimple_C",f.cls[L"BP_UIBase_C"]);auto main=f.klass(L"BP_CharaSelectMain_C",simple);
  f.arr(simple,L"Input_ArrObj",L"ObjectProperty",8);f.arr(simple,L"Gem_ArrObj",L"ObjectProperty",8);
  for(auto n:{L"BP_Screen_1P_Main",L"BP_Screen_1P_Sub",L"BP_Screen_2P_Main",L"BP_Screen_2P_Sub",L"BP_SelectGem_1P",L"BP_SelectGem_2P"})f.prop(simple,n,L"ObjectProperty",8);
  f.prop(main,L"SelectParentObj",L"ObjectProperty",8);f.prop(main,L"BP_Select_Parent",L"ObjectProperty",8);f.arr(main,L"DecideCnt_Arr",L"IntProperty",4);f.arr(main,L"Menu_ArrObj",L"ObjectProperty",8);
  f.arr(main,L"TeamSelect_Arr",L"ObjectProperty",8);f.prop(main,L"BattleSettingActive",L"BoolProperty",1);f.prop(main,L"BattleSettingObj",L"ObjectProperty",8);f.prop(main,L"BP_BattleSettingMain",L"ObjectProperty",8);
  auto parent=f.klass(L"BP_Select_Parent_C",f.cls[L"BP_UIMenuBase_C"]);f.arr(parent,L"FocusUniqId",L"IntProperty",4);f.arr(parent,L"ArrSelectChild",L"ObjectProperty",8);
  auto child=f.klass(L"BP_Select_Child_C",f.cls[L"BP_UIMenuBase_C"]);f.prop(child,L"UniqId",L"IntProperty",4);f.prop(child,L"CharaSelectData",L"StructProperty",8);f.prop(child,L"FocusTeamNoBit",L"IntProperty",4);
  auto sc=f.klass(L"BP_Screen_C",f.cls[L"BP_UIMenuBase_C"]);f.prop(sc,L"PlayerType",L"IntProperty",4);f.prop(sc,L"Random",L"BoolProperty",1);f.prop(sc,L"TeamNo",L"IntProperty",4);f.prop(sc,L"ChrRoot",L"ObjectProperty",8);f.prop(sc,L"SM_Character",L"ObjectProperty",8);
  root=f.actor(main,L"BP_CharaSelectMain_C_0");f.tick(root);
  sp=f.actor(parent,L"BP_Select_Parent_C_0");f.tick(sp);f.attach(root,L"BP_Select_Parent",sp);f.set(root,L"SelectParentObj",sp);
  const int types[3]{1,2,7};
  for(int i=0;i<3;++i){tiles[i]=f.actor(child,L"BP_Select_Child_C_"+std::to_wstring(i));f.tick(tiles[i]);f.set(tiles[i],L"UniqId",i);f.set(tiles[i],L"CharaSelectData",types[i]);f.set(tiles[i],L"FocusTeamNoBit",i==0?1:i==1?2:0);}
  f.items<Address>(sp,L"ArrSelectChild",{tiles[0],tiles[1],tiles[2]});f.items<int>(sp,L"FocusUniqId",{0,1});
  const wchar_t* fields[2][2]{{L"BP_Screen_1P_Main",L"BP_Screen_1P_Sub"},{L"BP_Screen_2P_Main",L"BP_Screen_2P_Sub"}};
  for(int t=0;t<2;++t)for(int p=0;p<2;++p){auto s=f.actor(sc,L"BP_Screen_C");f.tick(s);f.set(s,L"TeamNo",t);f.set(s,L"PlayerType",t==0?1:2);f.set(s,L"ChrRoot",f.comp(L"SceneComponent",L"ChrRoot",s));f.set(s,L"SM_Character",f.comp(L"StaticMeshComponent",L"SM_Character",s));f.attach(root,fields[t][p],s);screen[t][p]=s;}
  f.items<int>(root,L"DecideCnt_Arr",{0,0});f.items<Address>(root,L"Input_ArrObj",{sp,0});
  f.name(1,L"ULTRON");f.name(2,L"X");f.name(7,L"Captain Marvel");
 }
 void input(int team,Address owner){Address cur[2]{f.raw<Address>(f.raw<Address>(root+f.off(root,L"Input_ArrObj"))),f.raw<Address>(f.raw<Address>(root+f.off(root,L"Input_ArrObj"))+8)};cur[team]=owner;f.items<Address>(root,L"Input_ArrObj",{cur[0],cur[1]});}
 Address costume_menu(int team){
  auto widget=f.cls[L"UserWidget"];if(!menu_cls){menu_cls=f.klass(L"WD_Menu_Con_C",f.cls[L"WD_UIParent_C"]);for(auto n:{L"TeamNo",L"FocusMenu"})f.prop(menu_cls,n,L"IntProperty",4);for(auto n:{L"WD_ListItem00",L"WD_ListItem01",L"WD_KeyConfAttention_153"})f.prop(menu_cls,n,L"ObjectProperty",8);
   auto l0=f.klass(L"WD_ListItem00_C",f.cls[L"WD_UIPart_C"]);for(auto n:{L"TextBlock_Label",L"TextBlock_Subitem",L"CanvasPanel_Cursor"})f.prop(l0,n,L"ObjectProperty",8);f.prop(l0,L"CurrentSubitem",L"IntProperty",4);f.arr(l0,L"SubItemNames",L"TextProperty",24);
   auto l1=f.klass(L"WD_ListItem01_C",f.cls[L"WD_UIPart_C"]);for(auto n:{L"TextBlock_Label",L"CanvasPanel_Cursor"})f.prop(l1,n,L"ObjectProperty",8);f.prop(l1,L"ColorFocus",L"IntProperty",4);f.prop(l1,L"ColorMax",L"IntProperty",4);f.arr(l1,L"ColorNoTextOnArray",L"ObjectProperty",8);
   auto at=f.klass(L"WD_KeyConfAttention_C",f.cls[L"WD_UIParent_C"]);f.prop(at,L"TextBlock_Label",L"ObjectProperty",8);(void)widget;}
  auto m=f.user(menu_cls,L"WD_Menu_Con_C",true);f.tick(m);f.set(m,L"TeamNo",team);f.set(m,L"FocusMenu",0);
  auto l0=f.nested(m,L"WD_ListItem00",f.cls[L"WD_ListItem00_C"],L"WD_ListItem00");f.block(l0,L"TextBlock_Label",L"Costume");f.block(l0,L"TextBlock_Subitem",L"Normal");f.panel(l0,L"CanvasPanel_Cursor");f.set(l0,L"CurrentSubitem",0);f.texts(l0,L"SubItemNames",{L"Normal",L"Wave 1",L"Wave 2"});
  auto l1=f.nested(m,L"WD_ListItem01",f.cls[L"WD_ListItem01_C"],L"WD_ListItem01");f.block(l1,L"TextBlock_Label",L"Color");f.put<std::uint8_t>(f.panel(l1,L"CanvasPanel_Cursor")+0x91,2);f.set(l1,L"ColorFocus",1);f.set(l1,L"ColorMax",4);
  std::vector<Address> nums;for(int i=0;i<4;++i){auto b=f.object(L"TextBlock_Color_On",f.cls[L"TextBlock"]);f.slot(b,l1);f.text(b+0x140,std::to_wstring(i+1));nums.push_back(b);}f.items<Address>(l1,L"ColorNoTextOnArray",nums);
  menu[team]=m;std::vector<Address> all{menu[0],menu[1]};f.items<Address>(root,L"Menu_ArrObj",all);input(team,m);return m;
 }
 Address stone(int team){
  if(!f.cls.count(L"BP_SelectGem_C")){auto g=f.klass(L"BP_SelectGem_C",f.cls[L"BP_UIMenuBase_C"]);f.prop(g,L"TeamNo",L"IntProperty",4);f.arr(g,L"FocusGemNoArray",L"IntProperty",4);f.arr(g,L"InvalidGemType",L"BoolProperty",1);f.arr(g,L"ArrGemNameMesh",L"ObjectProperty",8);f.arr(g,L"MT_ArrGemName",L"ObjectProperty",8);f.prop(g,L"DetailWindow",L"ObjectProperty",8);f.prop(g,L"BP_DetailWindow",L"ObjectProperty",8);
   auto d=f.klass(L"BP_DetailWindow_C",f.cls[L"Actor"]);f.prop(d,L"GemTypeText",L"ObjectProperty",8);f.prop(d,L"GemDetail",L"ObjectProperty",8);
   auto w=f.klass(L"WD_GemDetail_C",f.cls[L"UserWidget"]);for(auto n:{L"TriggerText",L"TriggerEffectText",L"PossessionText",L"PossessionEffectText"})f.prop(w,n,L"ObjectProperty",8);
   auto item=f.klass(L"UIMenuItemDataAsset");f.arr(item,L"ArrText",L"TextProperty",24);}
  auto g=f.actor(f.cls[L"BP_SelectGem_C"],L"BP_SelectGem_C");f.tick(g);f.set(g,L"TeamNo",team);f.attach(root,team?L"BP_SelectGem_2P":L"BP_SelectGem_1P",g);
  gem[team]=g;f.items<Address>(root,L"Gem_ArrObj",{gem[0],gem[1]});
  f.items<int>(g,L"FocusGemNoArray",{1});f.items<std::uint8_t>(g,L"InvalidGemType",{0,0,0,0,0,0});
  std::vector<Address> meshes,mats;const wchar_t* m[6]{L"MT_Space",L"MT_Power",L"MT_Reality",L"MT_Soul",L"MT_Mind",L"MT_Time"};
  for(int i=0;i<6;++i){meshes.push_back(f.comp(L"StaticMeshComponent",L"SM_Name0"+std::to_wstring(i),g,i==1));mats.push_back(f.object(m[i],f.kind(L"MaterialInstanceConstant")));}
  f.items<Address>(g,L"ArrGemNameMesh",meshes);f.items<Address>(g,L"MT_ArrGemName",mats);
  auto d=f.actor(f.cls[L"BP_DetailWindow_C"],L"BP_DetailWindow_C");f.attach(g,L"BP_DetailWindow",d);f.set(g,L"DetailWindow",d);
  auto wc=f.comp(L"WidgetComponent",L"GemTypeText",d);f.set(d,L"GemTypeText",wc);auto w=f.user(f.cls[L"WD_GemDetail_C"],L"WD_GemDetail_C",false);f.set(wc,L"Widget",w);f.set(d,L"GemDetail",w);
  f.block(w,L"TriggerText",L"INFINITY STORM");f.block(w,L"TriggerEffectText",L"Unleashes a powerful attack.");f.block(w,L"PossessionText",L"INFINITY SURGE");f.block(w,L"PossessionEffectText",L"Knocks the enemy back.");
  input(team,g);return g;
 }
};

static MenuSnapshot read(Fixture& f,int team,std::uint64_t now=1000){SelectionCapture c(f.unreal);return c.sample(now,team);}

static void grid_reads_the_live_character_name_for_the_tracked_side(){
 Fixture f;Chara c(f);auto s=read(f,0);CHECK(s.valid);CHECK(!s.modal);CHECK(s.screen==L"Player 1");CHECK(s.label==L"ULTRON");
 CHECK(!read(f,1).valid); // Player 2 has no input owner yet.
 c.input(1,c.sp);auto two=read(f,1);CHECK(two.valid);CHECK(two.screen==L"Player 2");CHECK(two.label==L"X");CHECK(two.identity!=s.identity);
 CHECK(!read(f,2).valid);CHECK(!read(f,-1).valid);
 // Live PID 33524 exposes these interned FNames as teamNo and charaID.
 // The spelling can depend on which asset first interns a name in this launch.
 const auto team_field=f.unreal.property(c.screen[0][0],L"TeamNo"),id_field=f.unreal.property(f.raw<Address>(f.table+8),L"CharaID");
 CHECK(team_field&&id_field);f.put<int>(team_field->address+0x18,f.name_id(L"teamNo"));f.put<int>(id_field->address+0x18,f.name_id(L"charaID"));
 s=read(f,0);CHECK(s.valid);CHECK(s.label==L"ULTRON");
 f.set(c.screen[0][0],L"teamNo",1);CHECK(!read(f,0).valid);f.set(c.screen[0][0],L"teamNo",0);
 const auto asset=f.raw<Address>(f.table+8);f.set<std::uint8_t>(asset,L"charaID",2);CHECK(!read(f,0).valid);
}
static void partner_slot_reads_the_sub_screen(){
 Fixture f;Chara c(f);f.items<int>(c.root,L"DecideCnt_Arr",{1,0});f.items<int>(c.sp,L"FocusUniqId",{2,1});f.set(c.tiles[2],L"FocusTeamNoBit",1);
 f.set(c.screen[0][1],L"PlayerType",7);CHECK(read(f,0).label==L"Captain Marvel");
 f.items<int>(c.root,L"DecideCnt_Arr",{2,0});CHECK(!read(f,0).valid);
}
static void single_controller_character_select_follows_the_unique_input_side(){
 Fixture f;Chara c(f);
 const auto single=f.klass(L"BP_CharaSelectSingle_C",f.cls[L"BP_CharaSelectMain_C"]);f.put(c.root+0x10,single);
 c.input(0,0);c.input(1,c.sp);auto s=read(f,0);
 CHECK(s.valid);CHECK(s.screen==L"Player 2");CHECK(s.label==L"X");
 CHECK(read(f,1).label==L"X"); // Combat tracking does not silence the only local input owner.
 c.input(1,0);c.input(0,c.sp);s=read(f,1);CHECK(s.valid);CHECK(s.screen==L"Player 1");CHECK(s.label==L"ULTRON");
 c.input(1,c.sp);CHECK(read(f,0).label==L"ULTRON");CHECK(read(f,1).label==L"X"); // Two input owners retain explicit selection.
 c.input(0,0);c.input(1,0);CHECK(!read(f,0).valid);CHECK(!read(f,1).valid);
 // The unique-owner rule is specific to the native single-controller subclass.
 f.put(c.root+0x10,f.cls[L"BP_CharaSelectMain_C"]);c.input(1,c.sp);CHECK(!read(f,0).valid);CHECK(read(f,1).label==L"X");
}
static void random_placeholder_never_resolves_the_hidden_id(){
 Fixture f;Chara c(f);f.flag(c.screen[0][0],L"Random",true);auto s=read(f,0);CHECK(s.valid);CHECK(s.label==L"?????");CHECK(s.label.find(L"ULTRON")==std::wstring::npos);
 f.set(c.screen[0][0],L"PlayerType",254);CHECK(read(f,0).label==L"?????");
}
static void unknown_or_mismatched_character_assets_are_rejected(){
 {Fixture f;Chara c(f);f.set(c.screen[0][0],L"PlayerType",2);CHECK(!read(f,0).valid);} // Screen disagrees with focused tile.
 {Fixture f;Chara c(f);f.set(c.tiles[0],L"CharaSelectData",3);f.set(c.screen[0][0],L"PlayerType",3);CHECK(!read(f,0).valid);} // No asset for 3.
 {Fixture f;Chara c(f);f.put<int>(f.gs+0x30,1);CHECK(!read(f,0).valid);} // Index past the native table.
 {Fixture f;Chara c(f);auto a=f.raw<Address>(f.table+8);f.set<std::uint8_t>(a,L"CharaID",9);CHECK(!read(f,0).valid);} // CharaID mismatch.
 {Fixture f;Chara c(f);auto a=f.raw<Address>(f.table+8);f.field_text(a,L"CharaName",L"msg_sys_db1_0017");CHECK(!read(f,0).valid);}
 {Fixture f;Chara c(f);f.put<Address>(Fixture::base+0x348cb48,c.root);CHECK(!read(f,0).valid);} // Not a GameSetting.
}
static void root_state_world_and_visibility_gate_everything(){
 {Fixture f;Chara c(f);f.flag(c.root,L"bHidden",true);CHECK(!read(f,0).valid);}
 {Fixture f;Chara c(f);auto t=f.raw<Address>(c.root+f.off(c.root,L"TickStateObj"));f.set<std::uint8_t>(t,L"State",1);CHECK(!read(f,0).valid);}
 {Fixture f;Chara c(f);auto t=f.raw<Address>(c.root+f.off(c.root,L"TickStateObj"));f.flag(t,L"PreState",true);CHECK(!read(f,0).valid);}
 {Fixture f;Chara c(f);auto t=f.raw<Address>(c.sp+f.off(c.sp,L"TickStateObj"));f.set(t,L"ParentObj",c.root);CHECK(!read(f,0).valid);}
 {Fixture f;Chara c(f);auto other=f.object(L"OtherLevel",f.cls[L"Level"]);f.put(other+0xb0,f.object(L"OtherWorld",f.cls[L"World"]));f.put(c.root+0x20,other);CHECK(!read(f,0).valid);}
 {Fixture f;Chara c(f);f.put<Address>(f.engine+0x5d8,0);CHECK(!read(f,0).valid);}
 {Fixture f;Chara c(f);auto chr=f.raw<Address>(c.screen[0][0]+f.off(c.screen[0][0],L"ChrRoot"));f.flag(chr,L"bVisible",false);CHECK(!read(f,0).valid);}
 {Fixture f;Chara c(f);auto mesh=f.raw<Address>(c.screen[0][0]+f.off(c.screen[0][0],L"SM_Character"));f.put<Address>(mesh+0xb0,c.root);CHECK(!read(f,0).valid);}
 {Fixture f;Chara c(f);f.set(c.screen[0][0],L"TeamNo",1);CHECK(!read(f,0).valid);}
 {Fixture f;Chara c(f);f.set(c.tiles[0],L"FocusTeamNoBit",2);CHECK(!read(f,0).valid);}
 {Fixture f;Chara c(f);auto comp=f.raw<Address>(c.root+f.off(c.root,L"BP_Screen_1P_Main"));f.put(comp+0x308,c.screen[1][0]);CHECK(!read(f,0).valid);}
}
static void ambiguous_roots_are_not_resolved_by_picking_one(){
 Fixture f;Chara c(f);CHECK(read(f,0).valid);Chara second(f);CHECK(!read(f,0).valid);
 auto t=f.raw<Address>(second.root+f.off(second.root,L"TickStateObj"));f.set<std::uint8_t>(t,L"State",0);CHECK(read(f,0).valid);
}
static void malformed_arrays_are_rejected(){
 {Fixture f;Chara c(f);auto at=c.root+f.off(c.root,L"Input_ArrObj");f.put<int>(at+8,3);CHECK(!read(f,0).valid);f.put<int>(at+8,-1);CHECK(!read(f,0).valid);}
 {Fixture f;Chara c(f);f.items<int>(c.sp,L"FocusUniqId",{5,1});CHECK(!read(f,0).valid);}
 {Fixture f;Chara c(f);f.items<Address>(c.root,L"Input_ArrObj",{c.root,0});CHECK(!read(f,0).valid);} // Unknown input owner.
}
static void costume_and_color_rows_read_rendered_values(){
 Fixture f;Chara c(f);auto m=c.costume_menu(0);auto s=read(f,0);CHECK(s.valid);CHECK(s.screen==L"Player 1");CHECK(s.label==L"Costume");CHECK(s.value==L"Normal");CHECK(s.position==1);CHECK(s.count==3);
 auto l0=f.raw<Address>(m+f.off(m,L"WD_ListItem00")),l1=f.raw<Address>(m+f.off(m,L"WD_ListItem01"));
 f.set(m,L"FocusMenu",1);CHECK(!read(f,0).valid); // Color cursor highlight still hidden.
 f.put<std::uint8_t>(f.raw<Address>(l1+f.off(l1,L"CanvasPanel_Cursor"))+0x91,0);s=read(f,0);CHECK(s.label==L"Color");CHECK(s.value==L"2");CHECK(s.position==2);CHECK(s.count==4);
 f.set(l1,L"ColorFocus",4);CHECK(!read(f,0).valid);f.set(l1,L"ColorFocus",1);
 f.set(m,L"FocusMenu",0);f.put<std::uint8_t>(l0+0x91,2);CHECK(!read(f,0).valid); // Costume row hidden (no alternate costumes).
 f.put<std::uint8_t>(l0+0x91,0);f.set(l0,L"CurrentSubitem",1);CHECK(!read(f,0).valid); // Rendered "Normal" disagrees with entry 1.
 f.put<std::uint8_t>(f.raw<Address>(l0+f.off(l0,L"TextBlock_Subitem"))+0x91,2);CHECK(!read(f,0).valid);
}
static void costume_install_notice_is_appended_only_when_shown(){
 Fixture f;Chara c(f);auto m=c.costume_menu(0);auto at=f.nested(m,L"WD_KeyConfAttention_153",f.cls[L"WD_KeyConfAttention_C"],L"Attention");f.block(at,L"TextBlock_Label",L"This cannot be selected since data installation is incomplete.");f.tick(at,0);
 CHECK(read(f,0).value==L"Normal");auto t=f.raw<Address>(at+f.off(at,L"TickStateObj"));f.set<std::uint8_t>(t,L"State",2);CHECK(read(f,0).value==L"Normal. This cannot be selected since data installation is incomplete.");
 f.put<std::uint8_t>(at+0x91,2);CHECK(read(f,0).value==L"Normal");
 f.set(m,L"TeamNo",1);CHECK(!read(f,0).valid);
}
static void stone_reads_bounded_english_name_and_rendered_details(){
 Fixture f;Chara c(f);auto g=c.stone(0);auto s=read(f,0);CHECK(s.valid);CHECK(s.screen==L"Player 1");CHECK(s.label==L"Power Stone");
 CHECK(s.value==L"INFINITY STORM: Unleashes a powerful attack. INFINITY SURGE: Knocks the enemy back.");CHECK(s.position==2);CHECK(s.count==6);
 f.items<std::uint8_t>(g,L"InvalidGemType",{1,0,0,0,0,0});s=read(f,0);CHECK(s.position==1);CHECK(s.count==5);
 auto d=f.raw<Address>(g+f.off(g,L"DetailWindow"));auto wc=f.raw<Address>(d+f.off(d,L"GemTypeText"));
 f.flag(wc,L"bVisible",false);s=read(f,0);CHECK(s.valid);CHECK(s.value.empty()); // 3D detail widget hidden: name only.
 f.flag(wc,L"bVisible",true);f.flag(d,L"bHidden",true);CHECK(read(f,0).value.empty());f.flag(d,L"bHidden",false);
 auto w=f.raw<Address>(wc+f.off(wc,L"Widget"));f.put<std::uint8_t>(f.panel_of[w]+0x91,1);CHECK(read(f,0).value.empty());f.put<std::uint8_t>(f.panel_of[w]+0x91,0);
 f.set(d,L"GemDetail",f.user(f.cls[L"WD_GemDetail_C"],L"Other",false));CHECK(read(f,0).value.empty());
}
static void stone_rejects_unproven_focus(){
 {Fixture f;Chara c(f);auto g=c.stone(0);f.items<std::uint8_t>(g,L"InvalidGemType",{0,1,0,0,0,0});CHECK(!read(f,0).valid);}
 {Fixture f;Chara c(f);auto g=c.stone(0);f.items<int>(g,L"FocusGemNoArray",{6});CHECK(!read(f,0).valid);}
 {Fixture f;Chara c(f);auto g=c.stone(0);auto mats=f.raw<Address>(g+f.off(g,L"MT_ArrGemName"));f.put(mats+8,f.object(L"MT_Time",f.kind(L"MaterialInstanceConstant")));CHECK(!read(f,0).valid);}
 {Fixture f;Chara c(f);auto g=c.stone(0);auto meshes=f.raw<Address>(g+f.off(g,L"ArrGemNameMesh"));f.flag(f.raw<Address>(meshes+8),L"bVisible",false);CHECK(!read(f,0).valid);}
 {Fixture f;Chara c(f);auto g=c.stone(0);f.set(g,L"TeamNo",1);CHECK(!read(f,0).valid);}
}
static void resident_menu_data_text_is_preferred_for_stone_names(){
 Fixture f;Chara c(f);c.stone(0);auto item=f.object(L"PLAYER1_GemType",f.cls[L"UIMenuItemDataAsset"]);f.texts(item,L"ArrText",{L"Pierre de l'Espace",L"Pierre du Pouvoir",L"R",L"S",L"M",L"T"});
 CHECK(read(f,0).label==L"Pierre du Pouvoir");f.texts(item,L"ArrText",{L"a",L"msg_sys_tra_0169",L"c",L"d",L"e",L"f"});CHECK(read(f,0).label==L"Power Stone");
}

struct Stage {
 Fixture& f;Address root=0,sp=0,name=0,widget=0,wc=0,tiles[2]{};
 explicit Stage(Fixture& fx):f(fx){
  auto main=f.klass(L"BP_StageSelectMain_C",f.cls[L"BP_UIBase_C"]);f.prop(main,L"StageSelectTeamNo",L"IntProperty",4);f.prop(main,L"StageParentObj",L"ObjectProperty",8);f.prop(main,L"BP_Stage_Parent",L"ObjectProperty",8);
  auto parent=f.klass(L"BP_Stage_Parent_C",f.cls[L"BP_UIMenuBase_C"]);f.prop(parent,L"FocusUniqId",L"IntProperty",4);f.prop(parent,L"FocusStageType",L"ByteProperty",1);f.arr(parent,L"ArrChildStage",L"ObjectProperty",8);f.prop(parent,L"StageName",L"ObjectProperty",8);f.prop(parent,L"BP_Select_Stage_Name",L"ObjectProperty",8);
  auto tile=f.klass(L"BP_Stage_C",f.cls[L"BP_UIMenuBase_C"]);f.prop(tile,L"StageData",L"StructProperty",1);
  auto nm=f.klass(L"BP_Stage_Name_C",f.cls[L"BP_UIMenuBase_C"]);f.prop(nm,L"StageName",L"ObjectProperty",8);f.prop(nm,L"WD_StageName",L"ObjectProperty",8);
  auto wd=f.klass(L"WD_StageName_C",f.cls[L"UserWidget"]);f.prop(wd,L"TextBlock_StageName",L"ObjectProperty",8);
  root=f.actor(main,L"BP_StageSelectMain_C");f.tick(root);f.set(root,L"StageSelectTeamNo",1);
  sp=f.actor(parent,L"BP_Stage_Parent_C");f.tick(sp);f.attach(root,L"BP_Stage_Parent",sp);f.set(root,L"StageParentObj",sp);
  for(int i=0;i<2;++i){tiles[i]=f.actor(tile,L"BP_Stage_C");f.tick(tiles[i]);f.set<std::uint8_t>(tiles[i],L"StageData",static_cast<std::uint8_t>(i+1));}
  f.items<Address>(sp,L"ArrChildStage",{tiles[0],tiles[1]});f.set(sp,L"FocusUniqId",0);f.set<std::uint8_t>(sp,L"FocusStageType",1);
  name=f.actor(nm,L"BP_Stage_Name_C");f.tick(name);f.attach(sp,L"BP_Select_Stage_Name",name);f.set(sp,L"StageName",name);
  wc=f.comp(L"WidgetComponent",L"StageName",name);f.set(name,L"StageName",wc);widget=f.user(wd,L"WD_StageName_C",false);f.set(wc,L"Widget",widget);f.set(name,L"WD_StageName",widget);
  f.block(widget,L"TextBlock_StageName",L"XGARD - BIFROST BRIDGE");
 }
};
static void stage_reads_the_rendered_3d_name_and_controlling_side(){
 Fixture f;Stage st(f);auto s=read(f,0);CHECK(s.valid);CHECK(s.screen==L"Player 2");CHECK(s.label==L"XGARD - BIFROST BRIDGE");CHECK(read(f,1).label==s.label); // Independent of the tracked side.
 f.set(st.root,L"StageSelectTeamNo",0);CHECK(read(f,1).screen==L"Player 1");
 f.flag(st.wc,L"bVisible",false);CHECK(!read(f,0).valid);f.flag(st.wc,L"bVisible",true);
 f.flag(st.wc,L"bHiddenInGame",true);CHECK(!read(f,0).valid);f.flag(st.wc,L"bHiddenInGame",false);
 f.put<std::uint8_t>(f.panel_of[st.widget]+0x91,2);CHECK(!read(f,0).valid);f.put<std::uint8_t>(f.panel_of[st.widget]+0x91,0);
 f.put(st.wc+0xb0,st.sp);CHECK(!read(f,0).valid);f.put(st.wc+0xb0,st.name);
 f.set(st.name,L"WD_StageName",f.user(f.cls[L"WD_StageName_C"],L"Other",false));CHECK(!read(f,0).valid);f.set(st.name,L"WD_StageName",st.widget);
 f.set<std::uint8_t>(st.sp,L"FocusStageType",2);CHECK(!read(f,0).valid);f.set<std::uint8_t>(st.sp,L"FocusStageType",1);
 auto t=f.raw<Address>(st.name+f.off(st.name,L"TickStateObj"));f.set<std::uint8_t>(t,L"State",3);CHECK(!read(f,0).valid);f.set<std::uint8_t>(t,L"State",2);
 f.block(st.widget,L"TextBlock_StageName",L"");CHECK(!read(f,0).valid);
}
static void stage_and_character_roots_together_are_ambiguous(){
 Fixture f;Stage st(f);Chara c(f);CHECK(!read(f,0).valid);auto t=f.raw<Address>(c.root+f.off(c.root,L"TickStateObj"));f.set<std::uint8_t>(t,L"State",0);CHECK(read(f,0).label==L"XGARD - BIFROST BRIDGE");
}

struct Ctrl {
 Fixture& f;Address root=0,sym[3]{};
 explicit Ctrl(Fixture& fx):f(fx){
  auto main=f.klass(L"WD_CtrlSelectMain_C",f.cls[L"WD_MMBase_C"]);f.arr(main,L"CtrlSymbols",L"ObjectProperty",8);f.arr(main,L"CtrlSlotArray",L"IntProperty",4);f.arr(main,L"ConnectPadNoArray",L"IntProperty",4);for(auto n:{L"WD_Header",L"TextBlock_Caption1P",L"TextBlock_Caption2P"})f.prop(main,n,L"ObjectProperty",8);
  auto s=f.klass(L"WD_CtrlSymbol0_C",f.cls[L"UserWidget"]);f.prop(s,L"UserName",L"ObjectProperty",8);auto h=f.klass(L"WD_Header_C",f.cls[L"UserWidget"]);f.prop(h,L"TextBlock_Caption",L"ObjectProperty",8);
  root=f.user(main,L"WD_CtrlSelectMain_C",true);f.tick(root,2,true); // No IF_UI_Active: PreState never clears.
  auto hdr=f.nested(root,L"WD_Header",h,L"WD_Header");f.block(hdr,L"TextBlock_Caption",L"CONTROLLER SETTINGS");f.block(root,L"TextBlock_Caption1P",L"Player 1");f.block(root,L"TextBlock_Caption2P",L"Player 2");
  const wchar_t* who[3]{L"Controller 1",L"Controller 2",L""};std::vector<Address> all;
  for(int i=0;i<3;++i){sym[i]=f.user(s,L"WD_CtrlSymbol0_C",false);f.slot(sym[i],root);f.block(sym[i],L"UserName",who[i],L"UserNameTextBlock");all.push_back(sym[i]);}
  f.items<Address>(root,L"CtrlSymbols",all);f.items<int>(root,L"CtrlSlotArray",{1,-1});f.items<int>(root,L"ConnectPadNoArray",{0,1,-1});
 }
};
static void controller_assignment_lists_every_connected_controller_and_side(){
 Fixture f;Ctrl c(f);auto s=read(f,1);CHECK(s.valid);CHECK(s.screen==L"CONTROLLER SETTINGS");CHECK(s.label==L"Controller 1: center. Controller 2: Player 1");CHECK(read(f,0).label==s.label);
 f.items<int>(c.root,L"CtrlSlotArray",{1,0});CHECK(read(f,0).label==L"Controller 1: Player 2. Controller 2: Player 1");
 f.items<int>(c.root,L"ConnectPadNoArray",{0,-1,-1});CHECK(read(f,0).label==L"Controller 1: Player 2");
 f.put<std::uint8_t>(c.sym[0]+0x91,2);CHECK(!read(f,0).valid); // Nothing connected and drawn.
 f.put<std::uint8_t>(c.sym[0]+0x91,0);auto t=f.raw<Address>(c.root+f.off(c.root,L"TickStateObj"));f.set<std::uint8_t>(t,L"State",1);CHECK(!read(f,0).valid);
 f.set<std::uint8_t>(t,L"State",2);Address shared=f.raw<Address>(c.root+0x210);f.put<int>(shared+8,0);CHECK(!read(f,0).valid);
}

static void battle_settings_read_the_focused_row(){
 Fixture f;Chara c(f);
 auto bm=f.klass(L"BP_BattleSettingMain_C",f.cls[L"BP_UIBase_C"]);f.prop(bm,L"BattleSetting_Obj",L"ObjectProperty",8);
 auto con=f.klass(L"WD_BattleSetting_Con_C",f.cls[L"WD_UIParent_C"]);f.prop(con,L"WD_BattleSettigNew",L"ObjectProperty",8);
 auto pn=f.klass(L"WD_BattleSettig_New_C",f.cls[L"WD_UIParent_C"]);f.prop(pn,L"FocusListItem",L"ObjectProperty",8);f.arr(pn,L"ButtonArray",L"ObjectProperty",8);f.prop(pn,L"WD_Header",L"ObjectProperty",8);
 auto li=f.klass(L"WD_BS_ListItem0_C",f.cls[L"UserWidget"]);f.prop(li,L"TextBlock_Label",L"ObjectProperty",8);f.prop(li,L"TextBlock_Subitem",L"ObjectProperty",8);auto h=f.klass(L"WD_Header_C",f.cls[L"UserWidget"]);f.prop(h,L"TextBlock_Caption",L"ObjectProperty",8);
 auto actor=f.actor(bm,L"BP_BattleSettingMain_C");f.tick(actor);f.attach(c.root,L"BP_BattleSettingMain",actor);f.set(c.root,L"BattleSettingObj",actor);
 auto w=f.user(con,L"WD_BattleSetting_Con_C",true);f.tick(w);f.set(actor,L"BattleSetting_Obj",w);auto p=f.nested(w,L"WD_BattleSettigNew",pn,L"WD_BattleSettig_New");f.tick(p);
 f.block(f.nested(p,L"WD_Header",h,L"WD_Header"),L"TextBlock_Caption",L"BATTLE SETTINGS");
 std::vector<Address> rows;const wchar_t* labels[2][2]{{L"Time",L"99"},{L"Damage",L"Normal"}};for(auto& r:labels){auto row=f.user(li,L"Row",false);f.slot(row,p);f.block(row,L"TextBlock_Label",r[0]);f.block(row,L"TextBlock_Subitem",r[1]);rows.push_back(row);}
 f.items<Address>(p,L"ButtonArray",rows);f.set(p,L"FocusListItem",rows[1]);
 CHECK(read(f,0).label==L"ULTRON");f.flag(c.root,L"BattleSettingActive",true);
 auto s=read(f,1);CHECK(s.valid);CHECK(s.screen==L"BATTLE SETTINGS");CHECK(s.label==L"Damage");CHECK(s.value==L"Normal");CHECK(s.position==2);CHECK(s.count==2);
 f.put<std::uint8_t>(rows[0]+0x91,1);s=read(f,0);CHECK(s.position==1);CHECK(s.count==1);
 f.set(p,L"FocusListItem",f.user(li,L"Loose",false));CHECK(!read(f,0).valid);
}
static void team_preset_reports_only_caption_and_position(){
 Fixture f;Chara c(f);auto win=f.klass(L"WD_SelectTeamWindow_C",f.cls[L"WD_MenuBase_C"]);f.prop(win,L"TeamNo",L"IntProperty",4);f.prop(win,L"MenuFocus",L"IntProperty",4);f.arr(win,L"ArrSelectTeam",L"ObjectProperty",8);f.prop(win,L"TextBlock_Caption",L"ObjectProperty",8);
 auto item=f.klass(L"WD_SelectTeamList_C",f.cls[L"WD_UIPart_C"]);auto w=f.user(win,L"WD_SelectTeamWindow_C",true);f.tick(w,2,true);f.set(w,L"TeamNo",0);f.set(w,L"MenuFocus",1);f.block(w,L"TextBlock_Caption",L"TEAM SELECT");
 std::vector<Address> rows;for(int i=0;i<3;++i){auto r=f.user(item,L"WD_SelectTeamList",false);f.slot(r,w);rows.push_back(r);}f.items<Address>(w,L"ArrSelectTeam",rows);f.items<Address>(c.root,L"TeamSelect_Arr",{w,0});c.input(0,w);
 auto s=read(f,0);CHECK(s.valid);CHECK(s.label==L"TEAM SELECT");CHECK(s.value.empty());CHECK(s.position==2);CHECK(s.count==3);
 f.set(w,L"MenuFocus",3);CHECK(!read(f,0).valid);
}

// ---- Follow-up coverage: key/keyboard config, handicap, team icons, cache, tick class, alpha ----
// Native reflection layouts used here (same as the engine walk in unreal.cpp): UStructProperty.Struct and
// UArrayProperty.Inner at +0x78; UStruct.Children +0x38; UField.Next +0x28; UProperty offset +0x50.
// Struct members, FName pairs (index,number) and MaterialInstance parameter arrays are fixture encodings.
struct Media {
 Fixture& f;
 explicit Media(Fixture& fx):f(fx){
  if(f.cls.count(L"Image"))return;
  auto brush=strukt(L"SlateBrush");f.prop_at(brush,L"ResourceObject",L"ObjectProperty",0x48,8);
  auto image=f.klass(L"Image",f.cls[L"Widget"]);sprop(image,L"Brush",brush,0x88);
  auto scalar=strukt(L"ScalarParameterValue");f.prop_at(scalar,L"ParameterName",L"NameProperty",0,8);f.prop_at(scalar,L"ParameterValue",L"FloatProperty",8,4);
  auto texture=strukt(L"TextureParameterValue");f.prop_at(texture,L"ParameterName",L"NameProperty",0,8);f.prop_at(texture,L"ParameterValue",L"ObjectProperty",8,8);
  auto mi=f.klass(L"MaterialInstance");f.prop(mi,L"Parent",L"ObjectProperty",8);sarr(mi,L"ScalarParameterValues",scalar,28);sarr(mi,L"TextureParameterValues",texture,32);
  f.klass(L"MaterialInstanceConstant",mi);f.klass(L"MaterialInstanceDynamic",mi);
  auto panel=f.klass(L"PanelWidget",f.cls[L"Widget"]);f.arr(panel,L"Slots",L"ObjectProperty",8);f.klass(L"HorizontalBox",panel);
  auto slot=f.klass(L"PanelSlot");f.prop(slot,L"Content",L"ObjectProperty",8);f.klass(L"HorizontalBoxSlot",slot);
  auto key=strukt(L"Key");f.prop_at(key,L"KeyName",L"NameProperty",0,8);
 }
 Address strukt(const std::wstring& n){return f.cls[n]=f.object(n,f.kind(L"ScriptStruct"),0x100);}
 Address sprop(Address c,const std::wstring& n,Address s,int size){auto p=f.prop(c,n,L"StructProperty",size);f.put(p+0x78,s);return p;}
 Address sarr(Address c,const std::wstring& n,Address s,int stride){auto p=f.arr(c,n,L"StructProperty",stride);f.put(f.raw<Address>(p+0x78)+0x78,s);return p;}
 void fname(Address at,const std::wstring& n){f.put<int>(at,f.name_id(n));f.put<int>(at+4,0);}
 Address material(const std::wstring& name){return f.object(name,f.cls[L"MaterialInstanceConstant"]);}
 Address dynamic(Address parent,const std::vector<std::pair<std::wstring,float>>& scalars,const std::vector<std::pair<std::wstring,Address>>& textures){
  auto m=f.object(L"MaterialInstanceDynamic",f.cls[L"MaterialInstanceDynamic"]);f.set(m,L"Parent",parent);
  auto s=f.alloc(std::max<std::size_t>(1,scalars.size())*28);for(std::size_t i=0;i<scalars.size();++i){fname(s+i*28,scalars[i].first);f.put<float>(s+i*28+8,scalars[i].second);}
  auto at=m+f.off(m,L"ScalarParameterValues");f.put(at,s);f.put<int>(at+8,static_cast<int>(scalars.size()));f.put<int>(at+12,static_cast<int>(scalars.size()));
  auto t=f.alloc(std::max<std::size_t>(1,textures.size())*32);for(std::size_t i=0;i<textures.size();++i){fname(t+i*32,textures[i].first);f.put(t+i*32+8,textures[i].second);}
  at=m+f.off(m,L"TextureParameterValues");f.put(at,t);f.put<int>(at+8,static_cast<int>(textures.size()));f.put<int>(at+12,static_cast<int>(textures.size()));return m;}
 Address image(Address u,const std::wstring& field,Address resource){auto i=f.object(field,f.cls[L"Image"]);f.slot(i,u);f.put(i+f.off(i,L"Brush")+0x48,resource);if(!field.empty())f.set(u,field,i);return i;}
 // A runtime-built line (WD_KeyConfWaiting.SetHorizontalBox): TextBlocks and Images in a HorizontalBox.
 Address line(Address u,const std::wstring& field,const std::vector<std::pair<std::wstring,Address>>& pieces){
  auto box=f.object(field,f.cls[L"HorizontalBox"]);f.slot(box,u);f.set(u,field,box);std::vector<Address> slots;
  for(const auto& p:pieces){Address w=0;if(p.second){w=f.object(L"Image",f.cls[L"Image"]);f.put(w+f.off(w,L"Brush")+0x48,p.second);}else{w=f.object(L"TextBlock",f.cls[L"TextBlock"]);f.text(w+0x140,p.first);}
   auto s=f.object(L"HorizontalBoxSlot",f.cls[L"HorizontalBoxSlot"]);f.put(s+0x28,box);f.set(s,L"Content",w);f.put(w+0x30,s);f.put(w+0x20,u);slots.push_back(s);}
  f.items<Address>(box,L"Slots",slots);return box;}
};

struct KeyConf {
 Fixture& f;Media m;Address root=0,side[2]{},waiting[2]{},attention[2]{},bind=0;bool keyboard;
 KeyConf(Fixture& fx,bool kb):f(fx),m(fx),keyboard(kb){
  if(!f.cls.count(L"WD_KCbuttonParts_C")){
   auto user=f.cls[L"UserWidget"];
   auto p1=f.klass(L"WD_KCbuttonParts_C",user);f.prop(p1,L"TextBlock_Label",L"ObjectProperty",8);f.prop(p1,L"Image_Button",L"ObjectProperty",8);
   auto p2=f.klass(L"WD_KCbuttonParts2_C",user);f.prop(p2,L"TextBlock_Label",L"ObjectProperty",8);f.prop(p2,L"Image_Button",L"ObjectProperty",8);
   auto p3=f.klass(L"WD_KCbuttonParts3_C",user);f.prop(p3,L"TextBlock_Label",L"ObjectProperty",8);
   auto p4=f.klass(L"WD_KCbuttonParts4_C",user);f.prop(p4,L"TextBlock_Label",L"ObjectProperty",8);f.prop(p4,L"TextBlock_Subitem",L"ObjectProperty",8);
   auto p5=f.klass(L"WD_KCbuttonParts5_C",user);f.prop(p5,L"TextBlock_Label",L"ObjectProperty",8);f.prop(p5,L"TextBlock_SubItem",L"ObjectProperty",8);
   auto pad=f.klass(L"WD_KeyConf_C",f.cls[L"WD_UIParent_C"]);auto kbc=f.klass(L"WD_KeyboardConf_C",f.cls[L"WD_UIParent_C"]);
   for(auto c:{pad,kbc}){f.prop(c,L"TextBlock_Label",L"ObjectProperty",8);f.prop(c,L"ButtonFocusID",L"IntProperty",4);f.prop(c,L"IsActiveMainPanel",L"BoolProperty",1);
    for(auto n:{L"WD_KCbuttonParts_P1",L"WD_KCbuttonParts2_Burst",L"WD_KCbuttonParts3_Decide",L"WD_KCbuttonParts4_AutoCombo",L"WD_KCbuttonParts5_ToOperation",L"WD_KCbuttonParts_UP"})f.prop(c,n,L"ObjectProperty",8);}
   m.sarr(kbc,L"KeyboardConfigData",f.cls[L"Key"],24);f.prop(kbc,L"WD_KeyboardConfBind_138",L"ObjectProperty",8);
   auto b=f.klass(L"WD_KeyboardConfBind_C",f.cls[L"WD_UIParent_C"]);f.prop(b,L"TextBlock_Label",L"ObjectProperty",8);
   auto w=f.klass(L"WD_KeyConfWaiting_C",f.cls[L"WD_UIParent_C"]);f.prop(w,L"TextBlock_Label",L"ObjectProperty",8);for(auto n:{L"HorizontalBox_174",L"HorizontalBox_175",L"HorizontalBox_176",L"HorizontalBox_177"})f.prop(w,n,L"ObjectProperty",8);
   if(!f.cls.count(L"WD_KeyConfAttention_C")){auto a=f.klass(L"WD_KeyConfAttention_C",f.cls[L"WD_UIParent_C"]);f.prop(a,L"TextBlock_Label",L"ObjectProperty",8);}
   auto pm=f.klass(L"WD_KeyConfMain_C",f.cls[L"WD_MenuBase_C"]);for(auto n:{L"WD_KeyConf_Left",L"WD_KeyConf_Right",L"WD_KeyConf_WaitingL",L"WD_KeyConf_WaitingR",L"WD_KeyConf_Attention_L",L"WD_KeyConf_Attention_R"})f.prop(pm,n,L"ObjectProperty",8);
   auto km=f.klass(L"WD_KeyboardConfMain_C",f.cls[L"WD_MenuBase_C"]);for(auto n:{L"WD_KeyboardConf",L"WD_KeyConf_Right",L"WD_KeyConf_WaitingL",L"WD_KeyConf_WaitingR",L"WD_KeyboardConf_Attention",L"WD_KeyConf_Attention_R"})f.prop(km,n,L"ObjectProperty",8);
  }
  root=f.user(f.cls[kb?L"WD_KeyboardConfMain_C":L"WD_KeyConfMain_C"],L"KeyConfMain",true);f.tick(root,2,true); // Enter/Exit only: PreState never clears.
  const wchar_t* sides[2]{kb?L"WD_KeyboardConf":L"WD_KeyConf_Left",L"WD_KeyConf_Right"};const wchar_t* caption[2]{L"PLAYER 1",L"PLAYER 2"};
  for(int s=0;s<2;++s){
   auto c=f.nested(root,sides[s],f.cls[(kb&&!s)?L"WD_KeyboardConf_C":L"WD_KeyConf_C"],L"KeyConf");f.tick(c,s?0:2);side[s]=c;f.block(c,L"TextBlock_Label",caption[s]);f.set(c,L"ButtonFocusID",1);f.flag(c,L"IsActiveMainPanel",true);
   auto p=f.nested(c,L"WD_KCbuttonParts_P1",f.cls[L"WD_KCbuttonParts_C"],L"P1");f.block(p,L"TextBlock_Label",L"Light Punch");m.image(p,L"Image_Button",m.material(L"MI_Btn_A"));
   p=f.nested(c,L"WD_KCbuttonParts2_Burst",f.cls[L"WD_KCbuttonParts2_C"],L"Burst");f.block(p,L"TextBlock_Label",L"Infinity Surge");m.image(p,L"Image_Button",m.material(L"MI_Btn_RT"));
   p=f.nested(c,L"WD_KCbuttonParts3_Decide",f.cls[L"WD_KCbuttonParts3_C"],L"Decide");f.block(p,L"TextBlock_Label",L"Exit");
   p=f.nested(c,L"WD_KCbuttonParts4_AutoCombo",f.cls[L"WD_KCbuttonParts4_C"],L"AutoCombo");f.block(p,L"TextBlock_Label",L"Auto Combo");f.block(p,L"TextBlock_Subitem",L"ON");
   p=f.nested(c,L"WD_KCbuttonParts5_ToOperation",f.cls[L"WD_KCbuttonParts5_C"],L"ToOperation");f.block(p,L"TextBlock_Label",L"Control Type");f.block(p,L"TextBlock_SubItem",L"Standard");
   p=f.nested(c,L"WD_KCbuttonParts_UP",f.cls[L"WD_KCbuttonParts_C"],L"UP");f.block(p,L"TextBlock_Label",L"Up");m.image(p,L"Image_Button",m.dynamic(m.material(L"MI_KeyboardIcon"),{},{}));
   waiting[s]=f.nested(root,s?L"WD_KeyConf_WaitingR":L"WD_KeyConf_WaitingL",f.cls[L"WD_KeyConfWaiting_C"],L"Waiting");f.tick(waiting[s],0);
   attention[s]=f.nested(root,s?L"WD_KeyConf_Attention_R":(kb?L"WD_KeyboardConf_Attention":L"WD_KeyConf_Attention_L"),f.cls[L"WD_KeyConfAttention_C"],L"Attention");f.tick(attention[s],0);f.block(attention[s],L"TextBlock_Label",L"Settings saved.");
  }
  if(kb){std::vector<std::wstring> keys{L"W",L"S",L"A",L"D",L"SpaceBar",L"J",L"K",L"L"};auto data=f.alloc(19*24);for(int i=0;i<19;++i)m.fname(data+i*24,i<8?keys[i]:L"Escape");auto at=side[0]+f.off(side[0],L"KeyboardConfigData");f.put(at,data);f.put<int>(at+8,19);f.put<int>(at+12,19);
   bind=f.nested(side[0],L"WD_KeyboardConfBind_138",f.cls[L"WD_KeyboardConfBind_C"],L"Bind");f.tick(bind,0);f.block(bind,L"TextBlock_Label",L"Press the key to assign.");}
 }
 Address part(int s,const std::wstring& field){return f.raw<Address>(side[s]+f.off(side[s],field));}
 void retexture(int s,const std::wstring& field,const std::wstring& material){auto p=part(s,field);auto img=f.raw<Address>(p+f.off(p,L"Image_Button"));f.put(img+f.off(img,L"Brush")+0x48,m.material(material));}
};
static void pad_key_config_reads_focused_action_and_glyph_over_character_select(){
 Fixture f;Chara c(f);KeyConf k(f,false);auto s=read(f,0);CHECK(s.valid);CHECK(s.screen==L"PLAYER 1");CHECK(s.label==L"Light Punch");CHECK(s.value==L"A");
 CHECK(read(f,1).label==L"Light Punch"); // Only the left form is active: it is read with its own caption.
 f.set(k.side[0],L"ButtonFocusID",7);s=read(f,0);CHECK(s.label==L"Infinity Surge");CHECK(s.value==L"RT");
 k.retexture(0,L"WD_KCbuttonParts2_Burst",L"MI_Btn_SP_L");CHECK(read(f,0).value==L"(button icon)");
 f.set(k.side[0],L"ButtonFocusID",16);s=read(f,0);CHECK(s.label==L"Exit");CHECK(s.value.empty());
 f.set(k.side[0],L"ButtonFocusID",0);s=read(f,0);CHECK(s.label==L"Control Type");CHECK(s.value==L"Standard");
 f.set(k.side[0],L"ButtonFocusID",20);CHECK(!read(f,0).valid); // Sub-panel id while the main panel is active.
 f.flag(k.side[0],L"IsActiveMainPanel",false);s=read(f,0);CHECK(s.label==L"Auto Combo");CHECK(s.value==L"ON");
 f.set(k.side[0],L"ButtonFocusID",18);CHECK(!read(f,0).valid);f.set(k.side[0],L"ButtonFocusID",1);f.flag(k.side[0],L"IsActiveMainPanel",true);
 auto t=f.tick(k.side[1]);(void)t;CHECK(read(f,1).screen==L"PLAYER 2"); // Tracked side preferred once its form is active.
 f.put<std::uint8_t>(k.root+0x91,2);CHECK(read(f,0).label==L"ULTRON"); // Closed popup: the character phase again.
}
static void key_config_from_options_reads_waiting_prompt_and_notices(){
 Fixture f;KeyConf k(f,false);f.set<std::uint8_t>(f.raw<Address>(k.side[0]+f.off(k.side[0],L"TickStateObj")),L"State",0);
 CHECK(!read(f,0).valid);
 f.set<std::uint8_t>(f.raw<Address>(k.waiting[0]+f.off(k.waiting[0],L"TickStateObj")),L"State",2);
 k.m.line(k.waiting[0],L"HorizontalBox_174",{{L"Press",0},{L"",k.m.material(L"MI_Btn_A")},{L"to configure.",0}});
 auto s=read(f,0);CHECK(s.valid);CHECK(s.label==L"Press A to configure.");
 k.m.line(k.waiting[0],L"HorizontalBox_175",{{L"",k.m.material(L"MI_Btn_Dir")},{L"Move",0}});CHECK(read(f,0).label==L"Press A to configure. (button icon) Move");
 f.set<std::uint8_t>(f.raw<Address>(k.side[0]+f.off(k.side[0],L"TickStateObj")),L"State",2);
 f.set<std::uint8_t>(f.raw<Address>(k.attention[0]+f.off(k.attention[0],L"TickStateObj")),L"State",2);s=read(f,0);CHECK(s.label==L"Light Punch");CHECK(s.value==L"A. Settings saved.");
 auto t=f.raw<Address>(k.root+f.off(k.root,L"TickStateObj"));f.set<std::uint8_t>(t,L"State",1);CHECK(!read(f,0).valid);
}
static void keyboard_config_reads_bound_key_and_bind_prompt(){
 Fixture f;Chara c(f);KeyConf k(f,true);auto s=read(f,0);CHECK(s.valid);CHECK(s.label==L"Light Punch");CHECK(s.value==L"SpaceBar"); // Focus 1 -> KeyboardConfigData[4].
 f.set(k.side[0],L"ButtonFocusID",24);f.flag(k.side[0],L"IsActiveMainPanel",false);s=read(f,0);CHECK(s.label==L"Up");CHECK(s.value==L"W"); // Focus 24 -> index 0.
 f.set(k.side[0],L"ButtonFocusID",7);f.flag(k.side[0],L"IsActiveMainPanel",true);s=read(f,0);CHECK(s.label==L"Infinity Surge");CHECK(s.value.empty()); // Escape draws a blank icon.
 f.set<std::uint8_t>(f.raw<Address>(k.bind+f.off(k.bind,L"TickStateObj")),L"State",2);s=read(f,0);CHECK(s.label==L"Press the key to assign.");
 KeyConf pad(f,false);CHECK(!read(f,0).valid); // Two drawn config roots: no arbitrary owner.
}

struct Battle {
 Fixture& f;Address con=0,panel=0,rows[2]{};
 explicit Battle(Fixture& fx,Chara& c):f(fx){
  auto bm=f.klass(L"BP_BattleSettingMain_C",f.cls[L"BP_UIBase_C"]);f.prop(bm,L"BattleSetting_Obj",L"ObjectProperty",8);
  auto cc=f.klass(L"WD_BattleSetting_Con_C",f.cls[L"WD_UIParent_C"]);f.prop(cc,L"WD_BattleSettigNew",L"ObjectProperty",8);f.prop(cc,L"WD_Handicap_1P",L"ObjectProperty",8);f.prop(cc,L"WD_Handicap_2P",L"ObjectProperty",8);
  auto pn=f.klass(L"WD_BattleSettig_New_C",f.cls[L"WD_UIParent_C"]);f.prop(pn,L"FocusListItem",L"ObjectProperty",8);f.arr(pn,L"ButtonArray",L"ObjectProperty",8);
  auto li=f.klass(L"WD_BS_ListItem0_C",f.cls[L"UserWidget"]);f.prop(li,L"TextBlock_Label",L"ObjectProperty",8);f.prop(li,L"TextBlock_Subitem",L"ObjectProperty",8);
  auto h=f.klass(L"WD_Handicap_C",f.cls[L"WD_UIParent_C"]);for(auto n:{L"TextBlock_Label1P",L"TextBlock_Label2P",L"Image_Gauge",L"CanvasPanel_Handi"})f.prop(h,n,L"ObjectProperty",8);f.prop(h,L"TeamNo",L"IntProperty",4);f.prop(h,L"Focus",L"IntProperty",4);
  auto actor=f.actor(bm,L"BP_BattleSettingMain_C");f.tick(actor);f.attach(c.root,L"BP_BattleSettingMain",actor);f.set(c.root,L"BattleSettingObj",actor);f.flag(c.root,L"BattleSettingActive",true);
  con=f.user(cc,L"WD_BattleSetting_Con_C",true);f.tick(con);f.set(actor,L"BattleSetting_Obj",con);panel=f.nested(con,L"WD_BattleSettigNew",pn,L"WD_BattleSettig_New");f.tick(panel);
  for(int i=0;i<2;++i){rows[i]=f.user(li,L"Row",false);f.slot(rows[i],panel);f.block(rows[i],L"TextBlock_Label",i?L"Damage":L"Time");f.block(rows[i],L"TextBlock_Subitem",i?L"Normal":L"99");}
  f.items<Address>(panel,L"ButtonArray",{rows[0],rows[1]});f.set(panel,L"FocusListItem",rows[1]);
 }
 Address handicap(int team,int focus,float drawn){
  Media m(f);auto h=f.nested(con,team?L"WD_Handicap_2P":L"WD_Handicap_1P",f.cls[L"WD_Handicap_C"],L"WD_Handicap");f.tick(h);f.set(h,L"TeamNo",team);f.set(h,L"Focus",focus);
  auto canvas=f.panel(h,L"CanvasPanel_Handi");(void)canvas;f.block(h,team?L"TextBlock_Label2P":L"TextBlock_Label1P",L"HANDICAP");f.put<std::uint8_t>(f.block(h,team?L"TextBlock_Label1P":L"TextBlock_Label2P",L"HANDICAP")+0x91,2);
  m.image(h,L"Image_Gauge",m.dynamic(m.material(L"MI_HandicapGauge"),{{L"Switch",0.f},{L"index",drawn}},{}));return h;}
};
static void handicap_reads_the_drawn_gauge_level_for_the_tracked_side(){
 Fixture f;Chara c(f);Battle b(f,c);CHECK(read(f,0).label==L"Damage");auto h=b.handicap(0,2,3.f);
 auto s=read(f,0);CHECK(s.valid);CHECK(s.screen==L"Player 1");CHECK(s.label==L"HANDICAP");CHECK(s.value.empty());CHECK(s.position==3);CHECK(s.count==5);
 CHECK(read(f,1).label==L"Damage"); // Player 2 has no active handicap form: the shared list row.
 auto img=f.raw<Address>(h+f.off(h,L"Image_Gauge"));auto mid=f.raw<Address>(img+f.off(img,L"Brush")+0x48);auto values=f.raw<Address>(mid+f.off(mid,L"ScalarParameterValues"));
 f.put<float>(values+28+8,2.f);CHECK(!read(f,0).valid); // Drawn index must be Focus+1 (SetLevel writes Focus+1 last).
 f.put<float>(values+28+8,3.f);f.put<std::uint8_t>(f.raw<Address>(h+f.off(h,L"CanvasPanel_Handi"))+0x91,1);CHECK(!read(f,0).valid);
 f.put<std::uint8_t>(f.raw<Address>(h+f.off(h,L"CanvasPanel_Handi"))+0x91,0);f.set(h,L"TeamNo",1);CHECK(!read(f,0).valid);
}

static Address team_window(Fixture& f,Chara& c){
 Media m(f);auto win=f.klass(L"WD_SelectTeamWindow_C",f.cls[L"WD_MenuBase_C"]);f.prop(win,L"TeamNo",L"IntProperty",4);f.prop(win,L"MenuFocus",L"IntProperty",4);f.arr(win,L"ArrSelectTeam",L"ObjectProperty",8);f.prop(win,L"TextBlock_Caption",L"ObjectProperty",8);
 auto item=f.klass(L"WD_SelectTeamList_C",f.cls[L"WD_UIPart_C"]);f.prop(item,L"Image_MainChr",L"ObjectProperty",8);f.prop(item,L"Image_SubChr",L"ObjectProperty",8);
 auto w=f.user(win,L"WD_SelectTeamWindow_C",true);f.tick(w,2,true);f.set(w,L"TeamNo",0);f.set(w,L"MenuFocus",0);f.block(w,L"TextBlock_Caption",L"TEAM SELECT");
 auto icon=[&](const std::wstring& package){auto pkg=f.object(package,f.kind(L"Package"),0x100);auto tex=f.object(L"TEX_0",f.kind(L"Texture2D"),0x100,pkg);return m.dynamic(m.material(L"MI_TeamChr"),{},{{L"TEX",tex}});};
 auto r=f.user(item,L"WD_SelectTeamList",false);f.slot(r,w);m.image(r,L"Image_MainChr",icon(L"/Game/Chara/001/UI_N/HUD/1P/TEX_0"));m.image(r,L"Image_SubChr",icon(L"/Game/Chara/002/UI_AR1/HUD/1P/TEX_1"));
 f.items<Address>(w,L"ArrSelectTeam",{r});f.items<Address>(c.root,L"TeamSelect_Arr",{w,0});c.input(0,w);return r;
}
static void team_preset_names_come_from_the_drawn_portraits(){
 Fixture f;Chara c(f);auto r=team_window(f,c);auto s=read(f,0);CHECK(s.valid);CHECK(s.label==L"TEAM SELECT");CHECK(s.value==L"ULTRON, X");CHECK(s.position==1);CHECK(s.count==1);
 auto sub=f.raw<Address>(r+f.off(r,L"Image_SubChr"));f.put<std::uint8_t>(sub+0x91,2);CHECK(read(f,0).value==L"ULTRON"); // Hidden portrait is not named.
 f.put<std::uint8_t>(sub+0x91,0);auto mid=f.raw<Address>(sub+f.off(sub,L"Brush")+0x48);auto values=f.raw<Address>(mid+f.off(mid,L"TextureParameterValues"));
 auto pkg=f.object(L"/Game/UI/Select/Chr_Screen/parts/TEX_Chr150_1P",f.kind(L"Package"),0x100);f.put(values+8,f.object(L"TEX_Chr150_1P",f.kind(L"Texture2D"),0x100,pkg));CHECK(read(f,0).value==L"ULTRON");
}

static void new_and_replaced_roots_are_seen_without_waiting_for_a_rescan(){
 Fixture f;SelectionCapture capture(f.unreal);CHECK(!capture.sample(1000,0).valid);
 Stage a(f);CHECK(capture.sample(1010,0).valid); // Object count grew: rescanned at once.
 // A root re-created in a recycled registry slot keeps the count unchanged; the retired serial forces a rescan.
 Stage b(f);auto entry=f.entries+static_cast<Address>(f.raw<int>(b.root+0xc))*24;auto dummy=f.object(L"Dummy",f.kind(L"Object"));f.put(entry,dummy);
 CHECK(capture.sample(2000,0).valid);f.put(entry,b.root);auto old=f.entries+static_cast<Address>(f.raw<int>(a.root+0xc))*24;f.put(old,dummy);f.put<int>(old+0x10,99999); // a retired: its slot now holds another object.
 CHECK(capture.sample(2010,0).valid);
}
static void tick_state_object_must_be_the_exact_class(){
 Fixture f;Chara c(f);CHECK(read(f,0).valid);auto other=f.klass(L"OtherTick");f.prop(other,L"State",L"ByteProperty",1);f.prop(other,L"PreState",L"BoolProperty",1);f.prop(other,L"ParentObj",L"ObjectProperty",8);
 auto t=f.object(L"Tick",other);f.set(c.root,L"TickStateObj",t);f.set(t,L"ParentObj",c.root);f.set<std::uint8_t>(t,L"State",2);CHECK(!read(f,0).valid);
}
static void non_finite_alpha_hides_3d_widget_text(){
 Fixture f;Stage st(f);auto c=f.prop(f.cls[L"WD_StageName_C"],L"ColorAndOpacity",L"StructProperty",16);(void)c;f.put<float>(st.widget+f.off(st.widget,L"ColorAndOpacity")+12,1.f);CHECK(read(f,0).valid);
 f.put<float>(st.widget+f.off(st.widget,L"ColorAndOpacity")+12,std::numeric_limits<float>::infinity());CHECK(!read(f,0).valid);
}

static void active_options_popup_yields_to_the_config_reader(){
 Fixture f;KeyConf k(f,false);auto cls=f.cls[L"WD_MenuBase_C"];f.prop(cls,L"IsMenuBaseInput",L"BoolProperty",1);f.prop(cls,L"IsPopupActive",L"BoolProperty",1);f.prop(cls,L"ItemIndex",L"IntProperty",4);f.arr(cls,L"Items",L"ObjectProperty",8);auto row_cls=f.klass(L"OptionsItem",f.cls[L"UserWidget"]);f.prop(row_cls,L"ItemName",L"TextProperty",24);auto root=f.user(cls,L"Options",true),row=f.user(row_cls,L"ConfigRow",false);f.slot(row,root);f.field_text(row,L"ItemName",L"Button Configuration");f.items<Address>(root,L"Items",{row});f.flag(root,L"IsMenuBaseInput",true);f.flag(root,L"IsPopupActive",true);MenuCapture menus(f.unreal);SelectionCapture selections(f.unreal);auto s=menus.sample(1000);if(!s.valid&&!s.blocked)s=selections.sample(1000,0);CHECK(s.valid);CHECK(s.label==L"Light Punch");CHECK(s.value==L"A");f.flag(root,L"IsPopupActive",false);f.put<std::uint8_t>(k.root+0x91,2);s=menus.sample(1010);CHECK(s.label==L"Button Configuration");
}
static void grid_install_notice_is_visible_even_for_a_random_placeholder(){
 Fixture f;Chara c(f);auto cls=f.klass(L"WD_KeyConfAttention_C",f.cls[L"WD_UIParent_C"]);f.prop(cls,L"TextBlock_Label",L"ObjectProperty",8);f.arr(f.cls[L"BP_Select_Parent_C"],L"AttentionObj",L"ObjectProperty",8);auto note=f.user(cls,L"InstallNotice",true);f.tick(note);f.block(note,L"TextBlock_Label",L"This cannot be selected since data installation is incomplete.");f.items<Address>(c.sp,L"AttentionObj",{note,0});auto s=read(f,0);CHECK(s.label==L"ULTRON");CHECK(s.value.find(L"installation is incomplete")!=std::wstring::npos);f.put<std::uint8_t>(note+0x91,2);CHECK(read(f,0).value.empty());f.put<std::uint8_t>(note+0x91,0);f.flag(c.screen[0][0],L"Random",true);f.set(c.screen[0][0],L"PlayerType",999);s=read(f,0);CHECK(s.label==L"?????");CHECK(s.value.find(L"installation is incomplete")!=std::wstring::npos);
 f.flag(c.screen[0][0],L"Random",false);f.set(c.screen[0][0],L"PlayerType",1);f.put(f.table+8,Address(0));s=read(f,0);CHECK(s.valid);CHECK(s.label.find(L"installation is incomplete")!=std::wstring::npos);
}
static void team_preset_pairs_rendered_colors_with_their_portraits(){
 Fixture f;Chara c(f);auto item=team_window(f,c),cls=f.cls[L"WD_SelectTeamList_C"];f.prop(cls,L"TextBlock_Color1",L"ObjectProperty",8);f.prop(cls,L"TextBlock_Color2",L"ObjectProperty",8);f.block(item,L"TextBlock_Color1",L"2");auto color2=f.block(item,L"TextBlock_Color2",L"4");auto s=read(f,0);CHECK(s.value.find(L"ULTRON, color 2")!=std::wstring::npos);CHECK(s.value.find(L"X, color 4")!=std::wstring::npos);f.put<std::uint8_t>(color2+0x91,2);CHECK(read(f,0).value.find(L"color 4")==std::wstring::npos);
}
// GF_CharaSelect004 (Training, BP_CharaSelectSingle) return cycle, research/selection-return/report.md:
// StageSelect decide -> CharaSelect_Activate; grid cancel at DecideCnt 0 -> Single DK_CharaSelect_CancelAll
// (Input_Clear) -> flow CharaSelect_OnCancel_Internal -> CharaSelectObj->Deactivate(false) (screens exit,
// TL_FrameOut hides ChrRoot) -> stage again -> StageSelect decide -> CharaSelect_Activate: Simple.Activate ->
// ClearParamByActivate (DecideCnt 0, Screen_DecideMenuReactivate), BP_UIBase Activate (SetState Enter, PreState),
// ActivateExSingle(0): SetFocusChara, Screen_Activate (ChangeChara -> deferred ChangeChara_Internal re-shows
// ChrRoot), Input_Set(0, SelectParent). P2's owner stays null until P1's stone decision.
static void training_return_from_stage_select_reads_player_one_again(){
 Fixture f;Chara c(f);const auto single=f.klass(L"BP_CharaSelectSingle_C",f.cls[L"BP_CharaSelectMain_C"]);f.put(c.root+0x10,single);
 c.input(1,0);SelectionCapture cap(f.unreal);auto first=cap.sample(1000,0);CHECK(first.valid);CHECK(first.label==L"ULTRON");CHECK(cap.rejection().empty());
 auto tick=[&](Address o){return f.raw<Address>(o+f.off(o,L"TickStateObj"));};
 auto state=[&](Address o,int s,bool pre){f.set<std::uint8_t>(tick(o),L"State",static_cast<std::uint8_t>(s));f.flag(tick(o),L"PreState",pre);};
 auto chr=[&](int t,int p){return f.raw<Address>(c.screen[t][p]+f.off(c.screen[t][p],L"ChrRoot"));};
 // Cancel all and deactivate: no input owners, the actor and its screens exit and hide.
 c.input(0,0);CHECK(!cap.sample(1100,0).valid);
 state(c.root,3,true);state(c.sp,3,true);for(int t=0;t<2;++t)for(int p=0;p<2;++p)state(c.screen[t][p],3,true);CHECK(!cap.sample(1200,0).valid);
 state(c.root,0,true);state(c.sp,0,true);for(int t=0;t<2;++t)for(int p=0;p<2;++p){state(c.screen[t][p],0,true);f.flag(chr(t,p),L"bVisible",false);}
 CHECK(!cap.sample(1300,0).valid);CHECK(!cap.rejection().empty());
 // Re-activation: Enter with PreState, then Active; owner and focus restored, DecideCnt cleared.
 state(c.root,1,true);state(c.sp,1,true);f.items<int>(c.root,L"DecideCnt_Arr",{0,0});CHECK(!cap.sample(1400,0).valid);
 state(c.root,2,false);state(c.sp,2,false);for(int p=0;p<2;++p)state(c.screen[0][p],2,false);c.input(0,c.sp);
 // Before the deferred ChangeChara_Internal runs, the drawn screen character is still hidden.
 auto pending=cap.sample(1500,0);CHECK(!pending.valid);CHECK(cap.rejection()==L"grid: screen character hidden");
 f.flag(chr(0,0),L"bVisible",true);auto back=cap.sample(1600,0);CHECK(back.valid);CHECK(back.screen==L"Player 1");CHECK(back.label==L"ULTRON");CHECK(cap.rejection().empty());
 f.set(c.tiles[0],L"FocusTeamNoBit",0);f.items<int>(c.sp,L"FocusUniqId",{1,1});f.set(c.tiles[1],L"FocusTeamNoBit",1);f.set(c.screen[0][0],L"PlayerType",2);CHECK(cap.sample(1700,0).label==L"X");
}
static void rejection_names_the_first_failed_character_select_guard(){
 Fixture f;Chara c(f);SelectionCapture cap(f.unreal);CHECK(cap.sample(1000,0).valid);CHECK(cap.rejection().empty());
 auto tick=[&](Address o){return f.raw<Address>(o+f.off(o,L"TickStateObj"));};
 c.input(0,0);CHECK(!cap.sample(1001,0).valid);CHECK(cap.rejection()==L"chara: no input owner for Player 1");c.input(0,c.sp);
 f.flag(tick(c.screen[0][0]),L"PreState",true);CHECK(!cap.sample(1002,0).valid);CHECK(cap.rejection()==L"grid: screen not ready (tick 2, prestate 1)");f.flag(tick(c.screen[0][0]),L"PreState",false);
 f.set<std::uint8_t>(tick(c.sp),L"State",1);CHECK(!cap.sample(1003,0).valid);CHECK(cap.rejection()==L"chara: select parent not active (tick 1, prestate 0)");f.set<std::uint8_t>(tick(c.sp),L"State",2);
 f.set(c.tiles[0],L"FocusTeamNoBit",2);CHECK(!cap.sample(1004,0).valid);CHECK(cap.rejection()==L"grid: focused tile lacks this side's focus bit (bits 2)");f.set(c.tiles[0],L"FocusTeamNoBit",1);
 f.items<int>(c.root,L"DecideCnt_Arr",{2,0});CHECK(!cap.sample(1005,0).valid);CHECK(cap.rejection()==L"grid: decide count 2 out of range");f.items<int>(c.root,L"DecideCnt_Arr",{0,0});
 f.set(c.screen[0][0],L"TeamNo",1);CHECK(!cap.sample(1006,0).valid);CHECK(cap.rejection()==L"grid: screen belongs to another side");f.set(c.screen[0][0],L"TeamNo",0);
 f.set(c.screen[0][0],L"PlayerType",2);CHECK(!cap.sample(1007,0).valid);CHECK(cap.rejection()==L"grid: screen character differs from focused tile");f.set(c.screen[0][0],L"PlayerType",1);
 auto s=cap.sample(1008,0);CHECK(s.valid);CHECK(cap.rejection().empty());
 // Diagnostics never carry hidden identifiers: a Random screen with an unknown ID still reports only guard names.
 f.flag(c.screen[0][0],L"Random",true);f.set(c.screen[0][0],L"PlayerType",254);CHECK(cap.sample(1009,0).label==L"?????");CHECK(cap.rejection().empty());
 f.items<int>(c.root,L"DecideCnt_Arr",{5,0});cap.sample(1010,0);CHECK(cap.rejection().find(L"254")==std::wstring::npos);
}
static void visible_preview_can_remain_entering_after_the_stage_return(){
 // Live PID 49448: input root/parent Active, P1 preview Enter+PreState=false,
 // both rendered components visible, selected tile and PlayerType both MORRIGAN.
 Fixture f;Chara c(f);f.name(10,L"MORRIGAN");f.set(c.tiles[0],L"CharaSelectData",10);f.set(c.screen[0][0],L"PlayerType",10);
 const auto screen=c.screen[0][0],tick=f.raw<Address>(screen+f.off(screen,L"TickStateObj"));
 f.set<std::uint8_t>(tick,L"State",1);f.flag(tick,L"PreState",false);SelectionCapture cap(f.unreal);
 auto s=cap.sample(1000,0);CHECK(s.valid);CHECK(s.screen==L"Player 1");CHECK(s.label==L"MORRIGAN");CHECK(cap.rejection().empty());
 f.set(c.tiles[0],L"FocusTeamNoBit",0);f.items<int>(c.sp,L"FocusUniqId",{1,1});f.set(c.tiles[1],L"FocusTeamNoBit",1);f.set(screen,L"PlayerType",2);
 s=cap.sample(1010,0);CHECK(s.valid);CHECK(s.label==L"X");CHECK(cap.sample(2010,0).label==L"X"); // No wait for preview Active.
 f.flag(tick,L"PreState",true);CHECK(!cap.sample(2020,0).valid);f.flag(tick,L"PreState",false);
 for(int state:{0,3}){f.set<std::uint8_t>(tick,L"State",static_cast<std::uint8_t>(state));CHECK(!cap.sample(2030,0).valid);}f.set<std::uint8_t>(tick,L"State",1);
 const auto chr=f.raw<Address>(screen+f.off(screen,L"ChrRoot")),mesh=f.raw<Address>(screen+f.off(screen,L"SM_Character"));
 f.flag(chr,L"bVisible",false);CHECK(!cap.sample(2040,0).valid);f.flag(chr,L"bVisible",true);
 f.flag(mesh,L"bVisible",false);CHECK(!cap.sample(2050,0).valid);f.flag(mesh,L"bVisible",true);
 f.set(tick,L"ParentObj",c.root);CHECK(!cap.sample(2060,0).valid);f.set(tick,L"ParentObj",screen);
 const auto root_tick=f.raw<Address>(c.root+f.off(c.root,L"TickStateObj")),parent_tick=f.raw<Address>(c.sp+f.off(c.sp,L"TickStateObj"));
 f.set<std::uint8_t>(root_tick,L"State",1);CHECK(!cap.sample(2070,0).valid);f.set<std::uint8_t>(root_tick,L"State",2);
 f.set<std::uint8_t>(parent_tick,L"State",1);CHECK(!cap.sample(2080,0).valid);f.set<std::uint8_t>(parent_tick,L"State",2);
 f.set(screen,L"PlayerType",1);CHECK(!cap.sample(2090,0).valid);f.set(screen,L"PlayerType",2);
 f.set(c.tiles[1],L"FocusTeamNoBit",0);CHECK(!cap.sample(2100,0).valid);f.set(c.tiles[1],L"FocusTeamNoBit",1);
 f.flag(screen,L"Random",true);f.set(screen,L"PlayerType",254);CHECK(cap.sample(2110,0).label==L"?????");
}
static void selection_rejections_cover_arbitration_before_the_character_grid(){
 {Fixture f;KeyConf k(f,false);SelectionCapture cap(f.unreal);CHECK(cap.sample(1000,0).valid);
  f.set(k.side[0],L"ButtonFocusID",19);CHECK(!cap.sample(1010,0).valid);CHECK(cap.rejection()==L"select: active button configuration unreadable");
  auto second=f.user(f.cls[L"WD_KeyConfMain_C"],L"SecondButtonConfiguration",true);f.tick(second);
  CHECK(!cap.sample(1020,0).valid);CHECK(cap.rejection()==L"select: ambiguous button configuration");}
 {Fixture f;Chara c(f);SelectionCapture cap(f.unreal);CHECK(!cap.sample(1000,2).valid);CHECK(cap.rejection()==L"select: invalid player");
  f.put<int>(f.base+0x35d36f0+0x1c,-1);CHECK(!cap.sample(1010,0).valid);CHECK(cap.rejection()==L"select: object registry unreadable");}
}

static void shared_runtime_discovery_preserves_single_controller_ownership(){
 Fixture f;Chara c(f);SelectionCapture cap(f.unreal);CHECK(f.unreal.prepare_capture(1000));auto one=cap.sample(1000,0);CHECK(one.valid);CHECK(one.label==L"ULTRON");
 auto single=f.klass(L"BP_CharaSelectSingle_C",f.raw<Address>(c.root+0x10));f.put(c.root+0x10,single);c.input(0,0);c.input(1,c.sp);f.unreal.observe_event(c.root);
 CHECK(f.unreal.prepare_capture(1033));auto two=cap.sample(1033,0);CHECK(two.valid);CHECK(two.label==L"X");CHECK(two.screen==L"Player 2");CHECK(f.unreal.discovery_stats().full_passes==1);CHECK(f.unreal.discovery_stats().fallback_scans==0);
}

int main(){int failed=0;for(auto test:{shared_runtime_discovery_preserves_single_controller_ownership,visible_preview_can_remain_entering_after_the_stage_return,selection_rejections_cover_arbitration_before_the_character_grid,training_return_from_stage_select_reads_player_one_again,rejection_names_the_first_failed_character_select_guard,grid_reads_the_live_character_name_for_the_tracked_side,partner_slot_reads_the_sub_screen,single_controller_character_select_follows_the_unique_input_side,random_placeholder_never_resolves_the_hidden_id,unknown_or_mismatched_character_assets_are_rejected,root_state_world_and_visibility_gate_everything,ambiguous_roots_are_not_resolved_by_picking_one,malformed_arrays_are_rejected,costume_and_color_rows_read_rendered_values,costume_install_notice_is_appended_only_when_shown,stone_reads_bounded_english_name_and_rendered_details,stone_rejects_unproven_focus,resident_menu_data_text_is_preferred_for_stone_names,stage_reads_the_rendered_3d_name_and_controlling_side,stage_and_character_roots_together_are_ambiguous,controller_assignment_lists_every_connected_controller_and_side,battle_settings_read_the_focused_row,team_preset_reports_only_caption_and_position,pad_key_config_reads_focused_action_and_glyph_over_character_select,key_config_from_options_reads_waiting_prompt_and_notices,keyboard_config_reads_bound_key_and_bind_prompt,handicap_reads_the_drawn_gauge_level_for_the_tracked_side,team_preset_names_come_from_the_drawn_portraits,new_and_replaced_roots_are_seen_without_waiting_for_a_rescan,tick_state_object_must_be_the_exact_class,non_finite_alpha_hides_3d_widget_text,active_options_popup_yields_to_the_config_reader,grid_install_notice_is_visible_even_for_a_random_placeholder,team_preset_pairs_rendered_colors_with_their_portraits}){try{test();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';++failed;}}std::cout<<checks<<" selection checks, "<<failed<<" failed groups\n";return failed?1:0;}
