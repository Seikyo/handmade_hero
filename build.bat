@rem load VisualStudio Compiler before executing

@echo off

mkdir .\build
pushd .\build

set LIBRARIES=user32.lib gdi32.lib

@rem -F<num> 	stack size
@rem -Zi 		enable debugging
@rem -FC		full pathnames in diagnostics

set OPTIONS=-Zi

set CC=cl %OPTIONS% "D:\Dev\Project\Game\src\game.cpp" %LIBRARIES%
%CC%

popd