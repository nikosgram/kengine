@echo off

ctime -begin kengine.ctm

set CommonCompilerFlags=-diagnostics:column -WL -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -FC -Z7 -GS- -Gs9999999
set CommonCompilerFlags=-DKENGINE_INTERNAL=1 %CommonCompilerFlags%

REM unreferenced formal parameter
set CommonCompilerFlags=-wd4100 %CommonCompilerFlags%
REM local variable is initialized but not referenced
set CommonCompilerFlags=-wd4189 %CommonCompilerFlags%

set CommonLinkerFlags=-STACK:0x100000,0x100000 -incremental:no -opt:ref /NODEFAULTLIB kernel32.lib

IF NOT EXIST data mkdir data
IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

del *.pdb > NUL 2> NUL

echo WAITING FOR PDB > lock.tmp


REM Unit tests
cl %CommonCompilerFlags% -MTd ..\kengine\code\win32_kengine_tests.c /link /SUBSYSTEM:console %CommonLinkerFlags%
set LastError=%ERRORLEVEL%
win32_kengine_tests.exe

del lock.tmp

popd

ctime -end kengine.ctm %LastError%