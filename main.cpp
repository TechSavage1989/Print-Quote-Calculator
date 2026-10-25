#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <wincrypt.h>
#include <wrl/client.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <queue>
#include <vector>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "sdk/WebView2.h"
#ifdef __MINGW32__
#include "sdk/uuid_mingw.h"
#endif

using Microsoft::WRL::ComPtr;
using json=nlohmann::json;
namespace fs=std::filesystem;
HINSTANCE instance;
HWND mainWindow=nullptr;
HMODULE loader=nullptr;
bool closing=false, pageReady=false, handlingFile=false, started=false;
std::wstring configFolder, cacheFolder, status=L"Opening calculator...";
ComPtr<ICoreWebView2Environment> environment;
ComPtr<ICoreWebView2Controller> controller;
ComPtr<ICoreWebView2> view;
std::queue<json> requests;
constexpr UINT ProcessFile=WM_APP+1;
constexpr wchar_t Origin[]=L"https://printquote.local/";

std::wstring wide(const std::string&s){
 if(s.empty())return L"";
 int n=MultiByteToWideChar(CP_UTF8,0,s.data(),(int)s.size(),nullptr,0);
 std::wstring out(n,0);MultiByteToWideChar(CP_UTF8,0,s.data(),(int)s.size(),out.data(),n);return out;
}
std::string utf8(const std::wstring&s){
 if(s.empty())return "";
 int n=WideCharToMultiByte(CP_UTF8,0,s.data(),(int)s.size(),nullptr,0,nullptr,nullptr);
 std::string out(n,0);WideCharToMultiByte(CP_UTF8,0,s.data(),(int)s.size(),out.data(),n,nullptr,nullptr);return out;
}
std::wstring folder(int id){wchar_t path[MAX_PATH]={};SHGetFolderPathW(nullptr,id,nullptr,SHGFP_TYPE_CURRENT,path);return path;}
std::vector<BYTE> resource(int id){
 HRSRC r=FindResourceW(instance,MAKEINTRESOURCEW(id),RT_RCDATA);
 if(!r)throw std::runtime_error("Missing application resource");
 const BYTE* bytes=(const BYTE*)LockResource(LoadResource(instance,r));DWORD size=SizeofResource(instance,r);
 if(!bytes||!size)throw std::runtime_error("Empty application resource");
 return {bytes,bytes+size};
}
bool trusted(const wchar_t*url){return url&&(wcscmp(url,Origin)==0||wcscmp(url,L"https://printquote.local/index.html")==0);}

// Explicit COM callback ownership. No framework or browser runtime is bundled.
template<class Interface,class...Args> class Handler final:public Interface{
 LONG refs=1;
 std::function<HRESULT(Args...)> fn;
public:
 explicit Handler(std::function<HRESULT(Args...)> f):fn(std::move(f)){}
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out)override{
  if(!out)return E_POINTER;*out=nullptr;
  if(IsEqualIID(id,IID_IUnknown)||IsEqualIID(id,__uuidof(Interface))){*out=static_cast<Interface*>(this);AddRef();return S_OK;}return E_NOINTERFACE;
 }
 ULONG STDMETHODCALLTYPE AddRef()override{return InterlockedIncrement(&refs);}
 ULONG STDMETHODCALLTYPE Release()override{auto n=InterlockedDecrement(&refs);if(!n)delete this;return n;}
 HRESULT STDMETHODCALLTYPE Invoke(Args...args)override{try{return fn(args...);}catch(...){return E_FAIL;}}
};
template<class Interface,class...Args,class Fn> ComPtr<Interface> callback(Fn fn){
 ComPtr<Interface> p;p.Attach(new Handler<Interface,Args...>(fn));return p;
}

#include "update_windows.h"
std::unique_ptr<updates::Manager> updater;
std::wstring healthToken;
ULONGLONG readySince=0;
ULONGLONG healthReplyTick=0;
bool navigationHealthy=false,displayFailed=false,updateInstallRequested=false,webOperation=false;
std::string updateStartupError;
#include "placement.h"
void saveWindow(){
 if(!started||closing)return;
 try{json j;addPlacement(j);fs::create_directories(configFolder);auto temp=fs::path(configFolder)/L"window.tmp";
  {std::ofstream out(temp,std::ios::binary);out<<j.dump(2);if(!out)return;}
  MoveFileExW(temp.c_str(),(fs::path(configFolder)/L"window.json").c_str(),MOVEFILE_REPLACE_EXISTING);
 }catch(...){}
}
void loadWindow(){
 for(auto path:{fs::path(configFolder)/L"window.json",fs::path(folder(CSIDL_APPDATA))/L"PrintQuoteCalculatorNative"/L"settings.json"}){
  try{if(!fs::exists(path)||fs::file_size(path)>6*1024*1024)continue;std::ifstream in(path);auto j=json::parse(in);loadPlacement(j);if(havePlacement)return;}catch(...){}
 }
}
std::string initialSettings(){
 // One-time migration is conditional on empty WebView2 localStorage in bridge.js.
 try{
  auto path=fs::path(folder(CSIDL_APPDATA))/L"PrintQuoteCalculatorNative"/L"settings.json";
  if(!fs::exists(path)||fs::file_size(path)>6*1024*1024)return "null";
  std::ifstream in(path);json old=json::parse(in);if(!old.contains("states")||!old["states"].is_object())return "null";
  json seed={{"states",old["states"]},{"mode",old.value("mode","resin")},{"theme",old.value("theme","light")},{"logo",nullptr}};
  if(old.contains("nativeLogo")&&old["nativeLogo"].is_string()){
   fs::path image=wide(old["nativeLogo"].get<std::string>());
   if(fs::exists(image)&&fs::file_size(image)<=5*1024*1024){
    std::ifstream input(image,std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(input)),{});
    DWORD size=0;CryptBinaryToStringA((BYTE*)bytes.data(),bytes.size(),CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,nullptr,&size);
    std::string encoded(size,0);CryptBinaryToStringA((BYTE*)bytes.data(),bytes.size(),CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,encoded.data(),&size);
    while(!encoded.empty()&&!encoded.back())encoded.pop_back();seed["logo"]="data:image/jpeg;base64,"+encoded;
   }
  }
  return seed.dump();
 }catch(...){return "null";}
}
void fail(const wchar_t* message,HRESULT code){
 if(closing)return;wchar_t hex[32];swprintf(hex,32,L" (0x%08lX)",(unsigned long)code);
 status=std::wstring(message)+hex;InvalidateRect(mainWindow,nullptr,TRUE);
 MessageBoxW(mainWindow,(status+L"\n\nClose the calculator and try again. If this continues, repair Microsoft Edge WebView2 Runtime.").c_str(),L"Print Quote Calculator",MB_OK|MB_ICONERROR);
}
bool prepareLoader(){
 // The original signed Microsoft loader is cached once. Only ~160 KB, not a browser.
 auto bytes=resource(202);fs::path path=fs::path(cacheFolder)/L"Loader-1.0.3537.50"/L"WebView2Loader.dll";
 fs::create_directories(path.parent_path());
 bool same=false;
 if(fs::exists(path)&&fs::file_size(path)==bytes.size()){
  std::ifstream in(path,std::ios::binary);std::vector<BYTE> old((std::istreambuf_iterator<char>(in)),{});same=old==bytes;
 }
 if(!same){auto temp=path;temp+=L".tmp";{std::ofstream out(temp,std::ios::binary);out.write((const char*)bytes.data(),bytes.size());if(!out)return false;}
  if(!MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING))return false;
 }
 loader=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);return loader!=nullptr;
}
void reply(const json& result){if(!closing&&view)view->PostWebMessageAsJson(wide(result.dump()).c_str());}
std::wstring savePath(const std::string& suggestion,bool pdf){
 std::wstring name=wide(suggestion).substr(0,100);for(auto&c:name)if(c<32||wcschr(L"<>:\"/\\|?*",c))c=L'_';
 if(name.empty())name=pdf?L"Print_Quote.pdf":L"Print_Quote_Settings.json";
 const std::wstring extension=pdf?L".pdf":L".json";
 if(name.size()<extension.size()||_wcsicmp(name.c_str()+name.size()-extension.size(),extension.c_str())!=0)name+=extension;
 wchar_t path[32768]={};wcsncpy(path,name.c_str(),32767);auto documents=folder(CSIDL_PERSONAL);
 OPENFILENAMEW of={sizeof(of)};of.hwndOwner=mainWindow;of.lpstrFile=path;of.nMaxFile=32768;
 of.lpstrInitialDir=documents.c_str();of.lpstrFilter=pdf?L"PDF document (*.pdf)\0*.pdf\0\0":L"Calculator settings (*.json)\0*.json\0\0";
 of.lpstrDefExt=pdf?L"pdf":L"json";of.lpstrTitle=pdf?L"Save quote PDF":L"Export calculator settings";
 of.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_OVERWRITEPROMPT|OFN_NOCHANGEDIR;
 if(GetSaveFileNameW(&of))return path;
 if(CommDlgExtendedError())throw std::runtime_error("Could not open the Save As dialog.");return L"";
}
void processFile(){
 if(handlingFile||requests.empty()||closing)return;
 handlingFile=true;json job=std::move(requests.front());requests.pop();json result={{"id",job.value("id","")}};
 try{
  bool pdf=job.value("kind","")=="pdf";if(!pdf&&job.value("kind","")!="json")throw std::runtime_error("Unsupported export type.");
  std::string encoded=job.at("data").get<std::string>();if(encoded.empty()||encoded.size()>14*1024*1024)throw std::runtime_error("Export is too large.");
  DWORD size=0;if(!CryptStringToBinaryA(encoded.data(),encoded.size(),CRYPT_STRING_BASE64,nullptr,&size,nullptr,nullptr)||size>10*1024*1024)throw std::runtime_error("Invalid export data.");
  std::vector<BYTE> bytes(size);if(!CryptStringToBinaryA(encoded.data(),encoded.size(),CRYPT_STRING_BASE64,bytes.data(),&size,nullptr,nullptr))throw std::runtime_error("Invalid export data.");
  if(pdf){if(size<5||memcmp(bytes.data(),"%PDF-",5)!=0)throw std::runtime_error("Invalid PDF export.");}
  else{json check=json::parse(bytes.begin(),bytes.end());if(!check.contains("states"))throw std::runtime_error("Invalid settings export.");}
  auto path=savePath(job.value("name",""),pdf);
  if(path.empty())result["canceled"]=true;
  else{
   // Write beside the destination, then atomically replace it after successful output.
   auto temp=fs::path(path);temp+=L".printquote.tmp";
   {std::ofstream out(temp,std::ios::binary);out.write((const char*)bytes.data(),size);out.close();if(!out){fs::remove(temp);throw std::runtime_error("Could not write this file. Choose another folder.");}}
   if(!MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING)){fs::remove(temp);throw std::runtime_error("Could not save this file. Choose another folder.");}
   result["saved"]=true;
   if(pdf&&job.value("open",false)){auto code=(INT_PTR)ShellExecuteW(mainWindow,L"open",path.c_str(),nullptr,nullptr,SW_SHOWNORMAL);if(code<=32)result["openError"]=true;}
  }
 }catch(const std::exception&e){result["error"]=e.what();}
 reply(result);handlingFile=false;if(!requests.empty())PostMessageW(mainWindow,ProcessFile,0,0);
}

void configureView(){
 RECT r;GetClientRect(mainWindow,&r);controller->put_Bounds(r);
 ComPtr<ICoreWebView2Settings> settings;view->get_Settings(&settings);
 if(settings){settings->put_IsStatusBarEnabled(FALSE);settings->put_AreDefaultContextMenusEnabled(FALSE);settings->put_AreDevToolsEnabled(FALSE);settings->put_IsZoomControlEnabled(TRUE);}
 EventRegistrationToken token={};
 auto resourceHandler=callback<ICoreWebView2WebResourceRequestedEventHandler,ICoreWebView2*,ICoreWebView2WebResourceRequestedEventArgs*>([](auto*,auto* args)->HRESULT{
  if(closing)return S_OK;
  ComPtr<ICoreWebView2WebResourceRequest> request;args->get_Request(&request);LPWSTR url=nullptr;request->get_Uri(&url);
  bool local=trusted(url);CoTaskMemFree(url);
  ComPtr<IStream> content;
  if(local){static auto html=resource(201);content.Attach(SHCreateMemStream(html.data(),html.size()));if(!content)return E_OUTOFMEMORY;}
  ComPtr<ICoreWebView2WebResourceResponse> response;
  HRESULT hr=environment->CreateWebResourceResponse(content.Get(),local?200:403,local?L"OK":L"Blocked",
   L"Content-Type: text/html; charset=utf-8\r\nCache-Control: no-store\r\nContent-Security-Policy: default-src 'none'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; img-src data: blob:; font-src data:; connect-src 'none'; frame-src 'none'; object-src 'none'; base-uri 'none'; form-action 'none'\r\n",&response);
  return SUCCEEDED(hr)?args->put_Response(response.Get()):hr;
 });
 view->AddWebResourceRequestedFilter(L"*",COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
 view->add_WebResourceRequested(resourceHandler.Get(),&token);
 auto navigation=callback<ICoreWebView2NavigationStartingEventHandler,ICoreWebView2*,ICoreWebView2NavigationStartingEventArgs*>([](auto*,auto*args)->HRESULT{
  LPWSTR url=nullptr;args->get_Uri(&url);if(!trusted(url))args->put_Cancel(TRUE);CoTaskMemFree(url);return S_OK;
 });view->add_NavigationStarting(navigation.Get(),&token);
 auto popup=callback<ICoreWebView2NewWindowRequestedEventHandler,ICoreWebView2*,ICoreWebView2NewWindowRequestedEventArgs*>([](auto*,auto*args)->HRESULT{args->put_Handled(TRUE);return S_OK;});view->add_NewWindowRequested(popup.Get(),&token);
 auto permission=callback<ICoreWebView2PermissionRequestedEventHandler,ICoreWebView2*,ICoreWebView2PermissionRequestedEventArgs*>([](auto*,auto*args)->HRESULT{args->put_State(COREWEBVIEW2_PERMISSION_STATE_DENY);return S_OK;});view->add_PermissionRequested(permission.Get(),&token);
 auto message=callback<ICoreWebView2WebMessageReceivedEventHandler,ICoreWebView2*,ICoreWebView2WebMessageReceivedEventArgs*>([](auto*,auto*args)->HRESULT{
  LPWSTR source=nullptr;args->get_Source(&source);bool allowed=trusted(source);CoTaskMemFree(source);if(!allowed||closing)return S_OK;
  LPWSTR raw=nullptr;args->get_WebMessageAsJson(&raw);if(!raw)return S_OK;
  std::wstring text(raw);CoTaskMemFree(raw);if(text.size()>15*1024*1024)return S_OK;
  try{auto j=json::parse(utf8(text));std::string type=j.value("type","");
   if(type=="theme"){BOOL dark=j.value("dark",false);DwmSetWindowAttribute(mainWindow,20,&dark,sizeof(dark));}
   else if(type=="ready"){pageReady=true;readySince=GetTickCount64();if(updater)updater->refresh();InvalidateRect(mainWindow,nullptr,TRUE);}
   else if(type=="health"){if(j.value("healthy",false))healthReplyTick=GetTickCount64();}
   else if(type=="update-state"){if(updater)updater->refresh();else reply({{"type","update-state"},{"state",{{"installed",updates::Version},{"latest",""},{"phase","error"},{"message",updateStartupError.empty()?"The updater is not available.":updateStartupError},{"notes",json::array()},{"canDownload",false},{"canInstall",false}}}});}
   else if(type=="update-busy"){webOperation=j.value("busy",false);}
   else if(type=="update-check"){if(updater)updater->check("manual");}
   else if(type=="update-download"){try{if(updater)updater->download();}catch(const std::exception&e){if(updater)updater->error(e.what());}}
   else if(type=="update-notes"){if(updater)updater->openNotes();}
   else if(type=="update-install"){
    if(handlingFile||!requests.empty()||webOperation||!pageReady){if(updater)updater->error("Finish the current operation before installing.");}
    else try{if(updater)updateInstallRequested=updater->install();}catch(const std::exception&e){if(updater)updater->error(e.what());}
   }
   else if(type=="save-file"&&j.contains("id")&&j["id"].is_string()){
    if(!updateInstallRequested&&requests.size()<4){requests.push(std::move(j));PostMessageW(mainWindow,ProcessFile,0,0);}else reply({{"id",j["id"]},{"error","Finish the current export first."}});
   }
  }catch(...){}return S_OK;
 });view->add_WebMessageReceived(message.Get(),&token);
 auto completed=callback<ICoreWebView2NavigationCompletedEventHandler,ICoreWebView2*,ICoreWebView2NavigationCompletedEventArgs*>([](auto*,auto*args)->HRESULT{
  if(closing)return S_OK;BOOL success=FALSE;args->get_IsSuccess(&success);
  if(success){navigationHealthy=true;controller->put_IsVisible(TRUE);controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);}
  else fail(L"The local calculator page could not be loaded",E_FAIL);return S_OK;
 });view->add_NavigationCompleted(completed.Get(),&token);
 auto crashed=callback<ICoreWebView2ProcessFailedEventHandler,ICoreWebView2*,ICoreWebView2ProcessFailedEventArgs*>([](auto*,auto*)->HRESULT{
  displayFailed=true;fail(L"The WebView2 display process stopped",E_FAIL);return S_OK;
 });view->add_ProcessFailed(crashed.Get(),&token);
 std::wstring seed=L"if(location.origin==='https://printquote.local'){window.__PrintQuoteInitial="+wide(initialSettings())+L";}";
 auto injected=callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler,HRESULT,LPCWSTR>([](HRESULT hr,LPCWSTR)->HRESULT{
  if(closing)return S_OK;if(FAILED(hr)){fail(L"Could not initialize saved settings",hr);return hr;}return view->Navigate(Origin);
 });
 HRESULT hr=view->AddScriptToExecuteOnDocumentCreated(seed.c_str(),injected.Get());if(FAILED(hr))fail(L"Could not initialize the calculator",hr);
}
void startWebView(){
 try{
  if(!prepareLoader()){fail(L"Could not load the Microsoft WebView2 loader",HRESULT_FROM_WIN32(GetLastError()));return;}
  using GetVersion=HRESULT(STDAPICALLTYPE*)(PCWSTR,LPWSTR*);
  auto getVersion=(GetVersion)GetProcAddress(loader,"GetAvailableCoreWebView2BrowserVersionString");
  LPWSTR version=nullptr;HRESULT detected=getVersion?getVersion(nullptr,&version):E_NOINTERFACE;
  bool installed=SUCCEEDED(detected)&&version&&version[0];CoTaskMemFree(version);
  if(!installed){
   status=L"Microsoft Edge WebView2 Runtime is required.";InvalidateRect(mainWindow,nullptr,TRUE);
   if(MessageBoxW(mainWindow,L"Microsoft Edge WebView2 Runtime is not installed.\n\nInstall the Evergreen Runtime from Microsoft, then reopen this app.\n\nOpen the Microsoft download page?",L"WebView2 Runtime required",MB_YESNO|MB_ICONINFORMATION)==IDYES)ShellExecuteW(mainWindow,L"open",L"https://developer.microsoft.com/en-us/microsoft-edge/webview2/#download-section",nullptr,nullptr,SW_SHOWNORMAL);
   return;
  }
  using CreateEnvironment=HRESULT(STDAPICALLTYPE*)(PCWSTR,PCWSTR,ICoreWebView2EnvironmentOptions*,ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*);
  auto create=(CreateEnvironment)GetProcAddress(loader,"CreateCoreWebView2EnvironmentWithOptions");
  if(!create){fail(L"The WebView2 loader is not compatible",E_NOINTERFACE);return;}
  auto ready=callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler,HRESULT,ICoreWebView2Environment*>([](HRESULT hr,auto* env)->HRESULT{
   if(closing)return S_OK;
   if(FAILED(hr)||!env){
    if(hr==HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)){
     status=L"Microsoft Edge WebView2 Runtime is required.";InvalidateRect(mainWindow,nullptr,TRUE);
     if(MessageBoxW(mainWindow,L"Microsoft Edge WebView2 Runtime is not installed.\n\nDownload it from Microsoft, install the Evergreen Runtime, then reopen this app.\n\nOpen the Microsoft download page?",L"WebView2 Runtime required",MB_YESNO|MB_ICONINFORMATION)==IDYES)ShellExecuteW(mainWindow,L"open",L"https://developer.microsoft.com/en-us/microsoft-edge/webview2/#download-section",nullptr,nullptr,SW_SHOWNORMAL);
    }else fail(L"Could not start WebView2",hr);return S_OK;
   }
   environment=env;
   auto readyController=callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler,HRESULT,ICoreWebView2Controller*>([](HRESULT result,auto* ctrl)->HRESULT{
    if(closing){if(ctrl)ctrl->Close();return S_OK;}
    if(FAILED(result)||!ctrl){fail(L"Could not create the WebView2 window",result);return S_OK;}
    controller=ctrl;controller->put_IsVisible(FALSE);HRESULT hr=controller->get_CoreWebView2(&view);if(FAILED(hr)){fail(L"Could not initialize the WebView2 page",hr);return hr;}configureView();return S_OK;
   });
   HRESULT result=environment->CreateCoreWebView2Controller(mainWindow,readyController.Get());if(FAILED(result))fail(L"Could not create the calculator window",result);return S_OK;
  });
  auto userData=(fs::path(cacheFolder)/L"UserData").wstring();HRESULT hr=create(nullptr,userData.c_str(),nullptr,ready.Get());if(FAILED(hr))fail(L"Could not start WebView2",hr);
 }catch(const std::exception&e){fail(wide(e.what()).c_str(),E_FAIL);}
}
LRESULT CALLBACK WindowProc(HWND h,UINT message,WPARAM wp,LPARAM lp){
 switch(message){
  case WM_SIZE:if(wp!=SIZE_MINIMIZED){if(controller){RECT r;GetClientRect(h,&r);controller->put_Bounds(r);}if(started)SetTimer(h,1,300,nullptr);}return 0;
  case WM_MOVE:if(controller)controller->NotifyParentWindowPositionChanged();if(started)SetTimer(h,1,300,nullptr);return 0;
  case WM_EXITSIZEMOVE:saveWindow();return 0;
  case WM_TIMER:
   if(wp==1){KillTimer(h,1);saveWindow();}
   else if(wp==updates::TimerId&&updater){
    if(updateInstallRequested&&updater->readyToClose()){PostMessageW(h,WM_CLOSE,0,0);return 0;}
    if(updateInstallRequested&&!updater->installing())updateInstallRequested=false;
    updater->check("timer");
    if(pageReady&&navigationHealthy&&!displayFailed&&IsWindowVisible(h)&&!IsIconic(h)){reply({{"type","health-probe"}});}
    if(pageReady&&navigationHealthy&&!displayFailed&&IsWindowVisible(h)&&!IsIconic(h)&&GetTickCount64()-readySince>=3000&&healthReplyTick&&GetTickCount64()-healthReplyTick<2000){
     if(!healthToken.empty()){auto name=L"Local\\PrintQuoteUpdateHealth-"+healthToken;HANDLE event=OpenEventW(EVENT_MODIFY_STATE,FALSE,name.c_str());if(event){SetEvent(event);CloseHandle(event);}healthToken.clear();}
     updater->cleanup();
    }
   }return 0;
  case updates::ResultMessage:if(updater)updater->receive();return 0;
  case updates::ProgressMessage:if(updater)updater->progress();return 0;
  case WM_ACTIVATE:if(LOWORD(wp)!=WA_INACTIVE&&updater)updater->check("focus");return 0;
  case WM_DPICHANGED:{RECT*r=(RECT*)lp;SetWindowPos(h,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);return 0;}
  case WM_GETMINMAXINFO:{auto p=(MINMAXINFO*)lp;UINT dpi=GetDpiForWindow(h);p->ptMinTrackSize={MulDiv(760,dpi,96),MulDiv(540,dpi,96)};return 0;}
  case WM_SETFOCUS:if(controller)controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);return 0;
  case ProcessFile:processFile();return 0;
  case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);RECT r;GetClientRect(h,&r);FillRect(dc,&r,(HBRUSH)GetStockObject(WHITE_BRUSH));if(!pageReady){SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(70,75,90));auto font=(HFONT)GetStockObject(DEFAULT_GUI_FONT);auto old=SelectObject(dc,font);DrawTextW(dc,status.c_str(),-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);SelectObject(dc,old);}EndPaint(h,&ps);return 0;}
  case WM_ERASEBKGND:return TRUE;
  case WM_CLOSE:saveWindow();closing=true;if(updater)updater->stop();KillTimer(h,1);if(controller)controller->Close();view.Reset();controller.Reset();environment.Reset();DestroyWindow(h);return 0;
  case WM_DESTROY:PostQuitMessage(0);return 0;
 }return DefWindowProcW(h,message,wp,lp);
}
int WINAPI wWinMain(HINSTANCE inst,HINSTANCE,PWSTR,int){
 instance=inst;
 configFolder=(fs::path(folder(CSIDL_APPDATA))/L"PrintQuoteCalculatorWebView2").wstring();
 cacheFolder=(fs::path(folder(CSIDL_LOCAL_APPDATA))/L"PrintQuoteCalculatorWebView2").wstring();
 int argc=0;LPWSTR*argv=CommandLineToArgvW(GetCommandLineW(),&argc);
 if(argc==4&&wcscmp(argv[1],L"--apply-update")==0){int result=1;try{result=updates::applyUpdate(fs::path(cacheFolder)/L"Updates",utf8(argv[2]),std::stoul(argv[3]));}catch(...){MessageBoxW(nullptr,L"Invalid update helper arguments.",L"Print Quote Calculator",MB_OK|MB_ICONERROR);}LocalFree(argv);return result;}
 if(argc==3&&wcscmp(argv[1],L"--update-health")==0&&updates::validToken(utf8(argv[2])))healthToken=argv[2];LocalFree(argv);
 SetCurrentProcessExplicitAppUserModelID(L"PrintQuote.Calculator.WebView2");
 HANDLE mutex=CreateMutexW(nullptr,FALSE,L"Local\\PrintQuoteCalculator.WebView2");
 if(GetLastError()==ERROR_ALREADY_EXISTS){if(auto h=FindWindowW(L"PrintQuoteWebView2",nullptr)){if(IsIconic(h))ShowWindow(h,SW_RESTORE);SetForegroundWindow(h);}if(mutex)CloseHandle(mutex);return 0;}
 HRESULT com=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);if(FAILED(com))return 1;
 configFolder=(fs::path(folder(CSIDL_APPDATA))/L"PrintQuoteCalculatorWebView2").wstring();
 cacheFolder=(fs::path(folder(CSIDL_LOCAL_APPDATA))/L"PrintQuoteCalculatorWebView2").wstring();loadWindow();
 WNDCLASSEXW wc={sizeof(wc)};wc.hInstance=inst;wc.hIcon=LoadIconW(inst,MAKEINTRESOURCEW(101));wc.hIconSm=(HICON)LoadImageW(inst,MAKEINTRESOURCEW(101),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.lpszClassName=L"PrintQuoteWebView2";wc.lpfnWndProc=WindowProc;RegisterClassExW(&wc);
 mainWindow=CreateWindowExW(0,wc.lpszClassName,L"Print Quote Calculator 1.6.0",WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,CW_USEDEFAULT,CW_USEDEFAULT,1280,800,nullptr,nullptr,inst,nullptr);
 if(!mainWindow){CoUninitialize();if(mutex)CloseHandle(mutex);return 1;}
 restorePlacement(mainWindow);started=true;UpdateWindow(mainWindow);
 try{updater=std::make_unique<updates::Manager>(mainWindow,fs::path(cacheFolder)/L"Updates",[](const json&state){reply({{"type","update-state"},{"state",state}});});
  if(!SetTimer(mainWindow,updates::TimerId,1000,nullptr))throw std::runtime_error("Could not bind the update timer to the calculator window.");updater->check("startup");
 }catch(const std::exception&e){updater.reset();updateStartupError=std::string("Updates unavailable: ")+e.what();}
 startWebView();
 MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}
 // Keep the tiny loader mapped until process exit so asynchronous COM teardown is safe.
 updater.reset();CoUninitialize();if(mutex)CloseHandle(mutex);return (int)msg.wParam;
}
