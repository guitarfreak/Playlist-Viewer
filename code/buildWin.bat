@echo off

set APP_NAME=Playlist Viewer
set APP_VERSION=0.7
set GITHUB_LINK=www.github.com/guitarfreak/Playlist-Viewer

set                ZIP7=C:\Program Files\7-Zip\7z.exe
set              RCEDIT=C:\Standalone\rcedit.exe

set INNO_SETUP_COMPILER=C:\Program Files (x86)\Inno Setup 5\ISCC.exe
set        WIN_SDK_PATH=C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A
set             VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio 12.0

set scriptpath=%~d0%~p0
cd %scriptpath%

set PLATFORM=win64
if "%~1"=="-x86" set PLATFORM=win32

set PLATFORM2=x64
if "%~1"=="-x86" set PLATFORM2=x86

set BUILD_FOLDER=buildWin64
if "%~1"=="-x86" set BUILD_FOLDER=buildWin32

if NOT "%~4"=="-folder" goto buildSetupEnd
	set BUILD_FOLDER=releaseBuild
	if exist "..\%BUILD_FOLDER%" rmdir "..\%BUILD_FOLDER%" /S /Q
:buildSetupEnd

if not exist "..\%BUILD_FOLDER%" mkdir "..\%BUILD_FOLDER%"
pushd "..\%BUILD_FOLDER%"

set INC=
set LINC=

set LINKER_LIBS= -DEFAULTLIB:Opengl32.lib -DEFAULTLIB:ws2_32.lib -DEFAULTLIB:Shell32.lib -DEFAULTLIB:user32.lib -DEFAULTLIB:Gdi32.lib -DEFAULTLIB:Shlwapi.lib -DEFAULTLIB:Dwmapi.lib

set INC=%INC% -I"%VS_PATH%\VC\include"
set INC=%INC% -I"%WIN_SDK_PATH%\Include"

if "%~1"=="-x86" goto compilerSelectionX86
	set                  PATH=%VS_PATH%\VC\bin\amd64;%PATH%
	set LINC=%LINC% -LIBPATH:"%VS_PATH%\VC\lib\amd64"
	set LINC=%LINC% -LIBPATH:"%WIN_SDK_PATH%\Lib\x64"
goto compilerSelectionEnd

:compilerSelectionX86
	set                  PATH=%VS_PATH%\VC\bin;%PATH%
	set LINC=%LINC% -LIBPATH:"%VS_PATH%\VC\lib"
	set LINC=%LINC% -LIBPATH:"%WIN_SDK_PATH%\Lib"
:compilerSelectionEnd

set INC=%INC% -I"..\libs\libcurl\include"

set INC=%INC% -I"..\libs\freetype 2.9\include"
set LINC=%LINC% -LIBPATH:"..\libs\freetype 2.9\lib\%PLATFORM%"
set LINKER_LIBS=%LINKER_LIBS% -DEFAULTLIB:freetype.lib

set BUILD_MODE=-Od
set MODE_DEFINE=
if "%~3"=="-releaseMode" (
	rem -Oy -Zo
	set BUILD_MODE=-O2
	set MODE_DEFINE=-DRELEASE_BUILD
)

set RUNTIME=-MD

if "%~4"=="-folder" (
	set MODE_DEFINE=%MODE_DEFINE% -DSHIPPING_MODE
	set RUNTIME=-MT
)

if "%~5"=="-fullOptimize" (
	set MODE_DEFINE=%MODE_DEFINE% -DFULL_OPTIMIZE
)

rem -d2cgsummary -Bt
set COMPILER_OPTIONS= %RUNTIME% %BUILD_MODE% -nologo -Oi -FC -wd4838 -wd4005 -fp:fast -fp:except- -Gm- -GR- -EHa- -Z7
set LINKER_OPTIONS= -link -SUBSYSTEM:WINDOWS -OUT:"%APP_NAME%.exe" -incremental:no -opt:ref


if "%~4"=="-folder" goto noDLL

del main_*.pdb > NUL 2> NUL
echo. 2>lock.tmp
cl %COMPILER_OPTIONS% ..\code\app.cpp %MODE_DEFINE% -LD %INC% -link -incremental:no -opt:ref -PDB:main_%random%.pdb -EXPORT:appMain %LINC% %LINKER_LIBS%
del lock.tmp

:noDLL

cl %COMPILER_OPTIONS% ..\code\main.cpp %MODE_DEFINE% %INC% %LINKER_OPTIONS% %LINC% %LINKER_LIBS%



if NOT "%~4"=="-folder" goto packShippingFolderEnd

	cd ..

	rem Create release folder.

	mkdir ".\%BUILD_FOLDER%\data"
	xcopy ".\data" ".\%BUILD_FOLDER%\data" /E /Q

	del ".\%BUILD_FOLDER%\*.pdb"
	del ".\%BUILD_FOLDER%\*.exp"
	del ".\%BUILD_FOLDER%\*.lib"
	del ".\%BUILD_FOLDER%\*.obj"

	xcopy ".\libs\freetype 2.9\lib\%PLATFORM%\*.dll" ".\%BUILD_FOLDER%" /Q
	xcopy ".\libs\libcurl\lib\%PLATFORM%\*.dll" ".\%BUILD_FOLDER%" /Q

	xcopy ".\README.txt" ".\%BUILD_FOLDER%" /Q
	xcopy ".\Licenses.txt" ".\%BUILD_FOLDER%" /Q

	%RCEDIT% "%BUILD_FOLDER%\\%APP_NAME%.exe" --set-icon icon.ico

	set RELEASE_FOLDER=.\releases\%APP_NAME% %PLATFORM2%
	if exist "%RELEASE_FOLDER%" rmdir "%RELEASE_FOLDER%" /S /Q
	mkdir "%RELEASE_FOLDER%"

	xcopy %BUILD_FOLDER% "%RELEASE_FOLDER%" /E /Q

	rmdir ".\%BUILD_FOLDER%" /S /Q


	if NOT "%~6"=="-ship" shipEnd
		set COMPILE_TOKEN=true
		call buildCreateInstallerAndZip.bat
	:shipEnd

:packShippingFolderEnd

IF "%~2"=="-run" call "%APP_NAME%.exe"
