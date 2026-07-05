@echo off
setlocal

set WRAPPER_JAR=%~dp0gradle\wrapper\gradle-wrapper.jar
if exist "%WRAPPER_JAR%" (
  "%JAVA_HOME%\bin\java.exe" -classpath "%WRAPPER_JAR%" org.gradle.wrapper.GradleWrapperMain %*
  goto :end
)

where gradle >nul 2>nul
if %ERRORLEVEL% EQU 0 (
  gradle %*
  goto :end
)

echo ERROR: Neither Gradle wrapper jar nor system gradle found. Install Android Studio or Gradle, then run again.
exit /b 1

:end
exit /b %ERRORLEVEL%
