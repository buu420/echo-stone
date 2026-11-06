#pragma once
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>

namespace mvci {
enum class Language {
 English, Arabic, German, Spanish, LatinAmericanSpanish, French, Italian,
 Japanese, Korean, Polish, BrazilianPortuguese, Russian, Thai,
 SimplifiedChinese, TraditionalChinese
};

std::optional<Language> language_from_tag(std::wstring_view tag);
std::wstring_view language_tag(Language language);
Language formatting_language();

class LanguageScope {
 Language previous_;
public:
 explicit LanguageScope(Language language);
 ~LanguageScope();
 LanguageScope(const LanguageScope&) = delete;
 LanguageScope& operator=(const LanguageScope&) = delete;
};

std::wstring tr(std::wstring_view english, std::initializer_list<std::wstring_view> arguments = {});
}
