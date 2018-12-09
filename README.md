# capsicain

C++ tool that uses the Interception driver https://github.com/oblitum/Interception to monitor and modify keypresses, at a very low level.

##Why?

I touch type. I code. US standard layout is the only sane layout for writing C-style code.  
I need a keyboard setup where I can keep my fingers on the home row all the time.  
The position of Left/Right, Home/End, Insert and so on is bad.  
Also, I need to type Ümläuts quite often, which is a pain on US keyboards.  
While I'm at it, I like to have standard shortcuts on all my machines to start browser/Notepad++/Calculator/, and other niceties like Window minimize/maximize.

AutoHotKey is nice, did it for 10 years, but it fails whenever the target gets key input from low level (VMs, RDP, security boxes).  
KeyTweak works better, but it supports only basic key remapping, and requires a reboot for every change.

##Features

- optional basic key remapping   
    Y<>Z (for ze Germans)  
    WIN<>ALT on non-Apple keyboards (the Apple way is better)  
    \ and / to Shift (\ is Shift on ANSI (US) keyboards, / is its physical counterpart. Try it, it's comfortable).    
- CapsLock -> Cursor control layer  
    [I][J][K][L] = Cursor  
    [Z][U] = Home/End   
    [O][>] = PgUp/Down    
    [H]['] = Backspace/Delete  
    [N][M] = Ctrl+Left/Right  
    [=] = Insert  
    [P] '[' and ']' = Print/ScrLock/Pause  
    [ALT]+[Cursor] = 10 times  
- CapsLock -> Clipboard functions  
    [S][D][F] = Cut/Copy/Paste  
    [Backspace] = Undo  
    [Shift+Backspace] = Redo 
- Tapping CapsLock (Press+ Release) -> Special character layer  
    AOU = Ümläüte  
    SEDC = ß€°©  
- Special characters work more reliable with a corresponding AutoHotKey script active. (without AHK, ALT+NUMPAD codes are sent, not all apps support this)  
- 10 Clipboards (requires AHK script)   
    Caps+[0]..[9] copies to clipboard #n   
    Tapping Caps + [0]..[9] pastes from clipboard #n  
- Core commands, triggered with RControl + Caps + {key}  
    [ESC] Quit capsicain  
    [H] Help - list available commands  


NOTE: v1..12 was created in capsicain_interception repo. This was a non-VS project, now obsolete except for the history.

Why the name? This tool defines a lot of CapsLock Hotkeys. 'Capsaicin' is the chemical stuff that makes chilis hot, capsicain just has a better flow to it...
