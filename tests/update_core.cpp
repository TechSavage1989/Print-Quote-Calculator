#include "../src/update_core.h"
#include <cassert>
#include <iostream>
using namespace updates;
int main(){
 auto rejects=[](auto fn){bool rejected=false;try{fn();}catch(...){rejected=true;}assert(rejected);};
 assert(newer("1.10.0","1.9.9"));assert(!newer("1.6.0","1.6.0"));assert(!newer("1.5.1","1.6.0"));
 for(auto s:{"1.6","v1.6.0","1.6.0-beta","1.06.0","1.65536.0","-1.0.0","1.0.0/../../evil"})rejects([&]{version(s);});
 json j={{"schema",1},{"app_id",AppId},{"version","1.6.1"},{"url",std::string(Repo)+"/releases/download/v1.6.1/PrintQuoteCalculator.exe"},{"sha256",std::string(64,'a')},{"size",2000000},{"notes",{"Fix startup","Preserve settings"}}};
 auto m=manifest(j);assert(m.version=="1.6.1");
 for(auto key:{"schema","app_id","version","url","sha256","size"}){auto bad=j;bad.erase(key);rejects([&]{manifest(bad);});}
 for(auto url:{"http://github.com/TechSavage1989/Print-Quote-Calculator/releases/download/v1.6.1/PrintQuoteCalculator.exe","https://github.com.evil.test/a.exe","https://github.com/Other/Repo/releases/download/v1.6.1/PrintQuoteCalculator.exe","https://github.com/TechSavage1989/Print-Quote-Calculator/releases/download/v1.6.1/update.zip","https://github.com/TechSavage1989/Print-Quote-Calculator/releases/download/v1.6.0/PrintQuoteCalculator.exe"}){auto bad=j;bad["url"]=url;rejects([&]{manifest(bad);});}
 for(auto size:{0ll,-1ll,1023ll,33554433ll}){auto bad=j;bad["size"]=size;rejects([&]{manifest(bad);});}
 for(auto hash:{std::string("abc"),std::string(64,'z'),std::string(63,'a')}){auto bad=j;bad["sha256"]=hash;rejects([&]{manifest(bad);});}
 {auto bad=j;bad["notes"]={1};rejects([&]{manifest(bad);});bad=j;bad["release_url"]="https://evil.test";rejects([&]{manifest(bad);});}
 Policy p;assert(p.due(1000,"startup"));p.lastAttempt=1000;assert(!p.due(1059,"manual"));assert(p.due(1060,"manual"));assert(!p.due(1119,"focus"));assert(p.due(1120,"focus"));assert(!p.due(1299,"timer"));assert(p.due(1300,"timer"));
 p.failed(1000,900);assert(p.retryAt==1900);assert(!p.due(1899,"manual"));assert(p.due(1900,"manual"));p.success(1900);assert(p.failures==0&&p.retryAt==0&&p.lastSuccess==1900);
 for(int i=0;i<20;i++)p.failed(2000);assert(p.retryAt==5600);assert(!p.due(999,"focus"));
 std::cout<<"PASS: release schema, numeric versions, repository/asset allowlist, digest/size bounds, malformed notes, scheduling, cooldown and retry/backoff.\n";
}
