@echo off
if "%VS100COMNTOOLS%" == "" (
  msg "%username%" "Visual Studio 10 not detected"
  exit 1
)
if not exist vpx.sln (
  call make-solutions.bat
)
if exist vpx.sln (
  call "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat"
  MSBuild /property:Configuration="Release" vpx.sln
  MSBuild /property:Configuration="Debug" vpx.sln
  MSBuild /property:Configuration="RelWithDebInfo" vpx.sln
)
