"""Package the compiled add-on, ReShade 6.8.0, Noto fonts and original licenses.

External binaries default to the ignored .tools/release-deps directory.
Only runtime files, text installation instructions and license notices are distributed.
"""
from pathlib import Path
import argparse
import zipfile

ROOT = Path(__file__).resolve().parent.parent
DEPS = ROOT / '.tools/release-deps'
VERSION = '3.4.6-fork.1'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, default=DEPS / 'ReShade64.dll')
    parser.add_argument('--font', type=Path, default=DEPS / 'NotoSansCJKsc-Regular.otf')
    parser.add_argument('--bold-font', type=Path, default=DEPS / 'NotoSansCJKsc-Bold.otf')
    parser.add_argument('--font-license', type=Path, default=ROOT / 'third_party/noto-cjk/LICENSE.txt')
    parser.add_argument('--addon', type=Path, default=ROOT / 'build/SinkingStarSketch.addon64')
    args = parser.parse_args()

    payload = {
        'dxgi.dll': args.runtime,
        'SinkingStarSketch.addon64': args.addon,
        'ReShade.ini': ROOT / 'config/ReShade.ini',
        'Sketch-ReShadePreset.ini': ROOT / 'config/Sketch-ReShadePreset.ini',
        'SinkingStarSketch-assets/NotoSansCJKsc-Regular.otf': args.font,
        'SinkingStarSketch-assets/NotoSansCJKsc-Bold.otf': args.bold_font,
    }
    files = {'game_files/' + name: source for name, source in payload.items()}
    for name in ('INSTALL_EN.txt', '安装说明.txt'):
        files[name] = ROOT / 'docs' / name
    files.update({
        'LICENSE': ROOT / 'LICENSE',
        'THIRD_PARTY_NOTICES.txt': ROOT / 'third_party/NOTICE.txt',
        'licenses/ReShade-BSD-3-Clause.txt': ROOT / 'third_party/reshade-sdk/LICENSE-ReShade.md',
        'licenses/ImGui-MIT.txt': ROOT / 'third_party/reshade-sdk/LICENSE-ImGui.txt',
        'licenses/Noto-SIL-OFL-1.1.txt': args.font_license,
    })
    for folder in ('runtime-notices', 'reshade-runtime', 'reference-notices'):
        directory = ROOT / 'third_party' / folder
        for source in sorted(directory.rglob('*')):
            if source.is_file() and source.suffix.lower() != '.md':
                files['licenses/' + folder + '/' + source.relative_to(directory).as_posix()] = source
    for source in files.values():
        if not source.is_file():
            parser.error(f'Missing release input: {source}. See docs/RELEASE_zh-CN.md.')
    for source in (args.addon, args.runtime):
        with source.open('rb') as binary:
            if binary.read(2) != b'MZ':
                parser.error(f'Expected a Windows add-on/runtime binary: {source}')

    dist = ROOT / 'dist'
    dist.mkdir(exist_ok=True)
    archive = dist / f'SinkingStarSketchPad-{VERSION}.zip'
    temporary = archive.with_suffix('.zip.tmp')
    with zipfile.ZipFile(temporary, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as output:
        for name, source in files.items():
            output.write(source, name)
    temporary.replace(archive)
    print(archive)


if __name__ == '__main__':
    main()
