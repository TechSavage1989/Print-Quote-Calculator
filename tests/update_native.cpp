// Reuse the production host/updater. This console test is never shipped.
#define wWinMain ProductionEntry
#include "../main.cpp"
#undef wWinMain
#include <iostream>
#include <cassert>
using namespace updates;
void pump(unsigned milliseconds){auto until=GetTickCount64()+milliseconds;MSG m;while(GetTickCount64()<until){while(PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageW(&m);}Sleep(10);}}
void closeProcessWindows(DWORD pid){EnumWindows([](HWND h,LPARAM p)->BOOL{DWORD owner;GetWindowThreadProcessId(h,&owner);if(owner==(DWORD)p)PostMessageW(h,WM_CLOSE,0,0);return TRUE;},pid);}
int ticks=0,focusEvents=0;Manager*probeManager=nullptr;
LRESULT CALLBACK probe(HWND h,UINT m,WPARAM w,LPARAM l){if(m==WM_TIMER){ticks++;if(probeManager)probeManager->check("timer");return 0;}if(m==WM_ACTIVATE&&LOWORD(w)!=WA_INACTIVE){focusEvents++;if(probeManager)probeManager->check("focus");}if(m==ResultMessage){if(probeManager)probeManager->receive();return 0;}return DefWindowProcW(h,m,w,l);}
int wmain(int argc,wchar_t**argv){
 try{
  if(argc==5&&std::wstring(argv[1])==L"--test-apply")return applyUpdate(argv[2],utf8(argv[3]),std::stoul(argv[4]));
  if(argc==5&&std::wstring(argv[1])==L"--test-parent"){
   fs::path root=argv[2];std::string id=utf8(argv[3]);fs::path fixture=argv[4],tx=root/wide(id);fs::create_directories(tx);auto target=exePath();fs::copy_file(target,tx/L"helper.exe");fs::copy_file(fixture,tx/L"payload.exe");
   json release={{"schema",1},{"app_id",AppId},{"version","1.6.1"},{"url",std::string(Repo)+"/releases/download/v1.6.1/PrintQuoteCalculator.exe"},{"size",fs::file_size(fixture)},{"sha256",digest(fixture)}};
   writeJson(tx/L"job.json",{{"token",id},{"target",utf8(target.wstring())},{"parent",GetCurrentProcessId()},{"old_sha256",digest(target)},{"manifest",release}});
   auto event=CreateEventW(nullptr,TRUE,FALSE,(L"Local\\PrintQuoteInstallerReady-"+wide(id)).c_str());auto helper=launch(tx/L"helper.exe",{L"--test-apply",root.wstring(),wide(id),std::to_wstring(GetCurrentProcessId())});
   auto r=WaitForSingleObject(event,20000);CloseHandle(event);CloseHandle(helper.hProcess);if(r!=WAIT_OBJECT_0)return 4;return 0;
  }
  // The rolled-back test parent has no arguments. Do not recursively run the suite.
  if(exePath().parent_path().filename().wstring().rfind(L"case-",0)==0)return 0;
  instance=GetModuleHandleW(nullptr);auto root=fs::temp_directory_path()/(L"PQC-native-"+wide(token()));fs::create_directories(root);
  auto input=exePath().parent_path();auto good=input/L"update_fixture.exe",bad=input/L"update_fixture_unhealthy.exe";
  assert(peVersion(good)=="1.6.1");assert(digest(good).size()==64);
  {auto corrupt=root/L"invalid.exe";std::ofstream(corrupt)<<"MZnot-an-exe";bool rejected=false;try{peVersion(corrupt);}catch(...){rejected=true;}assert(rejected);}
  {
   auto fail=[](auto fn){bool rejected=false;try{fn();}catch(...){rejected=true;}assert(rejected);};
   Manifest m{"1.6.1","",digest(good),"",fs::file_size(good),json::array()};validateExe(good,m);auto wrong=m;wrong.sha256=std::string(64,'0');fail([&]{validateExe(good,wrong);});wrong=m;wrong.version="1.6.2";fail([&]{validateExe(good,wrong);});
   auto bytes=textFile(good,MaxExe);auto at=bytes.find(std::string("P\0r\0i\0n\0t\0 \0Q\0u\0o\0t\0e\0 \0C\0a\0l\0c\0u\0l\0a\0t\0o\0r\0",42));assert(at!=std::string::npos);bytes[at]='X';auto different=root/L"wrong-identity.exe";std::ofstream(different,std::ios::binary).write(bytes.data(),bytes.size());fail([&]{peVersion(different);});
   auto archive=root/L"archive.exe";std::ofstream(archive,std::ios::binary)<<std::string(2048,'P');auto archiveManifest=m;archiveManifest.size=fs::file_size(archive);archiveManifest.sha256=digest(archive);fail([&]{validateExe(archive,archiveManifest);});
  }
  for(auto scenario:{"healthy","unhealthy"}){
   auto area=root/(std::string("case-")+scenario);fs::create_directories(area);auto target=area/L"Custom Calculator Name.exe";fs::copy_file(exePath(),target);auto before=digest(target);auto storage=root/(std::string("storage-")+scenario);fs::create_directories(storage);std::ofstream(storage/L"settings-sentinel.json")<<"preserve-me";auto id=token();
   auto parent=launch(target,{L"--test-parent",storage.wstring(),wide(id),(std::string(scenario)=="healthy"?good:bad).wstring()});auto tx=storage/wide(id);json result;auto deadline=GetTickCount64()+100000;
   while(GetTickCount64()<deadline){pump(100);if(fs::exists(tx/L"result.json")){result=json::parse(textFile(tx/L"result.json"));break;}}
   assert(!result.is_null());DWORD helper=result.at("helper");HANDLE finished=OpenProcess(SYNCHRONIZE,FALSE,helper);if(finished){auto end=GetTickCount64()+10000;while(WaitForSingleObject(finished,0)==WAIT_TIMEOUT&&GetTickCount64()<end){closeProcessWindows(helper);pump(100);}assert(WaitForSingleObject(finished,0)==WAIT_OBJECT_0);CloseHandle(finished);}assert(result.at("success")== (std::string(scenario)=="healthy"));
   assert(textFile(storage/L"settings-sentinel.json")=="preserve-me");assert(fs::exists(storage/L"rollback"/L"previous.exe"));assert(digest(storage/L"rollback"/L"previous.exe")==before);
   if(std::string(scenario)=="healthy")assert(peVersion(target)=="1.6.1");else assert(digest(target)==before);
   EnumWindows([](HWND h,LPARAM p)->BOOL{DWORD pid;GetWindowThreadProcessId(h,&pid);HANDLE ph=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);if(ph){try{if(samePath(processImage(ph),*(fs::path*)p))PostMessageW(h,WM_CLOSE,0,0);}catch(...){}CloseHandle(ph);}return TRUE;},(LPARAM)&target);pump(1000);CloseHandle(parent.hProcess);
   Manager cleanup(nullptr,storage,[](const json&){});cleanup.cleanup();assert(!fs::exists(tx));assert(!fs::exists(area/(L".PrintQuoteCalculator-"+wide(id)+L".stage")));
   std::cout<<"PASS native install/rollback, custom launch filename, cross-directory staging, one backup, sentinel preservation and cleanup: "<<scenario<<std::endl;
  }
  WNDCLASSW wc{};wc.hInstance=instance;wc.lpfnWndProc=probe;wc.lpszClassName=L"PQCUpdateTimerProbe";RegisterClassW(&wc);auto h=CreateWindowW(wc.lpszClassName,L"Update timer probe",WS_OVERLAPPEDWINDOW|WS_VISIBLE,80,80,400,250,nullptr,nullptr,instance,nullptr);auto other=CreateWindowW(wc.lpszClassName,L"Focus probe",WS_OVERLAPPEDWINDOW|WS_VISIBLE,500,80,400,250,nullptr,nullptr,instance,nullptr);
  // Negative control: an unbound timer does not reach the window procedure.
  ticks=0;auto unbound=SetTimer(nullptr,TimerId,25,nullptr);pump(150);assert(ticks==0);KillTimer(nullptr,unbound);
  SetTimer(h,TimerId,25,nullptr);pump(150);assert(ticks>0);KillTimer(h,TimerId);
  int checks=0;json last;Manager manager(h,root/L"network",[&](const json&s){last=s;if(s.value("phase","")=="checking")checks++;});probeManager=&manager;
  // Real network, HWND timers, and elapsed wall time, without a fake clock.
  manager.check("startup");manager.check("manual");assert(checks==1);SetTimer(h,TimerId,1000,nullptr);
  auto begin=GetTickCount64();bool focused=false;while(GetTickCount64()-begin<435000){pump(100);auto elapsed=GetTickCount64()-begin;if(elapsed>125000&&!focused){SetForegroundWindow(other);SetActiveWindow(other);pump(100);SetForegroundWindow(h);SetActiveWindow(h);focused=true;}}
  auto finish=GetTickCount64()+60000;while(manager.active()&&GetTickCount64()<finish)pump(100);
  assert(focusEvents>0);assert(checks>=3&&checks<=4);assert(last.value("phase","")!="checking");manager.stop();probeManager=nullptr;DestroyWindow(h);DestroyWindow(other);
  // A failed/absent feed must never be represented as a successful current check.
  if(last.value("phase","")=="error")assert(last.value("message","").find("up to date")==std::string::npos);
  std::cout<<"PASS real native timers, unbound negative control, startup single flight, focus return after two minutes, five-minute polling and real HTTPS response handling."<<std::endl;
  fs::remove_all(root);return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}
}
