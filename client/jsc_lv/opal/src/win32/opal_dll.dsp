# Microsoft Developer Studio Project File - Name="OPAL_dll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=OPAL_dll - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "opal_dll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "opal_dll.mak" CFG="OPAL_dll - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "OPAL_dll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "OPAL_dll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "OPAL_dll - Win32 No Trace" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "OPAL_dll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\lib"
# PROP Intermediate_Dir "..\..\lib\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W4 /GR /GX /Zd /O2 /Ob2 /D "NDEBUG" /Yu"ptlib.h" /Fd"..\..\lib\opal.pdb" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "NDEBUG"
# ADD RSC /l 0xc09 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ptclib.lib ptlib.lib setupapi.lib Delayimp.lib winmm.lib msacm32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib /nologo /subsystem:windows /dll /pdb:none /debug /debugtype:both /machine:I386 /def:"..\..\lib\Release\opal.def" /out:"..\..\lib\opal.dll" /delayload:setupapi.dll
# Begin Custom Build - Extracting debug symbols
OutDir=.\..\..\lib
TargetName=opal
InputPath=\Work\opal\lib\opal.dll
SOURCE="$(InputPath)"

"$(OutDir)\$(TargetName).dbg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	rebase -b 0x30000000 -x . $(OutDir)\$(TargetName).dll

# End Custom Build

!ELSEIF  "$(CFG)" == "OPAL_dll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\lib"
# PROP Intermediate_Dir "..\..\lib\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MDd /W4 /GR /GX /Zi /Od /D "_DEBUG" /Fd"..\..\lib\opald.pdb" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "_DEBUG"
# ADD RSC /l 0xc09 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 ptclibd.lib ptlibd.lib mpr.lib setupapi.lib Delayimp.lib winmm.lib msacm32.lib wsock32.lib kernel32.lib user32.lib advapi32.lib shell32.lib gdi32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /def:"..\..\lib\Debug\opald.def" /out:"..\..\lib\opald.dll" /delayload:setupapi.dll
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "OPAL_dll - Win32 No Trace"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "NoTrace"
# PROP BASE Intermediate_Dir "NoTrace"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\lib"
# PROP Intermediate_Dir "..\..\lib\NoTrace"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W4 /GX /Zi /O2 /Ob2 /I "include" /D "NDEBUG" /Yu"ptlib.h" /FD /c
# ADD CPP /nologo /MD /W4 /GR /GX /Zi /O2 /Ob2 /D "NDEBUG" /Yu"ptlib.h" /Fd"..\..\lib\opaln.pdb" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "NDEBUG"
# ADD RSC /l 0xc09 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 lib/opals.lib winmm.lib mpr.lib snmpapi.lib wsock32.lib netapi32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib /nologo /subsystem:windows /dll /debug /debugtype:both /machine:I386 /out:"lib/opal.dll" /libpath:"lib"
# ADD LINK32 ptclib.lib ptlib.lib setupapi.lib Delayimp.lib winmm.lib msacm32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib /nologo /subsystem:windows /dll /machine:I386 /def:"..\..\lib\NoTrace\opaln.def" /out:"..\..\lib\opaln.dll" /delayload:setupapi.dll
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Extracting debug symbols
OutDir=.\..\..\lib
TargetName=opaln
InputPath=\Work\opal\lib\opaln.dll
SOURCE="$(InputPath)"

"$(OutDir)\$(TargetName).dbg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	rebase -b 0x10000000 -x . $(OutDir)\$(TargetName).dll

# End Custom Build

!ENDIF 

# Begin Target

# Name "OPAL_dll - Win32 Release"
# Name "OPAL_dll - Win32 Debug"
# Name "OPAL_dll - Win32 No Trace"
# Begin Source File

SOURCE=.\dllmain.cxx

!IF  "$(CFG)" == "OPAL_dll - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_dll - Win32 Debug"

!ELSEIF  "$(CFG)" == "OPAL_dll - Win32 No Trace"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libver.rc
# End Source File
# Begin Source File

SOURCE=.\lib\Release\opal.def
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\opal.dtf

!IF  "$(CFG)" == "OPAL_dll - Win32 Release"

USERDEP__OPAL_="$(OutDir)\opals.lib"	"$(InputDir)\Private.def"	
# Begin Custom Build - Merging exported library symbols
InputDir=.
IntDir=.\..\..\lib\Release
OutDir=.\..\..\lib
TargetName=opal
InputPath=.\opal.dtf

"$(IntDir)\$(TargetName).def" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	MergeSym -I "$(INCLUDE)" -x ptlib\msos\ptlib.dtf -x $(InputDir)\Private.def $(OutDir)\opals.lib $(InputPath) 
	copy $(InputPath)+nul $(IntDir)\$(TargetName).def > nul 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "OPAL_dll - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "OPAL_dll - Win32 No Trace"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=",.\..\lib\Debug\opald.def"

!IF  "$(CFG)" == "OPAL_dll - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "OPAL_dll - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "OPAL_dll - Win32 No Trace"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\opald.dtf

!IF  "$(CFG)" == "OPAL_dll - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "OPAL_dll - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__OPALD="$(OutDir)\opalsd.lib"	"$(InputDir)\Private.def"	
# Begin Custom Build - Merging exported library symbols
InputDir=.
IntDir=.\..\..\lib\Debug
OutDir=.\..\..\lib
TargetName=opald
InputPath=.\opald.dtf

"$(IntDir)\$(TargetName).def" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	MergeSym -I "$(INCLUDE)" -x ptlib\msos\ptlibd.dtf -x $(InputDir)\Private.def $(OutDir)\opalsd.lib $(InputPath) 
	copy $(InputPath)+nul $(IntDir)\$(TargetName).def  > nul 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "OPAL_dll - Win32 No Trace"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\lib\NoTrace\opaln.def
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\opaln.dtf

!IF  "$(CFG)" == "OPAL_dll - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "OPAL_dll - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "OPAL_dll - Win32 No Trace"

USERDEP__OPALN="$(OutDir)\opalsn.lib"	"$(InputDir)\Private.def"	
# Begin Custom Build - Merging exported library symbols
InputDir=.
IntDir=.\..\..\lib\NoTrace
OutDir=.\..\..\lib
TargetName=opaln
InputPath=.\opaln.dtf

"$(IntDir)\$(TargetName).def" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	MergeSym -I "$(INCLUDE)" -x ptlib\msos\ptlib.dtf -x $(InputDir)\Private.def $(OutDir)\opalsn.lib $(InputPath) 
	copy $(InputPath)+nul $(IntDir)\$(TargetName).def  > nul 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Private.def
# PROP Exclude_From_Build 1
# End Source File
# End Target
# End Project
