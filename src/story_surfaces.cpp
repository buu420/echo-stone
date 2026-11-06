#include "mvci/story.hpp"
#include "story_read.hpp"
#include <cmath>
namespace mvci {
namespace {
using story_detail::Frame;
std::uint64_t identity(const ObjectRef& r){return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(r.serial))<<32)|static_cast<std::uint32_t>(r.index);}
void append(std::wstring& text,const std::wstring& value){if(value.empty())return;if(!text.empty())text+=L". ";text+=value;}
bool current(Frame& f,Address actor,Address world){
 if(!f.retain(actor))return false;const auto level=f.get<Address>(actor+0x20);return level&&f.u.is_a(*level,L"Level")&&f.get<Address>(*level+0xb0)==std::optional<Address>(world);
}
bool asset(Frame& f,Address image,const wchar_t* name,const wchar_t* type){
 const auto resource=f.image_resource(image);if(!resource||!f.u.is_a(*resource,type)||!f.u.name_is(*resource,name))return false;
 const auto package=f.get<Address>(*resource+0x20);if(!package||!*package||!f.retain(*package)||!f.u.is_a(*package,L"Package"))return false;
 // The native localized packages preserve this suffix. No dynamic material or
 // guessed label is accepted; the shipped material/texture was inspected offline.
 const auto path=f.u.name(*package);const auto suffix=std::wstring(L"/UI/Battle/Continue/Parts/")+name;
 return path.rfind(L"/Game/",0)==0&&path.size()>=suffix.size()&&path.compare(path.size()-suffix.size(),suffix.size(),suffix)==0;
}
StoryTextSnapshot text_result(Unreal& u,Address root,const std::wstring& text){const auto r=u.object(root);return r?StoryTextSnapshot{true,identity(*r),text}:StoryTextSnapshot{};}
std::optional<MenuSnapshot> continue_menu(Frame& f,Address flow,Address world){
 if(!current(f,flow,world))return {};const auto w=f.field(flow,L"Continue_Obj");if(!w||!*w||!f.u.is_a(*w,L"WD_Continue_C")||!f.active(*w)||!f.shown(*w))return {};
 MenuSnapshot blocked;blocked.blocked=true;blocked.modal=true;
 const auto image=f.field(*w,L"Image_Continue");if(!image||!*image||!f.within(*image,*w)||!f.shown(*image)||!asset(f,*image,L"M_ContinueText",L"Material"))return blocked;
 const auto yes=f.text(*w,L"TextBlock_YES"),no=f.text(*w,L"TextBlock_NO");const auto hints=f.hints(*w,false);
 if(!yes||!no||!story_detail::readable(*yes)||!story_detail::readable(*no)||hints.empty()||!f.finish())return blocked;
 MenuSnapshot out;out.valid=true;out.modal=true;out.identity=identity(*f.u.object(*w));out.label=L"Continue";out.value=*yes+L". "+*no;out.help=hints;return out;
}
std::optional<StoryTextSnapshot> game_over(Frame& f,Address flow,Address world){
 if(!current(f,flow,world))return {};const auto w=f.field(flow,L"GameOver_Obj");if(!w||!*w||!f.u.is_a(*w,L"WD_GameOver_C")||!f.active(*w)||!f.shown(*w))return {};
 const auto image=f.field(*w,L"Image_GameOver");if(!image||!*image||!f.within(*image,*w)||!f.shown(*image)||!asset(f,*image,L"TEX_GameOver",L"Texture2D")||!f.finish())return {};
 return text_result(f.u,*w,L"Game over");
}
std::optional<StoryTextSnapshot> thanks(Frame& f,Address flow,Address world){
 if(!current(f,flow,world))return {};const auto w=f.field(flow,L"Thanks_Obj");if(!w||!*w||!f.u.is_a(*w,L"WD_Thanks_C")||!f.active(*w)||!f.shown(*w))return {};
 const auto image=f.field(*w,L"Image_Text");if(!image||!*image||!f.u.is_a(*image,L"Image")||!f.within(*image,*w)||!f.shown(*image))return {};
 const auto color=f.u.property(*image,L"ColorAndOpacity");if(!color||color->kind!=L"StructProperty"||color->size!=16)return {};
 const auto alpha=f.get<float>(*image+static_cast<Address>(color->offset)+12);if(!alpha||!std::isfinite(*alpha)||*alpha<=0||*alpha>1)return {};
 const auto resource=f.image_resource(*image);if(!resource||!f.u.is_a(*resource,L"Texture2D")||!f.u.name_is(*resource,L"tex_thanks_text"))return {};
 const auto package=f.get<Address>(*resource+0x20);if(!package||!*package||!f.retain(*package)||!f.u.is_a(*package,L"Package")||!f.u.name_is(*package,L"/Game/UI/Story/Parts/tex_thanks_text")||!f.finish())return {};
 // Literal text independently inspected in the shipped base texture. Localized
 // replacement images require their own mapping; no Story progress is queried.
 return text_result(f.u,*w,L"Thank you for playing.");
}
std::optional<StoryTextSnapshot> one_point(Frame& f,Address flow,Address world){
 if(!current(f,flow,world))return {};const auto w=f.field(flow,L"OnePoint");if(!w||!*w||!f.u.is_a(*w,L"WD_OnePoint_C")||!f.active(*w)||!f.shown(*w))return {};
 const auto count=f.integer(*w,L"LineNum");if(!count||*count<1||*count>5)return {};
 std::wstring sets[2];bool drawn[2]{};
 for(int set=0;set<2;++set){const auto boxes=f.array(*w,set?L"LineBoxArrayPC":L"LineBoxArray",5);if(!boxes||boxes->count<*count)return {};
  for(int i=0;i<*count;++i){const auto box=f.get<Address>(boxes->data+static_cast<Address>(i)*8);if(!box||!*box||!f.retain(*box)||!f.within(*box,*w))return {};if(!f.shown(*box))continue;drawn[set]=true;
   const auto line=f.line(*box,*w);if(!line)return {};if(!sets[set].empty())sets[set]+=L" ";sets[set]+=*line;
  }
 }
 if(drawn[0]==drawn[1])return {};const auto text=sets[drawn[1]?1:0];if(text.empty()||!f.finish())return {};return text_result(f.u,*w,text);
}
std::pair<LoadingSnapshot,std::optional<StoryTextSnapshot>> loading(Frame& f,Address root){
 if(!f.retain(root))return {};const auto ref=f.u.object(root);if(!ref)return {};
 if(!f.shown(root)){
  // SpawnLoading retains one widget across level travel. A proven detached
  // top-level viewport is an end signal; unreadable shared state is not.
  const auto slot=f.get<Address>(root+0x30),view=f.get<Address>(root+0x208),shared=f.get<Address>(root+0x210);
  bool detached=slot&&!*slot&&view&&shared&&(!*view||!*shared);
  if(slot&&!*slot&&view&&*view&&shared&&*shared){const auto count=f.get<int>(*shared+8);detached=count&&*count<=0;}
  return detached&&f.finish()?std::make_pair(LoadingSnapshot{true,false,identity(*ref)},std::optional<StoryTextSnapshot>{}):std::pair<LoadingSnapshot,std::optional<StoryTextSnapshot>>{};
 }
 const auto block=f.field(root,L"TextBlock_Loading");if(!block||!*block||!f.u.is_a(*block,L"TextBlock")||!f.within(*block,root))return {};
 const auto visible=f.integer(*block,L"Visibility");const bool drawn=f.shown(*block);if(!visible||(*visible!=0&&*visible!=2)||drawn!=(*visible==0))return {};
 LoadingSnapshot state{true,drawn,identity(*ref)};
 if(!drawn)return f.finish()?std::make_pair(state,std::optional<StoryTextSnapshot>{}):std::pair<LoadingSnapshot,std::optional<StoryTextSnapshot>>{};
 const auto alpha=f.text_alpha(*block);if(!alpha||*alpha<=0)return {};
 // Text can disappear or change while the same loading surface is still drawn.
 // Fence the caption independently so it cannot erase known presentation state.
 const auto text=f.u.text_field(*block,L"Text");
 if(!f.finish())return {};
 if(!text||!story_detail::readable(*text)||f.u.text_field(*block,L"Text")!=text)return {state,{}};
 return {state,StoryTextSnapshot{true,state.identity,*text}};
}
std::optional<MenuSnapshot> document(Frame& f,Address w){
 if(!f.active(w)||!f.shown(w)||f.boolean(w,L"IsMenuBaseInput")!=std::optional<bool>(true)||f.boolean(w,L"IsPopupActive")==std::optional<bool>(true)||f.integer(w,L"SelectListNo")!=std::optional<int>(0))return {};
 const auto d=f.field(w,L"WD_WindowControl_New_327");if(!d||!*d||!f.u.is_a(*d,L"WD_WindowControl_New_C")||!f.within(*d,w)||!f.shown(*d))return {};
 MenuSnapshot out;out.blocked=true;std::wstring text;
 for(auto field:{L"TextBlock_Title_V",L"TextBlock_Text_V",L"TextBlock_Text_S",L"TextBlock_Text_S_00",L"TextBlock_Text_S_01",L"TextBlock_Ability_Title",L"TextBlock_ChrName"}){
  if(const auto value=f.text(*d,field);value&&story_detail::readable(*value))append(text,*value);
 }
 if(const auto box=f.field(*d,L"HorizontalBox_S");box&&*box&&f.shown(*box)){const auto line=f.line(*box,*d);if(!line)return out;append(text,*line);}
 for(auto field:{L"WD_BuffList1PSub_New",L"WDBuffList1PMani_New",L"WD_BuffList2PMain_New",L"WD_BuffList2PSub_New"}){
  const auto buff=f.field(*d,field);if(!buff||!*buff||!f.u.is_a(*buff,L"WD_BuffListNew_C")||!f.within(*buff,*d)||!f.shown(*buff))continue;
  for(auto name:{L"TextBlock_ChrName",L"TextBlock_00",L"TextBlock_01",L"TextBlock_02"})if(const auto value=f.text(*buff,name);value&&story_detail::readable(*value))append(text,*value);
 }
 if(text.empty()||!f.finish())return out;out.valid=true;out.blocked=false;out.identity=identity(*f.u.object(w));out.screen=L"Story pause";out.label=text;
 if(const auto common=f.field(w,L"WD_PauseCommon");common&&*common&&f.within(*common,w)&&f.shown(*common)){
  if(const auto title=f.text(*common,L"TextBlock_Caption");title&&story_detail::readable(*title))out.screen=*title;
  if(const auto guide=f.field(*common,L"WD_BtnNavi_88");guide&&*guide&&f.within(*guide,*common)&&f.shown(*guide))out.help=f.hints(*guide,true);
 }
 if(!f.finish())return MenuSnapshot{false,false,true,0,L"",L"",L"",L"",0,0,true};return out;
}
}
StoryCaptureResult story_surfaces(Unreal& u,Address world,const std::function<std::optional<int>(Address)>& visibility){
 StoryCaptureResult out;std::array<bool,5> ambiguous{};bool menu_ambiguous=false;
 const auto merge_text=[&](std::size_t index,const std::optional<StoryTextSnapshot>& value){if(!value||!value->valid||ambiguous[index])return;auto& prior=out.story.captions[index];if(prior.valid&&(prior.identity!=value->identity||prior.text!=value->text)){prior={};ambiguous[index]=true;}else prior=*value;};
 const auto merge_menu=[&](const std::optional<MenuSnapshot>& menu){if(!menu||menu_ambiguous)return;if(out.menu.valid||out.menu.blocked){out.menu={};out.menu.blocked=true;menu_ambiguous=true;}else out.menu=*menu;};
 for(const auto& flow:u.instances(L"GF_FightStory_C")){
  {Frame f{u,visibility,{},{},{}};merge_menu(continue_menu(f,flow.address,world));}
  {Frame f{u,visibility,{},{},{}};merge_text(4,game_over(f,flow.address,world));}
  {Frame f{u,visibility,{},{},{}};merge_text(2,one_point(f,flow.address,world));}
 }
 for(const auto& flow:u.instances(L"GF_Thanks_C")){Frame f{u,visibility,{},{},{}};merge_text(4,thanks(f,flow.address,world));}
 for(const auto& pause:u.instances(L"WD_PauseMenu_Story_C")){Frame f{u,visibility,{},{},{}};if(f.retain(pause.address))merge_menu(document(f,pause.address));}
 for(const auto& candidate:u.instances(L"WD_Loading_C")){
  Frame f{u,visibility,{},{},{}};const auto captured=loading(f,candidate.address);
  const auto& next=captured.first;if(!next.known)continue;auto& prior=out.story.loading;
  if(!next.visible){
   if(!prior.known)prior=next;
   else if(!prior.visible&&prior.identity!=next.identity)prior.identity=0;
   continue;
  }
  // Detached cached owners cannot compete with a drawn one. Multiple drawn
  // owners still prove loading is on screen, but supply no unique text to read.
  if(!prior.visible)prior=next;
  else if(prior.identity!=next.identity){prior.identity=0;out.story.captions[3]={};ambiguous[3]=true;}
  merge_text(3,captured.second);
 }
 for(const auto& caption:out.story.captions)if(caption.valid){out.story.valid=true;out.story.context^=caption.identity;}
 return out;
}
}
