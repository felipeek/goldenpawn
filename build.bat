@echo off

SET COMPILER_FLAGS=/Zi /nologo

call cl %COMPILER_FLAGS% src/*.c