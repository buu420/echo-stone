#include "mvci/unreal.hpp"
#include "mvci/capture_pacer.hpp"
#include "collection_fixture.hpp"
#include <iostream>
#include <set>
using namespace mvci;
using mvci_test::Fixture;
static int checks=0;
#define CHECK(x) do{++checks;if(!(x))throw std::runtime_error(std::string(__func__)+":"+std::to_string(__LINE__)+" " #x);}while(false)
const wchar_t* const targets[]{L"UserWidget",L"GF_Collection_C",L"GF_Mission_C",L"WD_KeyConfMain_C",L"WD_KeyboardConfMain_C",L"WD_CtrlSelectMain_C",L"BP_CharaSelectSimple_C",L"BP_StageSelectMain_C",L"UIMenuItemDataAsset",L"BP_HUD_Root_C",L"WD_WindowBase_C",L"WD_MenuBase_C",L"WD_MainMenu_C",L"WD_Title_C",L"DemoSystem",L"MoviePlayBase",L"BP_SequencerMovieEnv_C",L"GF_FightStory_C",L"WD_PauseMenu_Story_C",L"WD_Loading_C",L"WD_CommandList_C",L"GF_Thanks_C"};
struct Workload:Fixture {
 Address asset=0,last_asset=0;
 explicit Workload(int count=12000){
  asset=klass(L"TextureAsset");const int id=name_id(L"Asset");const auto storage=alloc(static_cast<std::size_t>(count)*64);
  for(int i=0;i<count;++i){auto a=storage+static_cast<Address>(i)*64;int index=objects++;put(a,base+0x1000);put<int>(a+0xc,index);put(a+0x10,asset);put<int>(a+0x18,id);put(entries+static_cast<Address>(index)*24,a);put<int>(entries+static_cast<Address>(index)*24+0x10,index+100);last_asset=a;}
  put<int>(base+0x35d36f0+0x1c,objects);
 }
 Address title(const std::wstring& label){auto c=klass(L"WD_Title_C",cls[L"UserWidget"]);prop(c,L"TextBlock_213",L"ObjectProperty",8);auto w=user(c,L"Title",true);block(w,L"TextBlock_213",label);return w;}
};
static void one_catalog_serves_every_class_query(){
 Workload f;auto title=f.title(L"PRESS ANY BUTTON");
 for(std::size_t i=1;i<std::size(targets);++i){const bool widget=(i>=3&&i<=5)||(i>=10&&i<=13)||i==18||i==19||i==20;auto base=f.klass(targets[i],f.cls[widget?L"UserWidget":L"Actor"]),sub=f.klass(L"Derived_"+std::to_wstring(i),base);f.object(L"Instance",sub);}
 std::map<std::wstring,std::vector<ObjectRef>> expected;
 for(const auto target:targets)expected[target]=f.unreal.instances(target);
 std::uint64_t asset_reads=0,reads=0;auto raw=f.memory.reader;
 f.memory.reader=[&](Address a,void* out,std::size_t n){++reads;if(a==f.last_asset)++asset_reads;return raw(a,out,n);};
 CHECK(f.unreal.prepare_capture(1000));
 for(const auto target:targets){const auto actual=f.unreal.instances(target);CHECK(actual.size()==expected[target].size());for(std::size_t i=0;i<actual.size();++i)CHECK(actual[i].address==expected[target][i].address);}
 CHECK(asset_reads<=1);CHECK(reads<50000);auto stats=f.unreal.discovery_stats();CHECK(stats.full_passes==1);CHECK(stats.fallback_scans==0);
 MenuCapture menu(f.unreal);CHECK(menu.sample(1000).label==L"PRESS ANY BUTTON");CHECK(f.unreal.alive(*f.unreal.object(title)));
}
static void one_new_asset_does_not_rewalk_the_old_assets(){
 Workload f;f.title(L"Title");CHECK(f.unreal.prepare_capture(1000));
 std::uint64_t old_asset_reads=0,reads=0;auto raw=f.memory.reader;
 f.memory.reader=[&](Address a,void* out,std::size_t n){++reads;if(a==f.last_asset)++old_asset_reads;return raw(a,out,n);};
 f.object(L"NewAsset",f.asset);CHECK(f.unreal.prepare_capture(1033));
 for(const auto target:targets)f.unreal.instances(target);
 CHECK(old_asset_reads==0);CHECK(reads<2000);auto stats=f.unreal.discovery_stats();CHECK(stats.full_passes==1);CHECK(stats.append_passes==1);
}
static void new_title_and_modal_are_discovered_on_the_next_capture(){
 Workload f(10);MenuCapture menu(f.unreal);CHECK(f.unreal.prepare_capture(1000));CHECK(!menu.sample(1000).valid);
 f.title(L"Title prompt");CHECK(f.unreal.prepare_capture(1001));CHECK(menu.sample(1001).label==L"Title prompt");
 auto c=f.klass(L"WD_WindowBase_C",f.cls[L"UserWidget"]);f.prop(c,L"PopupActive",L"BoolProperty",1);f.prop(c,L"MessageText",L"TextProperty",24);f.prop(c,L"TextBlock_MessageMain",L"ObjectProperty",8);
 auto popup=f.user(c,L"Popup",true);f.flag(popup,L"PopupActive",true);f.field_text(popup,L"MessageText",L"Loading dialog");f.block(popup,L"TextBlock_MessageMain",L"Loading dialog");
 CHECK(f.unreal.prepare_capture(1002));auto s=menu.sample(1002);CHECK(s.valid);CHECK(s.modal);CHECK(s.label==L"Loading dialog");
}
static void retained_slot_replacement_and_failure_retire_old_references(){
 Workload f(10);auto one=f.title(L"Old");auto two=f.title(L"New");auto index=f.raw<int>(one+0xc),other=f.raw<int>(two+0xc);
 auto shared=f.raw<Address>(two+0x210);f.put(two+0x210,Address(0));CHECK(f.unreal.prepare_capture(1000));MenuCapture menu(f.unreal);CHECK(menu.sample(1000).label==L"Old");
 f.put(f.entries+static_cast<Address>(index)*24,two);f.put<int>(f.entries+static_cast<Address>(index)*24+0x10,888);f.put(f.entries+static_cast<Address>(other)*24,Address(0));f.put<int>(two+0xc,index);f.put(two+0x210,shared);
 CHECK(f.unreal.prepare_capture(1001));CHECK(menu.sample(1001).label==L"New");
 auto raw=f.memory.reader;bool fail=true;f.memory.reader=[&](Address a,void* out,std::size_t n){if(fail&&a>=f.entries&&a<f.entries+static_cast<Address>(Fixture::capacity)*24&&n>24)return false;return raw(a,out,n);};
 CHECK(!f.unreal.prepare_capture(1600));CHECK(f.unreal.instances(L"UserWidget").empty());fail=false;CHECK(f.unreal.prepare_capture(1601));CHECK(menu.sample(1601).label==L"New");
}
static void event_hint_discovers_a_new_dialog_in_an_existing_empty_slot(){
 Workload f(10);f.title(L"Title");auto c=f.klass(L"WD_WindowBase_C",f.cls[L"UserWidget"]);f.prop(c,L"PopupActive",L"BoolProperty",1);f.prop(c,L"MessageText",L"TextProperty",24);f.prop(c,L"TextBlock_MessageMain",L"ObjectProperty",8);
 auto popup=f.user(c,L"Popup",true);f.flag(popup,L"PopupActive",true);f.field_text(popup,L"MessageText",L"New confirmation");f.block(popup,L"TextBlock_MessageMain",L"New confirmation");
 auto slot=f.entries+static_cast<Address>(f.raw<int>(popup+0xc))*24;f.put(slot,Address(0));
 CHECK(f.unreal.prepare_capture(1000));MenuCapture menu(f.unreal);CHECK(menu.sample(1000).label==L"Title");
 f.put(slot,popup);f.unreal.observe_event(popup);CHECK(f.unreal.prepare_capture(1001));auto s=menu.sample(1001);CHECK(s.valid);CHECK(s.modal);CHECK(s.label==L"New confirmation");CHECK(f.unreal.discovery_stats().full_passes==1);
}
static void slow_captures_leave_a_gap_and_never_catch_up_in_a_burst(){
 CapturePacer pacer;CHECK(pacer.due(100));pacer.completed(160,33);CHECK(!pacer.due(161));CHECK(!pacer.due(192));CHECK(pacer.due(193));
 pacer.completed(500,33);CHECK(!pacer.due(532));CHECK(pacer.due(533));
 pacer.completed(UINT64_MAX-10,33);CHECK(!pacer.due(UINT64_MAX-1));CHECK(pacer.due(UINT64_MAX));
}
static void class_depth_limits_do_not_poison_sibling_queries(){
 Workload f(0);Address within=f.cls[L"UserWidget"];
 for(int i=0;i<63;++i)within=f.klass(L"Deep_"+std::to_wstring(i),within);
 const auto beyond=f.klass(L"BeyondDepth",within);f.object(L"TooDeep",beyond);const auto expected=f.object(L"WithinDepth",within);
 const auto original=f.unreal.instances(L"UserWidget");CHECK(original.size()==1);CHECK(original[0].address==expected);
 CHECK(f.unreal.prepare_capture(1000));const auto indexed=f.unreal.instances(L"UserWidget");CHECK(indexed.size()==1);CHECK(indexed[0].address==expected);
}
static void relocation_shrink_refresh_and_unknown_queries_are_safe(){
 Workload f(10);auto root=f.title(L"Title");CHECK(f.unreal.prepare_capture(1000));const auto old_table=f.entries;
 auto moved=f.alloc(static_cast<std::size_t>(Fixture::capacity)*24);std::memcpy(f.blocks.at(moved).data(),f.blocks.at(old_table).data(),f.blocks.at(old_table).size());f.entries=moved;f.put(Fixture::base+0x35d36f0+0x10,moved);
 CHECK(f.unreal.prepare_capture(1001));CHECK(f.unreal.discovery_stats().full_passes==2);CHECK(f.unreal.instances(L"UserWidget").size()==1);
 f.put<int>(Fixture::base+0x35d36f0+0x1c,f.raw<int>(root+0xc));CHECK(f.unreal.prepare_capture(1002));CHECK(f.unreal.instances(L"UserWidget").empty());
 f.put<int>(Fixture::base+0x35d36f0+0x1c,f.objects);CHECK(f.unreal.prepare_capture(1003));CHECK(f.unreal.instances(L"UserWidget").size()==1);
 CHECK(f.unreal.instances(L"TextureAsset").size()==10);CHECK(f.unreal.discovery_stats().fallback_scans==1);
 const auto full=f.unreal.discovery_stats().full_passes;CHECK(f.unreal.prepare_capture(1600));CHECK(f.unreal.discovery_stats().full_passes==full+1);CHECK(f.unreal.prepare_capture(900));CHECK(f.unreal.discovery_stats().full_passes==full+2);
}
static void one_shot_header_and_class_failures_never_publish_a_partial_catalog(){
 for(int failure=0;failure<2;++failure){
  Workload f(10);f.title(L"Underlying title");auto c=f.klass(L"WD_WindowBase_C",f.cls[L"UserWidget"]);f.prop(c,L"PopupActive",L"BoolProperty",1);f.prop(c,L"MessageText",L"TextProperty",24);f.prop(c,L"TextBlock_MessageMain",L"ObjectProperty",8);
  auto popup=f.user(c,L"Popup",true);f.flag(popup,L"PopupActive",true);f.field_text(popup,L"MessageText",L"Real modal");f.block(popup,L"TextBlock_MessageMain",L"Real modal");
  const auto fault=failure==0?popup:c+0x30;auto raw=f.memory.reader;bool failed=false;
  f.memory.reader=[&](Address a,void* out,std::size_t n){if(!failed&&a==fault){failed=true;return false;}return raw(a,out,n);};
  CHECK(!f.unreal.prepare_capture(1000));CHECK(failed);CHECK(f.unreal.instances(L"UserWidget").empty());CHECK(f.unreal.discovery_stats().failures==1);
  MenuCapture menu(f.unreal);CHECK(!menu.sample(1000).valid);
  f.object(L"LoadingAsset",f.asset);CHECK(f.unreal.prepare_capture(1033));auto s=menu.sample(1033);CHECK(s.valid);CHECK(s.modal);CHECK(s.label==L"Real modal");CHECK(f.unreal.discovery_stats().full_passes==2);
 }
}
static void generic_capture_does_not_read_irrelevant_widgets(){
 Workload f(10);f.title(L"PRESS ANY BUTTON");auto c=f.klass(L"NonMenuHud_C",f.cls[L"UserWidget"]);std::set<Address> irrelevant;
 for(int i=0;i<256;++i)irrelevant.insert(f.user(c,L"Hud",true));CHECK(f.unreal.prepare_capture(1000));
 std::uint64_t reads=0;auto raw=f.memory.reader;f.memory.reader=[&](Address a,void* out,std::size_t n){if(irrelevant.count(a))++reads;return raw(a,out,n);};
 MenuCapture menu(f.unreal);auto s=menu.sample(1000);CHECK(s.valid);CHECK(s.label==L"PRESS ANY BUTTON");CHECK(reads==0);
}
static void overlapping_menu_families_still_produce_one_candidate(){
 Workload f(10);auto window=f.klass(L"WD_WindowBase_C",f.cls[L"UserWidget"]),base=f.klass(L"WD_MenuBase_C",window),title=f.klass(L"WD_Title_C",base);
 f.prop(window,L"PopupActive",L"BoolProperty",1);f.prop(base,L"IsMenuBaseInput",L"BoolProperty",2);f.prop(title,L"TextBlock_213",L"ObjectProperty",8);
 auto w=f.user(title,L"Title",true);f.block(w,L"TextBlock_213",L"One title");MenuCapture menu(f.unreal);CHECK(menu.sample(1000).label==L"One title");
 CHECK(f.unreal.prepare_capture(1033));auto s=menu.sample(1033);CHECK(s.valid);CHECK(s.label==L"One title");
}
static void retained_dialogs_do_not_run_unrelated_adapter_type_walks(){
 Workload f(10);f.title(L"PRESS ANY BUTTON");auto window=f.klass(L"WD_WindowBase_C",f.cls[L"UserWidget"]);f.prop(window,L"PopupActive",L"BoolProperty",1);for(int i=0;i<100;++i)f.user(window,L"RetainedDialog",false);CHECK(f.unreal.prepare_capture(1000));
 // Original shared reader: 19,803 reads. Require at least roughly a 30 percent
 // reduction while preserving actual adapter/lifetime/visibility guards.
 std::uint64_t reads=0;auto raw=f.memory.reader;f.memory.reader=[&](Address a,void* out,std::size_t n){++reads;return raw(a,out,n);};MenuCapture menu(f.unreal);CHECK(menu.sample(1000).label==L"PRESS ANY BUTTON");std::cout<<"retained dialog workload reads="<<reads<<'\n';CHECK(reads<14000);
}
int main(){int failed=0;for(auto test:{retained_dialogs_do_not_run_unrelated_adapter_type_walks,one_catalog_serves_every_class_query,one_new_asset_does_not_rewalk_the_old_assets,new_title_and_modal_are_discovered_on_the_next_capture,retained_slot_replacement_and_failure_retire_old_references,event_hint_discovers_a_new_dialog_in_an_existing_empty_slot,slow_captures_leave_a_gap_and_never_catch_up_in_a_burst,class_depth_limits_do_not_poison_sibling_queries,relocation_shrink_refresh_and_unknown_queries_are_safe,one_shot_header_and_class_failures_never_publish_a_partial_catalog,generic_capture_does_not_read_irrelevant_widgets,overlapping_menu_families_still_produce_one_candidate}){try{test();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';++failed;}}std::cout<<checks<<" discovery checks, "<<failed<<" failed groups\n";return failed?1:0;}
