@echo off
call c:\vs98\vc98\bin\vcvars32
cl /MD /nologo /c /I ..\..\..\pgpsrc\libs\pfl\common /I ..\..\..\pgpsrc\libs\pfl\win32 /D PGP_WIN32 /I c:\vs98\vc98\mfc\include /I win32\ /I common\ %1
