#!/usr/bin/env python3
"""Build z80-debug VSIX package.

Usage:
  python3 make_vsix.py                        # compile + package
  python3 make_vsix.py --install              # compile + package + install in VS Code
  python3 make_vsix.py --version 1.2.3        # override version
  python3 make_vsix.py --no-compile           # skip tsc (use existing out/)
"""
import argparse
import json
import os
import subprocess
import sys
import zipfile

EXT_DIR = os.path.dirname(os.path.abspath(__file__))

EXCLUDE_DIRS = {
    "node_modules", ".git", "__pycache__",
    ".vscode", "src", "out", "test-project", "templates",
}
EXCLUDE_FILES = {
    ".gitignore", "package-lock.json",
    "make_vsix.py", "HARDWARE_PANELS_PLAN.md", "README.md",
    "webpack.config.js", "tsconfig.json",
}
EXCLUDE_SUFFIXES = (".ts", ".js.map", ".vsix", ".py", ".md")


def collect_files():
    result = []
    for root, dirs, fnames in os.walk(EXT_DIR):
        dirs[:] = sorted(
            d for d in dirs
            if d not in EXCLUDE_DIRS and not d.startswith(".")
        )
        for fname in fnames:
            if fname in EXCLUDE_FILES:
                continue
            if any(fname.endswith(s) for s in EXCLUDE_SUFFIXES):
                continue
            abs_path = os.path.join(root, fname)
            rel_path = os.path.relpath(abs_path, EXT_DIR)
            result.append((abs_path, rel_path))
    return result


def content_types_xml():
    return """\
<?xml version="1.0" encoding="utf-8"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension=".json"        ContentType="application/json"/>
  <Default Extension=".js"          ContentType="application/javascript"/>
  <Default Extension=".svg"         ContentType="image/svg+xml"/>
  <Default Extension=".png"         ContentType="image/png"/>
  <Default Extension=".vsixmanifest" ContentType="text/xml"/>
</Types>"""


def manifest_xml(pkg, version):
    engine = pkg.get("engines", {}).get("vscode", "^1.80.0")
    return f"""\
<?xml version="1.0" encoding="utf-8"?>
<PackageManifest Version="2.0.0"
  xmlns="http://schemas.microsoft.com/developer/vsx-schema/2011"
  xmlns:d="http://schemas.microsoft.com/developer/vsx-schema-design/2011">
  <Metadata>
    <Identity Language="en-US"
      Id="{pkg['name']}"
      Version="{version}"
      Publisher="{pkg['publisher']}"/>
    <DisplayName>{pkg['displayName']}</DisplayName>
    <Description xml:space="preserve">{pkg['description']}</Description>
    <Tags>debugger,z80,amstrad,cpc</Tags>
    <Categories>Debuggers</Categories>
    <GalleryFlags>Public</GalleryFlags>
    <Properties>
      <Property Id="Microsoft.VisualStudio.Code.Engine" Value="{engine}"/>
    </Properties>
  </Metadata>
  <Installation>
    <InstallationTarget Id="Microsoft.VisualStudio.Code"/>
  </Installation>
  <Dependencies/>
  <Assets>
    <Asset Type="Microsoft.VisualStudio.Code.Manifest"
      Path="extension/package.json" Addressable="true"/>
  </Assets>
</PackageManifest>"""


def main():
    parser = argparse.ArgumentParser(description="Build z80-debug VSIX")
    parser.add_argument("--version", help="Override version (default: from package.json)")
    parser.add_argument("--no-compile", action="store_true", help="Skip TypeScript compilation")
    parser.add_argument("--install", action="store_true", help="Install in VS Code after build")
    args = parser.parse_args()

    os.chdir(EXT_DIR)

    with open("package.json", encoding="utf-8") as f:
        pkg = json.load(f)

    version = args.version or pkg["version"]
    vsix_name = f"{pkg['name']}-{version}.vsix"

    if not args.no_compile:
        print("Compiling TypeScript + bundling...")
        r = subprocess.run(["npm", "run", "bundle"], capture_output=True, text=True)
        if r.returncode != 0:
            print(r.stdout)
            print(r.stderr)
            sys.exit(1)
        print("  OK")

    files = collect_files()
    print(f"Packaging {len(files)} files → {vsix_name}")

    with zipfile.ZipFile(vsix_name, "w", zipfile.ZIP_DEFLATED) as zf:
        zf.writestr("[Content_Types].xml", content_types_xml())
        zf.writestr("extension.vsixmanifest", manifest_xml(pkg, version))
        for abs_path, rel_path in files:
            zf.write(abs_path, f"extension/{rel_path}")

    size_kb = os.path.getsize(vsix_name) // 1024
    print(f"  {size_kb} KB")

    if args.install:
        print("Installing in VS Code...")
        r = subprocess.run(["code", "--install-extension", vsix_name])
        if r.returncode != 0:
            sys.exit(1)
        print("  Done — reload VS Code (Ctrl+Shift+P → Developer: Reload Window)")


if __name__ == "__main__":
    main()
