#include "mvci/mission.hpp"
#include "collection_fixture.hpp"
#include <iostream>
using namespace mvci;
using mvci_test::Fixture;
static int checks=0;
#define CHECK(x) do{++checks;if(!(x))throw std::runtime_error(std::string(__func__)+":"+std::to_string(__LINE__)+" " #x);}while(false)
// Independent reflected Blueprint layouts. Engine offsets come from the
// existing UE4.15 fixture; field names/types come from verified Mission assets.
struct Mission {
 Fixture& f;Address flow=0,main=0,tick=0;std::array<std::vector<Address>,4> rows;
 const wchar_t* arrays[4]{L"ListCharArray_C",L"ListCharArray_M",L"ListNameArray",L"ListCharArray_T"};
 const wchar_t* focuses[4]{L"IndexFocus_C",L"IndexFocus_M",L"IndexFocus_All",L"IndexFocus_T"};
 const wchar_t* columns[4]{L"IndexFocusX_C",L"IndexFocusX_M",L"IndexFocusX_All",L"IndexFocusX_T"};
 explicit Mission(Fixture& fixture):f(fixture){
  const auto obj=L"ObjectProperty";auto fc=f.klass(L"GF_Mission_C",f.klass(L"GF_FlowBase_C",f.cls[L"Actor"]));
  f.prop(fc,L"MissionMain",obj,8);f.prop(fc,L"IsTravel",L"BoolProperty",1);
  auto mc=f.klass(L"WD_MissionMain_C",f.cls[L"WD_MenuBase_C"]);
  for(auto n:{L"ViewState",L"IndexFocus_C",L"IndexFocus_M",L"IndexFocus_All",L"IndexFocus_T",L"IndexFocusX_C",L"IndexFocusX_M",L"IndexFocusX_All",L"IndexFocusX_T",L"ListMax_C",L"ListMax_M",L"ListMax_T"})f.prop(mc,n,L"IntProperty",4);
  for(auto n:arrays)f.arr(mc,n,obj,8);
  f.arr(mc,L"ArrTutorialStageNum",L"IntProperty",4);
  for(auto n:{L"TextBlock_ViewCaption",L"WD_MissionPopup",L"WD_MissionPopupTTL",L"WD_BtnNavi_176",L"WD_KeyConfAttention_272",L"Image_Cursor_C",L"Image_Cursor_M",L"Image_Cursor_All",L"Image_Cursor_T"})f.prop(mc,n,obj,8);
  for(auto n:{L"isDecide",L"suppressPopup"})f.prop(mc,n,L"BoolProperty",1);
  for(auto n:{L"WD_MissionListChr_C",L"WD_MissionListName_C"}){
   auto rc=f.klass(n,f.cls[L"UserWidget"]);for(auto field:{L"TextBlock_CharName",L"TextBlock_Clear",L"TextBlock_Command",L"MissionMain"})f.prop(rc,field,obj,8);
   f.prop(rc,L"IndexFocus",L"IntProperty",4);
   f.arr(rc,L"ProgressPartsArray",obj,8);
  }
  auto image=f.klass(L"Image",f.cls[L"Widget"]);
  auto brush=f.klass(L"SlateBrush");f.prop_at(brush,L"ResourceObject",obj,0x48,8);
  auto bp=f.prop(image,L"Brush",L"StructProperty",0x88);f.put(bp+0x78,brush);
  f.klass(L"MaterialInstance");auto mid=f.klass(L"MaterialInstanceDynamic",f.cls[L"MaterialInstance"]);f.prop(mid,L"Parent",obj,8);
  auto panel=f.klass(L"PanelWidget",f.cls[L"Widget"]);f.arr(panel,L"Slots",obj,8);f.klass(L"HorizontalBox",panel);
  f.klass(L"Spacer",f.cls[L"Widget"]);
  auto ps=f.klass(L"PanelSlot");f.prop(ps,L"Content",obj,8);f.klass(L"HorizontalBoxSlot",ps);
  for(auto n:{L"WD_MissionPopup_C",L"WD_MissionPopupTTL_C"}){
   auto c=f.klass(n,f.cls[L"UserWidget"]);for(auto field:{L"TextBlock_Number",L"TextBlock_Spec",L"TextBlock_Cleared",L"TextBlock_Try",L"TextBlock_Cancel",L"WD_BtnNavi",L"CanvasPanel_ButtonNavi",L"CanvasPanel_PCText"})f.prop(c,field,obj,8);
   for(int i=0;i<7;++i){f.prop(c,L"HorizontalBox_"+std::to_wstring(i),obj,8);f.prop(c,L"HorizontalBox_"+std::to_wstring(65+i),obj,8);}
  }
  auto notice=f.klass(L"WD_KeyConfAttention_C",f.cls[L"UserWidget"]);f.prop(notice,L"TextBlock_Label",obj,8);
  auto bn=f.klass(L"WD_BtnNavi_C",f.cls[L"UserWidget"]);f.arr(bn,L"TextArray",L"StrProperty",16);
  flow=f.actor(fc,L"GF_Mission_C_0");main=f.user(mc,L"WD_MissionMain_C_0",true);tick=f.tick(main);
  f.set(flow,L"MissionMain",main);f.flag(flow,L"IsTravel",false);f.flag(main,L"isDecide",false);f.flag(main,L"suppressPopup",true);
  for(auto n:{L"ListMax_C",L"ListMax_M"})f.set(main,n,1);f.set(main,L"ListMax_T",7);
  f.items<int>(main,L"ArrTutorialStageNum",{5,2,5,2,3,6,6,1});
  for(auto n:{L"Image_Cursor_C",L"Image_Cursor_M",L"Image_Cursor_All",L"Image_Cursor_T"}){auto c=f.object(n,image);f.slot(c,main);f.set(main,n,c);}
  for(int page=0;page<4;++page){
   for(int i=0;i<(page==2?4:page==3?8:2);++i){
    auto row=f.user(f.cls[page==2?L"WD_MissionListName_C":L"WD_MissionListChr_C"],L"Row",false);f.slot(row,main);f.set(row,L"MissionMain",main);f.set(row,L"IndexFocus",i);
    f.block(row,L"TextBlock_CharName",i?L"X":L"RYU");f.block(row,L"TextBlock_Clear",i?L"0 / 10":L"3 / 10");
    const wchar_t* const categories[]{L"Movement",L"Throws",L"Blocking",L"Active Switch",L"Combos",L"Infinity Surge",L"Infinity Storm",L"Easy Hyper Combo"};
    f.block(row,L"TextBlock_Command",categories[i]);if(page==3)hide(field(row,L"TextBlock_CharName"));else hide(field(row,L"TextBlock_Command"));
    const int stages[]{5,2,5,2,3,6,6,1};std::vector<Address> chips;for(int x=0;x<10;++x){auto chip=f.object(L"Image_P"+std::to_wstring(x),image);f.slot(chip,row);if(page==3&&x>=stages[i])hide(chip);chips.push_back(chip);}f.items<Address>(row,L"ProgressPartsArray",chips);rows[page].push_back(row);
   }
   f.items<Address>(main,arrays[page],rows[page]);f.set(main,focuses[page],0);f.set(main,columns[page],0);
  }
  f.block(main,L"TextBlock_ViewCaption",L"CAPCOM");
 }
 void hide(Address w){f.put<std::uint8_t>(w+0x91,2);}
 void show(Address w){f.put<std::uint8_t>(w+0x91,0);}
 void page(int p){f.set(main,L"ViewState",p);const wchar_t* labels[]{L"CAPCOM",L"MARVEL",L"ALL",L"TUTORIAL"};f.text(field(main,L"TextBlock_ViewCaption")+0x140,labels[p]);}
 Address field(Address a,const wchar_t* n){return f.raw<Address>(a+static_cast<Address>(f.off(a,n)));}
 Address popup(bool tutorial=false){
  const auto n=tutorial?L"WD_MissionPopupTTL":L"WD_MissionPopup",c=tutorial?L"WD_MissionPopupTTL_C":L"WD_MissionPopup_C";
  auto p=f.nested(main,n,f.cls[c],n);f.block(p,L"TextBlock_Number",L"MISSION 1");f.block(p,L"TextBlock_Cleared",L"COMPLETED");hide(field(p,L"TextBlock_Cleared"));f.block(p,L"TextBlock_Spec",L"Perform special moves.");f.flag(main,L"suppressPopup",false);
  return p;
 }
 Address navi(Address owner,const wchar_t* field_name,const std::vector<std::wstring>& texts){
  auto b=f.nested(owner,field_name,f.cls[L"WD_BtnNavi_C"],field_name);auto data=f.alloc(std::max<std::size_t>(1,texts.size())*16);for(std::size_t i=0;i<texts.size();++i)f.str(data+i*16,texts[i]);auto at=b+static_cast<Address>(f.off(b,L"TextArray"));f.put(at,data);f.put<int>(at+8,static_cast<int>(texts.size()));f.put<int>(at+12,static_cast<int>(texts.size()));return b;
 }
 Address line(Address popup,const std::wstring& name,const std::vector<std::pair<std::wstring,bool>>& pieces){
  auto box=f.object(name,f.cls[L"HorizontalBox"]);f.slot(box,popup);f.set(popup,name,box);std::vector<Address> slots;
  for(const auto& p:pieces){const wchar_t* cls=p.first==L"<spacer>"?L"Spacer":p.second?L"Image":L"TextBlock";Address w=f.object(cls,f.cls[cls]);if(p.second){auto material=f.object(p.first,f.cls[L"MaterialInstance"]);f.put(w+f.off(w,L"Brush")+0x48,material);}else if(p.first!=L"<spacer>")f.text(w+0x140,p.first);
   auto slot=f.object(L"HorizontalBoxSlot",f.cls[L"HorizontalBoxSlot"]);f.put(slot+0x28,box);f.set(slot,L"Content",w);f.put(w+0x30,slot);f.put(w+0x20,popup);slots.push_back(slot);
  }f.items<Address>(box,L"Slots",slots);return box;
 }
};
static MenuSnapshot read(Fixture& f){MissionCapture c(f.unreal);return c.sample(1000);}
static void focused_missions_row_reads_rendered_text_on_each_page(){
 Fixture f;Mission m(f);CHECK(read(f).valid);MissionCapture cap(f.unreal);
 for(int page=0;page<4;++page){m.page(page);auto s=cap.sample(1000+page);CHECK(s.valid);CHECK(s.label==(page==3?L"Movement":L"RYU"));CHECK(s.value.find(L"3 / 10")!=std::wstring::npos);CHECK(s.position==1);CHECK(s.count==(page==2?4:page==3?8:2));CHECK(!s.screen.empty());}
 f.set(m.main,L"IndexFocus_T",1);auto s=cap.sample(1005);CHECK(s.valid);CHECK(s.label==L"Throws");CHECK(s.value.find(L"0 / 10")!=std::wstring::npos);CHECK(s.position==2);
}
static void mission_popup_speaks_details_completion_and_confirmation_guide(){
 Fixture f;Mission m(f);auto p=m.popup();m.navi(m.main,L"WD_BtnNavi_176",{L"<_Decide>OK",L"<_Cancel>Back"});auto s=read(f);CHECK(s.valid);CHECK(s.value.find(L"MISSION 1")!=std::wstring::npos);CHECK(s.help.find(L"Perform special moves.")!=std::wstring::npos);CHECK(s.help.find(L"COMPLETED")==std::wstring::npos);
 m.show(m.field(p,L"TextBlock_Cleared"));s=read(f);CHECK(s.value.find(L"COMPLETED")!=std::wstring::npos);
 // OpenPopup(true) overrides the preview-hide flag without clearing it.
 f.flag(m.main,L"isDecide",true);f.flag(m.main,L"suppressPopup",true);f.panel(p,L"CanvasPanel_ButtonNavi");m.navi(p,L"WD_BtnNavi",{L"<_Decide>OK",L"<_Cancel>Cancel"});s=read(f);CHECK(s.valid);CHECK(s.modal);CHECK(s.help.find(L"Cancel: Cancel")!=std::wstring::npos);
 f.flag(m.main,L"suppressPopup",true);m.hide(p);f.flag(m.main,L"isDecide",false);s=read(f);CHECK(s.valid);CHECK(s.help.find(L"Perform special moves")==std::wstring::npos);CHECK(s.value.find(L"Mission 1 of 10")!=std::wstring::npos);
 f.set(m.main,L"IndexFocusX_C",1);auto n=read(f);CHECK(n.valid);CHECK(n.value.find(L"Mission 2 of 10")!=std::wstring::npos);CHECK(menu_fingerprint(n)!=menu_fingerprint(s));
}
static void tutorials_follow_only_drawn_box_text_and_glyph_order(){
 Fixture f;Mission m(f);m.page(3);auto p=m.popup(true);auto pc=f.panel(p,L"CanvasPanel_PCText");m.hide(pc);
 auto pad=m.line(p,L"HorizontalBox_65",{{L"Press",false},{L"<spacer>",false},{L"MI_Btn_A",true},{L"<spacer>",false},{L"to move.",false}});auto keyboard=m.line(p,L"HorizontalBox_0",{{L"Use keyboard.",false}});m.hide(keyboard);
 auto s=read(f);CHECK(s.valid);CHECK(s.label==L"Movement");CHECK(s.help.find(L"Press A to move.")!=std::wstring::npos);CHECK(s.help.find(L"Use keyboard") == std::wstring::npos);
 m.hide(pad);m.show(pc);m.show(keyboard);s=read(f);CHECK(s.valid);CHECK(s.help.find(L"Use keyboard.")!=std::wstring::npos);CHECK(s.help.find(L"Press A") == std::wstring::npos);
 m.show(pad);CHECK(!read(f).valid); // Both conflicting platform sets are drawn.
}
static void ownership_lifecycle_world_and_serial_gates(){
 Fixture f;Mission m(f);CHECK(read(f).valid);
 for(int st:{0,1,3}){f.set<std::uint8_t>(m.tick,L"State",static_cast<std::uint8_t>(st));auto s=read(f);CHECK(!s.valid);CHECK(s.blocked);}f.set<std::uint8_t>(m.tick,L"State",2);
 f.set(m.tick,L"ParentObj",m.flow);CHECK(!read(f).valid);f.set(m.tick,L"ParentObj",m.main);
 m.hide(m.main);CHECK(!read(f).valid);m.show(m.main);f.put(m.main+0x210,Address(0));CHECK(!read(f).valid);
 auto shared=f.alloc(16);f.put<int>(shared+8,1);f.put(m.main+0x210,shared);CHECK(read(f).valid);
 f.flag(m.flow,L"IsTravel",true);CHECK(!read(f).valid);f.flag(m.flow,L"IsTravel",false);
 auto old_world=f.object(L"OldWorld",f.cls[L"World"]),level=f.object(L"OldLevel",f.cls[L"Level"],0x2000,old_world);f.put(level+0xb0,old_world);
 f.put(m.flow+0x20,level);CHECK(!read(f).valid);f.put(m.flow+0x20,f.level);
 auto twin=f.actor(f.raw<Address>(m.flow+0x10),L"GF_Mission_C_1");f.set(twin,L"MissionMain",m.main);f.flag(twin,L"IsTravel",false);CHECK(!read(f).valid);f.put(twin+0x20,level);CHECK(read(f).valid);
 auto subclass=f.klass(L"WD_MissionMain_Update2_C",f.cls[L"WD_MissionMain_C"]);f.put(m.main+0x10,subclass);CHECK(read(f).valid);
 auto entry=f.entries+static_cast<Address>(f.raw<int>(m.main+0xc))*24;f.put<std::uint32_t>(entry+8,0x20000000);CHECK(!read(f).valid);f.put<std::uint32_t>(entry+8,0);
}
static void bounds_hidden_labels_and_selected_chips_are_required(){
 Fixture f;Mission m(f);CHECK(read(f).valid);
 for(int idx:{-1,2,300}){f.set(m.main,L"IndexFocus_C",idx);CHECK(!read(f).valid);}f.set(m.main,L"IndexFocus_C",0);
 for(int col:{-1,10,300}){f.set(m.main,L"IndexFocusX_C",col);CHECK(!read(f).valid);}f.set(m.main,L"IndexFocusX_C",0);
 f.set(m.main,L"ListMax_C",2);CHECK(!read(f).valid);f.set(m.main,L"ListMax_C",1);
 auto row=m.rows[0][0];f.set(row,L"MissionMain",m.flow);CHECK(!read(f).valid);f.set(row,L"MissionMain",m.main);
 m.hide(row);CHECK(!read(f).valid);m.show(row);auto name=m.field(row,L"TextBlock_CharName");m.hide(name);CHECK(!read(f).valid);m.show(name);
 const auto chips=f.raw<Address>(row+static_cast<Address>(f.off(row,L"ProgressPartsArray"))),chip=f.raw<Address>(chips);m.hide(chip);CHECK(!read(f).valid);m.show(chip);
 auto cursor=m.field(m.main,L"Image_Cursor_C");m.hide(cursor);CHECK(!read(f).valid);m.show(cursor);
 const auto at=m.main+static_cast<Address>(f.off(m.main,L"ListCharArray_C")),data=f.raw<Address>(at);f.put(data,Address(0));CHECK(!read(f).valid);f.put(data,row);
 f.put<int>(at+12,1);CHECK(!read(f).valid);f.put<int>(at+12,2);CHECK(read(f).valid);
 m.page(3);f.set(m.main,L"IndexFocus_T",1);f.set(m.main,L"IndexFocusX_T",2);CHECK(!read(f).valid);f.set(m.main,L"IndexFocusX_T",1);CHECK(read(f).valid);
}
static void designer_templates_and_incomplete_details_never_speak(){
 Fixture f;Mission m(f);auto name=m.field(m.rows[0][0],L"TextBlock_CharName"),clear=m.field(m.rows[0][0],L"TextBlock_Clear");
 for(auto text:{L"",L"msg_sys_mis_0000",L"Text Block"}){f.text(name+0x140,text);CHECK(!read(f).valid);}f.text(name+0x140,L"RYU");
 for(auto text:{L"CLEARED X",L"CLEARED 0"}){f.text(clear+0x140,text);CHECK(!read(f).valid);}f.text(clear+0x140,L"Completed: 0/10");
 auto popup=m.popup();f.text(m.field(popup,L"TextBlock_Number")+0x140,L"MISSION X");CHECK(!read(f).valid);f.text(m.field(popup,L"TextBlock_Number")+0x140,L"MISSION 1");
 f.text(m.field(popup,L"TextBlock_Spec")+0x140,L"Mission Data\r\nMission Data\r\nMission Data");CHECK(!read(f).valid);f.text(m.field(popup,L"TextBlock_Spec")+0x140,L"Perform special moves.");CHECK(read(f).valid);
 f.set(m.main,L"IndexFocusX_C",1);CHECK(!read(f).valid); // A visible old popup must match the cursor's mission number.
 m.page(3);f.text(m.field(m.rows[3][0],L"TextBlock_Command")+0x140,L"操作");CHECK(!read(f).valid);
}
static void late_read_mutations_retire_the_complete_snapshot(){
 for(int mutation=0;mutation<7;++mutation){
  Fixture f;Mission m(f);auto popup=m.popup();bool changed=false;const auto spec=m.field(popup,L"TextBlock_Spec");
  MissionCapture cap(f.unreal,[&](Address a)->std::optional<int>{
   if(a==spec&&!changed){changed=true;
    if(mutation==0)f.set(m.main,L"IndexFocus_C",1);
    if(mutation==1)f.set(m.flow,L"MissionMain",Address(0));
    if(mutation==2)f.set(m.main,L"ViewState",1);
    if(mutation==3){auto engine=f.raw<Address>(Fixture::base+0x36bc348),viewport=f.raw<Address>(engine+0x5d8);f.put(viewport+0x80,Address(0));}
    if(mutation==4){auto entry=f.entries+static_cast<Address>(f.raw<int>(m.main+0xc))*24;f.put<int>(entry+0x10,99999);}
    if(mutation==5)f.flag(m.main,L"suppressPopup",true);
    if(mutation==6)m.hide(m.rows[0][0]);
   }return f.unreal.integer(a,L"Visibility");
  });CHECK(!cap.sample(1000).valid);CHECK(changed);
 }
}
static void visible_download_notice_replaces_the_details(){
 Fixture f;Mission m(f);auto popup=m.popup();auto notice=f.nested(m.main,L"WD_KeyConfAttention_272",f.cls[L"WD_KeyConfAttention_C"],L"Notice");f.block(notice,L"TextBlock_Label",L"Content has not finished downloading.");
 m.hide(popup);auto s=read(f);CHECK(s.valid);CHECK(!s.enabled);CHECK(s.help.find(L"Content has not finished downloading.")!=std::wstring::npos);CHECK(s.help.find(L"Perform special moves") == std::wstring::npos);
 m.hide(notice);s=read(f);CHECK(s.valid);CHECK(s.enabled);CHECK(s.help.find(L"Content has not") == std::wstring::npos);
}
static void all_page_boundary_and_f8_use_the_current_owner(){
 Fixture f;Mission m(f);m.page(2);f.text(m.field(m.rows[2][1],L"TextBlock_CharName")+0x140,L"X");f.text(m.field(m.rows[2][2],L"TextBlock_CharName")+0x140,L"THOR");
 f.set(m.main,L"IndexFocus_All",1);auto s=read(f);CHECK(s.valid);CHECK(s.label==L"X");CHECK(s.position==2);CHECK(s.count==4);
 f.set(m.main,L"IndexFocus_All",2);s=read(f);CHECK(s.valid);CHECK(s.label==L"THOR");CHECK(s.position==3);
 Snapshot snapshot;snapshot.foreground=true;snapshot.observed_ms=1000;snapshot.menu=s;Model model;auto r=model.update(snapshot,1000,Request::ReadMenu);CHECK(r.speech);CHECK(r.speech->text.find(L"THOR")!=std::wstring::npos);CHECK(!r.beacon_interval_ms);
}
static void unknown_glyphs_hidden_children_and_line_mutations(){
 Fixture f;Mission m(f);m.page(3);auto popup=m.popup(true);auto panel=f.panel(popup,L"CanvasPanel_PCText");m.hide(panel);
 auto line=m.line(popup,L"HorizontalBox_65",{{L"Press",false},{L"MI_Btn_Unmapped",true},{L"to move.",false}});auto s=read(f);CHECK(s.valid);CHECK(s.help.find(L"Press (button icon) to move.")!=std::wstring::npos);
 auto slots=f.raw<Address>(line+static_cast<Address>(f.off(line,L"Slots"))),slot=f.raw<Address>(slots+8),image=m.field(slot,L"Content");m.hide(image);s=read(f);CHECK(s.valid);CHECK(s.help.find(L"button icon")==std::wstring::npos);m.show(image);
 bool changed=false;MissionCapture cap(f.unreal,[&](Address a)->std::optional<int>{if(a==image&&!changed){changed=true;f.items<Address>(line,L"Slots",{});}return f.unreal.integer(a,L"Visibility");});CHECK(!cap.sample(1000).valid);CHECK(changed);
}
static void cache_reentry_and_pending_menus_suppress_combat(){
 Fixture f;MissionCapture cap(f.unreal);CHECK(!cap.sample(1000).valid);Mission m(f);CHECK(cap.sample(1001).valid);
 auto entry=f.entries+static_cast<Address>(f.raw<int>(m.flow+0xc))*24;f.put<std::uint32_t>(entry+8,0x20000000);CHECK(!cap.sample(1002).valid);
 // A retired object cannot be resurrected in place. Real re-entry creates a
 // fresh flow/root, so the registry change invalidates the cached empty result.
 Mission next(f);CHECK(cap.sample(1003).valid);
 f.set<std::uint8_t>(next.tick,L"State",1);auto s=cap.sample(1004);CHECK(!s.valid);CHECK(s.blocked);CHECK(!cap.rejection().empty());
 Snapshot snapshot;snapshot.foreground=true;snapshot.observed_ms=1004;snapshot.menu=s;snapshot.combat.valid=true;snapshot.combat.playable=true;snapshot.combat.session=7;snapshot.combat.normalized_distance=0.3;Model model;auto result=model.update(snapshot,1004);CHECK(!result.beacon_interval_ms);CHECK(!result.speech);
 f.set<std::uint8_t>(next.tick,L"State",2);CHECK(cap.sample(1005).valid);CHECK(cap.rejection().empty());
}
static void captured_blocking_confirmation_uses_the_global_tutorial_number(){
 // Live PID 54616: Blocking category index 2, local mission index 1, popup
 // MISSION 9 (5 Movement + 2 Throws + the second Blocking mission).
 Fixture f;Mission m(f);m.page(3);f.set(m.main,L"IndexFocus_T",2);f.set(m.main,L"IndexFocusX_T",1);
 auto progress=m.field(m.rows[3][2],L"TextBlock_Clear"),data=f.alloc(0x80),vt=f.alloc(24);f.put(progress+0x140,data);f.put(data,vt);f.put(vt+8,Fixture::base+0x9677c0);f.str(data+0x38,L"Completed: 0/5");
 auto p=m.popup(true);f.text(m.field(p,L"TextBlock_Number")+0x140,L"MISSION 9");f.panel(p,L"CanvasPanel_PCText");
 auto first=m.line(p,L"HorizontalBox_0",{{L"Press",false},{L"<spacer>",false},{L"MI_Btn_Dir",true},{L"<spacer>",false},{L"diagonally down and away from the",false}});
 m.line(p,L"HorizontalBox_1",{{L"opponent as they attack to perform a crouching guard",false}});m.line(p,L"HorizontalBox_2",{{L"and block low attacks.",false}});
 auto slots=f.raw<Address>(first+static_cast<Address>(f.off(first,L"Slots"))),slot=f.raw<Address>(slots+16),image=m.field(slot,L"Content"),material=f.raw<Address>(image+f.off(image,L"Brush")+0x48);
 auto package=f.object(L"/Game/UI/BaseAsset/Platform/PC/MI_Btn_Dir",f.kind(L"Package"));f.put(material+0x20,package);
 f.flag(m.main,L"isDecide",true);f.flag(m.main,L"suppressPopup",true);f.panel(p,L"CanvasPanel_ButtonNavi");m.navi(p,L"WD_BtnNavi",{L"<_Decide>OK",L"<_Cancel>Cancel"});
 auto s=read(f);CHECK(s.valid);CHECK(s.modal);CHECK(s.label==L"Blocking");CHECK(s.value.find(L"MISSION 9")!=std::wstring::npos);CHECK(s.value.find(L"Completed: 0/5")!=std::wstring::npos);CHECK(s.position==3);CHECK(s.count==8);
 CHECK(s.help==L"Press directional controls diagonally down and away from the opponent as they attack to perform a crouching guard and block low attacks. Confirm: OK. Cancel: Cancel");
 f.text(m.field(p,L"TextBlock_Number")+0x140,L"MISSION 2");CHECK(!read(f).valid);
}

static void shared_runtime_discovery_preserves_every_missions_page(){
 Fixture f;Mission m(f);MissionCapture cap(f.unreal);
 for(int page=0;page<4;++page){m.page(page);CHECK(f.unreal.prepare_capture(1000+page*33));auto s=cap.sample(1000+page*33);CHECK(s.valid);CHECK(s.label==(page==3?L"Movement":L"RYU"));}
 CHECK(f.unreal.discovery_stats().full_passes==1);CHECK(f.unreal.discovery_stats().fallback_scans==0);
}

static void active_battle_tutorial_reads_its_own_prompt_and_guide(){
 Fixture f;Mission m(f);
 const auto fc=f.klass(L"GF_FightMission_C",f.cls[L"Actor"]);
 for(const auto name:{L"WD_TutorialWindow",L"MissionHud"})f.prop(fc,name,L"ObjectProperty",8);
 const auto wc=f.klass(L"WD_TutorialWindow_C",f.cls[L"WD_UIParent_C"]);
 for(const auto name:{L"WD_MissionPopupTTL_213",L"WD_BtnNavi_98"})f.prop(wc,name,L"ObjectProperty",8);
 const auto flow=f.actor(fc,L"GF_FightMission_C_0");
 const auto window=f.user(wc,L"WD_TutorialWindow",true);f.set(flow,L"WD_TutorialWindow",window);
 const auto tick=f.tick(window);
 const auto panel=f.nested(window,L"WD_MissionPopupTTL_213",f.cls[L"WD_MissionPopupTTL_C"],L"TutorialInstructions");
 f.block(panel,L"TextBlock_Number",L"MISSION 9");m.hide(f.panel(panel,L"CanvasPanel_PCText"));
 m.line(panel,L"HorizontalBox_65",{{L"Hold",false},{L"MI_Btn_Dir",true},{L"away to block.",false}});
 m.navi(window,L"WD_BtnNavi_98",{L"<_Decide>OK"});
 CHECK(f.unreal.prepare_capture(1000));MissionCapture capture(f.unreal);
 const auto result=capture.battle();CHECK(result.menu.valid&&result.menu.modal);
 CHECK(result.menu.screen==L"Mission tutorial");CHECK(result.menu.label==L"MISSION 9");
 CHECK(result.menu.help==L"Hold (button icon) away to block. Confirm: OK");
 CHECK(!result.objective.valid&&!result.commands.valid);
 f.set<std::uint8_t>(tick,L"State",3);CHECK(capture.battle().menu.blocked);
 f.set<std::uint8_t>(tick,L"State",2);m.hide(window);CHECK(!capture.battle().menu.valid&&!capture.battle().menu.blocked);
 m.show(window);f.set(flow,L"WD_TutorialWindow",Address(0));CHECK(!capture.battle().menu.valid);
 f.set(flow,L"WD_TutorialWindow",window);f.put(flow+0x20,Address(0));CHECK(!capture.battle().menu.valid);
 CHECK(f.unreal.discovery_stats().fallback_scans==0);
}

static void mission_hud_f8_reads_whole_drawn_commands_and_retires_with_the_hud(){
 Fixture f;Mission m(f);
 const auto flow_type=f.klass(L"GF_FightMission_C",f.cls[L"Actor"]);
 for(const auto name:{L"MissionHud",L"WD_TutorialWindow"})f.prop(flow_type,name,L"ObjectProperty",8);
 const auto hud_type=f.klass(L"WD_MissionHUD_C",f.cls[L"WD_UIParent_C"]);
 for(const auto name:{L"CanvasPanel_MissionSpec",L"TextBlock_Number",L"TextBlock_Spec",L"CanvasPanel_PosCondition",L"TextBlock_PosCondition",L"CanvasPanel_ListMain",L"CanvasPanel_List",L"VerticalBox_List"})f.prop(hud_type,name,L"ObjectProperty",8);
 f.prop(hud_type,L"checkIndex",L"IntProperty",4);f.prop(hud_type,L"ProgressOld",L"IntProperty",4);
 const auto command_type=f.klass(L"WD_MissionCmd_C",f.cls[L"UserWidget"]);
 for(const auto name:{L"CanvasPanel_1",L"CanvasPanel_Content",L"TextBlock_CmdText",L"Image_Judge"})f.prop(command_type,name,L"ObjectProperty",8);
 f.put(f.cls[L"CanvasPanel"]+0x30,f.cls[L"PanelWidget"]);
 const auto box_type=f.klass(L"VerticalBox",f.cls[L"PanelWidget"]);
 const auto canvas_slot=f.klass(L"CanvasPanelSlot",f.cls[L"PanelSlot"]);
 const auto box_slot=f.klass(L"VerticalBoxSlot",f.cls[L"PanelSlot"]);
 const auto structure=[&](const wchar_t* name){return f.object(name,f.kind(L"ScriptStruct"),0x100);};
 const auto member=[&](Address type,const wchar_t* name,int offset,int size,Address nested){
  const auto property=f.prop_at(type,name,L"StructProperty",offset,size);f.put(property+0x78,nested);
 };
 const auto vector=structure(L"Vector2D"), margin=structure(L"Margin"), anchors=structure(L"Anchors");
 f.prop_at(vector,L"X",L"FloatProperty",0,4);f.prop_at(vector,L"Y",L"FloatProperty",4,4);
 int offset=0;for(const auto name:{L"Left",L"Top",L"Right",L"Bottom"}){f.prop_at(margin,name,L"FloatProperty",offset,4);offset+=4;}
 member(anchors,L"Minimum",0,8,vector);member(anchors,L"Maximum",8,8,vector);
 const auto data=structure(L"AnchorData");
 member(data,L"Offsets",0,16,margin);member(data,L"Anchors",16,16,anchors);member(data,L"Alignment",32,8,vector);
 member(canvas_slot,L"LayoutData",0x100,40,data);f.prop_at(canvas_slot,L"bAutoSize",L"BoolProperty",0x130,1);
 const auto size=structure(L"SlateChildSize");f.prop_at(size,L"SizeRule",L"ByteProperty",4,1);
 member(box_slot,L"Padding",0x100,16,margin);member(box_slot,L"Size",0x110,8,size);
 const auto linear=structure(L"LinearColor"), color=structure(L"SlateColor");
 f.prop_at(linear,L"A",L"FloatProperty",12,4);member(color,L"SpecifiedColor",0,16,linear);
 f.prop_at(color,L"ColorUseRule",L"ByteProperty",16,1);
 member(f.cls[L"TextBlock"],L"ColorAndOpacity",0x180,20,color);
 member(f.cls[L"Image"],L"ColorAndOpacity",0x300,16,linear);
 const auto link=[&](Address parent,Address child,Address type){
  const auto slot=f.object(L"Slot",type);f.put(child+0x30,slot);f.put(slot+0x28,parent);f.set(slot,L"Content",child);return slot;
 };
 const auto geometry=[&](Address slot,float y,float height,bool autosize=false){
  f.put(slot+0x100,std::array<float,10>{0,y,1720,height,0,0,0,0,0,0});f.flag(slot,L"bAutoSize",autosize);
 };
 const auto flow=f.actor(flow_type,L"GF_FightMission_C_0");
 const auto hud=f.user(hud_type,L"MissionHud",true);f.set(flow,L"MissionHud",hud);f.tick(hud);
 const auto spec=f.panel(hud,L"CanvasPanel_MissionSpec"), condition=f.panel(hud,L"CanvasPanel_PosCondition");
 const auto label=[&](Address owner,Address panel,const wchar_t* field,const std::wstring& value){
  const auto block=f.block(owner,field,value);link(panel,block,canvas_slot);f.put(block+0x18c,1.f);return block;
 };
 label(hud,spec,L"TextBlock_Number",L"MISSION 4");
 const auto objective=label(hud,spec,L"TextBlock_Spec",L"Perform a combo.");
 label(hud,condition,L"TextBlock_PosCondition",L"Start close to the opponent.");
 const auto main=f.panel(hud,L"CanvasPanel_ListMain"), view=f.panel(hud,L"CanvasPanel_List");
 geometry(link(main,view,canvas_slot),12,574);
 const auto list=f.object(L"VerticalBox_List",box_type);f.set(hud,L"VerticalBox_List",list);
 const auto list_slot=link(view,list,canvas_slot);geometry(list_slot,0,0,true);
 std::vector<Address> slots, row_panels;
 for(int i=0;i<12;++i){
  const auto row=f.user(command_type,L"CommandRow",false), panel=f.panel(row,L"CanvasPanel_1");
  row_panels.push_back(panel);
  slots.push_back(link(list,row,box_slot));geometry(link(f.panel_of[row],panel,canvas_slot),0,52);
  const auto caption=label(row,panel,L"TextBlock_CmdText",i==0?L"First command only":i==11?L"Last command only":L"Middle move");
  const auto content=f.panel(row,L"CanvasPanel_Content");link(panel,content,canvas_slot);
  if(i!=1){m.hide(content);continue;}
  m.hide(caption);
  const auto icon=f.object(L"Icon",f.cls[L"Image"]), marker=f.object(L"Image_Judge",f.cls[L"Image"]);
  f.put(icon+0x300,std::array<float,4>{1.f,1.f,1.f,1.f});
  f.set(row,L"Image_Judge",marker);link(panel,marker,canvas_slot);
  f.put(marker+0x300,std::array<float,4>{1.f,0.875823974609375f,0.f,1.f});
  f.items<Address>(content,L"Slots",{link(content,icon,canvas_slot)});
  const auto package=f.object(L"/Game/UI/Menu/CommandList/Icon/mi_CmdIcon_ff71",f.kind(L"Package"));
  const auto glyph=f.object(L"mi_CmdIcon_ff71",f.cls[L"MaterialInstance"],0x2000,package);
  f.put(icon+f.off(icon,L"Brush")+0x48,glyph);
 }
 f.items<Address>(list,L"Slots",slots);
 CHECK(f.unreal.prepare_capture(1000));MissionCapture capture(f.unreal);
 auto result=capture.battle(true,false);CHECK(result.hud_identity&&result.objective.valid&&!result.commands.valid);
 CHECK(result.objective.text==L"MISSION 4. Perform a combo. Start close to the opponent.");
 result=capture.battle();CHECK(result.commands.valid&&result.commands.text.find(L"First command only")!=std::wstring::npos);
 CHECK(result.commands.text.find(L"Current command. quarter circle down to right")!=std::wstring::npos);
 CHECK(result.commands.text.find(L"Last command only")==std::wstring::npos);
 geometry(list_slot,-52,0,true);result=capture.battle();CHECK(result.commands.valid);
 CHECK(result.commands.text.find(L"First command only")==std::wstring::npos);
 CHECK(result.commands.text.find(L"Last command only")!=std::wstring::npos);

 Snapshot snapshot;snapshot.foreground=true;snapshot.observed_ms=1000;snapshot.mission_hud=result.hud_identity;
 snapshot.mission_command_view=result.command_view;
 snapshot.story.valid=true;snapshot.story.captions[2]=result.objective;snapshot.story.subtitle=result.commands;
 Model model;const auto reply=model.update(snapshot,1000,Request::ReadMenu);
 CHECK(reply.speech&&reply.speech->text.find(L"Last command only")!=std::wstring::npos);
 snapshot.story.subtitle={};CHECK(speech_source_fingerprint(snapshot,reply.speech->menu_key)==reply.speech->menu_key);
 f.set<int>(hud,L"checkIndex",1);
 snapshot.mission_command_view=capture.battle(true,false).command_view;
 CHECK(speech_source_fingerprint(snapshot,reply.speech->menu_key)!=reply.speech->menu_key);
 f.set<int>(hud,L"checkIndex",0);
 m.hide(row_panels[0]);snapshot.mission_command_view=capture.battle(true,false).command_view;
 CHECK(speech_source_fingerprint(snapshot,reply.speech->menu_key)!=reply.speech->menu_key);
 m.show(row_panels[0]);
 geometry(list_slot,0,0,true);
 const auto scrolled=capture.battle(true,false);
 CHECK(scrolled.hud_identity==result.hud_identity&&!scrolled.commands.valid);
 snapshot.mission_command_view=scrolled.command_view;
 CHECK(speech_source_fingerprint(snapshot,reply.speech->menu_key)!=reply.speech->menu_key);
 snapshot.menu.blocked=true;CHECK(speech_source_fingerprint(snapshot,reply.speech->menu_key).empty());

 f.put(objective+0x18c,0.f);m.hide(condition);CHECK(!capture.battle().objective.valid);
 f.put(slots.front()+0x104,1.f);CHECK(!capture.battle().commands.valid);
 m.hide(hud);CHECK(!capture.battle().hud_identity);
}

int main(){int failed=0;for(auto test:{shared_runtime_discovery_preserves_every_missions_page,focused_missions_row_reads_rendered_text_on_each_page,mission_popup_speaks_details_completion_and_confirmation_guide,tutorials_follow_only_drawn_box_text_and_glyph_order,ownership_lifecycle_world_and_serial_gates,bounds_hidden_labels_and_selected_chips_are_required,designer_templates_and_incomplete_details_never_speak,late_read_mutations_retire_the_complete_snapshot,visible_download_notice_replaces_the_details,all_page_boundary_and_f8_use_the_current_owner,unknown_glyphs_hidden_children_and_line_mutations,cache_reentry_and_pending_menus_suppress_combat,captured_blocking_confirmation_uses_the_global_tutorial_number,active_battle_tutorial_reads_its_own_prompt_and_guide,mission_hud_f8_reads_whole_drawn_commands_and_retires_with_the_hud}){try{test();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';++failed;}}std::cout<<checks<<" mission checks, "<<failed<<" failed groups\n";return failed?1:0;}
