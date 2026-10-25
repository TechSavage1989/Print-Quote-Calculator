// CI-only GUI payload. Never included in a release artifact.
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <shellapi.h>
#include <string>
std::wstring health;
LRESULT CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l){
 if(m==WM_TIMER){
#ifndef FAIL_HEALTH
  if(!health.empty()){auto e=OpenEventW(EVENT_MODIFY_STATE,FALSE,(L"Local\\PrintQuoteUpdateHealth-"+health).c_str());if(e){SetEvent(e);CloseHandle(e);}health.clear();}
#endif
  return 0;
 }if(m==WM_CLOSE){DestroyWindow(h);return 0;}if(m==WM_DESTROY){PostQuitMessage(0);return 0;}return DefWindowProcW(h,m,w,l);
}
int WINAPI wWinMain(HINSTANCE inst,HINSTANCE,PWSTR,int){int n;auto args=CommandLineToArgvW(GetCommandLineW(),&n);if(n==3&&std::wstring(args[1])==L"--update-health")health=args[2];LocalFree(args);WNDCLASSW wc{};wc.hInstance=inst;wc.lpfnWndProc=proc;wc.lpszClassName=L"PrintQuoteWebView2";RegisterClassW(&wc);auto h=CreateWindowW(wc.lpszClassName,L"CI-only calculator update fixture",WS_OVERLAPPEDWINDOW|WS_VISIBLE,80,80,800,600,nullptr,nullptr,inst,nullptr);SetTimer(h,1,1000,nullptr);MSG m;while(GetMessageW(&m,nullptr,0,0)>0){TranslateMessage(&m);DispatchMessageW(&m);}return 0;}
