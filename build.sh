#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
python3 build_assets.py
"${WINDRES:-x86_64-w64-mingw32-windres}" app.rc -O coff -o app.o
"${CXX:-x86_64-w64-mingw32-g++}" -std=c++17 -Os -s -static -static-libgcc -static-libstdc++ -municode -mwindows -Ivendor main.cpp app.o -o PrintQuoteCalculator.exe -lcomdlg32 -lole32 -lshell32 -lshlwapi -ldwmapi -lcrypt32 -luuid -lwinhttp -lbcrypt -lversion -Wl,--nxcompat,--dynamicbase
