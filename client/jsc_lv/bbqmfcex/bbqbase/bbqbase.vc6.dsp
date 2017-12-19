# Microsoft Developer Studio Project File - Name="bbqbase" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=bbqbase - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "bbqbase.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "bbqbase.mak" CFG="bbqbase - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "bbqbase - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "bbqbase - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/bbq/src/bbqbase", WWEAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "bbqbase - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(OPENSSLDIR)\inc32" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D P_SSL=0$(OPENSSLFLAG) /Yu"bbqbase.h" /FD /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\lib\bbqbases.lib"

!ELSEIF  "$(CFG)" == "bbqbase - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(OPENSSLDIR)\inc32" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "PTRACING" /D P_SSL=0$(OPENSSLFLAG) /FR /Yu"bbqbase.h" /FD /GZ /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\lib\bbqbasesd.lib"

!ENDIF 

# Begin Target

# Name "bbqbase - Win32 Release"
# Name "bbqbase - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\bbqclient.cxx
# End Source File
# Begin Source File

SOURCE=.\bbqfiledatabase.cxx
# End Source File
# Begin Source File

SOURCE=.\bbqidmsg.cxx
# End Source File
# Begin Source File

SOURCE=.\bbqproxy.cxx
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\bbqsvr.cxx
# End Source File
# Begin Source File

SOURCE=.\bytepack.cxx
# End Source File
# Begin Source File

SOURCE=.\intindex.cxx
# End Source File
# Begin Source File

SOURCE=.\log.cxx
# End Source File
# Begin Source File

SOURCE=.\md5.cxx
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\msgconnection.cxx
# End Source File
# Begin Source File

SOURCE=.\msgterminal.cxx
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\stdafx.cxx
# ADD CPP /Yc"bbqbase.h"
# End Source File
# Begin Source File

SOURCE=.\thread.cxx
# End Source File
# Begin Source File

SOURCE=.\vsocket.cxx
# End Source File
# Begin Source File

SOURCE=.\yasocket.cxx
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\bbqbase.h
# End Source File
# Begin Source File

SOURCE=.\bbqclient.h
# End Source File
# Begin Source File

SOURCE=.\bbqdatabase.h
# End Source File
# Begin Source File

SOURCE=.\bbqfiledatabase.h
# End Source File
# Begin Source File

SOURCE=.\bbqidfilerecord.h
# End Source File
# Begin Source File

SOURCE=.\bbqidmsg.h
# End Source File
# Begin Source File

SOURCE=.\bbqidrecord.h
# End Source File
# Begin Source File

SOURCE=.\bbqproxy.h
# End Source File
# Begin Source File

SOURCE=.\bbqsvr.h
# End Source File
# Begin Source File

SOURCE=.\bbquserinfo.h
# End Source File
# Begin Source File

SOURCE=.\bytepack.h
# End Source File
# Begin Source File

SOURCE=.\intindex.h
# End Source File
# Begin Source File

SOURCE=.\log.h
# End Source File
# Begin Source File

SOURCE=.\md5.h
# End Source File
# Begin Source File

SOURCE=.\msgconnection.h
# End Source File
# Begin Source File

SOURCE=.\msgterminal.h
# End Source File
# Begin Source File

SOURCE=.\rwlock.h
# End Source File
# Begin Source File

SOURCE=.\sfidmsg.h
# End Source File
# Begin Source File

SOURCE=.\thread.h
# End Source File
# Begin Source File

SOURCE=.\useridlist.h
# End Source File
# Begin Source File

SOURCE=.\vsocket.h
# End Source File
# Begin Source File

SOURCE=.\yasocket.h
# End Source File
# End Group
# End Target
# End Project
