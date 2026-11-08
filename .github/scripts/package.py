from pathlib import Path
import json
import struct
import subprocess
import sys
import tarfile
import tempfile
import shutil
import zipfile

root = Path(__file__).resolve().parents[2]
platform = sys.argv[1]
name = "ExampleAddon"
windows = platform == "windows-x86"
binary = root / "build" / "Release" / (name + ".dll") if windows else root / "build" / (name + ".so")
data = binary.read_bytes()
if windows:
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    assert data[pe:pe + 4] == b"PE\0\0" and struct.unpack_from("<H", data, pe + 4)[0] == 0x14C, "Expected an x86 DLL"
else:
    assert data[:5] == b"\x7fELF\x01" and struct.unpack_from("<H", data, 18)[0] == 3, "Expected an x86 ELF library"
    symbols = subprocess.check_output(["nm", "-D", "--defined-only", str(binary)], text=True)
    for symbol in ("PluginMain", "PluginUnload", "GetPluginApiVersion", "GetPluginSDKSignature"):
        assert symbol in symbols, f"Missing export: {symbol}"
manifest = {
    "repository": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip(),
    "sdk": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root / "SDK", text=True).strip(),
    "platform": platform,
    "api_version": 1,
}
output = root / "dist"
output.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory() as temporary:
    staging = Path(temporary) / name
    staging.mkdir()
    shutil.copy2(binary, staging / binary.name)
    for entry in ("README.md", "LICENSE", "docs", "examples", "vendor/sqlite/README.md"):
        source = root / entry
        if not source.exists():
            continue
        target = staging / entry
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copytree(source, target) if source.is_dir() else shutil.copy2(source, target)
    (staging / "build-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    if windows:
        with zipfile.ZipFile(output / f"{name}-{platform}.zip", "w", zipfile.ZIP_DEFLATED) as archive:
            for source in staging.rglob("*"):
                if source.is_file():
                    archive.write(source, source.relative_to(staging.parent))
    else:
        with tarfile.open(output / f"{name}-{platform}.tar.gz", "w:gz") as archive:
            archive.add(staging, arcname=name)
