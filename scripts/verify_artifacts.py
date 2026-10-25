from pathlib import Path
import hashlib, json, struct, zipfile

root = Path(__file__).resolve().parent.parent / 'release'
m = json.loads((root / 'update.json').read_text())
assert {p.name for p in root.iterdir()} == {'PrintQuoteCalculator.exe', 'PrintQuoteCalculator.html', 'Print_Quote_Icon.svg', 'Print_Quote_Icon.ico', 'Print_Quote_Icon.png', 'update.json', 'SHA256SUMS.txt', f"PrintQuoteCalculator-{m['version']}-Windows-x64.zip"}
exe = (root / 'PrintQuoteCalculator.exe').read_bytes()
assert len(exe) == m['size'] and hashlib.sha256(exe).hexdigest() == m['sha256']
assert exe[:2] == b'MZ'
offset = struct.unpack_from('<I', exe, 0x3c)[0]
assert exe[offset:offset+4] == b'PE\0\0'
assert struct.unpack_from('<H', exe, offset+4)[0] == 0x8664
assert struct.unpack_from('<H', exe, offset+24)[0] == 0x20b
assert m['url'] == f"https://github.com/TechSavage1989/Print-Quote-Calculator/releases/download/v{m['version']}/PrintQuoteCalculator.exe"
for line in (root / 'SHA256SUMS.txt').read_text().splitlines():
    digest, name = line.split('  ', 1)
    assert '/' not in name and '\\' not in name
    assert hashlib.sha256((root / name).read_bytes()).hexdigest() == digest, name
with zipfile.ZipFile(root / f"PrintQuoteCalculator-{m['version']}-Windows-x64.zip") as z:
    assert set(z.namelist()) == {'PrintQuoteCalculator.exe', 'READ_FIRST.txt'}
    assert z.read('PrintQuoteCalculator.exe') == exe
    assert all(i.file_size < 32 * 1024 * 1024 for i in z.infolist())
assert not any(p.name in {'UserData', 'Updates', 'node_modules'} for p in root.rglob('*'))
print('PASS: exact release bytes, PE x64, manifest, every asset checksum and sanitized portable ZIP.')
