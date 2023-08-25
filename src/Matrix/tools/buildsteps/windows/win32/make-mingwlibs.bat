@ECHO OFF

PUSHD %~dp0\..
CALL vswhere.bat x86
IF ERRORLEVEL 1 (
  ECHO ERROR! make-mingwlibs.bat: Something went wrong when calling vswhere.bat
  POPD
  EXIT /B 1
)
CALL make-mingwlibs.bat %*
POPD
