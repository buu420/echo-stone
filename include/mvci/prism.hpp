#pragma once
#include <memory>
#include <string>
#include <optional>
#include <cstdint>
namespace mvci {
class Prism {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 Prism();~Prism();
 Prism(const Prism&)=delete;Prism& operator=(const Prism&)=delete;
 bool open(const std::wstring& dll_path);
 bool speak(const std::wstring&,bool interrupt=true);
 bool stop();
 std::optional<bool> speaking()const;
 std::string backend_name()const;
};
}
