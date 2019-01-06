# capsicain

C++ tool that uses the Interception driver https://github.com/oblitum/Interception to monitor and modify keypresses, at a very low level.

## Why?

I touch type. I code. I need a keyboard setup where I can keep my fingers on the home row all the time.  
The position of Left/Right, Home/End, Insert/Delete and so on is bad. On laptops, it's awful.  
Also, I need to type Ümläuts quite often, which is a pain on US standard layout - the only sane layout for writing C-style code.  
While I'm at it, I like to have a ton of extra features.

**AutoHotKey** is nice, did it for 10 years, but it runs in userspace and fails whenever the target gets key input from low level (VMs, RDP fullscreen, security boxes, games). It is limited when comes to multi-modifier combos. I still use it as a deputy for capsicain, doing Windowsy userspace tasks.  

**Windows Keyboard Layout Creator** works more reliable, but it supports only very basic key remapping, and requires a reboot for every change.  

**Karabiner** is really good, somewhat limited modifier handling, but Mac-only.  

**TMK with Hasu's Usb-to-Usb stick** is very cool, but it cannot do laptop keyboards.  

**Capsicain** does everything I want. Reliably, everywhere.  

## Features

- Can remap EVERY key that is sent out by the keyboard.  

- Almost everything is configurable via .ini file   

- Modifier remapping   
  CapsLock to MOD9, LeftShift to Backspace, Escape to RightWin, anything goes.  
    
- Powerful modfier combos  
  Can do keycombos with all 15 modifiers with a single one-liner rule.  
  Combine Modifier-Down, Modifier-NOT-down and Modifier-tapped in one combo.  
  
- Simple, fast and pretty alpha key mapping, to define Workman, Colemak, Dvorak, or play with your own layout. Changing a key position is one character in the .ini file, [ESC]+[R] to reload and you're live.  

- Up to 9 layouts, switch with [ESC]+NumberKey  

- Fast. Low-fat C/C++ code.
    
### Features of the default config 
- Hold CapsLock + right hand -> Cursor control layer. I LOVE this!!  
    I J K L = Cursor  
    Z U = Home/End   
    H = Backspace  
    etc.  

- Hold TAB + right hand -> NumPad layer  
    U I O = 7 8 9  etc.  
    
- ALT + letter keys-> all regular symbol characters, easier to reach  
  ! @ # $ % ^ & ( ) ü ß - + * / = \ { } ö ä ` ~ | _ … < > [ ] ...  
  
- Tap ALT -> Special character layer  
    e © ° ¹²³ ...  
    
- Tap ALT, Tap Shift -> More special characters

- TAB (NumPad) + Ctrl -> "Table characters"  
```
    ┌────────────────────────┐  
    │ I like these things :) │  
    └────────────────────────┘  
```
- TAB (NumPad) + Ctrl + Shift -> "Fat Table characters"  
```
    ╔═════════════════════╦═══╦══╗  
    ║ NEEDZ MOAR TABELS!! ╠═══╬══╣  
    ╚═════════════════════╩═══╩══╝  
```
    
### Additional AutoHotKey features
An AHK script must run that catches F14 / F15 key combos.

- 10 Clipboards   
    Caps + [0]..[9] copies to clipboard #n   
    Tap Caps , [0]..[9] pastes from clipboard #n  
    
 - Start many apps with centrally configured hotkeys  
 
 - Windows control shortcuts (maximize minimize close)
  
### Core commands
These talk directly to Capsicain, trigger them with [ESC] + {key}  
    [X] eXit capsicain  
    [H] Help - list available commands  
There are various config options, like "flip Z/Y", "flip WIN/ALT on Apple keyboards", timing for macros, status, more.

## About Interception  
This is a signed low-level driver ("keyboard driver upper filter"), another project on github. It must be installed for capsicain to work. It also provides a DLL to interface with the driver (you could access it directly but then maybe every bug kills the keyboard driver stack).  
The driver does nothing (just forwards all key events from the keyboard driver to the next higher driver) unless a client wants to hook into all keyboard events.    
The DLL is free and open source, the driver is free but closed source, sources available for $1000 (the guy wants to make some money from commercial projects).  

Note: Capsicain is a normal userspace app, which means you can simply start and stop it anytime. It also means it cannot talk to the keyboard driver directly, so it needs the Interception driver. This is an unavoidable complication, but I actually see it is a good thing: because it is not that easy, no normal application or game will do this - and this means that Capsicain is alway #1 in the keyboard processing chain.  

But but rootkit keylogger exposing all my sekrits? True that. I don't know the source, I don't know the guy, but I sniffed around a bit and decided to trust him. Well, everytime you run a binary with admin privileges, it can do all this and more.  

## Notes
v1..12 was created in capsicain_interception repo. This was an experimental non-VS project, now obsolete, except for the history.

Why the name? Beer made me do it. This tool defines a lot of CapsLock Hotkeys. 'Capsaicin' is the chemical stuff that makes chilis hot, Capsicain just has a better flow to it (and is a unique name)
