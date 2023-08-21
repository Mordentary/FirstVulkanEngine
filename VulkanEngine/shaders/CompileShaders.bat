@echo off

setlocal enabledelayedexpansion

set "shader_dir=src"
set "output_dir=bin"
set "compiler=glslc.exe"

for %%f in ("%shader_dir%\*.vert" "%shader_dir%\*.frag") do (
    set "filename=%%~nf"
    set "extension=%%~xf"
    %compiler% "%%f" -o "%output_dir%\!filename!!extension!.spv"
)

endlocal

pause