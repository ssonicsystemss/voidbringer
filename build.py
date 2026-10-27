r"""Build voidbringer on Windows with the local Visual Studio (MSVC + its bundled CMake/Ninja).

    python build.py            # VST3 + CLAP + render test -> out\
    python build.py --install  # also copy voidbringer.vst3 into Common Files\VST3 (run as administrator, FL closed)

JUCE and clap-juce-extensions come from deps\ (JUCE 8.0.4). JOBS=2: more parallel builds crashed the laptop once.
"""

import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
DEPS = ROOT / "deps"
BUILD = ROOT / "build"
OUT = ROOT / "out"
VST3_DIR = Path(os.environ.get("CommonProgramFiles", r"C:\Program Files\Common Files")) / "VST3"
JOBS = 2
VSWHERE = Path(r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe")


def vs_env():
    """Environment of a 'x64 Native Tools' prompt (vcvars64) + VS's own CMake and Ninja."""
    vs = subprocess.run([str(VSWHERE), "-latest", "-prerelease", "-products", "*", "-requires",
                         "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"],
                        capture_output=True, text=True).stdout.strip()
    if not vs:
        sys.exit("Visual Studio with the C++ workload not found")
    dump = subprocess.run(f'"{Path(vs) / "VC/Auxiliary/Build/vcvars64.bat"}" >nul && set', shell=True,
                          capture_output=True, text=True).stdout
    env = dict(line.split("=", 1) for line in dump.splitlines() if "=" in line)
    key = "Path" if "Path" in env else "PATH"
    ext = Path(vs) / "Common7/IDE/CommonExtensions/Microsoft/CMake"
    parts = [p for p in env[key].split(";") if p and not any(x in p.lower() for x in ("msys", "devkitpro", "mingw", "git\\usr"))]
    env[key] = ";".join([str(ext / "CMake/bin"), str(ext / "Ninja")] + parts)
    for k in [k for k in env if k.upper() in ("PWD", "OLDPWD", "_", "SHELL", "HOME", "EXEPATH", "MSYSTEM", "TERM")]:
        env.pop(k)
    return env, ext / "CMake/bin/cmake.exe", ext / "Ninja/ninja.exe"


def run(cmd, env, log):
    with open(log, "a", encoding="utf-8", errors="replace") as f:
        f.write("\n> " + " ".join(map(str, cmd)) + "\n")
        f.flush()
        return subprocess.run(cmd, env=env, stdout=f, stderr=subprocess.STDOUT).returncode


def main():
    if not (DEPS / "JUCE/CMakeLists.txt").exists():
        sys.exit("deps\\JUCE missing: link or clone JUCE 8.0.4 into deps\\JUCE and clap-juce-extensions into deps\\cje")
    env, cmake, ninja = vs_env()
    BUILD.mkdir(exist_ok=True)
    log = BUILD / "build.log"
    log.write_text("", encoding="utf-8")
    if run([cmake, "-S", ROOT, "-B", BUILD, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release",
            f"-DFETCHCONTENT_SOURCE_DIR_JUCE={DEPS / 'JUCE'}", f"-DFETCHCONTENT_SOURCE_DIR_CLAP-JUCE-EXTENSIONS={DEPS / 'cje'}",
            "-DFETCHCONTENT_FULLY_DISCONNECTED=ON", f"-DCMAKE_MAKE_PROGRAM={ninja}", "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"], env, log):
        sys.exit(f"configure failed - see {log}")
    if run([cmake, "--build", BUILD, "--parallel", str(JOBS), "--target", "Voidbringer_VST3", "Voidbringer_CLAP",
            "VoidbringerRender"], env, log):
        sys.exit(f"build failed - see {log}")

    OUT.mkdir(exist_ok=True)
    exe = next(BUILD.rglob("VoidbringerRender.exe"))
    r = subprocess.run([str(exe), str(OUT / "demo.wav"), str(OUT / "gui.png")], cwd=OUT, capture_output=True, text=True,
                       errors="replace", timeout=600)
    (OUT / "render.log").write_text(r.stdout + r.stderr + f"\n[exit code {r.returncode}]\n", encoding="utf-8")
    passed = "RESULT: PASS" in r.stdout and r.returncode == 0
    print("render test:", "PASS" if passed else f"FAIL (see {OUT / 'render.log'})")
    if not passed:
        sys.exit(1)

    vst3 = next(BUILD.rglob("voidbringer.vst3"))
    shutil.rmtree(OUT / vst3.name, ignore_errors=True)
    shutil.copytree(vst3, OUT / vst3.name)
    clap = next(BUILD.rglob("voidbringer.clap"), None)
    if clap:
        shutil.copy2(clap, OUT / clap.name)
    print("built:", OUT / vst3.name)
    if "--install" in sys.argv:
        dest = VST3_DIR / vst3.name
        shutil.rmtree(dest, ignore_errors=True)
        shutil.copytree(vst3, dest)
        print("installed:", dest)


if __name__ == "__main__":
    main()
