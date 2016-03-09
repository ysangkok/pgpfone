# Microsoft Developer Studio Project File - Name="PGPfone" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=PGPfone - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "PGPfone.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "PGPfone.mak" CFG="PGPfone - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PGPfone - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "PGPfone - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "PGPfone - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I ".\\" /I "..\common" /I "..\bignum" /I "..\..\..\libs\pfl\win32" /I "..\..\..\libs\pfl\common" /I "..\..\..\libs\pfl\common\util" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D PGP_WIN32=1 /D PGPXFER=1 /D PGP_INTEL=1 /D BNINCLUDE="bni80386c.h" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 ole32.lib winmm.lib wsock32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "PGPfone - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /W2 /Gm /GX /ZI /Od /I ".\\" /I "..\common" /I "..\bignum" /I "..\..\..\libs\pfl\win32" /I "..\..\..\libs\pfl\common" /I "..\..\..\libs\pfl\common\util" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D PGP_WIN32=1 /D PGPXFER=1 /D PGP_INTEL=1 /D BNINCLUDE="bni80386c.h" /c
# SUBTRACT CPP /u /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ole32.lib winmm.lib wsock32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "PGPfone - Win32 Release"
# Name "PGPfone - Win32 Debug"
# Begin Source File

SOURCE=.\res\about1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\about4.bmp
# End Source File
# Begin Source File

SOURCE=.\res\about8.bmp
# End Source File
# Begin Source File

SOURCE=..\common\ADPCM.cpp
# End Source File
# Begin Source File

SOURCE=..\common\blowfish.cpp
# End Source File
# Begin Source File

SOURCE=..\bignum\bn.c
# End Source File
# Begin Source File

SOURCE=..\bignum\bn32.c
# End Source File
# Begin Source File

SOURCE=..\bignum\bni32.c
# End Source File
# Begin Source File

SOURCE=..\bignum\bni80386c.c
# End Source File
# Begin Source File

SOURCE=..\bignum\bnimem.c
# End Source File
# Begin Source File

SOURCE=..\bignum\bninit32.c
# End Source File
# Begin Source File

SOURCE=..\bignum\bnlegal.c
# End Source File
# Begin Source File

SOURCE=..\common\bytefifo.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cast5.cpp
# End Source File
# Begin Source File

SOURCE=.\CAuthWindow.cpp

!IF  "$(CFG)" == "PGPfone - Win32 Release"

!ELSEIF  "$(CFG)" == "PGPfone - Win32 Debug"

# ADD CPP /YX""

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\CCoderRegistry.cpp

!IF  "$(CFG)" == "PGPfone - Win32 Release"

!ELSEIF  "$(CFG)" == "PGPfone - Win32 Debug"

# ADD CPP /I "."
# SUBTRACT CPP /u

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\CControlThread.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CCounterEncryptor.cpp
# End Source File
# Begin Source File

SOURCE=.\CEncryptionDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CEncryptionStream.cpp
# End Source File
# Begin Source File

SOURCE=.\CFileTransferDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\CLevelMeter.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CMessageQueue.cpp
# End Source File
# Begin Source File

SOURCE=.\CModemDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\res\connecte.ico
# End Source File
# Begin Source File

SOURCE=..\common\CPFPackets.cpp
# End Source File
# Begin Source File

SOURCE=.\CPFTInternet.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CPFTransport.cpp
# End Source File
# Begin Source File

SOURCE=.\CPFTSerial.cpp
# End Source File
# Begin Source File

SOURCE=.\CPFWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\CPGPFone.cpp
# End Source File
# Begin Source File

SOURCE=.\res\CPGPFoneDoc.ico
# End Source File
# Begin Source File

SOURCE=.\CPGPFoneFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\CPGPFStatusBar.cpp
# End Source File
# Begin Source File

SOURCE=.\CPhoneDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CPipe.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CPriorityQueue.cpp
# End Source File
# Begin Source File

SOURCE=..\common\crc.cpp
# End Source File
# Begin Source File

SOURCE=.\res\credits1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\credits4.bmp
# End Source File
# Begin Source File

SOURCE=.\res\credits8.bmp
# End Source File
# Begin Source File

SOURCE=..\common\CSoundInput.cpp
# End Source File
# Begin Source File

SOURCE=.\CSoundLight.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CSoundOutput.cpp
# End Source File
# Begin Source File

SOURCE=.\CStatusPane.cpp
# End Source File
# Begin Source File

SOURCE=.\CTriThreshold.cpp
# End Source File
# Begin Source File

SOURCE=.\CWinFilePipe.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CXferThread.cpp
# End Source File
# Begin Source File

SOURCE=.\CXferWindow.cpp
# End Source File
# Begin Source File

SOURCE=..\common\des3.cpp
# End Source File
# Begin Source File

SOURCE=..\common\dh.cpp
# End Source File
# Begin Source File

SOURCE=..\common\DHPrimes.cpp
# End Source File
# Begin Source File

SOURCE=..\common\fastpool.cpp
# End Source File
# Begin Source File

SOURCE=..\common\gsm.c
# End Source File
# Begin Source File

SOURCE=..\common\HashWordList.cpp
# End Source File
# Begin Source File

SOURCE=.\res\insecure.ico
# End Source File
# Begin Source File

SOURCE=.\LMutexSemaphore.cpp
# End Source File
# Begin Source File

SOURCE=.\LSemaphore.cpp
# End Source File
# Begin Source File

SOURCE=.\LThread.cpp
# End Source File
# Begin Source File

SOURCE=.\res\micropho.bmp
# End Source File
# Begin Source File

SOURCE=.\res\microphone_m.bmp
# End Source File
# Begin Source File

SOURCE=.\res\PGPFone.ico
# End Source File
# Begin Source File

SOURCE=.\PGPFone.rc
# End Source File
# Begin Source File

SOURCE=.\res\PGPFoneLogo.bmp
# End Source File
# Begin Source File

SOURCE=.\res\PGPFoneLogoMask.bmp
# End Source File
# Begin Source File

SOURCE=..\common\PGPFoneUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\PGPFWinUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\res\recv.ico
# End Source File
# Begin Source File

SOURCE=..\common\samplerate.cpp
# End Source File
# Begin Source File

SOURCE=.\res\secure.ico
# End Source File
# Begin Source File

SOURCE=..\common\SHA.cpp
# End Source File
# Begin Source File

SOURCE=.\res\triangle.bmp
# End Source File
# Begin Source File

SOURCE=.\res\trianglemask.bmp
# End Source File
# Begin Source File

SOURCE=.\res\volume.bmp
# End Source File
# Begin Source File

SOURCE=.\res\volume_m.bmp
# End Source File
# Begin Source File

SOURCE=.\res\waiting.ico
# End Source File
# End Target
# End Project
