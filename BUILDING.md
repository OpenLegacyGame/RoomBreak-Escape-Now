# Building RoomBreak Escape Now

The game uses C++20, CMake and Axmol 2.11.5. Build tools download their dependencies on the first run. Run commands from the project root unless otherwise noted.

## Common setup

Install Git, CMake 3.22 or newer, Ninja, NASM and PowerShell 7 (`pwsh`). Prepare the pinned engine and its compatibility patch on macOS:

```sh
bash scripts/prepare-engine.sh
```

On Windows, run the PowerShell equivalent:

```powershell
pwsh -File scripts/prepare-engine.ps1
```

The default engine checkout is `external/axmol`. To prepare the engine elsewhere, set `AX_ROOT` before running the preparation and build scripts. Keep that checkout at the revision checked by the preparation script. If an `external/axmol` checkout exists, the project uses it.

## macOS

Use macOS with the Xcode command-line tools. The build targets macOS 12 or newer and the host CPU architecture.

```sh
bash scripts/build-macos.sh
```

The application is `build/macos-release/bin/RoomBreakEscapeNow/RoomBreak Escape Now.app`.

To make an installer:

```sh
mkdir -p dist
bash scripts/package-desktop.sh macos \
  'build/macos-release/bin/RoomBreakEscapeNow/RoomBreak Escape Now.app' \
  'dist/RoomBreak Escape Now.pkg'
```

The default app signature is ad hoc. Set `MAC_SIGN_IDENTITY` to an installed signing identity when preparing a signed distribution; notarization is a separate distribution step.

## Windows

Install Visual Studio 2022 C++ desktop build tools and a Windows SDK. Open an x64 developer shell with CMake, Ninja and PowerShell 7 on `PATH`:

```powershell
pwsh -File scripts/build-windows.ps1
```

Keep the generated executable together with its runtime libraries and `Content` directory. Desktop packaging requires NSIS and the official Microsoft Visual C++ x64 redistributable. Run this packaging command in Git Bash or on macOS:

```sh
bash scripts/package-desktop.sh windows RELEASE_DIRECTORY VC_redist.x64.exe \
  'RoomBreak Escape Now.exe'
```

### Cross-compiling Windows on macOS

Install LLVM and LLD, and prepare an x64 Windows SDK/CRT tree with xwin. Point `WINSDK_ROOT` at the directory containing `crt` and `sdk`. Set `LLVM_ROOT` and `LLD_ROOT` if the tools are not found on `PATH`.

```sh
export WINSDK_ROOT=/path/to/windows-sdk
export LLVM_ROOT=/path/to/llvm
export LLD_ROOT=/path/to/lld
bash scripts/build-windows-cross.sh
```

This produces an x64 Windows build. Building on macOS does not test Windows graphics or device behavior; validate the resulting game on Windows before distributing it.

## Android

Install JDK 21, Android SDK API 36, build-tools 36.0.0, NDK 30.0.16248370 and CMake 4.2 or newer (verified with 4.4.4). Set `JAVA_HOME` to the JDK and `ANDROID_HOME` to the SDK directory.

```sh
bash scripts/build-android.sh
```

The default target is arm64-v8a. The development APK is copied to `dist/android/RoomBreak Escape Now.apk`. Android requires an APK signature even for local installation; the debug build uses the local development key. Keep the same application ID and signing key when installing updates over an existing installation.

For a distribution build, configure your own release keystore outside version control using the Gradle signing properties, then run `:app:assembleRelease` from the Android Gradle project. Never commit signing keys or their passwords.

## iOS

Use macOS with the full Xcode installation selected by `xcode-select`. Complete Xcode's first-launch setup and install a simulator runtime.

```sh
bash scripts/build-ios.sh simulator
bash scripts/build-ios.sh device
```

Both builds target arm64 and iOS 15 or newer. The script produces unsigned apps. The simulator build can be installed into a compatible simulator. To run the device build on an iPhone or iPad, open the generated Xcode project and configure a valid development team and provisioning profile.

## Build verification

The project has compiled successfully for macOS arm64, Windows x64 (Clang/LLD with an xwin SDK), iOS arm64 device and simulator, and Android arm64-v8a. The native Windows Visual Studio command is provided for local builds. iOS device signing and Android installation are separate steps from compilation.

## Tests and saves

The data tests exercise puzzle events, inventory, progress, free shop claims and unlock behavior without a graphics context. From the project root, with `AX_ROOT` pointing to the prepared engine:

```sh
export AX_ROOT="${AX_ROOT:-$PWD/external/axmol}"
mkdir -p build/tests
c++ -std=c++20 -Isrc -I"$AX_ROOT/3rdparty" \
  tests/DataTests.cpp src/GameData.cpp src/EventRunner.cpp src/Progress.cpp \
  -o build/tests/data-tests
build/tests/data-tests assets/GameData.json
```

`RB_VISUAL_PROBE` is off in normal builds. Enable it only for the development playback and UI checks. Use a separate test save when running those checks.

Saves and settings are stored in the platform's writable application-data directory. Keep the application identifiers stable for upgrades. Windows keeps the earlier `RoomBreakRebuilt` save directory so the renamed executable can load existing progress. The Windows uninstaller preserves user saves.

For the included 30-stage route runner and continuous menu demo, see [the route usage guide (Chinese)](tests/README.md). It includes the separate `RB_VISUAL_PROBE=ON` build, commands, output files, and displayed-to-internal episode mapping.
