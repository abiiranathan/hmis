; HMIS Installer Script
; Based on Qt application settings:
;   - ApplicationName: "HMIS"
;   - Organization: "Yo Medical Files (U) Limited"
;   - Domain: "yomedicalfiles.com"

;--------------------------------
; Include Modern UI and utilities

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"

;--------------------------------
; General Definitions

Name "HMIS"
OutFile "HMIS_Installer.exe"
InstallDir "$PROGRAMFILES64\Yo Medical Files\HMIS"
InstallDirRegKey HKLM "Software\Yo Medical Files\HMIS" "Install_Dir"
RequestExecutionLevel admin
BrandingText "Yo Medical Files (U) Limited"

; Version info (set via /DVERSION from workflow)
!ifndef VERSION
  !define VERSION "1.0.0" ; Fallback if not passed
!endif
VIProductVersion "${VERSION}.0" ; NSIS expects X.Y.Z.W
VIAddVersionKey "ProductName" "HMIS"
VIAddVersionKey "CompanyName" "Yo Medical Files (U) Limited"
VIAddVersionKey "LegalCopyright" "© 2024 Yo Medical Files (U) Limited"
VIAddVersionKey "FileDescription" "HMIS Application Installer"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "ProductVersion" "${VERSION}"

;--------------------------------
; Interface Settings

!define MUI_ABORTWARNING
!define MUI_ICON "favicon.ico" ; Use favicon.ico from build/release/deploy
!define MUI_UNICON "favicon.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT
!define MUI_HEADERIMAGE_BITMAP "logo.png" ; Use logo.png as header image
!define MUI_WELCOMEFINISHTITLEBITMAP "logo.png" ; Use logo.png for welcome/finish pages

;--------------------------------
; Pages

!insertmacro MUI_PAGE_WELCOME
!if EXIST_LICENSE
  !insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
!endif
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

;--------------------------------
; Languages

!insertmacro MUI_LANGUAGE "English"

;--------------------------------
; Installer Sections

Section "HMIS Application" SecMain

  SectionIn RO
  
  ; Install files
  SetOutPath "$INSTDIR"
  File /r ".\*.*" ; Files in build/release/deploy (HMIS.exe, Qt DLLs, etc.)
  
  ; Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  ; Add registry entries
  WriteRegStr HKLM "Software\Yo Medical Files\HMIS" "Install_Dir" "$INSTDIR"
  WriteRegStr HKLM "Software\Yo Medical Files\HMIS" "Version" "${VERSION}"
  
  ; Add to Add/Remove Programs
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HMIS" "DisplayName" "HMIS"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HMIS" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HMIS" "DisplayIcon" '"$INSTDIR\HMIS.exe",0'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HMIS" "DisplayVersion" "${VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HMIS" "Publisher" "Yo Medical Files (U) Limited"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HMIS" "URLInfoAbout" "https://yomedicalfiles.com"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HMIS" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HMIS" "NoRepair" 1
  
  ; Create shortcuts
  CreateDirectory "$SMPROGRAMS\Yo Medical Files"
  CreateShortcut "$SMPROGRAMS\Yo Medical Files\HMIS.lnk" "$INSTDIR\HMIS.exe" "" "$INSTDIR\HMIS.exe" 0
  CreateShortcut "$SMPROGRAMS\Yo Medical Files\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  CreateShortcut "$DESKTOP\HMIS.lnk" "$INSTDIR\HMIS.exe" "" "$INSTDIR\HMIS.exe" 0

SectionEnd

;--------------------------------
; Uninstaller Section

Section "Uninstall"

  ; Remove registry entries
  DeleteRegKey HKLM "Software\Yo Medical Files\HMIS"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HMIS"

  ; Remove files and uninstaller
  RMDir /r "$INSTDIR"

  ; Remove shortcuts
  Delete "$SMPROGRAMS\Yo Medical Files\HMIS.lnk"
  Delete "$SMPROGRAMS\Yo Medical Files\Uninstall.lnk"
  Delete "$DESKTOP\HMIS.lnk"
  
  ; Remove program group if empty
  RMDir "$SMPROGRAMS\Yo Medical Files"

SectionEnd

;--------------------------------
; Functions

Function .onInit
  ; Check for previous installation
  ReadRegStr $R0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HMIS" "UninstallString"
  StrCmp $R0 "" done
  
  MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
    "HMIS is already installed. $\n$\nClick 'OK' to remove the previous version or 'Cancel' to abort this installation." \
    IDOK uninst
  Abort
  
  uninst:
    ClearErrors
    ExecWait '$R0 _?=$INSTDIR'
    
    IfErrors no_remove_uninstaller
      Delete $R0
    no_remove_uninstaller:
    
  done:
FunctionEnd
