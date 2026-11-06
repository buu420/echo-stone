#include "mvci/model.hpp"
#include <array>
#include <iostream>

int main() {
 using namespace mvci;
 int failures = 0;
 const auto check = [&](bool condition, const char* message) {
  if (!condition) { ++failures; std::cerr << message << '\n'; }
 };
 constexpr std::array tags{
  L"en", L"ar", L"de", L"es", L"es-419", L"fr", L"it", L"ja", L"ko",
  L"pl", L"pt-BR", L"ru", L"th", L"zh-Hans", L"zh-Hant"
 };
 for (const auto* tag : tags) {
  const auto language = language_from_tag(tag);
  check(language && language_tag(*language) == tag, "Supported native culture did not round-trip.");
 }
 check(language_from_tag(L"ZH-hANT") == Language::TraditionalChinese, "Culture tags are case insensitive.");
 check(!language_from_tag(L"en-unknown") && !language_from_tag(L"") &&
       !language_from_tag(L"zh"), "Unsupported or ambiguous culture must remain unknown.");
 check(tr(L"{0} of {1}", {L"2", L"8"}) == L"2 of 8", "English templates preserve the existing wording.");
 check(tr(L"{0} of {1}", {L"{1}", L"8"}) == L"{1} of 8", "Native arguments are copied verbatim.");
 {
  LanguageScope french(Language::French);
  check(tr(L"Left side.") == L"Côté gauche.", "French combat speech was not localized.");
  {
   LanguageScope japanese(Language::Japanese);
   check(tr(L"{0} of {1}", {L"2", L"8"}) == L"8項目中2番目", "Japanese can reorder arguments.");
  }
  check(formatting_language() == Language::French, "Nested scope did not restore the capture language.");
 }
 check(formatting_language() == Language::English, "Formatting language escaped its capture scope.");
 check(language_from_tag(L"fr-FR")==Language::French&&
       language_from_tag(L"zh-Hant-TW")==Language::TraditionalChinese&&
       !language_from_tag(L"pt-PT"), "Regional game culture fallback is incorrect.");
 for(const auto* alias:{L"es-LA",L"es-AR",L"es-BO",L"es-CL",L"es-CO",L"es-CR",L"es-CU",
                       L"es-DO",L"es-EC",L"es-GT",L"es-HN",L"es-MX",L"es-NI",L"es-PA",
                       L"es-PE",L"es-PR",L"es-PY",L"es-SV",L"es-US",L"es-UY",L"es-VE"})
  check(language_from_tag(alias)==Language::LatinAmericanSpanish,"Shipped Spanish region mapping was not used.");
 check(language_from_tag(L"es-ES")==Language::Spanish,"Spain Spanish selected the Latin American catalog.");

 Model model;
 Snapshot snapshot;
 snapshot.language=Language::French;
 snapshot.foreground=true;
 snapshot.observed_ms=1000;
 snapshot.menu.valid=true;
 snapshot.menu.identity=7;
 snapshot.menu.label=L"Réglages {0}";
 snapshot.menu.position=2;
 snapshot.menu.count=8;
 const auto french=model.update(snapshot,1000,Request::ReadMenu).speech;
 check(french&&french->text.find(L"Réglages {0}")!=std::wstring::npos&&
       french->text.find(L"2 of 8")==std::wstring::npos,
       "Menu formatting replaced native text or used English metadata.");

 snapshot.language=Language::Japanese;
 snapshot.observed_ms=1010;
 snapshot.menu.label=L"設定";
 const auto japanese=model.update(snapshot,1010).speech;
 check(japanese&&japanese->text==L"設定. 8項目中2番目.",
       "Menu did not change to the newly observed game language.");
 check(french&&!model.submission_fresh(french->token,1010),
       "Old language speech remained eligible after a culture change.");

 snapshot.language=Language::French;
 snapshot.menu={};
 snapshot.observed_ms=1020;
 snapshot.combat.valid=true;
 snapshot.combat.playable=true;
 snapshot.combat.session=1;
 snapshot.combat.fighters[0].x=-1;
 snapshot.combat.fighters[1].x=1;
 snapshot.combat.fighters[0].health_percent=80;
 snapshot.combat.fighters[1].health_percent=60;
 snapshot.combat.fighters[0].hyper_bars=2.25;
 const auto combat=model.update(snapshot,1020,Request::ReadCombat).speech;
 check(combat&&combat->text.find(L"Côté gauche.")!=std::wstring::npos&&
       combat->text.find(L"2,25")!=std::wstring::npos&&
       combat->text.find(L"Your health")==std::wstring::npos&&
       combat->text.find(L"80")!=std::wstring::npos&&combat->text.find(L"60")!=std::wstring::npos,
       "Combat status did not preserve localized numbers, side and both health values.");
 check(model.beacon_enabled()&&model.side_announcements_enabled()&&model.descriptions_enabled(),
       "Language selection changed the existing cue settings.");
 snapshot.language=Language::Japanese;
 snapshot.observed_ms=1030;
 const auto pending_combat=model.update(snapshot,1030).speech;
 check(combat&&pending_combat&&pending_combat->request==Request::ReadCombat&&
       pending_combat->token!=combat->token&&pending_combat->text.find(L"2.25")!=std::wstring::npos,
       "A pending F7 request was lost during a language change.");
 snapshot.observed_ms=1040;
 const auto toggle=model.update(snapshot,1040,Request::ToggleSide).speech;
 snapshot.language=Language::French;
 snapshot.observed_ms=1050;
 const auto pending_toggle=model.update(snapshot,1050).speech;
 {
  LanguageScope french_scope(Language::French);
  check(toggle&&pending_toggle&&pending_toggle->request==Request::ToggleSide&&
        pending_toggle->token!=toggle->token&&pending_toggle->text==tr(L"Side announcements off.")&&
        !model.side_announcements_enabled(),
        "A pending control confirmation was lost, left in the old language or toggled twice.");
 }
 return failures ? 1 : 0;
}
