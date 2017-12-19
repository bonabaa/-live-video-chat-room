# Microsoft Developer Studio Project File - Name="OPAL_lib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=OPAL_lib - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "opal_lib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "opal_lib.mak" CFG="OPAL_lib - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "OPAL_lib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "OPAL_lib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "OPAL_lib - Win32 No Trace" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 1
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "lib"
# PROP BASE Intermediate_Dir "lib\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\lib"
# PROP Intermediate_Dir "..\..\lib\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /Zd /O2 /Ob2 /D "NDEBUG" /D "PTRACING" /Yu"ptlib.h" /Fd"..\..\lib\opal.pdb" /FD /c
# SUBTRACT CPP /u
# ADD BASE RSC /l 0xc09
# ADD RSC /l 0xc09
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\lib\opals.lib"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "lib"
# PROP BASE Intermediate_Dir "lib\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\lib"
# PROP Intermediate_Dir "..\..\lib\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /D "_DEBUG" /D "PTRACING" /FR /Yu"ptlib.h" /Fd"..\..\lib\opald.pdb" /FD /c
# SUBTRACT CPP /u
# ADD BASE RSC /l 0xc09
# ADD RSC /l 0xc09
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\lib\opalsd.lib"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "lib"
# PROP BASE Intermediate_Dir "lib\NoTrace"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\lib"
# PROP Intermediate_Dir "..\..\lib\NoTrace"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W4 /GX /O1 /Ob2 /I "./include" /D "NDEBUG" /D "PTRACING" /Yu"ptlib.h" /FD /c
# ADD CPP /nologo /MD /W4 /GR /GX /O1 /Ob2 /D "NDEBUG" /D "PASN_NOPRINTON" /D "PASN_LEANANDMEAN" /Yu"ptlib.h" /Fd"..\..\lib\opaln.pdb" /FD /c
# SUBTRACT CPP /u
# ADD BASE RSC /l 0xc09
# ADD RSC /l 0xc09
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\lib\opalsn.lib"

!ENDIF 

# Begin Target

# Name "OPAL_lib - Win32 Release"
# Name "OPAL_lib - Win32 Debug"
# Name "OPAL_lib - Win32 No Trace"
# Begin Group "Source Files"

# PROP Default_Filter ".cxx"
# Begin Group "Opal Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\opal\call.cxx
# End Source File
# Begin Source File

SOURCE=..\opal\connection.cxx
# End Source File
# Begin Source File

SOURCE=..\opal\endpoint.cxx
# End Source File
# Begin Source File

SOURCE=..\opal\guid.cxx
# End Source File
# Begin Source File

SOURCE=..\opal\ivr.cxx
# End Source File
# Begin Source File

SOURCE=..\opal\manager.cxx
# End Source File
# Begin Source File

SOURCE=..\opal\mediafmt.cxx
# End Source File
# Begin Source File

SOURCE=..\opal\mediastrm.cxx
# End Source File
# Begin Source File

SOURCE=..\opal\opalvxml.cxx
# End Source File
# Begin Source File

SOURCE=..\codec\opalwavfile.cxx
# End Source File
# Begin Source File

SOURCE=..\opal\patch.cxx
# End Source File
# Begin Source File

SOURCE=..\opal\pcss.cxx
# End Source File
# Begin Source File

SOURCE=..\opal\transcoders.cxx
# End Source File
# Begin Source File

SOURCE=..\opal\transports.cxx
# End Source File
# End Group
# Begin Group "H323 Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\h323\channels.cxx
# End Source File
# Begin Source File

SOURCE=..\h323\gkclient.cxx
# End Source File
# Begin Source File

SOURCE=..\h323\gkserver.cxx
# End Source File
# Begin Source File

SOURCE=..\h323\h225ras.cxx
# End Source File
# Begin Source File

SOURCE=..\h323\h235auth.cxx
# End Source File
# Begin Source File

SOURCE=..\h323\h235auth1.cxx
# End Source File
# Begin Source File

SOURCE=..\h323\h323.cxx
# End Source File
# Begin Source File

SOURCE=..\h323\h323annexg.cxx
# End Source File
# Begin Source File

SOURCE=..\h323\h323caps.cxx
# End Source File
# Begin Source File

SOURCE=..\h323\h323ep.cxx
# End Source File
# Begin Source File

SOURCE=..\h323\h323neg.cxx
# End Source File
# Begin Source File

SOURCE=..\h323\h323pdu.cxx
# End Source File
# Begin Source File

SOURCE=..\h323\h323rtp.cxx
# End Source File
# Begin Source File

SOURCE=..\h323\h323trans.cxx
# End Source File
# Begin Source File

SOURCE=..\h323\h450pdu.cxx
# End Source File
# Begin Source File

SOURCE=..\h323\h501pdu.cxx
# End Source File
# Begin Source File

SOURCE=..\h323\peclient.cxx
# End Source File
# Begin Source File

SOURCE=..\h323\q931.cxx
# End Source File
# Begin Source File

SOURCE=..\h323\svcctrl.cxx
# End Source File
# Begin Source File

SOURCE=..\h323\transaddr.cxx
# End Source File
# End Group
# Begin Group "Codec Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\codec\echocancel.cxx
# End Source File
# Begin Source File

SOURCE=..\codec\g711.c
# ADD CPP /W1
# SUBTRACT CPP /D "PTRACING" /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\g711codec.cxx
# End Source File
# Begin Source File

SOURCE=..\codec\g726codec.cxx
# End Source File
# Begin Source File

SOURCE=..\codec\g729codec.cxx
# End Source File
# Begin Source File

SOURCE=..\codec\gsmcodec.cxx
# End Source File
# Begin Source File

SOURCE=..\codec\h261codec.cxx
# End Source File
# Begin Source File

SOURCE=..\codec\h263codec.cxx
# End Source File
# Begin Source File

SOURCE=..\codec\ilbccodec.cxx
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10codec.cxx
# End Source File
# Begin Source File

SOURCE=..\codec\mscodecs.cxx
# End Source File
# Begin Source File

SOURCE=..\codec\rfc2833.cxx
# End Source File
# Begin Source File

SOURCE=..\codec\silencedetect.cxx
# End Source File
# Begin Source File

SOURCE=..\codec\speexcodec.cxx
# End Source File
# Begin Source File

SOURCE=..\codec\vidcodec.cxx
# End Source File
# End Group
# Begin Group "LID Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\lids\ixjwin32.cxx
# End Source File
# Begin Source File

SOURCE=..\lids\lid.cxx
# End Source File
# Begin Source File

SOURCE=..\lids\lidep.cxx
# End Source File
# Begin Source File

SOURCE=..\lids\vblasterlid.cxx
# End Source File
# Begin Source File

SOURCE=..\lids\vpblid.cxx
# End Source File
# End Group
# Begin Group "RTP Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\rtp\jitter.cxx
# End Source File
# Begin Source File

SOURCE=..\rtp\rtp.cxx
# End Source File
# End Group
# Begin Group "T.120 Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\t120\h323t120.cxx
# End Source File
# Begin Source File

SOURCE=..\t120\t120proto.cxx
# End Source File
# Begin Source File

SOURCE=..\t120\x224.cxx
# End Source File
# End Group
# Begin Group "T.38 Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\t38\h323t38.cxx
# End Source File
# Begin Source File

SOURCE=..\t38\t38proto.cxx
# End Source File
# End Group
# Begin Group "SIP Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sip\sdp.cxx
# End Source File
# Begin Source File

SOURCE=..\sip\sipcon.cxx
# End Source File
# Begin Source File

SOURCE=..\sip\sipep.cxx
# End Source File
# Begin Source File

SOURCE=..\sip\sippdu.cxx
# End Source File
# End Group
# Begin Group "IAX2 Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\iax2\frame.cxx
# End Source File
# Begin Source File

SOURCE=..\iax2\iax2con.cxx
# End Source File
# Begin Source File

SOURCE=..\iax2\iax2ep.cxx
# End Source File
# Begin Source File

SOURCE=..\iax2\iax2medstrm.cxx
# End Source File
# Begin Source File

SOURCE=..\iax2\iedata.cxx
# End Source File
# Begin Source File

SOURCE=..\iax2\ies.cxx
# End Source File
# Begin Source File

SOURCE=..\iax2\processor.cxx
# End Source File
# Begin Source File

SOURCE=..\iax2\receiver.cxx
# End Source File
# Begin Source File

SOURCE=..\iax2\remote.cxx
# End Source File
# Begin Source File

SOURCE=..\iax2\safestrings.cxx
# End Source File
# Begin Source File

SOURCE=..\iax2\sound.cxx
# End Source File
# Begin Source File

SOURCE=..\iax2\transmit.cxx
# End Source File
# End Group
# Begin Group "H224 Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\h224\h224.cxx
# End Source File
# Begin Source File

SOURCE=..\h224\h281.cxx
# End Source File
# Begin Source File

SOURCE=..\h224\h323h224.cxx
# End Source File
# Begin Source File

SOURCE=..\h224\q922.cxx
# End Source File
# End Group
# Begin Source File

SOURCE=.\precompile.cxx
# ADD CPP /Yc"ptlib.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ".h"
# Begin Group "Opal Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\opal\call.h
# End Source File
# Begin Source File

SOURCE=..\..\include\opal\connection.h
# End Source File
# Begin Source File

SOURCE=..\..\include\opal\endpoint.h
# End Source File
# Begin Source File

SOURCE=..\..\include\opal\guid.h
# End Source File
# Begin Source File

SOURCE=..\..\include\opal\ivr.h
# End Source File
# Begin Source File

SOURCE=..\..\include\opal\manager.h
# End Source File
# Begin Source File

SOURCE=..\..\include\opal\mediafmt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\opal\mediastrm.h
# End Source File
# Begin Source File

SOURCE=..\..\include\opal\opalvxml.h
# End Source File
# Begin Source File

SOURCE=..\..\include\opal\patch.h
# End Source File
# Begin Source File

SOURCE=..\..\include\opal\pcss.h
# End Source File
# Begin Source File

SOURCE=..\..\include\opal\transcoders.h
# End Source File
# Begin Source File

SOURCE=..\..\include\opal\transports.h
# End Source File
# End Group
# Begin Group "H323 Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\h323\channels.h
# End Source File
# Begin Source File

SOURCE=..\..\include\h323\gkclient.h
# End Source File
# Begin Source File

SOURCE=..\..\include\h323\gkserver.h
# End Source File
# Begin Source File

SOURCE=..\..\include\h323\h225ras.h
# End Source File
# Begin Source File

SOURCE=..\..\include\h323\h235auth.h
# End Source File
# Begin Source File

SOURCE=..\..\include\h323\h323annexg.h
# End Source File
# Begin Source File

SOURCE=..\..\include\h323\h323caps.h
# End Source File
# Begin Source File

SOURCE=..\..\include\h323\h323con.h
# End Source File
# Begin Source File

SOURCE=..\..\include\h323\h323ep.h
# End Source File
# Begin Source File

SOURCE=..\..\include\h323\h323neg.h
# End Source File
# Begin Source File

SOURCE=..\..\include\h323\h323pdu.h
# End Source File
# Begin Source File

SOURCE=..\..\include\h323\h323rtp.h
# End Source File
# Begin Source File

SOURCE=..\..\include\h323\h323trans.h
# End Source File
# Begin Source File

SOURCE=..\..\include\h323\h450pdu.h
# End Source File
# Begin Source File

SOURCE=..\..\include\h323\h501pdu.h
# End Source File
# Begin Source File

SOURCE=..\..\include\h323\peclient.h
# End Source File
# Begin Source File

SOURCE=..\..\include\h323\q931.h
# End Source File
# Begin Source File

SOURCE=..\..\include\h323\svcctrl.h
# End Source File
# Begin Source File

SOURCE=..\..\include\h323\transaddr.h
# End Source File
# End Group
# Begin Group "Codec Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\codec\allcodecs.h
# End Source File
# Begin Source File

SOURCE=..\..\include\codec\echocancel.h
# End Source File
# Begin Source File

SOURCE=..\..\include\codec\g711codec.h
# End Source File
# Begin Source File

SOURCE=..\..\include\codec\g7231codec.h
# End Source File
# Begin Source File

SOURCE=..\..\include\codec\g726codec.h
# End Source File
# Begin Source File

SOURCE=..\..\include\codec\g729codec.h
# End Source File
# Begin Source File

SOURCE=..\..\include\codec\gsmcodec.h
# End Source File
# Begin Source File

SOURCE=..\..\include\codec\h261codec.h
# End Source File
# Begin Source File

SOURCE=..\..\include\codec\ilbccodec.h
# End Source File
# Begin Source File

SOURCE=..\..\include\codec\lpc10codec.h
# End Source File
# Begin Source File

SOURCE=..\..\include\codec\mscodecs.h
# End Source File
# Begin Source File

SOURCE=..\..\include\codec\opalwavfile.h
# End Source File
# Begin Source File

SOURCE=..\..\include\codec\rfc2833.h
# End Source File
# Begin Source File

SOURCE=..\..\include\codec\silencedetect.h
# End Source File
# Begin Source File

SOURCE=..\..\include\codec\speexcodec.h
# End Source File
# Begin Source File

SOURCE=..\..\include\codec\vidcodec.h
# End Source File
# End Group
# Begin Group "LID Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\lids\ixjlid.h
# End Source File
# Begin Source File

SOURCE=..\..\include\lids\lid.h
# End Source File
# Begin Source File

SOURCE=..\..\include\lids\lidep.h
# End Source File
# Begin Source File

SOURCE=..\..\include\lids\vblasterlid.h
# End Source File
# Begin Source File

SOURCE=..\..\include\lids\vpblid.h
# End Source File
# End Group
# Begin Group "RTP Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\rtp\jitter.h
# End Source File
# Begin Source File

SOURCE=..\..\include\rtp\rtp.h
# End Source File
# End Group
# Begin Group "T120 Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\t120\h323t120.h
# End Source File
# Begin Source File

SOURCE=..\..\include\t120\t120proto.h
# End Source File
# Begin Source File

SOURCE=..\..\include\t120\x224.h
# End Source File
# End Group
# Begin Group "T.38 Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\t38\h323t38.h
# End Source File
# Begin Source File

SOURCE=..\..\include\t38\t38proto.h
# End Source File
# End Group
# Begin Group "SIP Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\sip\sdp.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sip\sip.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sip\sipcon.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sip\sipep.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sip\sippdu.h
# End Source File
# End Group
# Begin Group "IAX2 Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\iax2\causecode.h
# End Source File
# Begin Source File

SOURCE=..\..\include\iax2\frame.h
# End Source File
# Begin Source File

SOURCE=..\..\include\iax2\iax2.h
# End Source File
# Begin Source File

SOURCE=..\..\include\iax2\iax2con.h
# End Source File
# Begin Source File

SOURCE=..\..\include\iax2\iax2ep.h
# End Source File
# Begin Source File

SOURCE=..\..\include\iax2\iax2medstrm.h
# End Source File
# Begin Source File

SOURCE=..\..\include\iax2\iedata.h
# End Source File
# Begin Source File

SOURCE=..\..\include\iax2\ies.h
# End Source File
# Begin Source File

SOURCE=..\..\include\iax2\processor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\iax2\receiver.h
# End Source File
# Begin Source File

SOURCE=..\..\include\iax2\remote.h
# End Source File
# Begin Source File

SOURCE=..\..\include\iax2\safestrings.h
# End Source File
# Begin Source File

SOURCE=..\..\include\iax2\sound.h
# End Source File
# Begin Source File

SOURCE=..\..\include\iax2\transmit.h
# End Source File
# Begin Source File

SOURCE=..\..\include\iax2\version.h
# End Source File
# End Group
# Begin Group "H224 Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\h224\h224.h
# End Source File
# Begin Source File

SOURCE=..\..\include\h224\h224handler.h
# End Source File
# Begin Source File

SOURCE=..\..\include\h224\h281.h
# End Source File
# Begin Source File

SOURCE=..\..\include\h224\h281handler.h
# End Source File
# Begin Source File

SOURCE=..\..\include\h224\h323h224.h
# End Source File
# Begin Source File

SOURCE=..\..\include\h224\q922.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\include\opal\buildopts.h
# End Source File
# Begin Source File

SOURCE=..\..\include\opal\buildopts.h.in

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

USERDEP__BUILD="..\..\configure.ac"	"..\..\configure.exe"	
# Begin Custom Build - Configuring Build Options
InputDir=\Storage\CVS_Head\opal\include\opal
InputPath=..\..\include\opal\buildopts.h.in

"$(InputDir)\buildopts.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cd ..\.. 
	.\configure 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

USERDEP__BUILD="..\..\configure.ac"	"..\..\configure.exe"	
# Begin Custom Build - Configuring Build Options
InputDir=\Storage\CVS_Head\opal\include\opal
InputPath=..\..\include\opal\buildopts.h.in

"$(InputDir)\buildopts.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cd ..\.. 
	.\configure 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

USERDEP__BUILD="..\..\configure.ac"	"..\..\configure.exe"	
# Begin Custom Build - Configuring Build Options
InputDir=\Storage\CVS_Head\opal\include\opal
InputPath=..\..\include\opal\buildopts.h.in

"$(InputDir)\buildopts.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cd ..\.. 
	.\configure 
	
# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "ASN Files"

# PROP Default_Filter ".asn"
# Begin Group "ASN Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\asn\gcc.h
# End Source File
# Begin Source File

SOURCE=..\..\include\asn\h225.h
# End Source File
# Begin Source File

SOURCE=..\..\include\asn\h235.h
# End Source File
# Begin Source File

SOURCE=..\..\include\asn\h245.h
# End Source File
# Begin Source File

SOURCE=..\..\include\asn\h248.h
# End Source File
# Begin Source File

SOURCE=..\..\include\asn\h4501.h
# End Source File
# Begin Source File

SOURCE=..\..\include\asn\h45010.h
# End Source File
# Begin Source File

SOURCE=..\..\include\asn\h45011.h
# End Source File
# Begin Source File

SOURCE=..\..\include\asn\h4502.h
# End Source File
# Begin Source File

SOURCE=..\..\include\asn\h4503.h
# End Source File
# Begin Source File

SOURCE=..\..\include\asn\h4504.h
# End Source File
# Begin Source File

SOURCE=..\..\include\asn\h4505.h
# End Source File
# Begin Source File

SOURCE=..\..\include\asn\h4506.h
# End Source File
# Begin Source File

SOURCE=..\..\include\asn\h4507.h
# End Source File
# Begin Source File

SOURCE=..\..\include\asn\h4508.h
# End Source File
# Begin Source File

SOURCE=..\..\include\asn\h4509.h
# End Source File
# Begin Source File

SOURCE=..\..\include\asn\h501.h
# End Source File
# Begin Source File

SOURCE=..\..\include\asn\ldap.h
# End Source File
# Begin Source File

SOURCE=..\..\include\asn\mcs.h
# End Source File
# Begin Source File

SOURCE=..\..\include\asn\t38.h
# End Source File
# Begin Source File

SOURCE=..\..\include\asn\x880.h
# End Source File
# End Group
# Begin Group "ASN Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\asn\gcc.cxx

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\asn\h225_1.cxx
# End Source File
# Begin Source File

SOURCE=..\asn\h225_2.cxx
# End Source File
# Begin Source File

SOURCE=..\asn\h235.cxx

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\asn\h245_1.cxx

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\asn\h245_2.cxx

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\asn\h245_3.cxx
# End Source File
# Begin Source File

SOURCE=..\asn\h248.cxx
# End Source File
# Begin Source File

SOURCE=..\asn\h4501.cxx

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\asn\h45010.cxx

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\asn\h45011.cxx

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\asn\h4502.cxx

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\asn\h4503.cxx

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\asn\h4504.cxx

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\asn\h4505.cxx

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\asn\h4506.cxx

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\asn\h4507.cxx

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\asn\h4508.cxx

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\asn\h4509.cxx

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\asn\h501.cxx
# End Source File
# Begin Source File

SOURCE=..\asn\mcs.cxx

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\asn\t38.cxx

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\asn\x880.cxx

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /I "..\..\include\asn"

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /I "$(IntDir)" /I "..\..\include\asn"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=..\asn\gcc.asn
# PROP Intermediate_Dir "..\..\include\asn"
# End Source File
# Begin Source File

SOURCE=..\asn\h225.asn
# PROP Intermediate_Dir "..\..\include\asn"
# End Source File
# Begin Source File

SOURCE=..\asn\h235.asn
# PROP Intermediate_Dir "..\..\include\asn"
# End Source File
# Begin Source File

SOURCE=..\asn\h245.asn
# PROP Intermediate_Dir "..\..\include\asn"
# End Source File
# Begin Source File

SOURCE=..\asn\h248.asn
# PROP Intermediate_Dir "..\..\include\asn"
# End Source File
# Begin Source File

SOURCE=..\asn\h4501.asn
# PROP Intermediate_Dir "..\..\include\asn"
# End Source File
# Begin Source File

SOURCE=..\asn\h45010.asn
# PROP Intermediate_Dir "..\..\include\asn"
# End Source File
# Begin Source File

SOURCE=..\asn\h45011.asn
# PROP Intermediate_Dir "..\..\include\asn"
# End Source File
# Begin Source File

SOURCE=..\asn\h4502.asn
# PROP Intermediate_Dir "..\..\include\asn"
# End Source File
# Begin Source File

SOURCE=..\asn\h4503.asn
# PROP Intermediate_Dir "..\..\include\asn"
# End Source File
# Begin Source File

SOURCE=..\asn\h4504.asn
# PROP Intermediate_Dir "..\..\include\asn"
# End Source File
# Begin Source File

SOURCE=..\asn\h4505.asn
# PROP Intermediate_Dir "..\..\include\asn"
# End Source File
# Begin Source File

SOURCE=..\asn\h4506.asn
# PROP Intermediate_Dir "..\..\include\asn"
# End Source File
# Begin Source File

SOURCE=..\asn\h4507.asn
# PROP Intermediate_Dir "..\..\include\asn"
# End Source File
# Begin Source File

SOURCE=..\asn\h4508.asn
# PROP Intermediate_Dir "..\..\include\asn"
# End Source File
# Begin Source File

SOURCE=..\asn\h4509.asn
# PROP Intermediate_Dir "..\..\include\asn"
# End Source File
# Begin Source File

SOURCE=..\asn\h501.asn
# PROP Intermediate_Dir "..\..\include\asn"
# End Source File
# Begin Source File

SOURCE=..\asn\ldap.asn
# PROP Intermediate_Dir "..\..\include\asn"
# End Source File
# Begin Source File

SOURCE=..\asn\mcs.asn
# PROP Intermediate_Dir "..\..\include\asn"
# End Source File
# Begin Source File

SOURCE=..\asn\t38.asn
# PROP Intermediate_Dir "..\..\include\asn"
# End Source File
# Begin Source File

SOURCE=..\asn\x880.asn
# PROP Intermediate_Dir "..\..\include\asn"
# End Source File
# End Group
# Begin Group "GSM Files"

# PROP Default_Filter ".c"
# Begin Group "GSM Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\codec\gsm\inc\config.h
# End Source File
# Begin Source File

SOURCE=..\codec\gsm\inc\gsm.h
# End Source File
# Begin Source File

SOURCE=..\codec\gsm\inc\private.h
# End Source File
# Begin Source File

SOURCE=..\codec\gsm\inc\proto.h
# End Source File
# Begin Source File

SOURCE=..\codec\gsm\inc\unproto.h
# End Source File
# End Group
# Begin Group "GSM Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\codec\gsm\src\add.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /w /W0 /Zi /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD BASE CPP /W1 /O2 /I "src\gsm\inc" /D NeedFunctionPrototypes=1
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\gsm\src\code.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /w /W0 /Zi /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD BASE CPP /W1 /O2 /I "src\gsm\inc" /D NeedFunctionPrototypes=1
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\gsm\src\decode.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /w /W0 /Zi /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD BASE CPP /W1 /O2 /I "src\gsm\inc" /D NeedFunctionPrototypes=1
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\gsm\src\gsm_create.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /w /W0 /Zi /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD BASE CPP /W1 /O2 /I "src\gsm\inc" /D NeedFunctionPrototypes=1
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\gsm\src\gsm_decode.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /w /W0 /Zi /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD BASE CPP /W1 /O2 /I "src\gsm\inc" /D NeedFunctionPrototypes=1
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\gsm\src\gsm_destroy.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /w /W0 /Zi /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD BASE CPP /W1 /O2 /I "src\gsm\inc" /D NeedFunctionPrototypes=1
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\gsm\src\gsm_encode.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /w /W0 /Zi /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD BASE CPP /W1 /O2 /I "src\gsm\inc" /D NeedFunctionPrototypes=1
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\gsm\src\gsm_lpc.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /w /W0 /Zi /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\gsm\src\gsm_option.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /w /W0 /Zi /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\gsm\src\long_term.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /w /W0 /Zi /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD BASE CPP /W1 /O2 /I "src\gsm\inc" /D NeedFunctionPrototypes=1
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\gsm\src\preprocess.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /w /W0 /Zi /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD BASE CPP /W1 /O2 /I "src\gsm\inc" /D NeedFunctionPrototypes=1
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\gsm\src\rpe.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /w /W0 /Zi /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD BASE CPP /W1 /O2 /I "src\gsm\inc" /D NeedFunctionPrototypes=1
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\gsm\src\short_term.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /w /W0 /Zi /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD BASE CPP /W1 /O2 /I "src\gsm\inc" /D NeedFunctionPrototypes=1
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\gsm\src\table.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /w /W0 /Zi /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD BASE CPP /W1 /O2 /I "src\gsm\inc" /D NeedFunctionPrototypes=1
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /w /W0 /O2 /I "..\codec\gsm\inc" /D NeedFunctionPrototypes=1 /D "WAV49"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# End Group
# Begin Group "VIC Files"

# PROP Default_Filter ""
# Begin Group "C Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\codec\vic\bv.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /w /W0 /D "WIN32"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /w /W0 /D "WIN32"
# SUBTRACT CPP /D "PTRACING" /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /w /W0 /D "WIN32"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\vic\huffcode.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /w /W0 /D "WIN32"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /w /W0 /D "WIN32"
# SUBTRACT CPP /D "PTRACING" /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /w /W0 /D "WIN32"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# Begin Group "CXX Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\codec\vic\dct.cxx

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /D "WIN32"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /D "WIN32"
# SUBTRACT CPP /D "PTRACING" /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /D "WIN32"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\codec\vic\encoder-h261.cxx"
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\vic\p64.cxx

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /D "WIN32"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /D "WIN32"
# SUBTRACT CPP /D "PTRACING" /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /D "WIN32"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\vic\p64encoder.cxx
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\vic\transmitter.cxx
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\vic\vid_coder.cxx
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# Begin Group "H Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\src\codec\vic\bsd-endian.h"
# End Source File
# Begin Source File

SOURCE=.\src\codec\vic\config.h
# End Source File
# Begin Source File

SOURCE=.\src\codec\vic\dct.h
# End Source File
# Begin Source File

SOURCE=".\src\codec\vic\encoder-h261.h"
# End Source File
# Begin Source File

SOURCE=.\src\codec\vic\grabber.h
# End Source File
# Begin Source File

SOURCE=".\src\codec\vic\p64-huff.h"
# End Source File
# Begin Source File

SOURCE=.\src\codec\vic\p64.h
# End Source File
# Begin Source File

SOURCE=.\src\codec\vic\p64encoder.h
# End Source File
# Begin Source File

SOURCE=.\src\codec\vic\transmitter.h
# End Source File
# Begin Source File

SOURCE=.\src\codec\vic\vid_coder.h
# End Source File
# End Group
# End Group
# Begin Group "LPC10 Files"

# PROP Default_Filter ""
# Begin Group "LPC10 Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\codec\lpc10\lpc10.h
# End Source File
# End Group
# Begin Group "LPC10 Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\codec\lpc10\src\analys.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\bsynz.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\chanwr.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\dcbias.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\decode_.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\deemp.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\difmag.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\dyptrk.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\encode_.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\energy.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\f2clib.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\ham84.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\hp100.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\invert.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\irc2pc.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\ivfilt.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\lpcdec.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\lpcenc.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\lpcini.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\lpfilt.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\median.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\mload.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\onset.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\pitsyn.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\placea.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\placev.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\preemp.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\prepro.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\random.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\rcchk.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\synths.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\tbdm.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\voicin.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\lpc10\src\vparms.c
# ADD CPP /W1 /I "../codec/lpc10"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# End Group
# Begin Group "G.726 Files"

# PROP Default_Filter ""
# Begin Group "G.726 Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\codec\g726\g72x.h
# End Source File
# Begin Source File

SOURCE=..\codec\g726\private.h
# End Source File
# End Group
# Begin Group "G.726 Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\codec\g726\g726_16.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\g726\g726_24.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\g726\g726_32.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\g726\g726_40.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\g726\g72x.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# End Group
# Begin Group "Speex Files"

# PROP Default_Filter ""
# Begin Group "Speex Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\codec\speex\libspeex\cb_search.h
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\filters.h
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\lpc.h
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\lsp.h
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\ltp.h
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\misc.h
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\modes.h
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\nb_celp.h
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\quant_lsp.h
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\sb_celp.h
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\speex.h
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\speex_bits.h
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\speex_callbacks.h
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\speex_header.h
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\speex_preprocess.h
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\stack_alloc.h
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\vbr.h
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\vq.h
# End Source File
# End Group
# Begin Group "Speex Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\codec\speex\libspeex\bits.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\cb_search.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\exc_10_16_table.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\exc_10_32_table.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\exc_20_32_table.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\exc_5_256_table.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\exc_5_64_table.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\exc_8_128_table.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\fftwrap.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\filters.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\gain_table.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\gain_table_lbr.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\hexc_10_32_table.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\hexc_table.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\high_lsp_tables.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\kiss_fft.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\kiss_fftr.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\lpc.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\lsp.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\lsp_tables_nb.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\ltp.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\math_approx.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\mdf.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\misc.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\modes.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\nb_celp.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\quant_lsp.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\sb_celp.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\smallft.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\speex.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\speex_callbacks.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\speex_header.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\speex_preprocess.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\vbr.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\codec\speex\libspeex\vq.c

!IF  "$(CFG)" == "OPAL_lib - Win32 Release"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 Debug"

# ADD CPP /W1 /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "OPAL_lib - Win32 No Trace"

# ADD CPP /W1 /O2 /D syn_filt=speex_syn_filt /D autocorr=speex_autocorr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# End Group
# Begin Group "iLBC Files"

# PROP Default_Filter ""
# Begin Group "iLBC Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\codec\iLBC\anaFilter.h
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\constants.h
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\createCB.h
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\doCPLC.h
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\enhancer.h
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\filter.h
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\FrameClassify.h
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\gainquant.h
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\getCBvec.h
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\helpfun.h
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\hpInput.h
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\hpOutput.h
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\iCBConstruct.h
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\iCBSearch.h
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\iLBC_decode.h
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\iLBC_define.h
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\iLBC_encode.h
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\LPCdecode.h
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\LPCencode.h
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\lsf.h
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\packing.h
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\StateConstructW.h
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\StateSearchW.h
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\syntFilter.h
# End Source File
# End Group
# Begin Group "iLBC Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\codec\iLBC\anaFilter.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\constants.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\createCB.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\doCPLC.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\enhancer.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\filter.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\FrameClassify.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\gainquant.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\getCBvec.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\helpfun.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\hpInput.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\hpOutput.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\iCBConstruct.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\iCBSearch.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\iLBC_decode.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\iLBC_encode.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\LPCdecode.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\LPCencode.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\lsf.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\packing.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\StateConstructW.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\StateSearchW.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\codec\iLBC\syntFilter.c
# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# End Group
# End Target
# End Project
