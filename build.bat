@echo off

IF NOT EXIST .\build mkdir .\build
pushd .\build

set OPTIONS=-MT -nologo -Gm- -GR- -EHa -Od -Oi -WX -W4 -DGAME_ASSERTS=1 -DINTERNAL=1 -FC -Z7 -Fmwin32_game.map -wd4201 -wd4189 -wd4100
set LINKER_OPTIONS=/link -opt:ref -subsystem:windows,5.02
:: x86 -subsystem:windows,5.01
:: x64 -subsystem:windows,5.02

set LIBRARIES=user32.lib gdi32.lib winmm.lib

set CC=cl %OPTIONS% "D:\Dev\Project\Game\src\win32_game.cpp" %LINKER_OPTIONS% %LIBRARIES%

%CC%

popd