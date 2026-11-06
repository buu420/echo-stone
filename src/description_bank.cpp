#include "mvci/description_bank.hpp"
#include "picosha2.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <set>
namespace mvci {
namespace {
template<class T>T value(const std::vector<unsigned char>& b,std::size_t at){T v{};std::memcpy(&v,b.data()+at,sizeof(T));return v;}
bool safe_id(const std::string& id){return !id.empty()&&id.size()<=128&&id.front()!='-'&&std::all_of(id.begin(),id.end(),[](char c){return (c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-';});}
bool digest(const std::string& id){return id.size()==64&&std::all_of(id.begin(),id.end(),[](char c){return (c>='a'&&c<='f')||(c>='0'&&c<='9');});}
bool canonical(const std::vector<unsigned char>& b){
 return b.size()>=46&&b.size()%2==0&&std::memcmp(b.data(),"RIFF",4)==0&&value<std::uint32_t>(b,4)==b.size()-8&&std::memcmp(b.data()+8,"WAVEfmt ",8)==0&&value<std::uint32_t>(b,16)==16&&value<std::uint16_t>(b,20)==1&&value<std::uint16_t>(b,22)==1&&value<std::uint32_t>(b,24)==24000&&value<std::uint32_t>(b,28)==48000&&value<std::uint16_t>(b,32)==2&&value<std::uint16_t>(b,34)==16&&std::memcmp(b.data()+36,"data",4)==0&&value<std::uint32_t>(b,40)==b.size()-44;
}
}
std::shared_ptr<const DescriptionBank> load_description_bank(const std::filesystem::path& root,const DescriptionMovie& movie,const std::function<bool()>& cancelled){
 if(movie.cues.empty()||movie.cues.size()>512)return {};
 std::uint64_t total=0;std::set<std::string> ids;
 for(const auto& c:movie.cues){if(!safe_id(c.id)||c.file!=c.id+".wav"||!digest(c.sha256)||c.bytes<46||c.bytes>2097152||!std::isfinite(c.duration)||c.duration<=0||!ids.insert(c.id).second)return {};total+=c.bytes;}
 if(total>33554432)return {};
 auto bank=std::make_shared<DescriptionBank>();bank->movie=&movie;bank->clips.reserve(movie.cues.size());
 try{for(const auto& cue:movie.cues){
  if(cancelled&&cancelled())return {};const auto file=root/cue.file;std::error_code ec;
  if(std::filesystem::is_symlink(file,ec)||ec||std::filesystem::file_size(file,ec)!=cue.bytes||ec)return {};
  std::ifstream input(file,std::ios::binary);if(!input)return {};std::vector<unsigned char> data(cue.bytes);
  if(!input.read(reinterpret_cast<char*>(data.data()),static_cast<std::streamsize>(data.size()))||input.peek()!=std::char_traits<char>::eof())return {};
  if(cancelled&&cancelled())return {};
  if(!canonical(data)||picosha2::hash256_hex_string(data)!=cue.sha256||std::abs(static_cast<double>(data.size()-44)/48000.-cue.duration)>.000001)return {};
  bank->clips.push_back(std::move(data));
 }}catch(const std::filesystem::filesystem_error&){return {};}
 if(cancelled&&cancelled())return {};return bank;
}
}
