# DarkEnforcer
Win32 Dark Mode enforcer

For stable use, see [Dark titlebars enforcing app](https://github.com/ChGen/DarkTitle)

 This projects demonstrates forcing native Win32 apps to `Dark mode`. This requires a lot of work to be stable and universal, so its still a prototype. This is done by combination fo `Win32 global hooks`, `Win32 Controls subclassing` and some `UXtheme API` and also few other things, including undocumented APIs.

For undocumented APIs and structures, credits to: [ysc3839/win32-darkmod](https://github.com/ysc3839/win32-darkmode/blob/master/win32-darkmode/DarkMode.h), [rounk-ctrl gist](https://gist.github.com/rounk-ctrl/b04e5622e30e0d62956870d5c22b7017), [npp work](https://github.com/notepad-plus-plus/notepad-plus-plus/labels/dark%20mode
).

## GitHub Actions CI/CD

This project includes automated building through GitHub Actions. The workflow automatically compiles both x64 and x86 versions of the application on every push and pull request. Built binaries are available as artifacts for download.

## How to Build

### Prerequisites
- Visual Studio 2022 (or MSBuild tools)
- Windows SDK
- C++ build tools

### Manual Build
1) Prepare MS VS2022, Windows 11 22H2 amd64.
2) Open `.sln` file with Visual Studio, build in Debug or Release mode.
3) Alternatively, build from command line:
   ```cmd
   nuget restore DarkEnforcer.sln
   msbuild DarkEnforcer.sln /p:Configuration=Release /p:Platform=x64
   ```

## How to Try

1) Switch to Windows 11 Dark theme. Note, that standard `cleanmgr.exe` isn't dark.
2) Before running DarkEnforcer, save important files, since it will inject `dll` to all processes and something may crash.
3) Run DarkEnforcer via Visual Studio (without extra elevation/admin rights). It won't show any window, probably. You will see new  running process in Task Manager.
4) Run standard `cleanmgr.exe` without extra elevation/admin rights. You should see that it's mostly dark now.
5) When you finished, kill DarkEnforcer process.

 Screenshots for MS Windows 11 22H2 cleanmgr.exe, which doesn't support dark mode out-of-the box and was put in dark by the project:
 
 ![Origincal disk cleanup](screenshots/cleanmgr-orig.png) 
 ![Dark disk cleanup](screenshots/cleanmgr-dark.png)
