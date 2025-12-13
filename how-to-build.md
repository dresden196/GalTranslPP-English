# GalTranslPP Build Guide

## 1. Environment Setup

Before building, ensure your development environment meets these requirements:

- **Operating System**: Windows 10 or Windows 11
- **IDE**: [Visual Studio 2026](https://visualstudio.microsoft.com/insiders/?rwnlp=zh-hans) and

    VS2022 (uses 2022's ToolChain - if you know what you're doing, you can install just VS2022 BuildTools without the IDE, but still need to check **Desktop development with C++**)
    ![BuildTools](img/BuildTools.png?raw=true)
  - **Required Workload**: `Desktop development with C++`
  - **Required Toolsets**: `MSVC v143` (VS 2022) and `MSVC v145`
- **Python 3.12.10**: `python-3.12.10-embed-amd64.zip` from this repository
- **Version Control**: [git](https://git-scm.com/)

## 2. Installing Core Dependencies

### 2.1 vcpkg Package Manager

vcpkg manages the C++ libraries required by the project.

```cmd
# 1. Clone vcpkg repository to any location
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg

# 2. Run bootstrap script to install
.\bootstrap-vcpkg.bat

# 3. (Important) Add vcpkg root directory to user PATH environment variable, or run vcpkg integrate install to bind vcpkg to Visual Studio
```

### 2.2 Qt Framework

- 1. Visit [Qt official website](https://www.qt.io/download-qt-installer-oss) to download and run the Qt Community Open Source (LGPL license) online installer (requires Qt account registration).
- 2. On the component selection page, ensure these components are checked:
  - `Qt` → `Qt 6.9.2 (or higher, but compatibility not guaranteed)` → `MSVC 2022 64-bit`

## 3. Getting the Source Code

Clone the GalTranslPP main repository along with submodule dependencies.

```cmd
git clone --recurse-submodules https://github.com/julixian/GalTranslPP.git
cd GalTranslPP
```

## 4. Building Dependencies

### 4.1 Configure Python Libraries

- 1. Manually create a folder named `lib` in the `GalTranslPP` root directory.
- 2. Extract `Python.zip` from the `GalTranslPP` folder to the current folder - the program needs the header files inside.

### 4.2 Configure Visual Studio and Qt

- 1. **Install VS Extension**:
  - Launch Visual Studio, select `Extensions` → `Manage Extensions` from the top menu bar.
  - Search for and install **"Qt Visual Studio Tools"** extension.
  - Restart Visual Studio as prompted to complete installation.
- 2. **Link Qt Version**:
  - After restart, select `Extensions` → `Qt VS Tools` → `Qt Versions` from the menu bar.
  - Click `Add New Qt Version`, point the path to your Qt MSVC installation directory (e.g., `C:\Qt\6.9.2\msvc2022_64`), and set it as the default version.

### 4.3 Build ElaWidgetTools

- 1. Open the `3rdParty\ElaWidgetTools` folder with Visual Studio.
- 2. In the top toolbar, switch build configuration from `Qt-Debug` to **`Qt-Release`**.
- 3. Select `Build` → `Build All` from the menu bar (if using Visual Studio and following the above steps, no need to change QT_SDK_DIR in CMakeLists).
- 4. **Deploy Build Artifacts**:
  - Move `ElaWidgetTools.lib` from `3rdParty\ElaWidgetTools\out\build\Release\ElaWidgetTools` to the `lib\` folder created earlier.

### 4.4 Build OpenCC

- 1. Open the `3rdParty\OpenCC` folder with Visual Studio.
- 2. If `x64-Release` build option doesn't exist, click Manage Configurations, click the green plus sign, select `x64-Release`, and save with Ctrl + S.
- 3. Select `x64-Release`, build opencc.dll (Install) (lib\opencc.dll).
- 4. **Deploy Build Artifacts**
  - Move `marisa.lib` and `opencc.lib` from `3rdParty\OpenCC\out\install\x64-Release\lib` to the `lib\` folder.
  - Ensure the `3rdParty\OpenCC\out\install\x64-Release\include` folder exists - the program needs the header files inside.

## 5. Building GalTranslPP (Main Project)

This step involves a temporary toolset switch to resolve specific dependency compatibility issues.

- 1. Open the `GalTranslPP.sln` solution file in the root directory with Visual Studio.

- 2. **Step 1: Build Dependencies with v143 Toolset**
  - In **Solution Explorer**, right-click the `GalTranslPP` module and select `Properties`.
  - In the properties page, navigate to `Configuration Properties` → `General`.
  - Change **Platform Toolset** to **`Visual Studio 2022 (v143)`** **(change this for both Debug and Release)**.
  - Click `Apply` to save.
  - Select `Build` → `Batch Build` from the menu, check `GalTranslPP|Release|x64` and click Build.
    > **Note**: This step specifically compiles dependencies like `mecab:x64-windows`.

- 3. **Step 2: Build Main Program with v145 Toolset**
  - Open `GalTranslPP` project properties again.
  - Switch **Platform Toolset** back to **`Visual Studio v18 (v145)`**.
  - Click `Apply` and close the properties window.
  - To ensure all modules compile correctly, use `Rebuild All` when building target modules in `Build` → `Batch Build` (if it still fails, use VS2022 and switch back to v143 toolset to compile).

## 6. Completion and Running

After successful compilation, all executables are generated in the `Release\` directory.

Additional files must be copied to the folder for the program to run properly.

- 0. First extract `python-3.12.10-embed-amd64.zip` from the `Example\BaseConfig` folder in the project root to the current folder.

### 6.1 GPPCLI

- 1. Copy the `BaseConfig` and `sampleProject` folders from `Example` to `GalTranslPP\Release\GPPCLI`
- 2. Copy these files to the program root directory:
  - `3rdParty\OpenCC\out\install\x64-Release\bin\opencc.dll`

### 6.2 GPPGUI

- 1. Copy the `BaseConfig` folder to `GalTranslPP\Release\GPPGUI`
- 2. Create a `Projects` folder in `GalTranslPP\Release\GPPGUI` and copy the entire `sampleProject` folder into it
- 3. Open a Qt-specific console, such as Qt 6.9.2 (MSVC 2022 64-bit), and run:

```cmd
windeployqt path/to/GalTranslPP_GUI.exe
```

- 4. Copy these files to the program root directory:
  - `3rdParty\OpenCC\out\install\x64-Release\bin\opencc.dll`
  - `3rdParty\ElaWidgetTools\out\build\Release\ElaWidgetTools\ElaWidgetTools.dll`

### 6.3 Private Deployment (Optional)

If you want to move GPPCLI and GPPGUI to other locations like `D:\GALGAME\GALGAMETOOLS\AIGC\GPPCLI`
and `D:\GALGAME\GALGAMETOOLS\AIGC\GPPGUI`, run these cmd commands in GalTranslPP\Release to create symbolic links:

#### GPPCLI

```cmd
mklink .\GPPCLI_PRIVATE "D:\GALGAME\GALGAMETOOLS\AIGC\GPPCLI"
```

#### GPPGUI

```cmd
mklink .\GPPGUI_PRIVATE "D:\GALGAME\GALGAMETOOLS\AIGC\GPPGUI"
```

This way, each build will copy the core files `GalTranslPP_CLI.exe`, `GalTranslPP_GUI.exe`, and `Updater_new.exe` to the corresponding directories.

All steps are now complete.
