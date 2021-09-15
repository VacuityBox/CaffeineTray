import os
import pathlib
import shutil

TEMP_PATH      = pathlib.Path("Obj") / "build_artifacts"
ARTIFACTS_PATH = pathlib.Path("Artifacts/")
ARCHIVE_EXT    = "zip"
BUILD_ROOT     = pathlib.Path("..")

COMMON_FILES = [
    pathlib.Path("./Data/ReadMe.txt"),
    pathlib.Path("./Data/License.txt"),
    pathlib.Path("./Data/CaffeineTray.Portable.json")
]

X32_FILES = [
    pathlib.Path("./Bin/Win32/Release/CaffeineTray.exe")
]

X64_FILES = [
    pathlib.Path("./Bin/x64/Release/CaffeineTray.exe")
]

VERSION = "2.0.1"
X32_ARTIFACT_NAME = f"CaffeineTray-{VERSION}-Portable-32bit"
X64_ARTIFACT_NAME = f"CaffeineTray-{VERSION}-Portable-64bit"

def build_arch(archive_name, bits, arch_files):
    build_temp = BUILD_ROOT / TEMP_PATH / bits
    build_temp.mkdir(parents=True, exist_ok=True)

    for file in COMMON_FILES:
        shutil.copyfile(BUILD_ROOT / file, build_temp / file.name)

    for file in arch_files:
        shutil.copyfile(BUILD_ROOT / file, build_temp / file.name)

    archive = shutil.make_archive(archive_name, ARCHIVE_EXT, build_temp)
    output = ARTIFACTS_PATH / pathlib.Path(archive).name

    return output

def run_build(archive_name, bits, arch_files):
    try:
        output = build_arch(archive_name, bits, arch_files)
    except Exception as e:
        print("failed to build {}".format(archive_name))
        print(e)
    else:
        print("build successful {}".format(output))

def cleanup():
    shutil.rmtree(BUILD_ROOT / TEMP_PATH)

def build():
    ARTIFACTS_PATH.parent.mkdir(parents=True, exist_ok=True)
    os.chdir(ARTIFACTS_PATH)

    run_build(X32_ARTIFACT_NAME, "32bit", X32_FILES)
    run_build(X64_ARTIFACT_NAME, "64bit", X64_FILES)

    cleanup()


build()
