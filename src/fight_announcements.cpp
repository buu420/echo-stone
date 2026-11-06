#include "widget_geometry.hpp"

namespace mvci {
namespace {
using story_detail::Frame;
constexpr Address flow_system_rva = 0x348cb40;
constexpr auto result_parts = L"/Game/UI/Battle/Result/Parts/";
constexpr auto score_parts = L"/Game/UI/Battle/Result/Parts/Material/";

bool current_actor(Frame& frame, Address actor, Address world) {
 if (!frame.retain(actor) || !frame.u.is_a(actor, L"Actor")) return false;
 const auto level = frame.get<Address>(actor + 0x20);
 return level && *level && frame.retain(*level) && frame.u.is_a(*level, L"Level") &&
        frame.get<Address>(*level + 0xb0) == std::optional<Address>(world);
}

std::optional<int> tick_state(Frame& frame, Address owner) {
 const auto tick = frame.field(owner, L"TickStateObj");
 if (!tick || !*tick || !frame.u.is_a(*tick, L"BP_TickStateObj_C") ||
     frame.field(*tick, L"ParentObj") != std::optional<Address>(owner) ||
     frame.boolean(*tick, L"PreState") != std::optional<bool>(false)) return {};
 return frame.integer(*tick, L"State");
}

bool viewport(Frame& frame, Address widget) {
 if (frame.get<Address>(widget + 0x30) != std::optional<Address>(0)) return false;
 const auto view = frame.get<Address>(widget + 0x208), shared = frame.get<Address>(widget + 0x210);
 if (!view || !*view || !shared || !*shared) return false;
 const auto count = frame.get<int>(*shared + 8);
 return count && *count > 0 && frame.shown(widget);
}

std::optional<Address> widget_child(Frame& frame, Address owner, const wchar_t* field, const wchar_t* type) {
 const auto child = frame.field(owner, field);
 if (!child || !*child || *child == owner || !frame.u.is_a(*child, type)) return {};
 const auto slot = frame.get<Address>(*child + 0x30);
 if (!slot || !*slot || !frame.retain(*slot)) return {};
 const auto parent = frame.get<Address>(*slot + 0x28);
 return parent && *parent && frame.within(*child, owner) ? child : std::nullopt;
}

bool position(Frame& frame, Address widget, float x, float y) {
 using widget_geometry::number;
 return widget_geometry::near(number(frame, widget, L"RenderTransform", {L"Translation", L"X"}), x) &&
        widget_geometry::near(number(frame, widget, L"RenderTransform", {L"Translation", L"Y"}), y);
}

bool positive_scale(Frame& frame, Address widget) {
 using widget_geometry::number;
 const auto x = number(frame, widget, L"RenderTransform", {L"Scale", L"X"});
 const auto y = number(frame, widget, L"RenderTransform", {L"Scale", L"Y"});
 return x && *x > 0.f && y && *y > 0.f;
}

std::optional<std::wstring> visible_text(Frame& frame, Address owner, const wchar_t* field, bool settled = false) {
 const auto block = widget_child(frame, owner, field, L"TextBlock");
 if (!block || !frame.shown(*block) || (settled && (!position(frame, *block, 0.f, 0.f) || !positive_scale(frame, *block)))) return {};
 if (frame.u.property(*block, L"ColorAndOpacity")) {
  const auto alpha = frame.text_alpha(*block);
  if (!alpha || *alpha <= 0.f) return {};
 }
 const auto text = frame.display(*block);
 if (!text || !story_detail::readable(*text) || text->rfind(L"Text Block", 0) == 0) return {};
 return text;
}

bool asset(Frame& frame, const std::optional<Address>& resource, const wchar_t* type,
           const wchar_t* directory, const wchar_t* name) {
 if (!resource || !*resource || !frame.u.is_a(*resource, type) || !frame.u.name_is(*resource, name)) return false;
 const auto package = frame.get<Address>(*resource + 0x20);
 return package && *package && frame.retain(*package) && frame.u.is_a(*package, L"Package") &&
        frame.u.name_is(*package, std::wstring(directory) + name);
}

StoryTextSnapshot snapshot(Frame& frame, Address owner, const std::wstring& text) {
 const auto ref = frame.u.object(owner);
 if (text.empty() || !ref || !frame.finish()) return {};
 const auto identity = (static_cast<std::uint64_t>(static_cast<std::uint32_t>(ref->serial)) << 32) |
                       static_cast<std::uint32_t>(ref->index);
 return {true, identity, text};
}

StoryTextSnapshot arcade_stage(Frame& frame, Address flow, Address world) {
 const auto versus = frame.field(flow, L"VersusObj");
 if (!versus || !*versus || !frame.u.is_a(*versus, L"BP_VersusArcade_C") || !current_actor(frame, *versus, world)) return {};
 const auto input = frame.integer(*versus, L"InputState");
 if (!input || (*input != 1 && *input != 2)) return {};
 const auto caption = frame.field(*versus, L"ArcadeText_Obj");
 if (!caption || !*caption || !frame.u.is_a(*caption, L"WD_ArcadeText_C") || !viewport(frame, *caption)) return {};
 const auto state = tick_state(frame, *caption);
 // WD_ArcadeText draws the localized stage label during its Enter animation.
 if (!state || (*state != 1 && *state != 2)) return {};
 const auto text = visible_text(frame, *caption, L"TextBlock_ArcadeStage");
 return text ? snapshot(frame, *caption, *text) : StoryTextSnapshot{};
}

std::optional<std::wstring> total_text(Frame& frame, Address total, int side) {
 if (!viewport(frame, total) || frame.integer(total, L"TeamNo") != std::optional<int>(side)) return {};
 const auto panel = widget_child(frame, total, L"CanvasPanel_Total", L"CanvasPanel");
 if (!panel || !frame.shown(*panel) || !position(frame, *panel, side ? 968.f : 200.f, 150.f) || !positive_scale(frame, *panel)) return {};
 const auto player = visible_text(frame, total, side ? L"TextBlock_Player2" : L"TextBlock_Player1");
 const auto art = widget_child(frame, total, L"Image_Result_Text", L"Image");
 const auto result = frame.integer(total, L"ResultType");
 constexpr const wchar_t* materials[]{L"MT_Inst_Win", L"MT_Inst_Lose", L"MT_Inst_Draw"};
 constexpr const wchar_t* labels[]{L"Winner", L"Loser", L"Draw"};
 // SetResultType updates this brush. The inspected atlas has these three words.
 if (!player || !art || !frame.shown(*art) || !result || *result < 0 || *result > 2 ||
     !asset(frame, frame.image_resource(*art), L"MaterialInstanceConstant", score_parts, materials[*result])) return {};
 std::wstring text = *player + L". " + labels[*result];
 for (const auto& pair : {std::pair{L"Text_Win", L"Text_WinNum"}, std::pair{L"Text_Lose", L"Text_LoseNum"},
                          std::pair{L"Text_Draw", L"Text_DrawNum"}}) {
  const auto label = visible_text(frame, total, pair.first), count = visible_text(frame, total, pair.second);
  if (!label || !count) return {};
  text += L". " + *label + L" " + *count;
 }
 return text;
}

std::optional<std::wstring> totals(Frame& frame, Address result) {
 const auto array = frame.array(result, L"WDTotal_Arrobj", 2);
 if (!array || (array->count != 0 && array->count != 2)) return {};
 std::wstring text;
 for (int i = 0; i < array->count; ++i) {
  const auto total = frame.get<Address>(array->data + static_cast<Address>(i) * 8);
  if (!total || !*total || !frame.retain(*total) || !frame.u.is_a(*total, L"WD_Total_C")) return {};
  if (!frame.shown(*total)) continue;
  const auto section = total_text(frame, *total, i);
  if (!section) return {};
  if (text.empty()) text = L"Results";
  text += L". " + *section;
 }
 return text;
}

StoryTextSnapshot winner(Frame& frame, Address result, bool arcade_clear) {
 const auto announcement = frame.field(result, L"WD_Announce");
 if (!announcement || !*announcement || !frame.u.is_a(*announcement, L"WD_Result_Announce_New_C") || !viewport(frame, *announcement)) return {};
 const auto left = widget_child(frame, *announcement, L"Winner_1P", L"Image");
 const auto right = widget_child(frame, *announcement, L"Winner_2P", L"Image");
 const auto clear = widget_child(frame, *announcement, L"CONGRATULATIONS", L"Image");
 if (!left || !right || !clear || left == right || left == clear || right == clear) return {};
 const auto drawn = [&](Address image) { return frame.shown(image) && position(frame, image, 0.f, 0.f) && positive_scale(frame, image); };
 const bool p1 = drawn(*left), p2 = drawn(*right), congrats = drawn(*clear);
 if (static_cast<int>(p1) + static_cast<int>(p2) + static_cast<int>(congrats) != 1 || arcade_clear != congrats) return {};
 const auto image = congrats ? *clear : p1 ? *left : *right;
 const auto material = congrats ? L"MT_CongText" : p1 ? L"MT_Wintext_1P" : L"MT_Wintext_2P";
 if (!asset(frame, frame.image_resource(image), L"MaterialInstanceConstant", result_parts, material)) return {};
 // The animation starts with these names hidden/off-screen. Arcade clear uses
 // the 2P layout for its shown names, regardless of the actual player side.
 const auto main = visible_text(frame, *announcement, p1 ? L"TextMainName1P" : L"TextMainName2P", true);
 const auto sub = visible_text(frame, *announcement, p1 ? L"TextSubName1P" : L"TextSubName2P", true);
 if (!main || !sub) return {};
 // Arcade clear plays the native SYS_NA congratulations cue. Its graphic
 // gates the two names while that audio supplies the congratulation.
 return snapshot(frame, *announcement, std::wstring(congrats ? L"Arcade team. " : L"Winner. ") + *main + L". " + *sub);
}

StoryTextSnapshot fight_result(Frame& frame, Address flow, Address world, bool arcade) {
 const auto result = frame.field(flow, L"Result_Obj");
 if (!result || !*result || !current_actor(frame, *result, world) || tick_state(frame, *result) != std::optional<int>(2)) return {};
 if (arcade && frame.u.is_a(*result, L"BP_ResultCtrlArcadeClear_C")) return winner(frame, *result, true);
 const auto ref = frame.u.object(*result);
 if (!ref || !frame.u.name_is(ref->cls, L"BP_ResultCtrl_C")) return {};
 const auto summary = totals(frame, *result);
 if (!summary) return {};
 if (!summary->empty()) return snapshot(frame, *result, *summary);
 // Victory dialogue is carried by a separate native subtitle/voice path.
 return winner(frame, *result, false);
}

}

StoryTextSnapshot fight_announcements(Unreal& unreal, Address world, const std::function<std::optional<int>(Address)>& visibility) {
 if (!world) return {};
 Frame frame{unreal, visibility, {}, {}, {}};
 // GetInstance (RVA 0x528bc0) and GetFlowBP (RVA 0x527b80) directly read these pointers.
 const auto system = frame.get<Address>(unreal.base() + flow_system_rva);
 if (!system || !*system || !frame.retain(*system) || !unreal.is_a(*system, L"GameFlowSystem")) return {};
 const auto flow = frame.get<Address>(*system + 0xe8);
 if (!flow || !*flow || !current_actor(frame, *flow, world)) return {};
 frame.bindings.push_back([&unreal, world] { return unreal.current_world() == std::optional<Address>(world); });
 const auto ref = unreal.object(*flow);
 if (!ref) return {};
 if (unreal.name_is(ref->cls, L"GF_VersusArcade_C")) return arcade_stage(frame, *flow, world);
 if (unreal.name_is(ref->cls, L"GF_FightArcade_C")) return fight_result(frame, *flow, world, true);
 if (unreal.name_is(ref->cls, L"GF_FightLocal_C")) return fight_result(frame, *flow, world, false);
 // Mission Clear and All Mission Complete already have native narration.
 return {};
}
}
