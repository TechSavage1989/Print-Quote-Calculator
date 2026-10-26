@echo off
setlocal
cd /d "%~dp0"
python scripts/build_assets.py || exit /b 1
windres assets/app.rc -O coff -o app.o || exit /b 1
g++ -std=c++17 -Os -s -static -static-libgcc -static-libstdc++ -municode -mwindows -Ivendor -I. src/main.cpp app.o -o PrintQuoteCalculator.exe -lcomdlg32 -lole32 -lshell32 -lshlwapi -ldwmapi -lcrypt32 -luuid -lwinhttp -lbcrypt -lversion -Wl,--nxcompat,--dynamicbase || exit /b 1
echo Built WebView2 executable and standalone HTML.
