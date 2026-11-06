#include "collection_fixture.hpp"
#include "mvci/story.hpp"
#include "mvci/descriptions.hpp"
#include <iostream>
#include <cmath>
using namespace mvci;
static int checks=0;
#define CHECK(x) do{++checks;if(!(x))throw std::runtime_error(std::string(__func__)+":"+std::to_string(__LINE__)+" " #x);}while(false)
struct StoryFixture:mvci_test::Fixture {
 Address pause_class=0,guide_class=0;
 StoryFixture(){
  pause_class=klass(L"WD_MoviePause_C",cls[L"WD_MenuBase_C"]);
  prop(pause_class,L"IsMenuBaseInput",L"BoolProperty",1);prop(pause_class,L"TextBlock_55",L"ObjectProperty",8);prop(pause_class,L"WD_BtnNavi",L"ObjectProperty",8);
  guide_class=klass(L"WD_BtnNavi_C",cls[L"UserWidget"]);auto p=prop(guide_class,L"TextArray",L"ArrayProperty",16),inner=object(L"Inner",kind(L"StrProperty"),0x100);put<int>(inner+0x34,16);put(p+0x78,inner);
  auto linear=object(L"LinearColor",kind(L"ScriptStruct"),0x100);prop_at(linear,L"A",L"FloatProperty",12,4);auto slate=object(L"SlateColor",kind(L"ScriptStruct"),0x100);auto specified=prop_at(slate,L"SpecifiedColor",L"StructProperty",0,16);put(specified+0x78,linear);prop_at(slate,L"ColorUseRule",L"ByteProperty",16,1);auto color=prop_at(cls[L"TextBlock"],L"ColorAndOpacity",L"StructProperty",0x1b0,24);put(color+0x78,slate);
 }
 Address block(Address owner,const std::wstring& field,const std::wstring& value,const std::wstring& type=L"TextBlock"){auto b=mvci_test::Fixture::block(owner,field,value,type);put<float>(b+0x1bc,1.f);return b;}
 Address pause(const std::wstring& action=L"Skip"){
  auto w=user(pause_class,L"MoviePause",true);flag(w,L"IsMenuBaseInput",true);block(w,L"TextBlock_55",L"Pause");
  auto nav=user(guide_class,L"Guide",false);slot(nav,w);set(w,L"WD_BtnNavi",nav);auto values=alloc(32);str(values,L"<_SP_R>Resume");str(values+16,L"<_RB>"+action);auto offset=off(nav,L"TextArray");put(nav+offset,values);put<int>(nav+offset+8,2);put<int>(nav+offset+12,2);return w;
 }
 Address title(){auto c=klass(L"WD_Title_C",cls[L"UserWidget"]);prop(c,L"TextBlock_213",L"ObjectProperty",8);auto w=user(c,L"Title",true);block(w,L"TextBlock_213",L"Background title");return w;}
};
struct MovieFixture:StoryFixture {
 Address system=0,sequencer=0,player=0,sequence=0,movie=0,component=0,texture=0;
 MovieFixture(){
  auto pc=klass(L"LevelSequencePlayer");prop_at(pc,L"Sequence",L"ObjectProperty",0x3b8,8);prop_at(pc,L"TimeCursorPosition",L"FloatProperty",0x3c0,4);prop_at(pc,L"StartTime",L"FloatProperty",0x3c4,4);prop_at(pc,L"EndTime",L"FloatProperty",0x3c8,4);
  auto package=object(L"/Game/Demo/Data/0_B/D01/SQ_Story",kind(L"Package"));sequence=object(L"SQ_Story",klass(L"LevelSequence"),0x2000,package);player=object(L"Player",pc);set(player,L"Sequence",sequence);set<float>(player,L"TimeCursorPosition",13.3f);set<float>(player,L"StartTime",0.f);set<float>(player,L"EndTime",81.483337f);
  sequencer=actor(klass(L"LevelSequenceActor",cls[L"Actor"]),L"Sequencer");put(sequencer+0x3c0,player);system=object(L"DemoSystem",klass(L"DemoSystem"));auto actors=alloc(16);put(actors,sequencer);put(system+0xa0,actors);put<int>(system+0xa8,2);put<int>(system+0xac,2);put<std::uint8_t>(system+0xb0,1);
  auto mc=klass(L"MoviePlayBase",cls[L"Actor"]);prop_at(mc,L"mpManaComponent",L"ObjectProperty",0x388,8);prop_at(mc,L"State",L"ByteProperty",0x3b8,1);prop_at(mc,L"ManaTexture",L"ObjectProperty",0x3c0,8);movie=actor(klass(L"BP_MoviePlay_C",mc),L"Movie");
  auto cc=klass(L"ManaComponent");prop_at(cc,L"mManaPlayTimeMS",L"IntProperty",0x1f0,4);component=object(L"ManaComponent",cc,0x2400,movie);texture=object(L"0-B_DEMO01",klass(L"ManaTexture"));set(movie,L"mpManaComponent",component);set(movie,L"ManaTexture",texture);set<std::uint8_t>(movie,L"State",2);put(component+0xf0,texture);put<int>(component+0x1f8,0);
  auto movies=alloc(16);put(movies,movie);put(system+0x90,movies);put<int>(system+0x98,2);put<int>(system+0x9c,2);
 }
 Address environment(){auto c=klass(L"BP_SequencerMovieEnv_C",cls[L"Actor"]);for(auto name:{L"StageTitle_Obj",L"DayText_Obj",L"Subtitle_Obj",L"ViewFade_Obj"})prop(c,name,L"ObjectProperty",8);return actor(c,L"Environment");}
 Address caption(Address environment,const std::wstring& field,const std::wstring& type,const std::wstring& value){
  auto c=klass(type,cls[L"WD_UIParent_C"]);prop(c,L"WD_FreeText_34",L"ObjectProperty",8);auto w=user(c,field,true);tick(w);auto fc=cls[L"WD_FreeText_C"];if(!fc){fc=klass(L"WD_FreeText_C",cls[L"UserWidget"]);prop(fc,L"TextBlock_32",L"ObjectProperty",8);}auto child=nested(w,L"WD_FreeText_34",fc,L"FreeText");block(child,L"TextBlock_32",value);set(environment,field,w);return w;
 }
 Address subtitle(Address environment){
  auto sc=klass(L"WD_StorySubtitle_C",cls[L"WD_UIParent_C"]);prop(sc,L"WD_Subtitling_30",L"ObjectProperty",8);auto root=user(sc,L"Subtitle",true);tick(root);set(environment,L"Subtitle_Obj",root);
  auto c=klass(L"WD_Subtitling_C",cls[L"WD_UIParent_C"]);for(auto field:{L"Text1",L"Text2",L"Text3",L"Text4",L"TextBlock_Left",L"TextBlock_Right"})prop(c,field,L"ObjectProperty",8);
  auto child=nested(root,L"WD_Subtitling_30",c,L"Subtitling");for(auto field:{L"Text1",L"Text2",L"Text3",L"Text4",L"TextBlock_Left",L"TextBlock_Right"}){auto b=block(child,field,L"Retained line");put<std::uint8_t>(b+0x91,2);}return child;
 }
 Address presentation(){
  prop(cls[L"MoviePlayBase"],L"MoviePlayWidget",L"ObjectProperty",8);auto wc=klass(L"WD_MoviePlay_C",cls[L"WD_UIParent_C"]);prop(wc,L"Image_48",L"ObjectProperty",8);prop(wc,L"MID_Image",L"ObjectProperty",8);auto w=user(wc,L"MovieWidget",true);tick(w);set(movie,L"MoviePlayWidget",w);
  auto ic=klass(L"Image",cls[L"Widget"]);auto brush=object(L"SlateBrush",kind(L"ScriptStruct"),0x100);prop_at(brush,L"ResourceObject",L"ObjectProperty",8,8);auto p=prop_at(ic,L"Brush",L"StructProperty",0x140,32);put(p+0x78,brush);auto image=object(L"MovieImage",ic);slot(image,w);set(w,L"Image_48",image);auto mid=object(L"MovieMaterial",klass(L"MaterialInstanceDynamic"));set(w,L"MID_Image",mid);put(image+0x148,mid);set<std::uint8_t>(movie,L"State",3);set<int>(component,L"mManaPlayTimeMS",5000);put<int>(component+0x1f8,150);return image;
 }
};
static void movie_descriptions_require_the_current_drawn_movie_widget(){
 MovieFixture f;auto image=f.presentation();StoryCapture c(f.unreal);CHECK(c.sample(1000).story.movie.presented);f.put<std::uint8_t>(image+0x91,2);CHECK(!c.sample(1001).story.movie.presented);f.put<std::uint8_t>(image+0x91,0);f.put(image+0x148,Address(0));CHECK(!c.sample(1002).story.movie.presented);
}
static void foreground_fade_covers_movie_and_lower_caption_layers(){
 MovieFixture f;auto movie_image=f.presentation();auto e=f.environment();f.caption(e,L"StageTitle_Obj",L"WD_StageTitle_C",L"Xgard");auto c=f.klass(L"WD_DemoViewFade_C",f.cls[L"UserWidget"]);f.prop(c,L"FadeImage",L"ObjectProperty",8);auto w=f.user(c,L"Fade",true);f.set(e,L"ViewFade_Obj",w);auto image=f.object(L"FadeImage",f.cls[L"Image"]);f.slot(image,w);f.set(w,L"FadeImage",image);f.prop_at(f.cls[L"Image"],L"ColorAndOpacity",L"StructProperty",0x180,16);f.put<float>(image+0x18c,1.f);f.put<float>(movie_image+0x18c,1.f);auto sub=f.subtitle(e),line=f.raw<Address>(sub+f.off(sub,L"Text1"));f.put<std::uint8_t>(line+0x91,0);f.text(line+0x140,L"Subtitle above fade");StoryCapture capture(f.unreal);auto s=capture.sample(1000).story;CHECK(!s.movie.presented);CHECK(!s.captions[0].valid);CHECK(s.subtitle.text==L"Subtitle above fade");
 f.put<float>(image+0x18c,0.f);s=capture.sample(1001).story;CHECK(s.movie.presented);CHECK(s.captions[0].text==L"Xgard");
}
struct SurfaceFixture:StoryFixture {
 Address flow=0;
 SurfaceFixture(){auto c=klass(L"GF_FightStory_C",cls[L"Actor"]);for(auto n:{L"Continue_Obj",L"GameOver_Obj",L"OnePoint"})prop(c,n,L"ObjectProperty",8);flow=actor(c,L"FightStory");}
 Address image(Address root,const std::wstring& field,const std::wstring& name,const std::wstring& type){
  auto c=cls[L"Image"];if(!c){c=klass(L"Image",cls[L"Widget"]);auto structure=object(L"SlateBrush",kind(L"ScriptStruct"),0x100);prop_at(structure,L"ResourceObject",L"ObjectProperty",8,8);auto p=prop_at(c,L"Brush",L"StructProperty",0x140,32);put(p+0x78,structure);}
  auto image=object(field,c);slot(image,root);set(root,field,image);auto pkg=object(L"/Game/UI/Battle/Continue/Parts/"+name,kind(L"Package"));auto resource=object(name,klass(type),0x2000,pkg);put(image+0x148,resource);return image;
 }
 Address finish_screen(bool cont){
  auto c=klass(cont?L"WD_Continue_C":L"WD_GameOver_C",cls[L"WD_UIParent_C"]);prop(c,cont?L"Image_Continue":L"Image_GameOver",L"ObjectProperty",8);auto w=user(c,L"Screen",true);tick(w);set(flow,cont?L"Continue_Obj":L"GameOver_Obj",w);
  image(w,cont?L"Image_Continue":L"Image_GameOver",cont?L"M_ContinueText":L"TEX_GameOver",cont?L"Material":L"Texture2D");
  if(cont){for(auto n:{L"TextBlock_YES",L"TextBlock_NO",L"WD_BtnNavi"})prop(c,n,L"ObjectProperty",8);block(w,L"TextBlock_YES",L"Yes");block(w,L"TextBlock_NO",L"No");auto guide=user(guide_class,L"ContinueGuide",false);slot(guide,w);set(w,L"WD_BtnNavi",guide);auto values=alloc(48);str(values,L"<_A>Yes");str(values+16,L"<_B>No");str(values+32,L"<_X>Continue at a lower difficulty");auto offset=off(guide,L"TextArray");put(guide+offset,values);put<int>(guide+offset+8,3);put<int>(guide+offset+12,3);}
  return w;
 }
 Address loading(){auto c=cls[L"WD_Loading_C"];if(!c){c=klass(L"WD_Loading_C",cls[L"UserWidget"]);prop(c,L"TextBlock_Loading",L"ObjectProperty",8);}auto w=user(c,L"Loading",true);block(w,L"TextBlock_Loading",L"Now loading...");return w;}
 Address thanks(){
  auto fc=klass(L"GF_Thanks_C",cls[L"Actor"]);prop(fc,L"Thanks_Obj",L"ObjectProperty",8);auto owner=actor(fc,L"ThanksFlow");
  auto c=klass(L"WD_Thanks_C",cls[L"WD_UIParent_C"]);prop(c,L"Image_Text",L"ObjectProperty",8);auto w=user(c,L"Thanks",true);tick(w);set(owner,L"Thanks_Obj",w);
  auto img=image(w,L"Image_Text",L"tex_thanks_text",L"Texture2D");auto resource=raw<Address>(img+0x148),pkg=raw<Address>(resource+0x20);put<int>(pkg+0x18,name_id(L"/Game/UI/Story/Parts/tex_thanks_text"));
  prop_at(cls[L"Image"],L"ColorAndOpacity",L"StructProperty",0x180,16);put<float>(img+0x18c,1.f);return w;
 }
 Address document(){auto c=klass(L"WD_PauseMenu_Story_C",cls[L"WD_MenuBase_C"]);prop(c,L"IsMenuBaseInput",L"BoolProperty",1);prop(c,L"SelectListNo",L"IntProperty",4);prop(c,L"WD_WindowControl_New_327",L"ObjectProperty",8);auto w=user(c,L"StoryPause",true);tick(w);flag(w,L"IsMenuBaseInput",true);set<int>(w,L"SelectListNo",0);auto dc=klass(L"WD_WindowControl_New_C",cls[L"UserWidget"]);for(auto n:{L"TextBlock_Title_V",L"TextBlock_Text_V",L"TextBlock_Ability_Title",L"TextBlock_ChrName"})prop(dc,n,L"ObjectProperty",8);auto d=nested(w,L"WD_WindowControl_New_327",dc,L"Document");block(d,L"TextBlock_Title_V",L"Win Conditions");block(d,L"TextBlock_Text_V",L"Defeat all enemies.");block(d,L"TextBlock_Ability_Title",L"Character Conditions");block(d,L"TextBlock_ChrName",L"Ryu");return w;}
 Address advice(){
  auto panel=klass(L"PanelWidget",cls[L"Widget"]);arr(panel,L"Slots",L"ObjectProperty",8);auto hc=klass(L"HorizontalBox",panel),sc=klass(L"HorizontalBoxSlot");prop_at(sc,L"Content",L"ObjectProperty",0x30,8);
  auto c=klass(L"WD_OnePoint_C",cls[L"WD_MenuBase_C"]);prop(c,L"LineNum",L"IntProperty",4);arr(c,L"LineBoxArray",L"ObjectProperty",8);arr(c,L"LineBoxArrayPC",L"ObjectProperty",8);auto w=user(c,L"Advice",true);tick(w);set(flow,L"OnePoint",w);set<int>(w,L"LineNum",1);
  for(auto field:{L"LineBoxArray",L"LineBoxArrayPC"}){auto box=object(field,hc);slot(box,w);auto block=object(L"LineText",cls[L"TextBlock"]);text(block+0x140,field==std::wstring(L"LineBoxArray")?L"Visible advice":L"Hidden PC advice");auto s=object(L"LineSlot",sc);put(s+0x28,box);put(s+0x30,block);put(block+0x30,s);put(block+0x20,tree_of[w]);items<Address>(box,L"Slots",{s});items<Address>(w,field,{box});if(field==std::wstring(L"LineBoxArrayPC"))put<std::uint8_t>(box+0x91,2);}
  return w;
 }
};
static void story_advice_selects_the_drawn_platform_and_rejects_ambiguity(){
 SurfaceFixture f;auto w=f.advice();StoryCapture c(f.unreal);auto s=c.sample(1000);CHECK(s.story.captions[2].text==L"Visible advice");CHECK(!s.menu.valid&&!s.menu.blocked);auto pc=f.raw<Address>(f.raw<Address>(w+f.off(w,L"LineBoxArrayPC")));f.put<std::uint8_t>(pc+0x91,0);CHECK(!c.sample(1001).story.captions[2].valid);f.put<std::uint8_t>(pc+0x91,2);
 auto box=f.raw<Address>(f.raw<Address>(w+f.off(w,L"LineBoxArray"))),slot=f.raw<Address>(f.raw<Address>(box+f.off(box,L"Slots"))),b=f.raw<Address>(slot+0x30);auto raw=f.memory.reader;bool fired=false;f.memory.reader=[&](Address at,void* out,std::size_t n){auto ok=raw(at,out,n);if(!fired&&at==b+0x140){fired=true;f.set(f.flow,L"OnePoint",Address(0));}return ok;};CHECK(!c.sample(1002).story.captions[2].valid);CHECK(fired);
}
static void story_caption_late_text_replacement_is_not_narrated(){
 MovieFixture f;auto e=f.environment(),w=f.caption(e,L"StageTitle_Obj",L"WD_StageTitle_C",L"Xgard"),child=f.raw<Address>(w+f.off(w,L"WD_FreeText_34")),b=f.raw<Address>(child+f.off(child,L"TextBlock_32"));auto raw=f.memory.reader;bool fired=false;
 f.memory.reader=[&](Address at,void* out,std::size_t n){auto ok=raw(at,out,n);if(!fired&&at==b+0x140){fired=true;f.text(b+0x140,L"Replaced during capture");}return ok;};StoryCapture c(f.unreal);CHECK(!c.sample(1000).story.captions[0].valid);CHECK(fired);
}
static void faded_caption_text_is_not_visible_information(){
 MovieFixture f;auto e=f.environment(),w=f.caption(e,L"StageTitle_Obj",L"WD_StageTitle_C",L"Doctor?"),child=f.raw<Address>(w+f.off(w,L"WD_FreeText_34")),b=f.raw<Address>(child+f.off(child,L"TextBlock_32"));StoryCapture c(f.unreal);
 f.put<float>(b+0x1bc,0.f);CHECK(!c.sample(1000).story.captions[0].valid);f.put<float>(b+0x1bc,.4f);CHECK(c.sample(1001).story.captions[0].text==L"Doctor?");f.put<std::uint8_t>(b+0x1c0,1);CHECK(!c.sample(1002).story.captions[0].valid);f.put<std::uint8_t>(b+0x1c0,0);
 auto raw=f.memory.reader;bool fired=false;f.memory.reader=[&](Address at,void* out,std::size_t n){auto ok=raw(at,out,n);if(!fired&&at==b+0x1bc){fired=true;f.put<float>(b+0x1bc,0.f);}return ok;};CHECK(!c.sample(1003).story.captions[0].valid);CHECK(fired);
}
static void faded_subtitle_blocks_are_skipped_without_hiding_a_drawn_sibling(){
 MovieFixture f;auto e=f.environment(),sub=f.subtitle(e),left=f.raw<Address>(sub+f.off(sub,L"TextBlock_Left")),right=f.raw<Address>(sub+f.off(sub,L"TextBlock_Right"));f.put<std::uint8_t>(left+0x91,0);f.put<std::uint8_t>(right+0x91,0);f.text(left+0x140,L"Faded old dialogue");f.text(right+0x140,L"Current dialogue");f.put<float>(left+0x1bc,0.f);StoryCapture c(f.unreal);CHECK(c.sample(1000).story.subtitle.text==L"Current dialogue");f.put<float>(right+0x1bc,0.f);CHECK(!c.sample(1001).story.subtitle.valid);
}
static void story_loading_and_game_over_are_informational_and_visible(){
 SurfaceFixture f;auto loading=f.loading();StoryCapture c(f.unreal);auto s=c.sample(1000);CHECK(s.story.captions[3].text==L"Now loading...");CHECK(!s.menu.valid);f.put<std::uint8_t>(loading+0x91,2);CHECK(!c.sample(1001).story.captions[3].valid);
 auto w=f.finish_screen(false);s=c.sample(1002);CHECK(s.story.captions[4].text==L"Game over");CHECK(!s.menu.valid);auto img=f.raw<Address>(w+f.off(w,L"Image_GameOver"));f.put<std::uint8_t>(img+0x91,2);CHECK(!c.sample(1003).story.captions[4].valid);
}
static void loading_end_is_explicit_native_hidden_state_not_an_empty_observation(){
 SurfaceFixture f;auto w=f.loading(),b=f.raw<Address>(w+f.off(w,L"TextBlock_Loading"));StoryCapture c(f.unreal);auto s=c.sample(1000).story;CHECK(s.loading.known&&s.loading.visible&&s.loading.identity);const auto owner=s.loading.identity;
 f.put<std::uint8_t>(b+0x91,2);s=c.sample(1001).story;CHECK(s.loading.known&&!s.loading.visible&&s.loading.identity==owner);CHECK(!s.captions[3].valid);
 f.put<std::uint8_t>(b+0x91,0);f.set(w,L"TextBlock_Loading",Address(0));CHECK(!c.sample(1002).story.loading.known);
 f.set(w,L"TextBlock_Loading",b);f.put(w+0x210,Address(0));s=c.sample(1003).story;CHECK(s.loading.known&&!s.loading.visible);
 auto raw=f.memory.reader;f.memory.reader=[&](Address at,void* out,std::size_t n){return at!=w+0x210&&raw(at,out,n);};CHECK(!c.sample(1004).story.loading.known);
}
static Snapshot loading_observation(StoryCapture& capture,std::uint64_t now,bool replacement=false){
 Snapshot out;out.foreground=true;out.observed_ms=now;out.story=capture.sample(now).story;
 if(replacement){out.menu.valid=true;out.menu.identity=123;out.menu.screen=L"Title";out.menu.label=L"Press any button";}
 return out;
}
static void drawn_loading_with_unreadable_or_changing_text_does_not_rearm(){
 for(int change=0;change<4;++change){
  SurfaceFixture f;const auto w=f.loading(),b=f.raw<Address>(w+f.off(w,L"TextBlock_Loading"));StoryCapture c(f.unreal);Model model;
  auto out=loading_observation(c,1000);const auto owner=out.story.loading.identity;
  auto speech=model.update(out,1000).speech;CHECK(speech&&speech->priority==Priority::Story);CHECK(model.acknowledge(speech->token,true,1000));
  const auto raw=f.memory.reader;bool fired=false;int alpha_reads=0;
  if(change==0)f.text(b+0x140,L"msg_unresolved");
  if(change==1)f.put(b+0x140,Address(0));
  if(change==2)f.memory.reader=[&](Address at,void* data,std::size_t n){const auto ok=raw(at,data,n);if(!fired&&at==b+0x140){fired=true;f.put(b+0x140,Address(0));}return ok;};
  if(change==3)f.memory.reader=[&](Address at,void* data,std::size_t n){const auto ok=raw(at,data,n);if(at==b+0x1bc&&++alpha_reads==2){fired=true;f.put(b+0x140,Address(0));}return ok;};
  out=loading_observation(c,1033,true);
  CHECK(out.story.loading.known&&out.story.loading.visible&&out.story.loading.identity==owner);CHECK(!out.story.captions[3].valid);
  CHECK(change<2||fired);speech=model.update(out,1033).speech;CHECK(speech&&speech->priority==Priority::Menu);CHECK(model.acknowledge(speech->token,true,1033));
  f.memory.reader=raw;f.text(b+0x140,L"Now loading...");
  out=loading_observation(c,1066);CHECK(out.story.captions[3].valid);CHECK(!model.update(out,1066).speech);
  f.put<std::uint8_t>(b+0x91,2);out=loading_observation(c,1099,true);speech=model.update(out,1099).speech;CHECK(speech&&speech->priority==Priority::Menu);CHECK(model.acknowledge(speech->token,true,1099));
  f.put<std::uint8_t>(b+0x91,0);speech=model.update(loading_observation(c,1132),1132).speech;CHECK(speech&&speech->priority==Priority::Story);
 }
}
static void two_drawn_loading_owners_block_rearm_without_exposing_ambiguous_text(){
 SurfaceFixture f;const auto w=f.loading(),other=f.loading();const auto b=f.raw<Address>(other+f.off(other,L"TextBlock_Loading"));
 f.put<std::uint8_t>(b+0x91,2);StoryCapture c(f.unreal);Model model;
 auto out=loading_observation(c,1000);CHECK(out.story.loading.known&&out.story.loading.visible&&out.story.loading.identity);
 auto speech=model.update(out,1000).speech;CHECK(speech&&speech->priority==Priority::Story);CHECK(model.acknowledge(speech->token,true,1000));
 f.put<std::uint8_t>(b+0x91,0);out=loading_observation(c,1033,true);
 CHECK(out.story.loading.known&&out.story.loading.visible&&!out.story.loading.identity);CHECK(!out.story.captions[3].valid);
 f.text(b+0x140,L"msg_unresolved");out=loading_observation(c,1066,true);
 CHECK(out.story.loading.known&&out.story.loading.visible&&!out.story.loading.identity);CHECK(!out.story.captions[3].valid);
 speech=model.update(out,1066).speech;CHECK(speech&&speech->priority==Priority::Menu);CHECK(model.acknowledge(speech->token,true,1066));
 f.put<std::uint8_t>(other+0x91,2);out=loading_observation(c,1099);
 CHECK(out.story.loading.known&&out.story.loading.visible&&out.story.captions[3].text==L"Now loading...");
 CHECK(!model.update(out,1099).speech);CHECK(w!=other);
}
static void story_continue_uses_drawn_asset_and_direct_actions(){
 SurfaceFixture f;auto w=f.finish_screen(true);StoryCapture c(f.unreal);auto s=c.sample(1000).menu;CHECK(s.valid&&s.modal);CHECK(s.label==L"Continue");CHECK(s.help.find(L"lower difficulty")!=std::wstring::npos);CHECK(s.position==0);auto img=f.raw<Address>(w+f.off(w,L"Image_Continue"));f.put(img+0x148,Address(0));s=c.sample(1001).menu;CHECK(!s.valid&&s.blocked);f.put(f.flow+0x20,Address(0));CHECK(!c.sample(1002).menu.valid);
}
static void thanks_reads_only_the_owned_drawn_texture(){
 SurfaceFixture f;auto w=f.thanks(),img=f.raw<Address>(w+f.off(w,L"Image_Text"));StoryCapture c(f.unreal);auto s=c.sample(1000);CHECK(s.story.captions[4].text==L"Thank you for playing.");CHECK(!s.menu.valid&&!s.menu.blocked);
 f.put<float>(img+0x18c,0.f);CHECK(!c.sample(1001).story.captions[4].valid);f.put<float>(img+0x18c,1.f);f.put<std::uint8_t>(img+0x91,2);CHECK(!c.sample(1002).story.captions[4].valid);f.put<std::uint8_t>(img+0x91,0);
 auto tick=f.raw<Address>(w+f.off(w,L"TickStateObj"));f.set<std::uint8_t>(tick,L"State",1);CHECK(!c.sample(1003).story.captions[4].valid);f.set<std::uint8_t>(tick,L"State",2);
 auto resource=f.raw<Address>(img+0x148),pkg=f.raw<Address>(resource+0x20);f.put<int>(pkg+0x18,f.name_id(L"/Game/Unrelated/UI/Story/Parts/tex_thanks_text"));CHECK(!c.sample(1004).story.captions[4].valid);f.put<int>(pkg+0x18,f.name_id(L"/Game/UI/Story/Parts/tex_thanks_text"));
 auto owner=f.unreal.instances(L"GF_Thanks_C")[0].address;f.put(owner+0x20,Address(0));CHECK(!c.sample(1005).story.captions[4].valid);
}
static void thanks_rejects_duplicate_and_late_replaced_owners(){
 SurfaceFixture f;auto w=f.thanks(),img=f.raw<Address>(w+f.off(w,L"Image_Text")),owner=f.unreal.instances(L"GF_Thanks_C")[0].address;StoryCapture c(f.unreal);auto raw=f.memory.reader;bool fired=false;
 f.memory.reader=[&](Address a,void* out,std::size_t n){auto ok=raw(a,out,n);if(!fired&&a==img+0x148){fired=true;f.set(owner,L"Thanks_Obj",Address(0));}return ok;};CHECK(!c.sample(1000).story.captions[4].valid);CHECK(fired);
 SurfaceFixture ambiguous;ambiguous.thanks();ambiguous.thanks();StoryCapture ac(ambiguous.unreal);CHECK(!ac.sample(1001).story.captions[4].valid);
}
static void story_pause_document_reads_shown_conditions_and_yields_to_list(){
 SurfaceFixture f;auto w=f.document();StoryCapture c(f.unreal);auto s=c.sample(1000).menu;CHECK(s.valid);CHECK(s.label.find(L"Win Conditions")!=std::wstring::npos);CHECK(s.label.find(L"Defeat all enemies")!=std::wstring::npos);f.set<int>(w,L"SelectListNo",1);CHECK(!c.sample(1001).menu.valid);f.set<int>(w,L"SelectListNo",0);f.flag(w,L"IsMenuBaseInput",false);CHECK(!c.sample(1002).menu.valid);
}
static void subtitles_copy_only_rendered_text_and_reject_late_change(){
 MovieFixture f;auto e=f.environment(),sub=f.subtitle(e);auto left=f.raw<Address>(sub+f.off(sub,L"TextBlock_Left")),right=f.raw<Address>(sub+f.off(sub,L"TextBlock_Right"));f.put<std::uint8_t>(left+0x91,0);f.text(left+0x140,L"Visible left");f.put<std::uint8_t>(right+0x91,0);f.text(right+0x140,L"Visible right");StoryCapture capture(f.unreal);auto s=capture.sample(1000).story;
 CHECK(s.subtitle.valid);CHECK(s.subtitle.text==L"Visible left Visible right");f.text(left+0x140,L"");CHECK(capture.sample(1000).story.subtitle.text==L"Visible right");f.text(left+0x140,L"Visible left");f.put<std::uint8_t>(sub+0x91,2);CHECK(!capture.sample(1001).story.subtitle.valid);f.put<std::uint8_t>(sub+0x91,0);
 auto raw=f.memory.reader;bool fired=false;f.memory.reader=[&](Address at,void* out,std::size_t n){auto ok=raw(at,out,n);if(!fired&&at==left+0x140){fired=true;f.text(left+0x140,L"Changed during capture");}return ok;};CHECK(!capture.sample(1002).story.subtitle.valid);CHECK(fired);
}
static void story_captions_follow_the_current_environment_and_rendered_children(){
 MovieFixture f;auto e=f.environment(),stage=f.caption(e,L"StageTitle_Obj",L"WD_StageTitle_C",L"XGARD");f.caption(e,L"DayText_Obj",L"WD_DayText_C",L"88 DAYS SINCE THE CONVERGENCE");StoryCapture capture(f.unreal);auto s=capture.sample(1000).story;
 CHECK(s.captions[0].valid);CHECK(s.captions[0].text==L"XGARD");CHECK(s.captions[1].valid);CHECK(s.captions[1].text==L"88 DAYS SINCE THE CONVERGENCE");CHECK(s.captions[0].identity!=s.captions[1].identity);
 const auto child=f.raw<Address>(stage+f.off(stage,L"WD_FreeText_34")),b=f.raw<Address>(child+f.off(child,L"TextBlock_32"));f.put<std::uint8_t>(b+0x91,2);CHECK(!capture.sample(1001).story.captions[0].valid);f.put<std::uint8_t>(b+0x91,0);f.text(b+0x140,L"msg_designer_placeholder");CHECK(!capture.sample(1002).story.captions[0].valid);f.text(b+0x140,L"XGARD");
 const auto tick=f.raw<Address>(stage+f.off(stage,L"TickStateObj"));f.set<std::uint8_t>(tick,L"State",3);CHECK(!capture.sample(1003).story.captions[0].valid);f.set<std::uint8_t>(tick,L"State",2);f.put(e+0x20,Address(0));CHECK(!capture.sample(1004).story.captions[0].valid);
}
static void ambiguous_or_replaced_caption_owners_are_not_narrated(){
 MovieFixture f;auto e=f.environment(),a=f.caption(e,L"StageTitle_Obj",L"WD_StageTitle_C",L"XGARD");auto other=f.environment();f.caption(other,L"StageTitle_Obj",L"WD_StageTitle_C",L"Different title");StoryCapture capture(f.unreal);CHECK(!capture.sample(1000).story.captions[0].valid);f.put(other+0x20,Address(0));CHECK(capture.sample(1001).story.captions[0].text==L"XGARD");
 auto child=f.raw<Address>(a+f.off(a,L"WD_FreeText_34")),b=f.raw<Address>(child+f.off(child,L"TextBlock_32"));auto raw=f.memory.reader;bool fired=false;
 f.memory.reader=[&](Address at,void* out,std::size_t n){auto ok=raw(at,out,n);if(!fired&&at==b+0x140){fired=true;f.set(e,L"StageTitle_Obj",Address(0));}return ok;};CHECK(!capture.sample(1002).story.captions[0].valid);CHECK(fired);
}
static void sequence_clock_distinguishes_preroll_pause_and_movie_playback(){
 MovieFixture f;StoryCapture capture(f.unreal);auto s=capture.sample(1000).story;CHECK(s.valid);CHECK(s.movie.valid);CHECK(s.movie.id==L"0-B_DEMO01");CHECK(s.movie.paused);CHECK(!s.movie.playing);CHECK(s.movie.sequence_seconds&&std::abs(*s.movie.sequence_seconds-13.3)<.001);CHECK(!s.movie.frame);CHECK(!s.movie.movie_seconds);
 f.put<std::uint8_t>(f.system+0xb0,0);f.set<float>(f.player,L"TimeCursorPosition",49.7f);f.set<std::uint8_t>(f.movie,L"State",3);f.set<int>(f.component,L"mManaPlayTimeMS",5000);f.put<int>(f.component+0x1f8,150);s=capture.sample(1033).story;CHECK(s.movie.valid&&s.movie.playing&&!s.movie.paused);CHECK(s.movie.frame==150);CHECK(s.movie.movie_seconds==5.0);
 f.put<std::uint8_t>(f.system+0xb0,1);s=capture.sample(1066).story;CHECK(s.movie.paused);CHECK(s.movie.frame==150);
}
static void movie_soundtrack_time_uses_the_owned_native_sync_origin(){
 MovieFixture f;
 StoryCapture capture(f.unreal);
 f.put<int>(f.movie+0x390,50033);
 f.put<int>(f.component+0x1fc,44700);
 CHECK(!capture.sample(1000).story.movie.soundtrack);
 f.presentation();
 const auto active=capture.sample(1033).story.movie;
 CHECK(active.valid&&active.presented&&active.movie_seconds==5.0);
 CHECK(active.soundtrack&&active.soundtrack->origin_ms==44700&&active.soundtrack->elapsed_ms==5333);
 f.put<int>(f.movie+0x390,-1);
 CHECK(!capture.sample(1066).story.movie.soundtrack);
 f.put<int>(f.movie+0x390,44000);
 CHECK(!capture.sample(1100).story.movie.soundtrack);
}
static void clocks_reject_wrong_world_unbound_texture_and_late_replacement(){
 MovieFixture f;StoryCapture capture(f.unreal);f.put(f.sequencer+0x20,Address(0));CHECK(!capture.sample(1000).story.valid);f.put(f.sequencer+0x20,f.level);CHECK(capture.sample(1001).story.valid);
 auto wrong=f.object(L"OtherMovie",f.cls[L"ManaTexture"]);f.set(f.movie,L"ManaTexture",wrong);CHECK(!capture.sample(1002).story.movie.valid);f.set(f.movie,L"ManaTexture",f.texture);
 auto raw=f.memory.reader;bool fired=false;f.memory.reader=[&](Address a,void* out,std::size_t n){auto ok=raw(a,out,n);if(!fired&&a==f.player+0x3c0){fired=true;f.put(f.sequencer+0x3c0,Address(0));}return ok;};CHECK(!capture.sample(1003).story.valid);CHECK(fired);
}
static void a_replaced_sequence_player_is_a_new_description_owner(){
 MovieFixture f;StoryCapture c(f.unreal);const auto before=c.sample(1000).story.movie.context;auto player=f.object(L"NewPlayer",f.cls[L"LevelSequencePlayer"]);f.set(player,L"Sequence",f.sequence);f.set<float>(player,L"TimeCursorPosition",13.3f);f.set<float>(player,L"StartTime",0.f);f.set<float>(player,L"EndTime",81.483337f);f.put(f.sequencer+0x3c0,player);const auto after=c.sample(1001).story;CHECK(after.valid);CHECK(after.movie.context!=before);
}
static void a_replaced_movie_actor_component_or_texture_is_a_new_description_owner(){
 struct Output:DescriptionOutput {bool busy=false;int stops=0;bool playing()const override{return busy;}bool start(const DescriptionCue&,double)override{busy=true;return true;}void pause()override{}void resume()override{}void stop()override{busy=false;++stops;}};
 const DescriptionMovie film{L"0-B_DEMO01",37,44.7,{{"cue","cue.wav","",L"Current movie",5,10,1,48044}}};
 for(int change=0;change<3;++change){MovieFixture f;f.presentation();f.set<float>(f.player,L"TimeCursorPosition",49.7f);f.put<std::uint8_t>(f.system+0xb0,0);StoryCapture c(f.unreal);
  const auto snapshot=[&](std::uint64_t now){Snapshot s;s.foreground=true;s.observed_ms=now;s.story=c.sample(now).story;return s;};
  Output output;DescriptionSession session(output);const auto before=snapshot(1000);CHECK(before.story.movie.valid&&before.story.movie.presented);session.observe(before,&film,1000,true);f.set<int>(f.component,L"mManaPlayTimeMS",5033);f.set<float>(f.player,L"TimeCursorPosition",49.733f);session.observe(snapshot(1033),&film,1033,true);CHECK(output.busy);const auto stops=output.stops;
  if(change==0){auto movie=f.actor(f.cls[L"BP_MoviePlay_C"],L"ReplacementMovie");f.set(movie,L"mpManaComponent",f.component);f.set(movie,L"ManaTexture",f.texture);f.set(movie,L"MoviePlayWidget",f.raw<Address>(f.movie+f.off(f.movie,L"MoviePlayWidget")));f.set<std::uint8_t>(movie,L"State",3);f.put(f.component+0x20,movie);auto movies=f.raw<Address>(f.system+0x90);f.put(movies,movie);}
  else if(change==1){auto component=f.object(L"ReplacementComponent",f.cls[L"ManaComponent"],0x2400,f.movie);f.set<int>(component,L"mManaPlayTimeMS",5033);f.put<int>(component+0x1f8,150);f.put(component+0xf0,f.texture);f.set(f.movie,L"mpManaComponent",component);}
  else {auto texture=f.object(L"0-B_DEMO01",f.cls[L"ManaTexture"]);f.set(f.movie,L"ManaTexture",texture);f.put(f.component+0xf0,texture);}
  const auto after=snapshot(1066);CHECK(after.story.movie.valid&&after.story.movie.presented);CHECK(after.story.movie.context!=before.story.movie.context);session.observe(after,&film,1066,true);CHECK(!output.busy&&output.stops>stops);
 }
}
static void clocks_reject_a_late_pause_or_component_texture_change(){
 for(int change=0;change<2;++change){MovieFixture f;f.presentation();auto raw=f.memory.reader;bool fired=false;const auto ms=f.component+f.off(f.component,L"mManaPlayTimeMS");
  f.memory.reader=[&](Address at,void* out,std::size_t n){auto ok=raw(at,out,n);if(!fired&&at==ms){fired=true;if(change==0)f.put<std::uint8_t>(f.system+0xb0,0);else f.put(f.component+0xf0,Address(0));}return ok;};
  StoryCapture c(f.unreal);CHECK(!c.sample(1000).story.movie.valid);CHECK(fired);
 }
}
static void movie_pause_reads_direct_actions_without_inventing_focus(){
 StoryFixture f;auto w=f.pause();MenuCapture c(f.unreal);auto s=c.sample(1000);CHECK(s.valid);CHECK(s.label==L"Pause");CHECK(s.help==L"Resume. Skip");CHECK(s.position==0&&s.count==0);
 CHECK(f.unreal.prepare_capture(1033));s=c.sample(1033);CHECK(s.valid);CHECK(s.help==L"Resume. Skip");
 f.flag(w,L"IsMenuBaseInput",false);CHECK(!c.sample(1034).valid);
 StoryFixture collection;collection.pause(L"Back");MenuCapture cc(collection.unreal);CHECK(cc.sample(2000).help==L"Resume. Back");
}
static void unreadable_active_pause_blocks_background_text(){
 StoryFixture f;auto w=f.pause();f.title();MenuCapture c(f.unreal);auto b=f.raw<Address>(w+f.off(w,L"TextBlock_55"));f.put<std::uint8_t>(b+0x91,2);auto s=c.sample(1000);CHECK(!s.valid);CHECK(s.blocked);CHECK(s.label.empty());
 f.put<std::uint8_t>(b+0x91,0);f.text(b+0x140,L"msg_sys_pau_0037");s=c.sample(1001);CHECK(!s.valid);CHECK(s.blocked);
 f.text(b+0x140,L"Pause");CHECK(c.sample(1002).label==L"Pause");f.put<std::uint8_t>(w+0x91,2);CHECK(c.sample(1003).label==L"Background title");
}
static void movie_pause_rejects_duplicate_and_late_changed_owners(){
 StoryFixture ambiguous;ambiguous.pause();ambiguous.pause();MenuCapture ac(ambiguous.unreal);CHECK(!ac.sample(1000).valid);
 for(int change=0;change<2;++change){StoryFixture f;auto w=f.pause(),b=f.raw<Address>(w+f.off(w,L"TextBlock_55"));auto replacement=f.object(L"Replacement",f.cls[L"TextBlock"]);f.slot(replacement,w);f.text(replacement+0x140,L"Other caption");auto raw=f.memory.reader;bool fired=false;
  f.memory.reader=[&](Address a,void* out,std::size_t n){auto ok=raw(a,out,n);if(!fired&&a==b+0x140){fired=true;if(change==0)f.flag(w,L"IsMenuBaseInput",false);else f.set(w,L"TextBlock_55",replacement);}return ok;};
  MenuCapture c(f.unreal);CHECK(!c.sample(1000).valid);CHECK(fired);
 }
}
static void native_modal_keeps_priority_over_movie_pause(){
 StoryFixture f;f.pause();auto c=f.klass(L"WD_WindowBase_C",f.cls[L"UserWidget"]);f.prop(c,L"PopupActive",L"BoolProperty",1);f.prop(c,L"MessageText",L"TextProperty",24);f.prop(c,L"TextBlock_MessageMain",L"ObjectProperty",8);auto w=f.user(c,L"Dialog",true);f.flag(w,L"PopupActive",true);f.field_text(w,L"MessageText",L"Actual warning");f.block(w,L"TextBlock_MessageMain",L"Actual warning");
 MenuCapture capture(f.unreal);auto s=capture.sample(1000);CHECK(s.valid);CHECK(s.modal);CHECK(s.label==L"Actual warning");
}
int main(){int failed=0;for(auto test:{loading_end_is_explicit_native_hidden_state_not_an_empty_observation,drawn_loading_with_unreadable_or_changing_text_does_not_rearm,two_drawn_loading_owners_block_rearm_without_exposing_ambiguous_text,faded_caption_text_is_not_visible_information,faded_subtitle_blocks_are_skipped_without_hiding_a_drawn_sibling,a_replaced_movie_actor_component_or_texture_is_a_new_description_owner,clocks_reject_a_late_pause_or_component_texture_change,thanks_reads_only_the_owned_drawn_texture,thanks_rejects_duplicate_and_late_replaced_owners,a_replaced_sequence_player_is_a_new_description_owner,foreground_fade_covers_movie_and_lower_caption_layers,movie_descriptions_require_the_current_drawn_movie_widget,story_advice_selects_the_drawn_platform_and_rejects_ambiguity,story_caption_late_text_replacement_is_not_narrated,story_loading_and_game_over_are_informational_and_visible,story_continue_uses_drawn_asset_and_direct_actions,story_pause_document_reads_shown_conditions_and_yields_to_list,movie_pause_reads_direct_actions_without_inventing_focus,unreadable_active_pause_blocks_background_text,movie_pause_rejects_duplicate_and_late_changed_owners,native_modal_keeps_priority_over_movie_pause,sequence_clock_distinguishes_preroll_pause_and_movie_playback,movie_soundtrack_time_uses_the_owned_native_sync_origin,clocks_reject_wrong_world_unbound_texture_and_late_replacement,story_captions_follow_the_current_environment_and_rendered_children,ambiguous_or_replaced_caption_owners_are_not_narrated,subtitles_copy_only_rendered_text_and_reject_late_change}){try{test();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';++failed;}}std::cout<<checks<<" story checks, "<<failed<<" failed groups\n";return failed?1:0;}
