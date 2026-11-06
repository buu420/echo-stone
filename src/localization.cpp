#include "mvci/localization.hpp"
#include <algorithm>
#include <array>

namespace mvci {
namespace {
thread_local Language current_language = Language::English;
constexpr std::array<std::wstring_view,15> tags{
 L"en", L"ar", L"de", L"es", L"es-419", L"fr", L"it", L"ja", L"ko",
 L"pl", L"pt-BR", L"ru", L"th", L"zh-Hans", L"zh-Hant"
};

wchar_t ascii_lower(wchar_t c) {
 return c>=L'A'&&c<=L'Z' ? static_cast<wchar_t>(c+L'a'-L'A') : c;
}
bool same_tag(std::wstring_view left, std::wstring_view right) {
 return std::equal(left.begin(),left.end(),right.begin(),right.end(),
  [](wchar_t a,wchar_t b){return ascii_lower(a)==ascii_lower(b);});
}
bool region(std::wstring_view suffix) {
 return suffix.size()==2&&std::all_of(suffix.begin(),suffix.end(),
  [](wchar_t c){return ascii_lower(c)>=L'a'&&ascii_lower(c)<=L'z';});
}
struct Phrase {std::wstring_view english,translated;};
struct Table {const Phrase* begin; const Phrase* end;};
template<std::size_t N> constexpr Table table(const Phrase (&phrases)[N]) {
 return {phrases,phrases+N};
}
constexpr Phrase none[]={{}};
constexpr Phrase ar[]={{},
#include "locale/ar.inc"
};
constexpr Phrase de[]={{},
#include "locale/de.inc"
};
constexpr Phrase es[]={{},
#include "locale/es.inc"
};
constexpr Phrase es419[]={{},
#include "locale/es-419.inc"
};
constexpr Phrase fr[]={{},
#include "locale/fr.inc"
};
constexpr Phrase it[]={{},
#include "locale/it.inc"
};
constexpr Phrase ja[]={{},
#include "locale/ja.inc"
};
constexpr Phrase ko[]={{},
#include "locale/ko.inc"
};
constexpr Phrase pl[]={{},
#include "locale/pl.inc"
};
constexpr Phrase ptBR[]={{},
#include "locale/pt-BR.inc"
};
constexpr Phrase ru[]={{},
#include "locale/ru.inc"
};
constexpr Phrase th[]={{},
#include "locale/th.inc"
};
constexpr Phrase zhHans[]={{},
#include "locale/zh-Hans.inc"
};
constexpr Phrase zhHant[]={{},
#include "locale/zh-Hant.inc"
};
constexpr std::array tables{
 table(none),table(ar),table(de),table(es),table(es419),table(fr),table(it),table(ja),
 table(ko),table(pl),table(ptBR),table(ru),table(th),table(zhHans),table(zhHant)
};
static_assert(tags.size()==tables.size());

std::wstring_view translation(std::wstring_view english) {
 const auto index=static_cast<std::size_t>(current_language);
 if(index>=tables.size())return english;
 const auto& phrases=tables[index];
 const auto found=std::lower_bound(phrases.begin,phrases.end,english,
  [](const Phrase& phrase,std::wstring_view key){return phrase.english<key;});
 return found!=phrases.end&&found->english==english?found->translated:english;
}
}

std::optional<Language> language_from_tag(std::wstring_view tag) {
 for(std::size_t i=0;i<tags.size();++i)if(same_tag(tag,tags[i]))return static_cast<Language>(i);
 const auto separator=tag.rfind(L'-');
 if(separator==std::wstring_view::npos||!region(tag.substr(separator+1)))return {};
 const auto parent=tag.substr(0,separator);
 // The effective DefaultGame.ini maps these Spanish locales to es-419
 // even when FInternationalization still holds the original regional tag.
 constexpr std::array<std::wstring_view,21> latin_american_regions{
  L"LA",L"AR",L"BO",L"CL",L"CO",L"CR",L"CU",L"DO",L"EC",L"GT",L"HN",
  L"MX",L"NI",L"PA",L"PE",L"PR",L"PY",L"SV",L"US",L"UY",L"VE"
 };
 if(same_tag(parent,L"es")&&std::any_of(latin_american_regions.begin(),latin_american_regions.end(),
    [&](std::wstring_view code){return same_tag(code,tag.substr(separator+1));}))
  return Language::LatinAmericanSpanish;
 if(same_tag(tag,L"zh-CN")||same_tag(tag,L"zh-SG"))return Language::SimplifiedChinese;
 if(same_tag(tag,L"zh-TW")||same_tag(tag,L"zh-HK")||same_tag(tag,L"zh-MO"))return Language::TraditionalChinese;
 for(std::size_t i=0;i<tags.size();++i)if(same_tag(parent,tags[i]))return static_cast<Language>(i);
 return {};
}
std::wstring_view language_tag(Language language) {
 const auto index=static_cast<std::size_t>(language);
 return index<tags.size()?tags[index]:std::wstring_view{};
}
Language formatting_language() { return current_language; }
LanguageScope::LanguageScope(Language language) : previous_(current_language) { current_language = language; }
LanguageScope::~LanguageScope() { current_language = previous_; }
std::wstring tr(std::wstring_view english, std::initializer_list<std::wstring_view> arguments) {
 const auto pattern=translation(english);
 if(arguments.size()==0)return std::wstring(pattern);
 std::wstring out;
 out.reserve(pattern.size());
 for(std::size_t i=0;i<pattern.size();++i){
  if(pattern[i]==L'{'&&i+2<pattern.size()&&pattern[i+2]==L'}'&&pattern[i+1]>=L'0'&&pattern[i+1]<=L'9'){
   const auto argument=static_cast<std::size_t>(pattern[i+1]-L'0');
   if(argument<arguments.size()){
    out+=arguments.begin()[argument];i+=2;continue;
   }
  }
  out+=pattern[i];
 }
 return out;
}
}
