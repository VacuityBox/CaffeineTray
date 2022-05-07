@echo off

if "%1"=="" goto CustomBuild
if /i "%1"=="Minimal" goto MinimalBuild
if /i "%1"=="Standard" goto StandardBuild
if /i "%1"=="Full" goto FullBuildBuild
if /i "%1"=="Custom" goto CustomBuild

if /i not "%1"=="All" goto ExitError


:MinimalBuild
echo Building feature set: Minimal
call MSBuild CaffeineTake.sln /p:FeatureSet=Minimal;Configuration=Debug;Platform=x64
call MSBuild CaffeineTake.sln /p:FeatureSet=Minimal;Configuration=Debug;Platform=x86
call MSBuild CaffeineTake.sln /p:FeatureSet=Minimal;Configuration=Release;Platform=x64
call MSBuild CaffeineTake.sln /p:FeatureSet=Minimal;Configuration=Release;Platform=x86
echo Done. FeatureSet: Minimal

if /i not "%1"=="All" goto Exit

:StandardBuild
echo Building feature set: Standard
call MSBuild -m CaffeineTake.sln /p:FeatureSet=Standard;Configuration=Debug;Platform=x64
call MSBuild -m CaffeineTake.sln /p:FeatureSet=Standard;Configuration=Debug;Platform=x86
call MSBuild -m CaffeineTake.sln /p:FeatureSet=Standard;Configuration=Release;Platform=x64
call MSBuild -m CaffeineTake.sln /p:FeatureSet=Standard;Configuration=Release;Platform=x86
echo Done. FeatureSet: Standard

if /i not "%1"=="All" goto Exit

:FullBuild
echo Building feature set: Full
call MSBuild CaffeineTake.sln /p:FeatureSet=Full;Configuration=Debug;Platform=x64
call MSBuild CaffeineTake.sln /p:FeatureSet=Full;Configuration=Debug;Platform=x86
call MSBuild CaffeineTake.sln /p:FeatureSet=Full;Configuration=Release;Platform=x64
call MSBuild CaffeineTake.sln /p:FeatureSet=Full;Configuration=Release;Platform=x86
echo Done. FeatureSet: Full

goto Exit

:CustomBuild
echo Building feature set: Custom
call MSBuild CaffeineTake.sln /p:Configuration=Debug;Platform=x64
call MSBuild CaffeineTake.sln /p:Configuration=Debug;Platform=x86
call MSBuild CaffeineTake.sln /p:Configuration=Release;Platform=x64
call MSBuild CaffeineTake.sln /p:Configuration=Release;Platform=x86
echo Done. FeatureSet: Custom

goto Exit

:ExitError
echo Invalid build type specified '%1'

:Exit
