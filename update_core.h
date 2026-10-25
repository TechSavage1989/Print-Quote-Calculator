#pragma once
#include <nlohmann/json.hpp>
#include <array>
#include <string>
#include <regex>
#include <stdexcept>
#include <algorithm>
#include <cstdint>
namespace updates {
using json=nlohmann::json;
inline constexpr const char* AppId="PrintQuoteCalculator.WebView2";
inline constexpr const char* Version="1.6.0";
inline constexpr const char* Repo="https://github.com/TechSavage1989/Print-Quote-Calculator";
inline constexpr const char* Feed="https://github.com/TechSavage1989/Print-Quote-Calculator/releases/latest/download/update.json";
inline constexpr uint64_t MaxExe=32*1024*1024;
inline std::array<unsigned,3> version(const std::string&s){
 std::smatch m;if(!std::regex_match(s,m,std::regex("(0|[1-9][0-9]{0,4})\\.(0|[1-9][0-9]{0,4})\\.(0|[1-9][0-9]{0,4})")))throw std::runtime_error("Invalid release version.");
 std::array<unsigned,3> v;for(int i=0;i<3;i++){v[i]=std::stoul(m[i+1]);if(v[i]>65535)throw std::runtime_error("Version component too large.");}return v;
}
inline bool newer(const std::string&a,const std::string&b=Version){return version(a)>version(b);}
struct Manifest {std::string version,url,sha256,release;uint64_t size;json notes;};
inline Manifest manifest(const json&j){
 if(j.at("schema")!=1||j.at("app_id")!=AppId)throw std::runtime_error("Wrong update application or schema.");
 Manifest m;m.version=j.at("version").get<std::string>();version(m.version);
 m.url=j.at("url").get<std::string>();m.sha256=j.at("sha256").get<std::string>();m.size=j.at("size").get<uint64_t>();
 m.release=std::string(Repo)+"/releases/tag/v"+m.version;
 if(m.url!=std::string(Repo)+"/releases/download/v"+m.version+"/PrintQuoteCalculator.exe")throw std::runtime_error("Update asset is not in the trusted repository release.");
 if(!std::regex_match(m.sha256,std::regex("[a-f0-9]{64}"))||m.size<1024||m.size>MaxExe)throw std::runtime_error("Invalid update size or SHA-256.");
 m.notes=j.value("notes",json::array());if(!m.notes.is_array()||m.notes.size()>20)throw std::runtime_error("Invalid release notes.");
 for(const auto&n:m.notes){if(!n.is_string()||n.get<std::string>().empty()||n.get<std::string>().size()>500)throw std::runtime_error("Invalid release note.");}
 if(j.value("release_url",m.release)!=m.release)throw std::runtime_error("Invalid release notes URL.");return m;
}
struct Policy {
 int64_t lastAttempt=0,lastSuccess=0,retryAt=0;unsigned failures=0;
 bool due(int64_t now,const std::string&reason)const{
  if(now<retryAt)return false;
  const int64_t elapsed=std::max<int64_t>(0,now-lastAttempt);
  if(!lastAttempt)return true;
  return elapsed>=(reason=="timer"?300:reason=="focus"?120:60);
 }
 void success(int64_t now){lastSuccess=now;failures=0;retryAt=0;}
 void failed(int64_t now,int64_t retryAfter=0){failures=std::min(failures+1,8u);retryAt=now+std::max<int64_t>(std::min<int64_t>(3600,60ll<<(failures-1)),retryAfter);}
};
}
