# capsicain

C++ tool that uses the Interception driver https://github.com/oblitum/Interception to monitor and modify keypresses, at a very low level.

## Why?

I touch type. I code. US standard layout is the only sane layout for writing C-style code.  
I need a keyboard setup where I can keep my fingers on the home row all the time.  
The position of Left/Right, Home/End, Insert and so on is bad.  
Also, I need to type Ümläuts quite often, which is a pain on US keyboards.  
While I'm at it, I like to have standard shortcuts on all my machines to start browser/Notepad++/Calculator/, and other niceties like Window minimize/maximize.

**AutoHotKey** is nice, did it for 10 years, but it runs in userspace and fails whenever the target gets key input from low level (VMs, RDP fullscreen, security boxes, games). I still use it as a deputy for capsicain, doing Windowsy userspace tasks.  

**KeyTweak** works better, but it supports only basic key remapping, and requires a reboot for every change.  

**Capsicain** does everything I want.  

## Features

- optional basic key remapping   
    Y<>Z (for ze Germans)  
    WIN<>ALT on non-Apple keyboards (the Apple way is better)  
    \ and / to Shift ('\' on ISO (Euro) keyboards is left Shift on ANSI (US) boards. / is its physical counterpart, it should have been right shift all along. Try it, it's comfortable).    
    
- Hold CapsLock + right hand -> Cursor control layer  
    [I][J][K][L] = Cursor  
    [Z][U] = Home/End   
    [O][>] = PgUp/Down    
    [H]['] = Backspace/Delete  
    [N][M] = Ctrl+Left/Right  
    [=] = Insert  
    [P] '[' and ']' = Print/ScrLock/Pause  
    [ALT]+[Cursor] = 10 times  
    
- Hold CapsLock + left hand -> Clipboard functions  
    [A][S][D][F] = Undo/Cut/Copy/Paste (I never learned to hit Ctrl-Z/X/C/V reliably)  
    [Backspace] = Undo  
    [Shift+Backspace] = Redo 
    
- Tap CapsLock (Press+Release) -> Special character layer  
    AOU = Ümläüte  
    SEDC = ß€°©  
    
### Additional AutoHotKey features
An AHK script must run that catches F14 / F15 key combos.

- Special characters work more reliable with a corresponding AutoHotKey script active. (without AHK, ALT+NUMPAD codes are sent, not all apps/machines support this properly)  

- 10 Clipboards   
    Caps + [0]..[9] copies to clipboard #n   
    Tap Caps , [0]..[9] pastes from clipboard #n  
  
### Core commands
These talk directly to Capsicain, trigger them with RControl + Caps + {key}  
    [ESC] Quit capsicain  
    [H] Help - list available commands  
There are various config options, like "flip Z/Y", "flip WIN/ALT", timing for macros, status, more.

## About Interception  
This is a signed low-level driver ("keyboard driver upper filter"), another project on github. It must be installed for capsicain to work. It also provides a DLL to interface with the driver (you could access it directly but then maybe every bug kills the keyboard driver stack).  
The driver does nothing (just forwards all key events from the keyboard driver to the next higher driver) unless a client wants to hook into all keyboard events.    
The DLL is free and open source, the driver is free but closed source, sources available for $1000 (the guy wants to make some money from commercial projects).  

But but rootkit keylogger exposing all my sekrits? True that. I don't know the source, I don't know the guy, but I sniffed around a bit and decided to trust him. Well, everytime you run a binary with admin priviledges, it can do all this and more.  

## Notes
v1..12 was created in capsicain_interception repo. This was a non-VS project, now obsolete, except for the history.

Why the name? Beer made me do it. This tool defines a lot of CapsLock Hotkeys. 'Capsaicin' is the chemical stuff that makes chilis hot, Capsicain just has a better flow to it...
