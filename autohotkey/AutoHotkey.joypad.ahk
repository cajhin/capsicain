; CHANGELOG
; 20201121 Changed buttons to XBox Series X Controller. Nicer mouse acceleration

;************JOYPAD as Mouse
; Tune the following values for Small/Med/Large stick movement
  JoyMultiplierSlow := 0.30
  JoyMultiplierMedi := 0.60
  JoyMultiplierFast := 1.2
; Deadzone - increase if you have sloppy joystick displacement-from-center
  JoyDeadzone := 3
; Change these values to use joystick button numbers other than 1, 2, and 3 for
; the left, right, and middle mouse buttons.  Available numbers are 1 through 32.
; Use the Joystick Test Script to find out your joystick's numbers more easily.
  ButtonLeft := 1
  ButtonRight := 2
;  ButtonMiddle := 10  ;I don't need the middle mouse button
; If your system has more than one joystick, set this value to hardcode
  JoystickNumber := 1

;constants below
JoystickActive:=0
JoystickPolltimerInactive:=1000
JoystickPolltimerActive := 10
JoystickPrefix = %JoystickNumber%Joy
Hotkey, %JoystickPrefix%%ButtonLeft%, ButtonLeft
Hotkey, %JoystickPrefix%%ButtonRight%, ButtonRight
;Hotkey, %JoystickPrefix%%ButtonMiddle%, ButtonMiddle

;xbox controller goes from 0..100, with 50 as axis center
JoyCenter := 50
JoyMaxFromCenter := 50 ; max +-50 from center

SetTimer, WatchJoystick, %JoystickPolltimerInactive%  ; Monitor the movement of the joystick.

return  ; End of auto-execute section.

;************************Gamepad*********
; The subroutines below do not use KeyWait because that would sometimes trap the
; WatchJoystick quasi-thread beneath the wait-for-button-up thread, which would
; effectively prevent mouse-dragging with the joystick.

ButtonLeft:
SetMouseDelay, -1  ; Makes movement smoother.
MouseClick, left,,, 1, 0, D  ; Hold down the left mouse button.
SetTimer, WaitForLeftButtonUp, 10
return

ButtonRight:
SetMouseDelay, -1  ; Makes movement smoother.
MouseClick, right,,, 1, 0, D  ; Hold down the right mouse button.
SetTimer, WaitForRightButtonUp, 10
return

/*
ButtonMiddle:
SetMouseDelay, -1  ; Makes movement smoother.
MouseClick, middle,,, 1, 0, D  ; Hold down the right mouse button.
SetTimer, WaitForMiddleButtonUp, 10
return
*/

WaitForLeftButtonUp:
if GetKeyState(JoystickPrefix . ButtonLeft)
    return  ; button is still down, so keep waiting. Otherwise, the button has been released
SetTimer, WaitForLeftButtonUp, off
SetMouseDelay, -1  ; Makes movement smoother.
MouseClick, left,,, 1, 0, U  ; Release the mouse button.
return

WaitForRightButtonUp:
if GetKeyState(JoystickPrefix . ButtonRight)
    return  ; The button is still, down, so keep waiting.
SetTimer, WaitForRightButtonUp, off
MouseClick, right,,, 1, 0, U  ; Release the mouse button.
return

WaitForMiddleButtonUp:
if GetKeyState(JoystickPrefix . ButtonMiddle)
    return  ; The button is still, down, so keep waiting.
SetTimer, WaitForMiddleButtonUp, off
MouseClick, middle,,, 1, 0, U  ; Release the mouse button.
return


;;;;;;;;;;;;;;;;;;;;;;;
^#j::
{
	if JoystickActive=1
	{
		SetTimer, WatchJoystick, Off
		SplashTextOn, , , Joystick OFF by J
		Sleep, 1000
		SplashTextOff
		JoystickActive :=0
		return
	}
		SetTimer, WatchJoystick, On
		SplashTextOn, , , Joystick ON by J
		Sleep, 1000
		SplashTextOff
		JoystickActive :=1
		return
	
}

WatchJoystick:

;test if a Joystick is there
GetKeyState, JoyPOV, %JoystickNumber%JoyPOV
if JoyPOV =
{
	if JoystickActive=1
	{
		SetTimer, WatchJoystick, Off
		SplashTextOn, , , Joystick turned OFF
		Sleep, 3000
		SplashTextOff
		JoystickActive :=0
		SetTimer, WatchJoystick, %JoystickPolltimerInactive%
	}
	return
}
if JoystickActive=0
{
	SetTimer, WatchJoystick, Off
	SplashTextOn, , , Joystick ON
	Sleep, 1000
	SplashTextOff
	JoystickActive :=1
	SetTimer, WatchJoystick, %JoystickPolltimerActive%
}

;handle analog sticks
;only interested in xy (left stick) and r (right stick y)
SetFormat, float, 03
GetKeyState, joyx, %JoystickNumber%JoyX
GetKeyState, joyy, %JoystickNumber%JoyY
;GetKeyState, joyz, %JoystickNumber%JoyZ
GetKeyState, joyr, %JoystickNumber%JoyR
;GetKeyState, joyu, %JoystickNumber%JoyU
;GetKeyState, joyv, %JoystickNumber%JoyV

if (joyx > JoyCenter + JoyDeadzone)
    DeltaX := (joyx - JoyCenter) - JoyDeadzone
else if (joyx < JoyCenter - JoyDeadzone)
{
    DeltaX := (joyx - JoyCenter) + JoyDeadzone
}
else
    DeltaX := 0
	
if (joyy > JoyCenter + JoyDeadzone)
    DeltaY := (joyy - JoyCenter) - JoyDeadzone
else if (joyy < JoyCenter - JoyDeadzone)
    DeltaY := (joyy - JoyCenter) + JoyDeadzone
else
    DeltaY := 0
	
if (DeltaX != 0 or DeltaY != 0 )
{
    SetMouseDelay, -1  ; Makes movement smoother.
	if(Abs(DeltaX) < 20)
		DeltaX*=JoyMultiplierSlow
	else if (Abs(DeltaX) > (JoyMaxFromCenter -5))
		DeltaX*=JoyMultiplierFast
	else
		DeltaX*=JoyMultiplierMedi

	if(Abs(DeltaY) < 20)
		DeltaY*=JoyMultiplierSlow
	else if (Abs(DeltaY) > (JoyMaxFromCenter -5))
		DeltaY*=JoyMultiplierFast
	else
		DeltaY*=JoyMultiplierMedi
    MouseMove, DeltaX, DeltaY, 0, R
}

if (joyr > 60) ; JoyCenter + JoyDeadzone
{
    Send {WheelDown}
    if joyr <65
      Sleep 200
    if joyr <95
      Sleep 50
    Sleep 10
}
else if (joyr < 40) ; JoyCenter - JoyDeadzone
{
    Send {WheelUp}
    if joyr >35
      Sleep 200
    if joyr >5
      Sleep 50
    Sleep 10
}

;check POV
GetKeyState, JoyPOV, %JoystickNumber%JoyPOV
if JoyPOV = -1 
   return 
If JoyPov = 9000 
   Send {Right} 
else if JoyPov = 4500 
   Send {Up}{Right} 
else if JoyPov = 0 
   Send {UP} 
else if JoyPov = 13500 
   Send {Down}{Right} 
else if JoyPov = 18000 
   Send {Down} 
else if JoyPov = 22500 
   Send {Down}{Left} 
else if JoyPov = 27000 
   Send {Left} 
else if JoyPov = 31500 
   Send {Up}{Left}
Sleep 100
return


;test joy buttons
/*
Joy1::A
Joy2::B
Joy3::C
Joy4::D
Joy5::E
Joy6::F
Joy7::G
Joy8::H
Joy9::I
Joy10::J
Joy11::K
Joy12::L
Joy13::M
Joy14::N
Joy15::O
Joy16::P
Joy17::Q
*/

;Map Xbox series X buttons
;Joy1::Click Left ;Bt_A handled above for drag&drop
;Joy2::Click Right ;Bt_B dito
;Joy3::C
Joy4::Click X1 ;Bt_X
Joy5::Click X2 ;Bt_Y
;Joy6::F
Joy7::Send +{Left} ;Bt_LeftShoulder
Joy8::Send +{Right} ;Bt_RightShoulder
;Joy9::I
;Joy10::J
Joy11::Send {PgUp} ;Bt_back
Joy12::Send {PgDn} ;Bt_menu
Joy13::M ;Bt_Xbox
Joy14::N ;Bt_leftStick
Joy15::O ;Bt_rightStick
Joy16::Send {Launch_Mail} ;Bt_send
;Joy17::Q

/*
;map Xbox 360 buttons
Joy3::Click X1
Joy4::Click X2
Joy5::Send +{Left}
Joy6::Send +{Right}
Joy7::Send {Launch_Mail}
Joy8::Send {PgDn}
Joy9::Click Left
Joy10::Click Left
*/

;**Emulate Mouse buttons 3,4,5 with keyboard
AppsKey::MButton
<#1::Click Middle
<#2::Click X1
<#3::Click X2
<#PgUp::WheelUp
<#PgDn::WheelDown

