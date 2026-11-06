"""Build a verified Echo Stone package; never writes to the game."""
import argparse
import hashlib
import json
import shutil
import struct
import zipfile
import sys
from pathlib import Path

GAME_HASH = "ccf02d07677f259165cc8830eba0fcfaeea32f70c756871735e7188c367be2e6"
BIN = "MVCI/Binaries/Win64/"
MOD = BIN + "scripts/MVCIAccess/"
PINS = {
    "dependencies/ual-x64/dinput8.dll": "031a3e5576d91dce1e438d36b9a3d462c7334ab4791990a8ff1e3ddc0e132daf",
    "dependencies/prism/prism.dll": "402cc717d01b8b6574adbcb7a1627d30c078061d6be2b9664958f661ea0aec75",
}

def sha(data):
    return hashlib.sha256(data).hexdigest()

def x64(data):
    if len(data) < 64 or data[:2] != b"MZ":
        return False
    offset = struct.unpack_from("<I", data, 0x3c)[0]
    return offset + 6 <= len(data) and data[offset:offset+6] == b"PE\0\0\x64\x86"

def build(root, output):
    root, output = Path(root), Path(output)
    if output.exists() and any(output.iterdir()):
        raise ValueError("Package output must be a new empty directory.")
    mapping = {
        BIN + "dinput8.dll": "dependencies/ual-x64/dinput8.dll",
        BIN + "dinput8.ini": "config/dinput8.ini",
        BIN + "scripts/MVCIAccess.asi": "build/Release/MVCIAccess.asi",
        MOD + "prism.dll": "dependencies/prism/prism.dll",
        MOD + "README.md": "README.md",
        MOD + "licenses/MVCIAccess-GPL-3.0.txt": "LICENSE",
        MOD + "licenses/Prism-MPL-2.0.txt": "LICENSES/Prism-MPL-2.0.txt",
        MOD + "licenses/MinHook.txt": "LICENSES/MinHook.txt",
        MOD + "licenses/PicoSHA2.txt": "LICENSES/PicoSHA2.txt",
        MOD + "licenses/Ultimate-ASI-Loader.txt": "LICENSES/Ultimate-ASI-Loader.txt",
    }
    sys.dont_write_bytecode = True
    sys.path.insert(0, str(root / 'tools/audio_descriptions'))
    from build_story import validate_pack, catalog_source
    narration = validate_pack(root / 'data/descriptions')
    if (root / 'src/description_catalog.cpp').read_text(encoding='utf-8') != catalog_source(narration):
        raise ValueError('Narration catalog differs from its public recordings.')
    for name in ['manifest.json', 'coverage.json'] + [c['file'] for c in narration['clips']]:
        mapping[MOD + 'descriptions/' + name] = 'data/descriptions/' + name
    # Validate everything before creating a partial package.
    payload = {}
    for relative, source in mapping.items():
        data = (root / source).read_bytes()
        # These files used LF in tester packages. Keep their installed bytes
        # independent of Git's checkout settings, especially for shared loaders.
        if source in ("config/dinput8.ini", "LICENSES/PicoSHA2.txt", "LICENSES/Ultimate-ASI-Loader.txt"):
            data = data.replace(b"\r\n", b"\n")
        if source in PINS and sha(data) != PINS[source]:
            raise ValueError(f"Pinned dependency mismatch: {source}")
        if relative.endswith((".dll", ".asi")) and not x64(data):
            raise ValueError(f"Not a native x64 artifact: {source}")
        payload[relative] = data
    version = "0.1.1"
    manifest = dict(format=1, product="MVCIAccess", version=version, game_sha256=GAME_HASH, files=[])
    for relative, data in sorted(payload.items()):
        path = output / "payload" / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        manifest["files"].append(dict(path=relative.replace("/", "\\"), size=len(data), sha256=sha(data)))
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2)+"\n", encoding="utf-8")
    for relative in (
        "Install-EchoStone.cmd",
        "tools/Install.ps1",
        "tools/Uninstall.ps1",
        "tools/Deployment.Common.ps1",
        "README.md",
        "docs/coverage.md",
        "docs/story-accessibility.md",
        "dependencies/versions.json",
    ):
        target = output / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(root / relative, target)
    # Include the inputs for building the plugin without duplicating the
    # recordings and runtime dependencies already included under payload.
    source = output / "source"
    for relative in (
        "CMakeLists.txt",
        "LICENSE",
        "tools/audio_descriptions/build_story.py",
        "tools/audio_descriptions/render_story.py",
    ):
        target = source / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(root / relative, target)
    for relative in ("include", "src", "third_party"):
        shutil.copytree(root / relative, source / relative)
    archive = output.parent / (output.name + ".zip")
    if archive.exists():
        raise ValueError("Refusing to overwrite an existing archive.")
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as zipped:
        for path in sorted(output.rglob("*")):
            if path.is_file():
                zipped.write(path, output.name + "/" + path.relative_to(output).as_posix())
    with zipfile.ZipFile(archive) as zipped:
        if zipped.testzip() is not None:
            raise ValueError("ZIP CRC failure.")
        files = {output.name+"/"+p.relative_to(output).as_posix(): p for p in output.rglob("*") if p.is_file()}
        if set(files) != set(zipped.namelist()):
            raise ValueError("Archive membership mismatch.")
        for name, path in files.items():
            if sha(zipped.read(name)) != sha(path.read_bytes()):
                raise ValueError(f"Archive entry differs: {name}")
    print(json.dumps(dict(package=str(output), archive=str(archive), files=len(payload), archive_sha256=sha(archive.read_bytes())), indent=2))
    return manifest

def build_amethyst(root, output, portable):
    version = portable["version"]
    archive = output.parent / f"EchoStone-{version}-Amethyst.zip"
    checks = [
        dict(type="hashEquals", path=f["path"].replace("\\", "/"), sha256=f["sha256"])
        for f in portable["files"]
    ]
    check_script = "files/Check-EchoStoneInstall.ps1"
    manifest = {
        "pluginId": "buu420",
        "gameId": "marvel-vs-capcom-infinite",
        "modVersion": version,
        "targetPlatform": "windows",
        "installActions": [{"type": "copyFolder", "sourceDir": "MVCI", "targetDir": "MVCI"}],
        "verify": checks,
        "preInstall": {
            "executable": check_script,
            "what": "Check the game build, running game process and existing loader files.",
            "why": "Echo Stone needs the supported Steam executable and compatible loader files.",
            "modifies": "Read-only compatibility check.",
            "needsAdmin": False,
            "failureFatal": True,
            "runOnUpdate": True,
            "installToGameFolder": False,
            "runFromGameFolder": False,
        },
    }
    with zipfile.ZipFile(archive, "x", zipfile.ZIP_DEFLATED) as zipped:
        zipped.writestr("manifest.json", json.dumps(manifest, indent=2) + "\n")
        zipped.write(root / "tools/Check-EchoStoneInstall.ps1", check_script)
        for check in checks:
            relative = check["path"]
            zipped.write(output / "payload" / relative, "files/" + relative)
    with zipfile.ZipFile(archive) as zipped:
        if zipped.testzip() is not None:
            raise ValueError("Amethyst ZIP CRC failure.")
        for check in checks:
            if sha(zipped.read("files/" + check["path"])) != check["sha256"]:
                raise ValueError(f"Amethyst payload differs: {check['path']}")
    print(json.dumps(dict(archive=str(archive), files=len(checks), archive_sha256=sha(archive.read_bytes())), indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    parser.add_argument("--amethyst", action="store_true")
    args = parser.parse_args()
    source = Path(__file__).resolve().parent.parent
    output = args.output or source / "artifacts/EchoStone-0.1.1-Windows"
    portable = build(source, output)
    if args.amethyst:
        build_amethyst(source, output, portable)
