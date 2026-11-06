#pragma once
#include "mvci/descriptions.hpp"
#include <filesystem>
namespace mvci {
struct DescriptionBank {
 const DescriptionMovie* movie=nullptr;
 std::vector<std::vector<unsigned char>> clips;
};
std::shared_ptr<const DescriptionBank> load_description_bank(const std::filesystem::path&,const DescriptionMovie&,const std::function<bool()>& cancelled={});
enum class DescriptionBankStage {Idle,Loading,Ready,Failed};
struct DescriptionBankView {const DescriptionMovie* movie=nullptr;DescriptionBankStage stage=DescriptionBankStage::Idle;std::shared_ptr<const DescriptionBank> bank;};
class DescriptionBankLoader {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 using Load=std::function<std::shared_ptr<const DescriptionBank>(const std::filesystem::path&,const DescriptionMovie&,const std::function<bool()>&)>;
 explicit DescriptionBankLoader(std::filesystem::path,Load={});
 ~DescriptionBankLoader();
 void request(const DescriptionMovie*);
 DescriptionBankView view()const;
};
}
