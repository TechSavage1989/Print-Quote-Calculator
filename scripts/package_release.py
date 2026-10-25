"""Produce only allowlisted release artifacts; never include profile/cache/test data."""
from pathlib import Path
import hashlib, json, re, shutil, zipfile, sys

ROOT = Path(__file__).resolve().parent.parent
VERSION = re.search(r'Version="([0-9.]+)"', (ROOT / 'update_core.h').read_text(encoding="utf-8")).group(1)
OUT = ROOT / 'release'

def main():
    exe = ROOT / 'PrintQuoteCalculator.exe'
    if not exe.is_file() or exe.read_bytes()[:2] != b'MZ':
        raise SystemExit('Build the Windows executable first.')
    if not 1024 <= exe.stat().st_size <= 32 * 1024 * 1024:
        raise SystemExit('Executable exceeds the current updater size limit.')
    rc = (ROOT / 'app.rc').read_text(encoding="utf-8")
    if f'FILEVERSION {VERSION.replace(".", ",")},0' not in rc or f'L"Print Quote Calculator {VERSION}"' not in (ROOT / 'main.cpp').read_text(encoding="utf-8"):
        raise SystemExit('Version mismatch between core, resource and native window.')
    OUT.mkdir(exist_ok=True)
    expected = {'PrintQuoteCalculator.exe', 'PrintQuoteCalculator.html', 'Print_Quote_Icon.svg', 'Print_Quote_Icon.ico', 'Print_Quote_Icon.png', 'update.json', 'SHA256SUMS.txt', f'PrintQuoteCalculator-{VERSION}-Windows-x64.zip'}
    if any(p.name not in expected or not p.is_file() for p in OUT.iterdir()):
        raise SystemExit('Release staging contains unexpected files. Use a clean app-owned release directory.')
    shutil.copy2(exe, OUT / 'PrintQuoteCalculator.exe')
    shutil.copy2(ROOT / '3D_Print_Quote_Calculator_Offline.html', OUT / 'PrintQuoteCalculator.html')
    for name in ['Print_Quote_Icon.svg', 'Print_Quote_Icon.ico', 'Print_Quote_Icon.png']:
        shutil.copy2(ROOT / name, OUT / name)
    notes = ['Background startup, five-minute and focus-return update checks.',
             'Explicit verified download and install/restart actions with rollback.',
             'Preserves Resin, FDM, UV, logo/PDF, converters and saved window placement.',
             'Stable portable executable filename and per-user update staging.']
    manifest = {'schema': 1, 'app_id': 'PrintQuoteCalculator.WebView2', 'version': VERSION,
                'url': f'https://github.com/TechSavage1989/Print-Quote-Calculator/releases/download/v{VERSION}/PrintQuoteCalculator.exe',
                'size': exe.stat().st_size, 'sha256': hashlib.sha256(exe.read_bytes()).hexdigest(),
                'notes': notes, 'release_url': f'https://github.com/TechSavage1989/Print-Quote-Calculator/releases/tag/v{VERSION}'}
    (OUT / 'update.json').write_text(json.dumps(manifest, indent=2) + '\n')
    # ZIP is for a first manual install; the updater accepts only the EXE asset.
    with zipfile.ZipFile(OUT / f'PrintQuoteCalculator-{VERSION}-Windows-x64.zip', 'w', zipfile.ZIP_DEFLATED) as z:
        z.write(OUT / 'PrintQuoteCalculator.exe', 'PrintQuoteCalculator.exe')
        warning = 'LOCAL CANDIDATE: cross-compiled only. Windows launch, hosted CI and real release upgrades are not verified. The update feed is not published yet.\n\n' if '--candidate' in sys.argv else ''
        z.writestr('READ_FIRST.txt', warning + 'Extract and run PrintQuoteCalculator.exe. Windows 10/11 x64 and Microsoft Edge WebView2 Runtime are required. Settings stay in your Windows user profile. This build is not publisher signed.\n')
    files = [p for p in OUT.iterdir() if p.is_file() and p.name != 'SHA256SUMS.txt']
    (OUT / 'SHA256SUMS.txt').write_text(''.join(f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n' for p in sorted(files)))
    print(f'Packaged {VERSION}; executable SHA-256 {manifest["sha256"]}')

if __name__ == '__main__':
    main()
