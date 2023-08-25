@ECHO OFF

SET EXITCODE=0

SET install=false
SET clean=false
SET package=false
SET addon=
SET store=

SETLOCAL EnableDelayedExpansion
FOR %%b IN (%*) DO (
  IF %%~b == install (
    SET install=true
  ) ELSE ( IF %%~b == clean (
    SET clean=true
  ) ELSE ( IF %%~b == package (
    SET package=true
  ) ELSE ( IF %%~b == win10 (
    SET store=store
  ) ELSE (
    SET addon=!addon! %%~b
  ))))
)
SETLOCAL DisableDelayedExpansion

PUSHD %~dp0\..\..\..
SET WORKDIR=%CD%
POPD

rem setup some paths that we need later
SET CUR_PATH=%CD%
SET BASE_PATH=%WORKDIR%\cmake
SET SCRIPTS_PATH=%BASE_PATH%\scripts\windows%store%
SET ADDONS_PATH=%BASE_PATH%\addons
SET ADDON_DEPENDS_PATH=%ADDONS_PATH%\output
SET ADDONS_BUILD_PATH=%ADDONS_PATH%\build

SET ADDONS_SUCCESS_FILE=%ADDONS_PATH%\.success
SET ADDONS_FAILURE_FILE=%ADDONS_PATH%\.failure

SET ERRORFILE=%ADDONS_PATH%\make-addons.error

rem remove the success and failure files from a previous build
DEL /F %ADDONS_SUCCESS_FILE% > NUL 2>&1
DEL /F %ADDONS_FAILURE_FILE% > NUL 2>&1

IF %clean% == true (
  rem remove the build directory if it exists
  IF EXIST "%ADDONS_BUILD_PATH%" (
    ECHO Cleaning build directory...
    RMDIR "%ADDONS_BUILD_PATH%" /S /Q > NUL
  )

  rem remove the build directory if it exists
  IF EXIST "%ADDON_DEPENDS_PATH%" (
    ECHO Cleaning dependencies...
    RMDIR "%ADDON_DEPENDS_PATH%" /S /Q > NUL
  )

  GOTO END
)

rem create the depends directory
IF NOT EXIST "%ADDON_DEPENDS_PATH%" MKDIR "%ADDON_DEPENDS_PATH%"

rem create the build directory
IF NOT EXIST "%ADDONS_BUILD_PATH%" MKDIR "%ADDONS_BUILD_PATH%"

rem go into the build directory
CD "%ADDONS_BUILD_PATH%"

rem determine the proper install path for the built addons
IF %install% == true (
  SET ADDONS_INSTALL_PATH=%WORKSPACE%\addons
) ELSE (
  SET ADDONS_INSTALL_PATH=%WORKDIR%\project\Win32BuildSetup\BUILD_WIN32\addons
)

ECHO --------------------------------------------------
ECHO Building addons
ECHO --------------------------------------------------

IF "%addon%" NEQ "" (
  SET CMAKE_EXTRA=%CMAKE_EXTRA% -DADDONS_TO_BUILD="%addon%"
)

IF "%ADDON_SRC_PREFIX%" NEQ "" (
  SET CMAKE_EXTRA=%CMAKE_EXTRA% -DADDON_SRC_PREFIX=%ADDON_SRC_PREFIX%
)

IF "%ADDONS_DEFINITION_DIR" NEQ "" (
  SET CMAKE_EXTRA=%CMAKE_EXTRA% -DADDONS_DEFINITION_DIR=%ADDONS_DEFINITION_DIR%
)

IF "%store%" NEQ "" (
  SET CMAKE_EXTRA=%CMAKE_EXTRA% -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=%UCRTVersion%
)

rem execute cmake to generate makefiles processable by nmake
cmake "%ADDONS_PATH%" -G "NMake Makefiles" ^
      -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_USER_MAKE_RULES_OVERRIDE="%SCRIPTS_PATH%/CFlagOverrides.cmake" ^
      -DCMAKE_USER_MAKE_RULES_OVERRIDE_CXX="%SCRIPTS_PATH%/CXXFlagOverrides.cmake" ^
      -DCMAKE_INSTALL_PREFIX=%ADDONS_INSTALL_PATH% ^
      -DCMAKE_SOURCE_DIR=%WORKDIR% ^
      -DBUILD_DIR=%ADDONS_BUILD_PATH% ^
      -DADDON_DEPENDS_PATH=%ADDON_DEPENDS_PATH% ^
      -DPACKAGE_ZIP=ON ^
      %CMAKE_EXTRA%

IF ERRORLEVEL 1 (
  ECHO cmake error level: %ERRORLEVEL% > %ERRORFILE%
  GOTO ERROR
)

rem get the list of addons that can actually be built
SET ADDONS_TO_MAKE=
SETLOCAL EnableDelayedExpansion
FOR /f "delims=" %%i IN ('cmake --build . --target supported_addons') DO (
  SET line="%%i"
  SET addons=!line:ALL_ADDONS_BUILDING=!
  IF NOT "!addons!" == "!line!" (
    SET ADDONS_TO_MAKE=!addons:~3,-1!
  )
)
SETLOCAL DisableDelayedExpansion

rem loop over all addons to build
FOR %%a IN (%ADDONS_TO_MAKE%) DO (
  ECHO Building %%a...
  rem execute cmake to build the addons
  cmake --build . --target %%a
  IF ERRORLEVEL 1 (
    ECHO nmake %%a error level: %ERRORLEVEL% > %ERRORFILE%
    ECHO %%a >> %ADDONS_FAILURE_FILE%
  ) ELSE (
    if %package% == true (
      nmake package-%%a
      IF ERRORLEVEL 1 (
        ECHO nmake package-%%a error level: %ERRORLEVEL% > %ERRORFILE%
        ECHO %%a >> %ADDONS_FAILURE_FILE%
      ) ELSE (
        ECHO %%a >> %ADDONS_SUCCESS_FILE%
      )
    ) ELSE (
      ECHO %%a >> %ADDONS_SUCCESS_FILE%
    )
  )
)

rem everything was fine
GOTO END

:ERROR
rem something went wrong
FOR %%a IN (%ADDONS_TO_BUILD%) DO (
  ECHO %%a >> %ADDONS_FAILURE_FILE%
)
ECHO Failed to build addons
ECHO See %ERRORFILE% for more details
SET EXITCODE=1

:END
rem go back to the original directory
cd %CUR_PATH%

rem exit the script with the defined exitcode
EXIT /B %EXITCODE%
