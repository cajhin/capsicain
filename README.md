# capsicain

C++ tool that uses the Interception driver https://github.com/oblitum/Interception to monitor and modify keypresses, at a very low level.

AutoHotKey is nice but fails whenever the target gets key input from low level (VMs, RDP, security boxes).
KeyTweak works better, but supports only basic key remapping, and requires a reboot for every change.

Features:

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
    [P][[][]] = Print/ScrLock/Pause  
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

Why the name? This tool defines a lot of CapsLock Hotkeys. 'Capsicain' is the chemical stuff that makes chilis hot, so that was my first association...
