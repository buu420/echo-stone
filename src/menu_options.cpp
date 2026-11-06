#include "story_read.hpp"
#include "mvci/localization.hpp"

namespace mvci {
namespace {
using story_detail::Frame;

enum class OptionValue { Text, Radio, Checkbox, Slider };
struct OptionLayout {
 const wchar_t* type;
 const wchar_t* label;
 OptionValue value;
};

constexpr OptionLayout option_layouts[] = {
 {L"WD_OptionItem_PressBtn_C", L"WD_OptionItem_Base_71", OptionValue::Text},
 {L"WD_OptionItem_SpinText_C", L"WD_OptionItem_Base_71", OptionValue::Text},
 {L"WD_OptionItem_RadioBtn_C", L"WD_OptionItem_Base_71", OptionValue::Radio},
 {L"WD_OptionItem_Checkbox_C", L"WD_OptionItem_Base_140", OptionValue::Checkbox},
 {L"WD_OptionItem_Slider_C", L"WD_OptionItem_Base_88", OptionValue::Slider},
};

std::optional<Address> attached_widget(Frame& frame, Address owner, const wchar_t* field, const wchar_t* type) {
 const auto child = frame.field(owner, field);
 if (!child || !*child || *child == owner || !frame.u.is_a(*child, type)) return {};
 // A retained WidgetTree Outer can outlive the actual canvas attachment.
 const auto slot = frame.get<Address>(*child + 0x30);
 if (!slot || !*slot || !frame.retain(*slot)) return {};
 const auto parent = frame.get<Address>(*slot + 0x28);
 if (!parent || !*parent || !frame.within(*child, owner)) return {};
 return child;
}

std::optional<std::wstring> visible_text(Frame& frame, Address owner, const wchar_t* field) {
 const auto block = attached_widget(frame, owner, field, L"TextBlock");
 if (!block) return {};
 if (!frame.shown(*block)) return std::wstring{};
 if (frame.u.property(*block, L"ColorAndOpacity")) {
  const auto alpha = frame.text_alpha(*block);
  if (!alpha) return {};
  if (*alpha == 0) return std::wstring{};
 }
 const auto value = frame.display(*block);
 if (!value) return {};
 auto text = normalize_text(*value);
 if (!text.empty() && (!story_detail::readable(text) || text.rfind(L"Text Block", 0) == 0)) return {};
 return text;
}

void append(std::wstring& text, const std::wstring& piece) {
 if (piece.empty() || text == piece) return;
 if (!text.empty()) text += text.back() == L'.' ? L" " : L". ";
 text += piece;
}

std::optional<std::wstring> checkbox_value(Frame& frame, Address row) {
 const auto first = attached_widget(frame, row, L"Check1", L"Image");
 const auto second = attached_widget(frame, row, L"Check2", L"Image");
 if (!first || !second || first == second) return {};
 const bool first_drawn = frame.shown(*first), second_drawn = frame.shown(*second);
 if (first_drawn == second_drawn) return std::wstring{};
 return visible_text(frame, row, first_drawn ? L"Element1" : L"Element2");
}

std::optional<std::wstring> slider_value(Frame& frame, Address row) {
 const auto thumb = attached_widget(frame, row, L"Image_Slider", L"Image");
 const auto fill = attached_widget(frame, row, L"Image_SliderLine", L"Image");
 if (!thumb || !fill || thumb == fill) return {};
 if (!frame.shown(*thumb) || !frame.shown(*fill)) return std::wstring{};
 // WD_OptionItem_Slider.UpdateSubItemDispF and UpdateSubItemDispMouse both
 // draw the current fill using Scale.X. The saved index can lag a mouse drag.
 const auto transform = frame.u.property(*fill, L"RenderTransform");
 if (!transform) return {};
 const auto scale = frame.member(*transform, L"WidgetTransform", L"Scale");
 if (!scale) return {};
 const auto x = frame.member(*scale, L"Vector2D", L"X"), y = frame.member(*scale, L"Vector2D", L"Y");
 if (!x || !y || x->kind != L"FloatProperty" || y->kind != L"FloatProperty" || x->size != 4 || y->size != 4) return {};
 const auto fraction = frame.get<float>(*fill + static_cast<Address>(x->offset));
 const auto height = frame.get<float>(*fill + static_cast<Address>(y->offset));
 if (!fraction || !std::isfinite(*fraction) || *fraction < 0 || *fraction > 1 || height != std::optional<float>(1.f)) return {};
 return tr(L"{0} percent", {std::to_wstring(std::lround(*fraction * 100.f))});
}

std::optional<std::wstring> option_value(Frame& frame, Address row, OptionValue kind) {
 if (kind == OptionValue::Checkbox) return checkbox_value(frame, row);
 if (kind == OptionValue::Slider) return slider_value(frame, row);
 auto text = visible_text(frame, row, L"Element");
 if (!text || kind != OptionValue::Radio) return text;
 const auto marker = attached_widget(frame, row, L"Image_Radio", L"Image");
 if (!marker) return {};
 if (frame.shown(*marker)) append(*text, tr(L"Selected"));
 return text;
}
}

std::optional<MenuSnapshot> rendered_menu_option(Unreal& unreal, Address item,
                                                 const std::function<std::optional<int>(Address)>& visibility) {
 const auto ref = unreal.object(item);
 if (!ref) return {};
 const auto layout = std::find_if(std::begin(option_layouts), std::end(option_layouts),
                                 [&](const OptionLayout& entry) { return unreal.name_is(ref->cls, entry.type); });
 if (layout == std::end(option_layouts)) return {};

 MenuSnapshot blocked;
 blocked.blocked = true;
 Frame frame{unreal, visibility, {}, {}, {}};
 if (!frame.retain(item) || !frame.shown(item)) return blocked;
 const auto label_owner = attached_widget(frame, item, layout->label, L"WD_OptionItem_Base_C");
 if (!label_owner || !frame.shown(*label_owner)) return blocked;
 // Recording rows write their duration only to this nested ItemLabel.
 const auto label = visible_text(frame, *label_owner, L"ItemLabel");
 if (!label || label->empty()) return blocked;
 const auto value = option_value(frame, item, layout->value);
 if (!value || !frame.finish()) return blocked;

 MenuSnapshot result;
 result.valid = true;
 result.label = *label;
 result.value = *value;
 return result;
}

void append_pause_context(Unreal& unreal, Address menu, MenuSnapshot& result,
                          const std::function<std::optional<int>(Address)>& visibility) {
 if (!result.valid || !unreal.is_a(menu, L"WD_PauseMenu_Base_C")) return;

 Frame frame{unreal, visibility, {}, {}, {}};
 if (!frame.retain(menu) || !frame.shown(menu) || !frame.active(menu) ||
     frame.boolean(menu, L"IsMenuBaseInput") != std::optional<bool>(true) ||
     frame.boolean(menu, L"IsPopupActive") == std::optional<bool>(true)) return;
 const auto common = attached_widget(frame, menu, L"WD_PauseCommon", L"WD_PauseCommon_C");
 if (!common || !frame.shown(*common)) return;
 const auto caption = visible_text(frame, *common, L"TextBlock_Caption");
 const auto guide = attached_widget(frame, *common, L"WD_BtnNavi_88", L"WD_BtnNavi_C");
 const auto help = guide && frame.shown(*guide) ? frame.hints(*guide, true) : std::wstring{};
 if (!frame.finish()) return;
 if (caption && !caption->empty()) result.screen = *caption;
 append(result.help, help);
}
}
