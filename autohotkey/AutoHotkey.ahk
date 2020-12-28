; This is a support script for capsicain
; See autohotkey.com - it's a mighty tool

; AHK Syntax:
;  # Win   ! Alt  ^ Ctrl  + Shift  <# LeftWin
; * any modifier
; ~ the key is always forwarded to windows
; $ prevents the hotkey from triggering itself

; CHANGELOG
; 20181203: F14/F15 + char translates to Umlaut etc.
;           Support CapsLock
; 20181207: Sleep first when receiving F14/15 commands
; 20181210: Alt+Win+H Exit AHK. Cleanup. Back to single script.
; 20181210: Cleanup, Z<>Y

;*********init**********
SendMode Input  ; Recommended for new scripts due to its superior speed and reliability.
#include AutoHotkey.joypad.ahk
return  ; End of auto-execute section.
;*************************END AUTORUN**************


;;;;middle mouse with a key - disabled for now
;*Launch_Mail::  ; ~ on logitech does right click. Remapped ~ to SC_MAIL with capsicain
;SendEvent {Blind}{MButton down}
;keyWait Launch_Mail  ; Prevents keyboard auto-repeat from repeating the mouse click.
;SendEvent {Blind}{MButton up}
;return


;****************************************************
; 10 CLIPBOARDS
; capsicain must send F14
; CapsLock+Number copies any object to Clipboard 0..9
; Windows clipboard does not change
;****************************************************
F14 & 1 up::jCopy2Clipboard(1)
F14 & 2 up::jCopy2Clipboard(2)
F14 & 3 up::jCopy2Clipboard(3)
F14 & 4 up::jCopy2Clipboard(4)
F14 & 5 up::jCopy2Clipboard(5)
F14 & 6 up::jCopy2Clipboard(6)
F14 & 7 up::jCopy2Clipboard(7)
F14 & 8 up::jCopy2Clipboard(8)
F14 & 9 up::jCopy2Clipboard(9)
F14 & 0 up::jCopy2Clipboard(0)

jCopy2Clipboard(clipNumber)
{
  global ;do not create a local copy of ClipSavedN
  clipname=ClipSaved%clipnumber%
  clipTmp:=clipboardAll
  clipboard=
  Send ^c
  ClipWait,1,1
  %clipname%:=clipboardAll
  clipboard:=clipTmp
  clipTmp=
}

F15 & 1 up::jPasteFromClipboard(1)
F15 & 2 up::jPasteFromClipboard(2)
F15 & 3 up::jPasteFromClipboard(3)
F15 & 4 up::jPasteFromClipboard(4)
F15 & 5 up::jPasteFromClipboard(5)
F15 & 6 up::jPasteFromClipboard(6)
F15 & 7 up::jPasteFromClipboard(7)
F15 & 8 up::jPasteFromClipboard(8)
F15 & 9 up::jPasteFromClipboard(9)
F15 & 0 up::jPasteFromClipboard(0)

jPasteFromClipboard(clipNumber)
{
    Sleep 20
	ClipTmp:=ClipboardAll
	clipboard:=ClipSaved%clipnumber%
	Send ^v
	Sleep 50
	clipboard:=ClipTmp
	ClipTmp=
}


;*******************************************************************************
; LEFT WIN KEY I: special functions (the right win key remains windows standard)
;*******************************************************************************

<#h::reload  ;reload this script
!<#h::ExitApp ;exit AHK

;*****Window control
; QAZX: Maximize, Restore, Minimize, Close
<#q::  ;toggle the Maximized state
{
	WinGet, isMaximized, MinMax, A
	if %isMaximized%
		WinRestore A
	else
		WinMaximize A
	return
}

<#a::   ;restore the last minimized window
{
	if %last_minimized_id%
	{
		WinRestore ahk_id %last_minimized_id%
		WinActivate ahk_id %last_minimized_id%
		last_minimized_id=0
	}
	return
}

<#z::   ;minimize active win
{
	WinGet, last_minimized_id, ID, A
	IfWinActive PSPad                   ;PSPad doesn't minimize correctly with WinMinimize
		PostMessage, 0x112, 0xF020,,, A       ; 0x112 = WM_SYSCOMMAND, 0xF020 = SC_MINIMIZE
	else 
		WinMinimize ahk_id %last_minimized_id%
	return
}

<#x::Send !{F4}


;******************************************************************************
;LEFT WIN KEY II : Start apps
;******************************************************************************

;Interception debug and release
<#.::RunApp("ahk_exe capsicain.exe","capsicain.exe")
<#,::RunApp("ahk_exe capsicain.exe","C:\wsl\home\git\capsicain\x64\Debug\capsicain.exe")
<#f::RunApp("ahk_exe LibBrowser.exe","..\mine\libbrowser\libbrowser.exe")
<#u::RunApp("ahk_exe ubuntu1804.exe","wsl.exe")
<#d::RunApp("ahk_exe cmd.exe","cmd.exe /k cd..")
<#c::RunApp("ahk_exe calc.exe","calc.exe")
<#t::RunApp("ahk_exe cherrytree.exe","..\cherrytree\bin\cherrytree.exe")
<#i::RunApp("ahk_exe iview.exe","..\irfanview\iview.exe")
<#k::RunApp("ahk_exe keepass.exe","C:\app\keepass\KeePass.exe")
<#w::RunApp("ahk_class Notepad++","..\npp\notepad++.exe")

; LWin+E for XYPlorer, RWin+E or Alt+Win+E for Windows Explorer
<#!e::RunApp("ahk_class ExploreWClass","explorer.exe /e,c:\")
<#e::
	if !RunApp("ahk_exe XYPlorer.exe","..\XYPlorer\XYPlorer.exe /ini=..\conf\local\XYPlorer\xyplorer.ini")
		RunApp("ahk_class ExploreWClass","explorer.exe /e,c:\")
return

; FireFox, or Edge if FF is not installed
<#b::
if !RunApp("ahk_class MozillaWindowClass","C:\Program Files\Mozilla Firefox\firefox.exe")
	RunApp("Edge","start microsoft-edge:")
return


;START / ACTIVATE / MINIMIZE applications
;if not running or ctrl+win: start   else if active: minimize  else activate
;appId can be either (a part of) the window title ("- Opera")
;or the ahk_class / ahk_exe ("ahk_class ExploreWClass" for Explorer - use Windows Spy from the AHK tray icon menu)
RunApp(appId,exePath)
{
    Sleep 20
	if !WinExist(appId) or GetKeyState("Control")
	{
		Run %exePath%,"",UseErrorLevel
		if ErrorLevel
			return 0
	}
	else IfWinActive %appId%
	{
	WinMinimize %appId%
		return 1
	}
	WinWait %appId%,,3
	WinActivate
	return 1
}

