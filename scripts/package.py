"""Build a public manual-install package from the compiled add-on.

External runtime/font binaries are supplied explicitly and checksum-validated.
No game resources, local installation records, probes or build tools are packed.
"""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import zipfile

ROOT = Path(__file__).resolve().parent.parent
PINNED = {
    'runtime': '0CEE63F9C9F13F3AC909C5B4903F4DBB4B719A7AB3B4F13B0DEAF83C814B94F7',
    'font': '2C76254F6FC379FDDFCE0A7E84FB5385BB135D3E399294F6EEB6680D0365B74B',
    'font_license': '6A73F9541C2DE74158C0E7CF6B0A58EF774F5A780BF191F2D7EC9CC53EFE2BF2',
}

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--font', type=Path, required=True)
    parser.add_argument('--font-license', type=Path, required=True)
    parser.add_argument('--addon', type=Path, default=ROOT/'build/SinkingStarSketch.addon64')
    parser.add_argument('--staging', type=Path, default=ROOT/'build/package/SinkingStarSketchPad-3.4.6-Manual')
    args = parser.parse_args()
    for key, expected in PINNED.items():
        path = getattr(args, key)
        if not path.is_file() or digest(path) != expected:
            parser.error(f'Use the tested, unmodified {key} dependency; SHA256 must be {expected}')
    if not args.addon.is_file() or args.addon.read_bytes()[:2] != b'MZ':
        parser.error('Build the native add-on first, or pass --addon.')

    staging = args.staging.resolve()
    if staging.exists():
        parser.error(f'Staging folder already exists: {staging}. Move it aside before packaging again.')
    staging.mkdir(parents=True)
    payload = {
        'dxgi.dll': args.runtime,
        'SinkingStarSketch.addon64': args.addon,
        'ReShade.ini': ROOT/'config/ReShade.ini',
        'SinkingStarSketch-assets\\NotoSansCJKsc-Regular.otf': args.font,
    }
    manifest = {'Name':'SinkingStarSketchPad', 'Version':'3.4.6', 'ReleaseStatus':'Beta',
                'Source':'https://github.com/ouyangzhixuan/SinkingStarSketchPad', 'License':'MIT',
                'Languages':['en','zh-CN'], 'InstallMode':'Manual', 'PackagingRevision':1, 'Files':[]}
    for name, source in payload.items():
        target = staging/'game_files'/Path(name.replace('\\','/'))
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
        manifest['Files'].append({'Name':name, 'SHA256':digest(source)})
    (staging/'package.json').write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8-sig')
    for name in ('INSTALL_EN.txt','安装说明.txt','README_EN.md','README_zh-CN.md','CHANGELOG.md','VALIDATION.md'):
        shutil.copy2(ROOT/'docs'/name, staging/name)
    shutil.copy2(ROOT/'LICENSE', staging/'LICENSE')
    notice=(ROOT/'THIRD_PARTY_NOTICES.md').read_text(encoding='utf-8').replace('third_party/reshade-sdk/LICENSE-ReShade.md','licenses/ReShade-BSD-3-Clause.txt').replace('third_party/reshade-sdk/LICENSE-ImGui.txt','licenses/ImGui-MIT.txt').replace('third_party/runtime-notices','licenses')
    (staging/'THIRD_PARTY_NOTICES.md').write_text(notice,encoding='utf-8')
    licenses = staging/'licenses'
    shutil.copytree(ROOT/'third_party/runtime-notices', licenses)
    shutil.copy2(ROOT/'third_party/reshade-sdk/LICENSE-ReShade.md', licenses/'ReShade-BSD-3-Clause.txt')
    shutil.copy2(ROOT/'third_party/reshade-sdk/LICENSE-ImGui.txt', licenses/'ImGui-MIT.txt')
    shutil.copy2(args.font_license, licenses/'Noto-SIL-OFL-1.1.txt')
    dist = ROOT/'dist'
    dist.mkdir(exist_ok=True)
    forbidden = {'.cmd','.bat','.ps1','.exe','.msi','.vbs','.js','.zip','.7z','.rar'}
    if any(path.is_file() and path.suffix.lower() in forbidden for path in staging.rglob('*')):
        raise RuntimeError('Manual package must not contain installer scripts, executables or nested archives')
    archive = dist/'SinkingStarSketchPad-3.4.6-Manual.zip'
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as output:
        for path in sorted(staging.rglob('*')):
            if path.is_file():
                output.write(path, path.relative_to(staging).as_posix())
    with zipfile.ZipFile(archive) as output:
        if output.testzip() is not None:
            raise RuntimeError('ZIP integrity check failed')
        for file in manifest['Files']:
            data = output.read('game_files/'+file['Name'].replace('\\','/'))
            if hashlib.sha256(data).hexdigest().upper() != file['SHA256']:
                raise RuntimeError('Payload checksum mismatch')
    archive.with_suffix('.zip.sha256.txt').write_text(digest(archive)+'  '+archive.name+'\n', encoding='utf-8')
    print(archive)
    print('SHA256:', digest(archive))

if __name__ == '__main__':
    main()
