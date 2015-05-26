@echo off

IF NOT EXIST .\build mkdir .\build
pushd .\build

set C_FLAGS=-MTd -nologo -Gm- -GR- -EHa -Od -Oi -WX -W4 -DGAME_ASSERTS=1 -DINTERNAL=1 -DWIN32=1 -FC -Z7 -wd4201 -wd4189 -wd4100 -wd4505
set L_FLAGS=-incremental:no -opt:ref 
set LIBS=user32.lib gdi32.lib winmm.lib

REM Append custom datetime to pdb to avoid VS debugger file lock
del *.pdb > NUL 2> NUL

if "%time:~0,1%" == " " (
	set datetime=%date:~-4,4%.%date:~-10,2%.%date:~-7,2%_0%time:~1,1%.%time:~3,2%.%time:~6,2%
) else (
	set datetime=%date:~-4,4%.%date:~-10,2%.%date:~-7,2%_%time:~0,2%.%time:~3,2%.%time:~6,2%
)

REM 32-bit build
REM cl %C_FLAGS% ".\build\win32_game.cpp" /link %L_FLAGS% -subsystem:windows,5.01 %LIBS%

REM 64-bit build
REM Optimization switches /O2 /Oi /fp:fast
cl %C_FLAGS% "..\src\game.cpp" -Fmgame.map -LD /link -incremental:no -PDB:game_%datetime%.pdb /DLL -EXPORT:GameGetSoundSamples -EXPORT:GameUpdateAndRender
cl %C_FLAGS% "..\src\win32_game.cpp" -Fmwin32_game.map /link %L_FLAGS% -subsystem:windows,5.02 %LIBS%

popd