# Building model

## Prerequisites

- CMake 3.25+
- C++20 compiler (GCC 11+, Clang 13+, MSVC 2022+)
- [vcpkg](https://github.com/microsoft/vcpkg)

All other dependencies (`fetch`, `nlohmann-json`) are declared in `vcpkg.json`
and installed automatically.

---

## 1. Clone & Bootstrap vcpkg

Clone vcpkg to a standard global location so it can be shared across projects:

```bash
# Linux / macOS
git clone https://github.com/microsoft/vcpkg.git ~/.vcpkg
~/.vcpkg/bootstrap-vcpkg.sh
```

```powershell
# Windows (PowerShell)
git clone https://github.com/microsoft/vcpkg.git "$env:USERPROFILE\.vcpkg"
& "$env:USERPROFILE\.vcpkg\bootstrap-vcpkg.bat"
```

Then export `VCPKG_ROOT` so CMake can find it automatically. Add the
appropriate line to your shell's startup file to make it permanent:

```bash
# Linux / macOS — add to ~/.bashrc or ~/.zshrc
export VCPKG_ROOT="$HOME/.vcpkg"
export PATH="$VCPKG_ROOT:$PATH"
```

```powershell
# Windows — run once in an admin PowerShell to set it permanently
[System.Environment]::SetEnvironmentVariable("VCPKG_ROOT", "$env:USERPROFILE\.vcpkg", "User")
[System.Environment]::SetEnvironmentVariable("Path", "$env:USERPROFILE\.vcpkg;$env:Path", "User")
```

> **Note:** Restart your terminal (or re-source your shell config) after
> setting these for the changes to take effect.

---

## 2. Clone model

```bash
git clone https://github.com/yourname/model.git
cd model
```

---

## 3. Configure

CMake will invoke vcpkg manifest mode automatically and install all
dependencies declared in `vcpkg.json` (`fetch`, `nlohmann-json`).

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
```

> On Windows (PowerShell):
> ```powershell
> cmake -B build `
>   -DCMAKE_BUILD_TYPE=Release `
>   -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
> ```

If `VCPKG_ROOT` is set in your environment, the toolchain file is picked up
automatically and `-DCMAKE_TOOLCHAIN_FILE` can be omitted.

---

## 4. Build

```bash
cmake --build build
```

For a specific config on multi-config generators (Visual Studio, Xcode):

```bash
cmake --build build --config Release
```

---

## 5. Install

```bash
cmake --install build --prefix /usr/local
```

This installs:
- `libmodel.a` / `libmodel.so` → `lib/`
- Headers → `include/model/`
- CMake package files → `lib/cmake/model/`

---

## 6. Consuming model in Another Project

### After installing

```cmake
find_package(model REQUIRED)
target_link_libraries(myapp PRIVATE model::model)
```

### As a subdirectory

```cmake
add_subdirectory(third_party/model)
target_link_libraries(myapp PRIVATE model::model)
```

### Via CMake FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
    model
    GIT_REPOSITORY https://github.com/yourname/model.git
    GIT_TAG        v1.0.0
)
FetchContent_MakeAvailable(model)
target_link_libraries(myapp PRIVATE model::model)
```

---

## Build Options

| Option              | Default   | Description                                   |
|---------------------|-----------|-----------------------------------------------|
| `CMAKE_BUILD_TYPE`  | `Release` | Debug / Release / RelWithDebInfo / MinSizeRel |

---

## Tested Platforms

| Platform      | Compiler         | Status |
|---------------|------------------|--------|
| Ubuntu 22.04+ | GCC 12, Clang 15 | ✅     |
| macOS 13+     | Apple Clang 15   | ✅     |
| Windows 11    | MSVC 2022        | ✅     |
```