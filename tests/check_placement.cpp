#include <nlohmann/json.hpp>
#include <algorithm>
#include <cassert>
#include <iostream>
using json=nlohmann::json;using LONG=long;using HWND=void*;using HMONITOR=void*;
struct RECT{LONG left=0,top=0,right=0,bottom=0;};
struct POINT{LONG x=0,y=0;};
struct WINDOWPLACEMENT{unsigned length=0,flags=0,showCmd=0;POINT ptMinPosition,ptMaxPosition;RECT rcNormalPosition;};
struct MONITORINFO{unsigned size;RECT rcMonitor,rcWork;unsigned flags;};
constexpr unsigned SW_SHOWNORMAL=1,SW_SHOWMINIMIZED=2,SW_SHOWMAXIMIZED=3,WPF_RESTORETOMAXIMIZED=2,MONITOR_DEFAULTTONEAREST=2;
HWND mainWindow=(HWND)1;WINDOWPLACEMENT actual;int shown=0;RECT work={0,0,1920,1080};
bool IsWindow(HWND h){return h!=nullptr;}
bool GetWindowPlacement(HWND,WINDOWPLACEMENT*p){*p=actual;return true;}
HMONITOR MonitorFromRect(RECT*,int){return (HMONITOR)1;}
bool GetMonitorInfoW(HMONITOR,MONITORINFO*p){p->rcWork=work;return true;}
bool IntersectRect(RECT*out,RECT*a,RECT*b){out->left=std::max(a->left,b->left);out->top=std::max(a->top,b->top);out->right=std::min(a->right,b->right);out->bottom=std::min(a->bottom,b->bottom);if(out->right<=out->left||out->bottom<=out->top){*out={};return false;}return true;}
bool SetWindowPlacement(HWND,WINDOWPLACEMENT*p){actual=*p;return true;}
void ShowWindow(HWND,int cmd){shown=cmd;}
#include "../src/placement.h"
int main(){json config={{"window",{{"x",123},{"y",87},{"width",1150},{"height",740},{"maximized",false}}}};loadPlacement(config);assert(havePlacement);restorePlacement(mainWindow);assert(shown==SW_SHOWNORMAL);assert(actual.rcNormalPosition.left==123&&actual.rcNormalPosition.top==87);assert(actual.rcNormalPosition.right-actual.rcNormalPosition.left==1150);json saved;addPlacement(saved);assert(saved["window"]["x"]==123&&saved["window"]["height"]==740);
actual.showCmd=SW_SHOWMINIMIZED;actual.flags=WPF_RESTORETOMAXIMIZED;capturePlacement();restorePlacement(mainWindow);assert(shown==SW_SHOWMAXIMIZED);
config["window"]["x"]=3500;config["window"]["y"]=2000;config["window"]["maximized"]=false;loadPlacement(config);restorePlacement(mainWindow);assert(actual.rcNormalPosition.right<=work.right&&actual.rcNormalPosition.bottom<=work.bottom);assert(actual.rcNormalPosition.left>=work.left&&actual.rcNormalPosition.top>=work.top);
config["window"]["width"]=5000;config["window"]["height"]=3000;loadPlacement(config);restorePlacement(mainWindow);assert(actual.rcNormalPosition.right-actual.rcNormalPosition.left<=1920);assert(actual.rcNormalPosition.bottom-actual.rcNormalPosition.top<=1080);
config["window"]["width"]=-10;loadPlacement(config);assert(!havePlacement);restorePlacement(mainWindow);assert(shown==SW_SHOWMAXIMIZED);
std::cout<<"PASS: normal position/size persistence, maximized/minimized handling, missing-monitor recovery, smaller-screen recovery, and invalid preference fallback.\n";}
