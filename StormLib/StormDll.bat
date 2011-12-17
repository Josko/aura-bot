@echo off
rem Post-build batch for StormDll project
rem Called as StormDll.bat $(PlatformName) $(ConfigurationName)
rem Example: StormDll.bat x64 Debug

copy .\StormDll\StormDll.h  .\StormLib\StormDll.h

if x%1 == xWin32 goto PlatformWin32
if x%1 == xx64 goto PlatformWin64
goto exit

:PlatformWin32
copy .\bin\StormDll\%1\%2\*.lib    .
goto exit

:PlatformWin64
copy .\bin\StormDll\%1\%2\*.lib    .
goto exit

:exit

