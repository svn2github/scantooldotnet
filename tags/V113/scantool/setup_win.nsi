; Script generated by the HM NIS Edit Script Wizard.

; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "ScanTool.net for Windows"
!define PRODUCT_VERSION "v1.13"
!define PRODUCT_VERSION_NUM "113"
!define PRODUCT_PUBLISHER "ScanTool.net, LLC"
!define PRODUCT_WEB_SITE "http://www.scantool.net"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\${PRODUCT_NAME}"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"
!define PRODUCT_STARTMENU_REGVAL "NSIS:StartMenuDir"

; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!insertmacro MUI_PAGE_LICENSE "gpl.txt"
; Components page
!insertmacro MUI_PAGE_COMPONENTS
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Start menu page
var ICONS_GROUP
!define MUI_STARTMENUPAGE_NODISABLE
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "OBD-II Software\ScanTool.net"
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "${PRODUCT_STARTMENU_REGVAL}"
!insertmacro MUI_PAGE_STARTMENU Application $ICONS_GROUP
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\ScanTool.exe"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\readme.txt"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "scantool_net${PRODUCT_VERSION_NUM}win.exe"
InstallDir "$PROGRAMFILES\ScanTool.net_win"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

Section "" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer
  File "ScanTool.exe"
  File "scantool.dat"
  File "codes.dat"
  File "readme.txt"
  File "gpl.txt"

; Shortcuts
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  CreateDirectory "$SMPROGRAMS\$ICONS_GROUP"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\$(^Name).lnk" "$INSTDIR\ScanTool.exe"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\$(^Name) Read Me.lnk" "$INSTDIR\readme.txt"
  CreateShortCut "$DESKTOP\$(^Name).lnk" "$INSTDIR\ScanTool.exe"
  !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section /o "Source Code" SEC02
  SetOutPath "$INSTDIR\source"
  File "version.h"
  File "trouble_code_reader.h"
  File "trouble_code_reader.c"
  File "serial.h"
  File "serial.c"
  File "sensors.h"
  File "sensors.c"
  File "scantool.rc"
  File "scantool.ico"
  File "scantool.h"
  File "about.c"
  File "scantool.dat"
  File "resource.h"
  File "readme.txt"
  File "todo.txt"
  File "options.h"
  File "options.c"
  File "makefile"
  File "main_menu.h"
  File "main_menu.c"
  File "main.c"
  File "gpl.txt"
  File "globals.h"
  File "error_handlers.h"
  File "error_handlers.c"
  File "datafile.h"
  File "custom_gui.h"
  File "custom_gui.c"
  File "codes.dat"
  File "about.h"
  File "ScanTool.dev"
  
  ; Shortcuts
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\$(^Name) Source Code.lnk" "$INSTDIR\source\"
  !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section -AdditionalIcons
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  WriteIniStr "$SMPROGRAMS\$ICONS_GROUP\Website.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Uninstall $(^Name).lnk" "$INSTDIR\uninst.exe"
  !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\ScanTool.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\ScanTool.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd

; Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC01} ""
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC02} "Select this option to install C source files."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort
FunctionEnd

Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd

Section Uninstall
  !insertmacro MUI_STARTMENU_GETFOLDER "Application" $ICONS_GROUP
  Delete "$INSTDIR\source\*.*"
  Delete "$INSTDIR\*.*"

  Delete "$SMPROGRAMS\$ICONS_GROUP\Uninstall $(^Name).lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\$(^Name) Source Code.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\$(^Name) Read Me.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\$(^Name).lnk"
  Delete "$DESKTOP\$(^Name).lnk"

  ClearErrors
  ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ScanTool.net for DOS" "UninstallString"
  IfErrors 0 skipDeletion
  Delete "$SMPROGRAMS\$ICONS_GROUP\Website.url"
  RMDir "$SMPROGRAMS\$ICONS_GROUP"
  skipDeletion:

  RMDir "$INSTDIR\source"
  RMDir "$INSTDIR"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd