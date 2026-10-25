#pragma once
#include "update_core.h"
#include <winhttp.h>
#include <bcrypt.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <memory>
#include <winver.h>

namespace updates {
namespace fs=std::filesystem;
constexpr UINT ResultMessage=WM_APP+80, ProgressMessage=WM_APP+81, TimerId=80;
inline int64_t now(){return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();}
inline std::string textFile(const fs::path&p,size_t limit=65536){if(!fs::is_regular_file(p)||fs::file_size(p)>limit)throw std::runtime_error("Missing or oversized update file.");std::ifstream in(p,std::ios::binary);return std::string(std::istreambuf_iterator<char>(in),{});}
inline void writeJson(const fs::path&p,const json&j){fs::create_directories(p.parent_path());auto tmp=p;tmp+=L".tmp";{std::ofstream f(tmp,std::ios::binary);f<<j.dump(2);f.close();if(!f)throw std::runtime_error("Could not write update state.");}if(!MoveFileExW(tmp.c_str(),p.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Could not commit update state.");}
inline fs::path exePath(){wchar_t p[32768];DWORD n=GetModuleFileNameW(nullptr,p,32768);if(!n||n>=32768)throw std::runtime_error("Could not identify the running executable.");return fs::path(p);}
inline std::string token(){unsigned char bytes[16];if(BCryptGenRandom(nullptr,bytes,16,BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0)throw std::runtime_error("Could not create update transaction.");std::ostringstream o;for(auto b:bytes)o<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)b;return o.str();}
inline bool validToken(const std::string&s){return std::regex_match(s,std::regex("[a-f0-9]{32}"));}
inline std::string digest(const fs::path&p){
 BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
 if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("SHA-256 is unavailable.");
 struct Cleanup{BCRYPT_ALG_HANDLE a;BCRYPT_HASH_HANDLE&h;~Cleanup(){if(h)BCryptDestroyHash(h);BCryptCloseAlgorithmProvider(a,0);}}cleanup{alg,hash};
 if(BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)<0)throw std::runtime_error("Could not initialize SHA-256.");
 std::ifstream in(p,std::ios::binary);if(!in)throw std::runtime_error("Could not read downloaded update.");
 char buffer[65536];while(in){in.read(buffer,sizeof(buffer));auto n=in.gcount();if(n&&BCryptHashData(hash,(PUCHAR)buffer,(ULONG)n,0)<0)throw std::runtime_error("Could not hash update.");}
 if(!in.eof())throw std::runtime_error("Could not read complete update.");unsigned char bytes[32];if(BCryptFinishHash(hash,bytes,32,0)<0)throw std::runtime_error("Could not finish SHA-256.");
 std::ostringstream o;for(auto b:bytes)o<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)b;return o.str();
}
inline bool reparse(const fs::path&p){DWORD a=GetFileAttributesW(p.c_str());return a!=INVALID_FILE_ATTRIBUTES&&(a&FILE_ATTRIBUTE_REPARSE_POINT);}
inline void noLinks(const fs::path&p){for(auto q=fs::absolute(p);!q.empty();){if(reparse(q))throw std::runtime_error("Update paths cannot use symbolic links or junctions.");auto parent=q.parent_path();if(parent==q)break;q=parent;}}
inline std::string peVersion(const fs::path&p){
 noLinks(p);std::ifstream in(p,std::ios::binary);IMAGE_DOS_HEADER dos{};in.read((char*)&dos,sizeof(dos));if(!in||dos.e_magic!=IMAGE_DOS_SIGNATURE||dos.e_lfanew<64||dos.e_lfanew>1024*1024)throw std::runtime_error("Update is not a valid Windows executable.");
 in.seekg(dos.e_lfanew);DWORD signature;IMAGE_FILE_HEADER file{};in.read((char*)&signature,4);in.read((char*)&file,sizeof(file));WORD magic;in.read((char*)&magic,2);
 if(!in||signature!=IMAGE_NT_SIGNATURE||file.Machine!=IMAGE_FILE_MACHINE_AMD64||magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC||!(file.Characteristics&IMAGE_FILE_EXECUTABLE_IMAGE)||(file.Characteristics&IMAGE_FILE_DLL)||file.NumberOfSections<1||file.NumberOfSections>96)throw std::runtime_error("Update must be an x64 Windows application.");
 DWORD unused=0,size=GetFileVersionInfoSizeW(p.c_str(),&unused);if(!size||size>1024*1024)throw std::runtime_error("Update application identity is missing.");std::vector<BYTE> data(size);if(!GetFileVersionInfoW(p.c_str(),0,size,data.data()))throw std::runtime_error("Could not read update identity.");
 auto value=[&](const wchar_t*name){LPVOID v=nullptr;UINT n=0;std::wstring key=L"\\StringFileInfo\\040904B0\\";key+=name;if(!VerQueryValueW(data.data(),key.c_str(),&v,&n)||!v||!n)return std::wstring();return std::wstring((wchar_t*)v);};
 if(value(L"ProductName")!=L"Print Quote Calculator"||value(L"OriginalFilename")!=L"PrintQuoteCalculator.exe")throw std::runtime_error("Update belongs to a different application.");
 VS_FIXEDFILEINFO*fixed=nullptr;UINT n=0;if(!VerQueryValueW(data.data(),L"\\",(LPVOID*)&fixed,&n)||n<sizeof(*fixed)||fixed->dwSignature!=0xFEEF04BD||LOWORD(fixed->dwFileVersionLS)!=0)throw std::runtime_error("Invalid executable version.");
 std::string v=std::to_string(HIWORD(fixed->dwFileVersionMS))+"."+std::to_string(LOWORD(fixed->dwFileVersionMS))+"."+std::to_string(HIWORD(fixed->dwFileVersionLS));
 if(value(L"FileVersion")!=wide(v)||fixed->dwProductVersionMS!=fixed->dwFileVersionMS||fixed->dwProductVersionLS!=fixed->dwFileVersionLS)throw std::runtime_error("Executable versions disagree.");return v;
}
inline void validateExe(const fs::path&p,const Manifest&m){if(!fs::is_regular_file(p)||fs::file_size(p)!=m.size||digest(p)!=m.sha256)throw std::runtime_error("Downloaded update size or SHA-256 does not match the release.");if(peVersion(p)!=m.version)throw std::runtime_error("Downloaded executable has the wrong version.");}
inline std::wstring quoteArg(const std::wstring&s){std::wstring o=L"\"";unsigned slashes=0;for(wchar_t c:s){if(c==L'\\'){slashes++;continue;}if(c==L'\"')o.append(slashes*2+1,L'\\');else o.append(slashes,L'\\');slashes=0;o+=c;}o.append(slashes*2,L'\\');return o+L"\"";}
inline PROCESS_INFORMATION launch(const fs::path&exe,const std::vector<std::wstring>&args){std::wstring command=quoteArg(exe.wstring());for(auto&a:args)command+=L" "+quoteArg(a);STARTUPINFOW si{sizeof(si)};PROCESS_INFORMATION pi{};if(!CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,FALSE,0,nullptr,exe.parent_path().c_str(),&si,&pi))throw std::runtime_error("Could not launch the update process.");CloseHandle(pi.hThread);pi.hThread=nullptr;return pi;}
inline bool visibleWindow(DWORD pid){struct Search{DWORD pid;bool found=false;}s{pid};EnumWindows([](HWND h,LPARAM param)->BOOL{auto&s=*(Search*)param;DWORD p=0;GetWindowThreadProcessId(h,&p);wchar_t cls[100];GetClassNameW(h,cls,100);RECT r{};GetWindowRect(h,&r);if(p==s.pid&&IsWindowVisible(h)&&!IsIconic(h)&&r.right>r.left&&r.bottom>r.top&&wcscmp(cls,L"PrintQuoteWebView2")==0){s.found=true;return FALSE;}return TRUE;},(LPARAM)&s);return s.found;}
inline std::wstring processImage(HANDLE h){wchar_t p[32768];DWORD n=32768;if(!QueryFullProcessImageNameW(h,0,p,&n))throw std::runtime_error("Could not verify the application process.");return std::wstring(p,n);}
inline bool samePath(const fs::path&a,const fs::path&b){return _wcsicmp(fs::absolute(a).lexically_normal().c_str(),fs::absolute(b).lexically_normal().c_str())==0;}

struct HttpResult {DWORD status=0;std::string body,etag;int64_t retry=0;};
class Http {
 std::mutex lock;HINTERNET active=nullptr;std::atomic<bool> canceled{false};
 static std::wstring header(HINTERNET h,DWORD id,const wchar_t*name=nullptr){DWORD n=0;WinHttpQueryHeaders(h,id,name,nullptr,&n,WINHTTP_NO_HEADER_INDEX);if(n>65536)return L"";std::wstring out(n/sizeof(wchar_t),0);if(n&&!WinHttpQueryHeaders(h,id,name,out.data(),&n,WINHTTP_NO_HEADER_INDEX))return L"";while(!out.empty()&&!out.back())out.pop_back();return out;}
 static int64_t retryAfter(const std::wstring&s){if(s.empty())return 0;if(s.find_first_not_of(L"0123456789")==std::wstring::npos){try{return std::min<int64_t>(604800,std::stoll(s));}catch(...){return 3600;}}SYSTEMTIME st{};FILETIME ft{};if(WinHttpTimeToSystemTime(s.c_str(),&st)&&SystemTimeToFileTime(&st,&ft)){ULARGE_INTEGER t;t.LowPart=ft.dwLowDateTime;t.HighPart=ft.dwHighDateTime;return std::clamp<int64_t>(int64_t(t.QuadPart/10000000ull)-11644473600ll-now(),0,604800);}return 0;}
public:
 void cancel(){canceled=true;std::lock_guard<std::mutex>g(lock);if(active){WinHttpCloseHandle(active);active=nullptr;}}
 HttpResult get(std::string url,const std::string&etag,const fs::path&output,uint64_t limit,std::function<void(int)>progress={}){
  struct Internet{HINTERNET h=nullptr;~Internet(){if(h)WinHttpCloseHandle(h);}}session,connection;
  session.h=WinHttpOpen(L"PrintQuoteCalculator/1.6.0",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);if(!session.h)throw std::runtime_error("Update check could not start.");WinHttpSetTimeouts(session.h,4000,4000,4000,4000);
  const auto deadline=GetTickCount64()+(output.empty()?25000:180000);
  for(int redirects=0;redirects<5;redirects++){
   if(canceled)throw std::runtime_error("Update operation canceled.");
   auto uri=wide(url);URL_COMPONENTS u{};u.dwStructSize=sizeof(u);u.dwHostNameLength=u.dwUrlPathLength=u.dwExtraInfoLength=u.dwUserNameLength=u.dwPasswordLength=(DWORD)-1;
   if(!WinHttpCrackUrl(uri.c_str(),0,0,&u)||u.nScheme!=INTERNET_SCHEME_HTTPS||u.nPort!=443||u.dwUserNameLength||u.dwPasswordLength)throw std::runtime_error("Update URL must use trusted HTTPS.");
   std::wstring host(u.lpszHostName,u.dwHostNameLength),path(u.lpszUrlPath,u.dwUrlPathLength),extra(u.lpszExtraInfo,u.dwExtraInfoLength);
   if(host!=L"github.com"&&host!=L"release-assets.githubusercontent.com")throw std::runtime_error("Update redirect left the allowed hosts.");
   if(host==L"github.com"&&url!=Feed&&!std::regex_match(url,std::regex("https://github\\.com/TechSavage1989/Print-Quote-Calculator/releases/download/v[0-9]+\\.[0-9]+\\.[0-9]+/(update\\.json|PrintQuoteCalculator\\.exe)")))throw std::runtime_error("Update URL left the repository asset allowlist.");
   if(host==L"release-assets.githubusercontent.com"&&path.rfind(L"/github-production-release-asset/",0)!=0)throw std::runtime_error("Invalid GitHub release asset path.");
   if(connection.h){WinHttpCloseHandle(connection.h);connection.h=nullptr;}connection.h=WinHttpConnect(session.h,host.c_str(),443,0);if(!connection.h)throw std::runtime_error("Cannot connect to the update service.");
   HINTERNET request=WinHttpOpenRequest(connection.h,L"GET",(path+extra).c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE);if(!request)throw std::runtime_error("Could not create update request.");
   {std::lock_guard<std::mutex>g(lock);if(canceled){WinHttpCloseHandle(request);throw std::runtime_error("Canceled.");}active=request;}
   struct RequestGuard{Http*self;HINTERNET h;~RequestGuard(){std::lock_guard<std::mutex>g(self->lock);if(self->active==h){WinHttpCloseHandle(h);self->active=nullptr;}}}guard{this,request};
   DWORD never=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;WinHttpSetOption(request,WINHTTP_OPTION_REDIRECT_POLICY,&never,sizeof(never));
   // No tokens, cookies, or credentials are sent. Conditional headers contain only a bounded server ETag.
   std::wstring headers=L"Accept: application/octet-stream\r\n";if(!etag.empty()&&etag.size()<512&&etag.find_first_of("\r\n")==std::string::npos)headers+=L"If-None-Match: "+wide(etag)+L"\r\n";
   if(!WinHttpSendRequest(request,headers.c_str(),headers.size(),WINHTTP_NO_REQUEST_DATA,0,0,0)||!WinHttpReceiveResponse(request,nullptr))throw std::runtime_error("Could not check for updates. You may be offline.");
   HttpResult result;DWORD n=sizeof(result.status);if(!WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,nullptr,&result.status,&n,WINHTTP_NO_HEADER_INDEX))throw std::runtime_error("Invalid update response.");
   if(result.status>=300&&result.status<=399&&result.status!=304){auto location=header(request,WINHTTP_QUERY_LOCATION);if(location.empty())throw std::runtime_error("Missing update redirect.");if(location.front()==L'/')location=L"https://"+host+location;url=utf8(location);continue;}
   result.etag=utf8(header(request,WINHTTP_QUERY_ETAG));if(result.etag.size()>512)result.etag.clear();result.retry=retryAfter(header(request,WINHTTP_QUERY_CUSTOM,L"Retry-After"));
   if(result.status!=200)return result;
   const auto length=header(request,WINHTTP_QUERY_CONTENT_LENGTH);uint64_t expected=0;if(!length.empty()){try{expected=std::stoull(length);}catch(...){throw std::runtime_error("Invalid response length.");}if(expected>limit)throw std::runtime_error("Update response exceeds its size limit.");}
   std::ofstream file;if(!output.empty()){file.open(output,std::ios::binary|std::ios::trunc);if(!file)throw std::runtime_error("Could not create the update download.");}
   uint64_t total=0;char buffer[65536];DWORD got=0;while(true){if(canceled||GetTickCount64()>deadline)throw std::runtime_error("Update operation timed out or was canceled.");if(!WinHttpReadData(request,buffer,sizeof(buffer),&got))throw std::runtime_error("Update download was interrupted.");if(!got)break;total+=got;if(total>limit)throw std::runtime_error("Update response is too large.");if(output.empty())result.body.append(buffer,got);else{file.write(buffer,got);if(!file)throw std::runtime_error("Could not write update download.");if(progress)progress((int)(total*100/limit));}}
   if(expected&&total!=expected)throw std::runtime_error("Update download is incomplete.");if(file.is_open()){file.close();if(!file)throw std::runtime_error("Could not finish update download.");}return result;
  }throw std::runtime_error("Too many update redirects.");
 }
};

class Manager {
 HWND window;fs::path root;std::thread worker;std::shared_ptr<Http> http;std::mutex deliveryLock;json delivery;std::atomic<int> percent{0};
 bool busy=false,downloaded=false;Policy policy;json cached;std::string etag;fs::path transaction;std::function<void(const json&)>notify;HANDLE installerReady=nullptr,helperProcess=nullptr;
 json state={{"installed",Version},{"latest",""},{"phase","idle"},{"message","Updates have not been checked."},{"notes",json::array()}};
 void emit(){state["canDownload"]=!cached.is_null()&&newer(manifest(cached).version)&&!downloaded;state["canInstall"]=downloaded;notify(state);}
 void saveCache(){try{writeJson(root/L"check-cache.json",{{"lastAttempt",policy.lastAttempt},{"lastSuccess",policy.lastSuccess},{"retryAt",policy.retryAt},{"failures",policy.failures},{"etag",etag},{"manifest",cached}});}catch(...) {}}
 template<class Fn>void start(Fn fn){busy=true;if(worker.joinable())worker.join();http=std::make_shared<Http>();worker=std::thread([this,fn]{json result;try{result=fn();}catch(const std::exception&e){result={{"error",e.what()}};} {std::lock_guard<std::mutex>g(deliveryLock);delivery=std::move(result);}PostMessageW(window,ResultMessage,0,0);});}
public:
 Manager(HWND h,fs::path p,std::function<void(const json&)>cb):window(h),root(p),notify(cb){
  fs::create_directories(root);noLinks(root);try{auto j=json::parse(textFile(root/L"check-cache.json"));policy.lastAttempt=j.value("lastAttempt",0ll);policy.lastSuccess=j.value("lastSuccess",0ll);policy.retryAt=j.value("retryAt",0ll);policy.failures=std::min(j.value("failures",0u),8u);etag=j.value("etag","");cached=j.value("manifest",json());if(!cached.is_null()){auto m=manifest(cached);state["latest"]=m.version;state["notes"]=m.notes;state["release_url"]=m.release;state["phase"]=newer(m.version)?"available":"idle";state["message"]="Cached release information. Checking when allowed.";}}catch(...){cached=nullptr;etag.clear();}
 }
 ~Manager(){stop();}
 void stop(){if(http)http->cancel();if(worker.joinable())worker.join();KillTimer(window,TimerId);if(installerReady){CloseHandle(installerReady);installerReady=nullptr;}if(helperProcess){CloseHandle(helperProcess);helperProcess=nullptr;}}
 void refresh(){emit();}
 bool active()const{return busy;}
 bool installing()const{return installerReady!=nullptr;}
 void check(const std::string&reason){
  if(busy||installerReady)return;auto t=now();if(!policy.due(t,reason)){if(reason=="manual"){state["message"]="Please wait "+std::to_string(std::max<int64_t>(1,std::max(policy.retryAt,policy.lastAttempt+60)-t))+" seconds before checking again.";emit();}return;}
  policy.lastAttempt=t;saveCache();state["phase"]="checking";state["message"]="Checking for updates...";emit();
  auto old=cached;auto condition=etag;
  start([this,old,condition]{auto r=http->get(Feed,condition,{},65536);if(r.status==304){if(old.is_null())throw std::runtime_error("Update service returned no cached release.");manifest(old);return json{{"kind","check"},{"manifest",old},{"etag",condition}};}if(r.status!=200)return json{{"kind","check"},{"error",r.status==404?"No update release has been published yet.":r.status==429||r.status==403?"Update service is throttling requests. Try again later.":"Update service returned HTTP "+std::to_string(r.status)+"."},{"retry",r.retry}};auto j=json::parse(r.body);manifest(j);return json{{"kind","check"},{"manifest",j},{"etag",r.etag}};});
 }
 void download(){
  if(busy||cached.is_null())return;if(now()<policy.retryAt){state["message"]="Wait "+std::to_string(policy.retryAt-now())+" seconds before retrying the download.";emit();return;}const auto m=manifest(cached);if(!newer(m.version))return;
  transaction=root/wide(token());fs::create_directories(transaction);noLinks(transaction);writeJson(transaction/L"download.json",cached);auto path=transaction/L"payload.exe";state["phase"]="downloading";state["message"]="Downloading update...";downloaded=false;emit();
  start([this,m,path]{auto r=http->get(m.url,"",path,m.size,[this](int p){percent=std::min(p,100);PostMessageW(window,ProgressMessage,0,0);});if(r.status!=200)return json{{"kind","download"},{"error","Update download returned HTTP "+std::to_string(r.status)+"."},{"retry",r.retry}};validateExe(path,m);return json{{"kind","download"}};});
 }
 void progress(){if(busy&&state["phase"]=="downloading"){state["message"]="Downloading update: "+std::to_string(percent.load())+"%";emit();}}
 void receive(){
  if(worker.joinable())worker.join();busy=false;json r;{std::lock_guard<std::mutex>g(deliveryLock);r=delivery;}
  if(r.contains("error")){policy.failed(now(),r.value("retry",0ll));state["phase"]="error";state["message"]=r["error"];saveCache();emit();return;}
  if(r.value("kind","")=="check"){if(cached!=r["manifest"])downloaded=false;cached=r["manifest"];etag=r.value("etag","");policy.success(now());saveCache();auto m=manifest(cached);state["latest"]=m.version;state["notes"]=m.notes;state["release_url"]=m.release;state["phase"]=downloaded?"downloaded":newer(m.version)?"available":"current";state["message"]=downloaded?"Update downloaded and verified.":newer(m.version)?"An update is available.":"You are up to date.";}
  else{downloaded=true;policy.success(now());state["phase"]="downloaded";state["message"]="Update downloaded and verified.";}
  emit();
 }
 bool install(){
  if(busy||installerReady||!downloaded||cached.is_null())return false;auto m=manifest(cached);auto target=exePath();noLinks(target);validateExe(transaction/L"payload.exe",m);
  const auto id=utf8(transaction.filename().wstring());auto helper=transaction/L"helper.exe";fs::copy_file(target,helper,fs::copy_options::overwrite_existing);
  // Keep shortcut/custom-filename targets unchanged. Only this running executable is replaced.
  writeJson(transaction/L"job.json",{{"token",id},{"target",utf8(target.wstring())},{"parent",GetCurrentProcessId()},{"old_sha256",digest(target)},{"manifest",cached}});
  installerReady=CreateEventW(nullptr,TRUE,FALSE,(L"Local\\PrintQuoteInstallerReady-"+wide(id)).c_str());if(!installerReady)throw std::runtime_error("Could not initialize the installer.");
  try{auto pi=launch(helper,{L"--apply-update",wide(id),std::to_wstring(GetCurrentProcessId())});helperProcess=pi.hProcess;}catch(...){CloseHandle(installerReady);installerReady=nullptr;throw;}
  state["phase"]="installing";state["message"]="Preparing the verified update. The app will restart when ready...";emit();return true;
 }
 bool readyToClose(){if(!installerReady)return false;if(WaitForSingleObject(installerReady,0)==WAIT_OBJECT_0)return true;if(fs::exists(transaction/L"result.json")||(helperProcess&&WaitForSingleObject(helperProcess,0)==WAIT_OBJECT_0)){CloseHandle(installerReady);installerReady=nullptr;if(helperProcess){CloseHandle(helperProcess);helperProcess=nullptr;}state["phase"]="error";state["message"]="The installer could not prepare the update. Your current app is still open.";emit();}return false;}
 void error(const std::string&s){state["phase"]="error";state["message"]=s;emit();}
 void openNotes(){if(!cached.is_null()){auto m=manifest(cached);ShellExecuteW(window,L"open",wide(m.release).c_str(),nullptr,nullptr,SW_SHOWNORMAL);}}
 void cleanup(){
  if(busy||installerReady)return;
  // Called after a healthy visible window, then periodically. A live helper owns its transaction.
  try{for(const auto&e:fs::directory_iterator(root)){
   auto id=utf8(e.path().filename().wstring());if(!validToken(id)||!e.is_directory()||reparse(e.path()))continue;auto result=e.path()/L"result.json";
   if(!fs::exists(result)){
    // Abandoned download-only transactions have no helper and cannot be installing.
    if(!samePath(e.path(),transaction)&&fs::exists(e.path()/L"download.json")&&!fs::exists(e.path()/L"job.json")){
     for(auto name:{L"payload.exe",L"download.json"}){auto p=e.path()/name;if(fs::is_regular_file(p)&&!reparse(p))fs::remove(p);}fs::remove(e.path());
    }continue;
   }
   auto j=json::parse(textFile(result));DWORD pid=j.value("helper",0u);if(!pid)continue;HANDLE h=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);if(h){bool alive=WaitForSingleObject(h,0)==WAIT_TIMEOUT;CloseHandle(h);if(alive)continue;}
   auto stage=wide(j.value("stage",""));auto job=json::parse(textFile(e.path()/L"job.json"));auto target=fs::path(wide(job.at("target").get<std::string>()));auto expected=target.parent_path()/(L".PrintQuoteCalculator-"+wide(id)+L".stage");if(!stage.empty()&&samePath(stage,expected)&&!reparse(expected))fs::remove(expected);
   if(j.value("success",false)){state["message"]="Update installed; the new calculator window is healthy.";state["phase"]="current";}else{state["message"]=j.value("error","The update failed and the previous version was restored.");state["phase"]="error";}emit();
   // Only fixed app-owned files; never recursive deletion of arbitrary contents.
   for(auto name:{L"payload.exe",L"helper.exe",L"job.json",L"result.json",L"download.json"}){auto p=e.path()/name;if(fs::is_regular_file(p)&&!reparse(p))fs::remove(p);}fs::remove(e.path());
  }}catch(...){}
 }
};

inline int applyUpdate(const fs::path&root,const std::string&id,DWORD parentPid){
 fs::path tx=root/wide(id),target,stage;bool replaced=false,parentExited=false,attemptedReplace=false;PROCESS_INFORMATION child{};HANDLE parent=nullptr,health=nullptr;std::string error;
 auto backup=root/L"rollback"/L"previous.exe";json job;
 try{
  if(!validToken(id)||!samePath(exePath(),tx/L"helper.exe"))throw std::runtime_error("Invalid update helper location.");noLinks(tx);job=json::parse(textFile(tx/L"job.json"));if(job.at("token")!=id||job.at("parent")!=parentPid)throw std::runtime_error("Update transaction mismatch.");
  target=wide(job.at("target").get<std::string>());noLinks(target);parent=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,parentPid);if(!parent||!samePath(processImage(parent),target))throw std::runtime_error("The update target is not the original running app.");
  auto m=manifest(job.at("manifest"));if(!newer(m.version,Version))throw std::runtime_error("Update version is not newer than the installer.");validateExe(tx/L"payload.exe",m);
  if(digest(target)!=job.at("old_sha256"))throw std::runtime_error("The running application file changed before installation.");
  // Copy to the target volume before replacement; the download/cache volume can differ.
  stage=target.parent_path()/(L".PrintQuoteCalculator-"+wide(id)+L".stage");if(fs::exists(stage))throw std::runtime_error("Update staging path already exists.");fs::copy_file(tx/L"payload.exe",stage);validateExe(stage,m);
  HANDLE ready=OpenEventW(EVENT_MODIFY_STATE,FALSE,(L"Local\\PrintQuoteInstallerReady-"+wide(id)).c_str());if(!ready)throw std::runtime_error("The parent app is no longer waiting for installation.");SetEvent(ready);CloseHandle(ready);
  if(WaitForSingleObject(parent,30000)!=WAIT_OBJECT_0)throw std::runtime_error("The calculator did not close. Installation was canceled.");CloseHandle(parent);parent=nullptr;parentExited=true;
  fs::create_directories(backup.parent_path());noLinks(backup.parent_path());noLinks(backup);auto next=backup;next+=L".new";noLinks(next);fs::copy_file(target,next,fs::copy_options::overwrite_existing);if(digest(next)!=job.at("old_sha256"))throw std::runtime_error("Rollback backup verification failed.");if(!MoveFileExW(next.c_str(),backup.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Could not retain the rollback backup.");
  attemptedReplace=true;if(!ReplaceFileW(target.c_str(),stage.c_str(),nullptr,0,nullptr,nullptr))throw std::runtime_error("Could not replace the application. Check folder permissions or file locks.");replaced=true;
  auto event=L"Local\\PrintQuoteUpdateHealth-"+wide(id);health=CreateEventW(nullptr,TRUE,FALSE,event.c_str());if(!health)throw std::runtime_error("Could not create update health check.");
  child=launch(target,{L"--update-health",wide(id)});HANDLE waits[]={health,child.hProcess};auto result=WaitForMultipleObjects(2,waits,FALSE,60000);
  if(result!=WAIT_OBJECT_0||!visibleWindow(child.dwProcessId))throw std::runtime_error("The new calculator did not show a healthy visible window. Restoring the previous version.");
  writeJson(tx/L"result.json",{{"success",true},{"helper",GetCurrentProcessId()},{"stage",utf8(stage.wstring())}});CloseHandle(child.hProcess);CloseHandle(health);return 0;
 }catch(const std::exception&e){error=e.what();}
 if(parent)CloseHandle(parent);if(health)CloseHandle(health);
 try{
  if(attemptedReplace){
   if(child.hProcess){if(WaitForSingleObject(child.hProcess,0)==WAIT_TIMEOUT){EnumWindows([](HWND h,LPARAM pid)->BOOL{DWORD p=0;GetWindowThreadProcessId(h,&p);if(p==(DWORD)pid)PostMessageW(h,WM_CLOSE,0,0);return TRUE;},child.dwProcessId);if(WaitForSingleObject(child.hProcess,5000)==WAIT_TIMEOUT){TerminateProcess(child.hProcess,1);WaitForSingleObject(child.hProcess,5000);}}CloseHandle(child.hProcess);child.hProcess=nullptr;}
   if(!fs::exists(backup)||digest(backup)!=job.at("old_sha256"))throw std::runtime_error("Rollback backup is unavailable or changed.");
   if(fs::exists(stage))fs::remove(stage);fs::copy_file(backup,stage);if(!MoveFileExW(stage.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Could not restore the previous executable.");
  }
  if(validToken(id))writeJson(tx/L"result.json",{{"success",false},{"error",error},{"helper",GetCurrentProcessId()},{"stage",utf8(stage.wstring())}});
  if(parentExited){auto restored=launch(target,{});CloseHandle(restored.hProcess);}
 }catch(const std::exception&e){error+=" Rollback needs attention: ";error+=e.what();}
 MessageBoxW(nullptr,wide(error).c_str(),L"Print Quote Calculator update",MB_OK|MB_ICONERROR);return 1;
}
}
