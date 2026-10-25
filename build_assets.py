"""Build the embedded WebView2 page and the standalone browser edition offline."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parent

def bundle(desktop=False):
    html = (ROOT / 'web/index.html').read_text()
    license_text = (ROOT / 'web/vendor/pdf-lib.LICENSE.md').read_text()
    html = html.replace('</head>', '<!-- Bundled pdf-lib license:\n' + license_text.replace('--', '- -') + '\n--></head>')
    for name in ['style.css', 'compact.css']:
        html = html.replace(f'<link rel="stylesheet" href="{name}">', '<style>' + (ROOT / 'web' / name).read_text() + '</style>')
    for name in ['calculator.js', 'vendor/pdf-lib.min.js', 'export.js', 'converters.js', 'app.js']:
        code = (ROOT / 'web' / name).read_text()
        code = re.sub(r'//# sourceMappingURL=.*', '', code)
        if desktop and name == 'app.js':
            code = code.replace('exportBusy=true;', "exportBusy=true;window.desktopAPI.operation('export',true);")
            code = code.replace('finally{exportBusy=false;}', "finally{exportBusy=false;window.desktopAPI.operation('export',false);}")
            code = code.replace("logoBusy=true;", "logoBusy=true;window.desktopAPI.operation('logo',true);")
            code = code.replace("logoBusy=false;$('remove-logo')", "logoBusy=false;window.desktopAPI.operation('logo',false);$('remove-logo')")
            code = code.replace("if(!file)return;try{if(file.size", "if(!file)return;window.desktopAPI.operation('import',true);try{if(file.size")
            code = code.replace("finally{$('settings-file').value='';}", "finally{window.desktopAPI.operation('import',false);$('settings-file').value='';}")
            old = "if(print)tab=window.open('about:blank','_blank');"
            assert code.count(old) == 1
            code = code.replace(old, "if(print&&!window.desktopAPI)tab=window.open('about:blank','_blank');")
            old = "if(print&&tab){"
            assert code.count(old) == 1
            code = code.replace(old, """if(window.desktopAPI){
 const name=(s.name||type+'-print-quote').replace(/[^a-z0-9_-]/gi,'_').slice(0,80)+'.pdf';
 const result=await window.desktopAPI.savePDF(bytes,name,print);
 $('export-status').textContent=result.canceled?'Export canceled.':result.openError?'PDF saved. Open the saved file to print.':print?'PDF saved and opened in your PDF viewer for printing.':'PDF saved.';
 URL.revokeObjectURL(url);return;
 }
 if(print&&tab){""")
            old = "function setTheme(){"
            code = code.replace(old, "function setTheme(){if(window.desktopAPI)window.desktopAPI.setTheme(theme);")
            start = code.index("$('export-settings').addEventListener('click',")
            end = code.index("\n$('import-settings')", start)
            code = code[:start] + """$('export-settings').addEventListener('click',async()=>{
 try{const result=await window.desktopAPI.saveSettings({mode,states,theme,logo});$('transfer-status').textContent=result.canceled?'Export canceled.':'Settings exported.';}
 catch(error){$('transfer-status').textContent='Could not save settings. Please try again.';}
});""" + code[end:]
        code = re.sub(r'</script', r'<\\/script', code, flags=re.I)
        html = html.replace(f'<script src="{name}"></script>', '<script>' + code + '</script>')
    if desktop:
        bridge = (ROOT / 'bridge.js').read_text()
        html = html.replace('</head>', '<script>' + bridge + '</script></head>')
        html = html.replace('</body>', '<script>' + (ROOT / 'updater_ui.js').read_text() + '</script></body>')
    assert not re.search(r'<script[^>]+src=', html)
    assert not re.search(r'<link[^>]+stylesheet', html)
    return html

if __name__ == '__main__':
    (ROOT / 'embedded.html').write_text(bundle(True))
    (ROOT / '3D_Print_Quote_Calculator_Offline.html').write_text(bundle(False))
    print('Built embedded WebView2 page and standalone HTML.')
