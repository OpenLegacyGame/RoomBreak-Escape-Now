Unicode true
!include "MUI2.nsh"
!include "x64.nsh"
!include "LogicLib.nsh"
!ifndef PACKAGE_DIR
!error "Set PACKAGE_DIR to the Windows release directory"
!endif
!ifndef VC_REDIST
!error "Set VC_REDIST to Microsoft's vc_redist.x64.exe"
!endif
!ifndef OUTPUT_FILE
!define OUTPUT_FILE "RoomBreak Escape Now Setup.exe"
!endif
Name "RoomBreak Escape Now"
OutFile "${OUTPUT_FILE}"
InstallDir "$PROGRAMFILES64\RoomBreak Escape Now"
InstallDirRegKey HKLM "Software\RoomBreakRebuilt" "InstallDir"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
ShowInstDetails show
!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "SimpChinese"
Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_ICONSTOP "This application requires 64-bit Windows."
    Abort
  ${EndIf}
  SetRegView 64
  SetShellVarContext all
FunctionEnd
Section "RoomBreak Escape Now"
  InitPluginsDir
  SetOutPath "$PLUGINSDIR"
  File /oname=vc_redist.x64.exe "${VC_REDIST}"
  ExecWait '"$PLUGINSDIR\vc_redist.x64.exe" /install /quiet /norestart' $0
  ${If} $0 == 3010
    SetRebootFlag true
  ${ElseIf} $0 != 0
  ${AndIf} $0 != 1638
    MessageBox MB_ICONSTOP "Microsoft Visual C++ runtime installation failed ($0)."
    Abort
  ${EndIf}
  SetOutPath "$INSTDIR"
  ; Only program payload belongs here. User progress and settings are never included.
  File /r "${PACKAGE_DIR}\*"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "Software\RoomBreakRebuilt" "InstallDir" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\RoomBreakRebuilt" "DisplayName" "RoomBreak Escape Now"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\RoomBreakRebuilt" "DisplayVersion" "1.0.1"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\RoomBreakRebuilt" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\RoomBreakRebuilt" "InstallLocation" "$INSTDIR"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\RoomBreakRebuilt" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\RoomBreakRebuilt" "NoRepair" 1
  CreateDirectory "$SMPROGRAMS\RoomBreak Escape Now"
  Delete "$SMPROGRAMS\RoomBreakRebuilt\RoomBreakRebuilt.lnk"
  Delete "$SMPROGRAMS\RoomBreakRebuilt\RoomBreak - Escape Now.lnk"
  RMDir "$SMPROGRAMS\RoomBreakRebuilt"
  Delete "$INSTDIR\RoomBreakRebuilt.exe"
  CreateShortcut "$SMPROGRAMS\RoomBreak Escape Now\RoomBreak Escape Now.lnk" "$INSTDIR\RoomBreak Escape Now.exe"
SectionEnd
Section "Uninstall"
  SetRegView 64
  SetShellVarContext all
  Delete "$SMPROGRAMS\RoomBreak Escape Now\RoomBreak Escape Now.lnk"
  RMDir "$SMPROGRAMS\RoomBreak Escape Now"
  Delete "$INSTDIR\RoomBreak Escape Now.exe"
  Delete "$SMPROGRAMS\RoomBreakRebuilt\RoomBreakRebuilt.lnk"
  Delete "$SMPROGRAMS\RoomBreakRebuilt\RoomBreak - Escape Now.lnk"
  RMDir "$SMPROGRAMS\RoomBreakRebuilt"
  Delete "$INSTDIR\RoomBreakRebuilt.exe"
  Delete "$INSTDIR\OpenAL32.dll"
  Delete "$INSTDIR\d3dcompiler_47.dll"
  Delete "$INSTDIR\libEGL.dll"
  Delete "$INSTDIR\libGLESv2.dll"
  Delete "$INSTDIR\libcrypto-3-x64.dll"
  Delete "$INSTDIR\libcurl.dll"
  Delete "$INSTDIR\libssl-3-x64.dll"
  Delete "$INSTDIR\zlib1.dll"
  Delete "$INSTDIR\COPYRIGHT.txt"
  RMDir /r "$INSTDIR\Content"
  RMDir /r "$INSTDIR\axslc"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"
  DeleteRegKey HKLM "Software\RoomBreakRebuilt"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\RoomBreakRebuilt"
  ; Deliberately preserve every user's LocalAppData\RoomBreakRebuilt directory.
SectionEnd
