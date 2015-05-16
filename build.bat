@echo off

IF NOT EXIST .\build mkdir .\build
pushd .\build

set OPTIONS=-WX -W4 -DGAME_ASSERTS=1 -DINTERNAL=1 -FC -Zi -wd4201 -wd4189 -wd4100
set LIBRARIES=user32.lib gdi32.lib winmm.lib

set CC=cl %OPTIONS% "D:\Dev\Project\Game\src\win32_game.cpp" %LIBRARIES%

%CC%

popd