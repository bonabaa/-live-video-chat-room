;NSIS Modern User Interface version 1.70
;Start Menu Folder Selection Example Script
;Written by Joost Verburg

;--------------------------------
;Include Modern UI

  !include "MUI.nsh"
 ; !include "Image.nsh"

;--------------------------------
;General
!define MUI_ICON "${BBQBIN_DIR}\yy.ico"
!define MUI_UNICON "${BBQBIN_DIR}\yy.ico"

!define X_PRODUCT "CCStreamer" ;Define your own software name here
!define X_VERSION "1.0.1" ;Define your own software version here

!define X_REGPATH "SOFTWARE\CCStreamer"

!define BBQBIN_DIR  "."

  ;Name and file
Name "${X_PRODUCT} ${X_VERSION}"
;OutFile "./CCStreamer_installer.exe" ; specified in command line, /D"OutFile ./vfonsvr_[d/nd]_installer.exe"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\${X_PRODUCT}"
  
  ;Get installation folder from registry if available
  InstallDirRegKey HKLM "${X_REGPATH}" ""

;--------------------------------
;Variables

  Var MUI_TEMP
  Var STARTMENU_FOLDER

;--------------------------------
;Interface Settings

  !define MUI_HEADERIMAGE
 
  ShowInstDetails show
  ShowUninstDetails show

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "license.rtf"
  ;!insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  
  ;Start Menu Folder Page Configuration
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM" 
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "${X_REGPATH}" 
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
  
  !insertmacro MUI_PAGE_STARTMENU Application $STARTMENU_FOLDER
  !insertmacro MUI_PAGE_INSTFILES

  !define MUI_FINISHPAGE_LINK "Visit http://www.yymeeting.com/"
  !define MUI_FINISHPAGE_LINK_LOCATION "http://www.yymeeting.com/"
  !define MUI_FINISHPAGE_LINK_COLOR "0000FF"

  ;define MUI_FINISHPAGE_RUN "$INSTDIR\bbqsvrcfg.exe"

  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"
  !insertmacro MUI_LANGUAGE "SimpChinese"


;--------------------------------
;Language Strings

  ;Description
  LangString DESC_SecCore ${LANG_ENGLISH} "Copy the ${X_PRODUCT} to the application folder and install as system service."

;--------------------------------
;Installer Sections

!define SF_SELECTED 1
ShowInstDetails nevershow
ShowUnInstDetails nevershow
 
Section "CCStreamer (required)" SecCore
  SectionIn 1 2 RO
  SetOutPath "$INSTDIR"
  RMDir /r "$INSTDIR\yy"
  File "${BBQBIN_DIR}\YYSpriteX-2.ocx"
  File "${BBQBIN_DIR}\libyy.dll"
  File "${BBQBIN_DIR}\msvcr71.dll"
  File "${BBQBIN_DIR}\yavcodec.dll"
  File "${BBQBIN_DIR}\yImageEngine.dll"
  File "${BBQBIN_DIR}\CCVisionMcuStreams.exe"
  File "${BBQBIN_DIR}\yclient.dll"
  File "${BBQBIN_DIR}\ythread.dll"
  File "${BBQBIN_DIR}\sqlite3.dll"
  File "${BBQBIN_DIR}\cc.dat"
  
  ;Store installation folder
  WriteRegStr HKLM "${X_REGPATH}" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
    
	;Create shortcuts
	CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\CC Streamer.lnk" "$INSTDIR\CCVisionMcuStreams.exe"

   CreateShortCut "$DESKTOP\CC Streamer.lnk" "$INSTDIR\CCVisionMcuStreams.exe"

  !insertmacro MUI_STARTMENU_WRITE_END


  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${X_PRODUCT}" "DisplayName" "${X_PRODUCT}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${X_PRODUCT}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  
 
  ExecWait 'regsvr32.exe /s "$INSTDIR\YYSpriteX-2.ocx"'
  CopyFiles '$INSTDIR\yavcodec.dll'  "$SYSDIR\yavcodec.dll"
  
 
SectionEnd

;--------------------------------
;Descriptions

  ;Assign language strings to sections
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecCore} $(DESC_SecCore)
!insertmacro MUI_FUNCTION_DESCRIPTION_END
 
 Function .onInit

  ReadRegStr $R0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
   StrCmp $R0 "" 0 is_winnt
   MessageBox MB_OK "Your operation system is not a Windows 2000, XP, or Server, ${X_PRODUCT} cannot be installed."
     Abort ; we are not NT.
   is_winnt:

  ;ReadRegStr $R0 HKLM "${X_REGPATH}\CurrentVersion" CurrentVersion
  ;ReadRegStr $R0 HKLM "${X_REGPATH}" "Start Menu Folder"
   new_installation:
 

  !insertmacro MUI_LANGDLL_DISPLAY
 


FunctionEnd
 
;Section "Main"
;  !insertmacro DisplayImage 'cc.bmp'
;SectionEnd
Function .onInstSuccess
 	Exec 'CCVisionMcuStreams.exe'
Functionend
;--------------------------------
;Uninstaller Section

Section "Uninstall"
	 
  ;ADD YOUR OWN FILES HERE...
  ExecWait 'regsvr32.exe /u /s "$INSTDIR\YYSpriteX-2.ocx"'

  Delete "$INSTDIR\libyy.dll"
  Delete "$INSTDIR\yavcodec.dll"
  Delete "$INSTDIR\msvcr71.dll"
  Delete "$INSTDIR\YYSpriteX-2.ocx"
  Delete "$INSTDIR\CCVisionMcuStreams.exe"
   Delete "$INSTDIR\yImageEngine.exe"
  Delete "$INSTDIR\CCStreamerRecord.exe"
  Delete "$DESKTOP\CC Streamer.lnk"
  
  RMDir /r "$INSTDIR\cb"
  RMDir /r "$INSTDIR\yy"
  RMDir /r "$INSTDIR\"

  Delete "$INSTDIR\Uninstall.exe"

  RMDir "$INSTDIR"
  
  !insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP
    
    Delete "$SMPROGRAMS\$MUI_TEMP\Uninstall.lnk" 
    Delete "$SMPROGRAMS\$MUI_TEMP\CC Streamer.lnk" 
    RMDir /r "$SMPROGRAMS\$MUI_TEMP" 
	;RMDir /r "$SMPROGRAMS\$STARTMENU_FOLDER"
  ;Delete empty start menu parent diretories
  StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP"
 
  startMenuDeleteLoop:
    RMDir $MUI_TEMP
    GetFullPathName $MUI_TEMP "$MUI_TEMP\.."
    
    IfErrors startMenuDeleteLoopDone
  
    StrCmp $MUI_TEMP $SMPROGRAMS startMenuDeleteLoopDone startMenuDeleteLoop
  startMenuDeleteLoopDone:

  ;DeleteRegKey HKLM "${X_REGPATH}"
  
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${X_PRODUCT}"

SectionEnd
