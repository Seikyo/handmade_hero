@echo off

IF NOT EXIST .\build mkdir .\build
pushd .\build

set OPTIONS=-Zi -FC
set LIBRARIES=user32.lib gdi32.lib

set CC=cl %OPTIONS% "D:\Dev\Project\Game\src\win32_game.cpp" %LIBRARIES%

%CC%

popd