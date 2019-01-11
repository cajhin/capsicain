# capsicain

Keyboard configuration tool that re-maps keys and modifier-key-combos at a very low level.  
Uses the Interception driver https://github.com/oblitum/Interception to receive keys before almost everyone else.  

## Why?

I touch type. I code. I need a keyboard setup where I can keep my fingers on the home row most of the time.  

The position of Left/Right, Home/End, Insert/Delete and so on is bad. On laptops, it's awful.  
Also, I need to type Ümläuts quite often, which is a pain on US standard layout - the only sane layout for writing C-style code.  
My right pinky complains a lot, because C-style code requires a lot of special characters that are done with a stretched right pinky. Then I see the ALT keys, which are in a great position but rather useless.  

And while I'm at it, I like to have a ton of extra features.   

## Why I don't use tool X instead
**AutoHotKey** is nice, did it for 10 years, but it runs in userspace and fails whenever the target gets key input from low level (VMs, RDP fullscreen, security boxes, games). It is limited when comes to multi-modifier combos. I still use it as a deputy for capsicain, doing Windowsy userspace tasks.  

**Windows Keyboard Layout Creator** works more reliable, but it supports only very basic key remapping, and requires a reboot for every change.  

**Karabiner** is really good, but Mac-only (and oh so awful config syntax).  

**Tmk / Qmk with Hasu's Usb-to-Usb stick** is very cool, but it cannot do laptop keyboards.  

**Capsicain** does everything I want, the way I want it.  

## Features

- Like a hardware programmable keyboard, Capsicain can mess with key strokes before anyone else does.  

- Can remap EVERY key that is sent out by the keyboard.  

- Almost everything is configurable via config file.   
  Nine separate layers, switch with [ESC]+NumberKey.   
  Flexible with 'INCLUDE sub_section'.  

- Modifier remapping   
  F and J to Shift, CapsLock to MOD9, LCtrl to Return, Rotate Alt>Shift>Control>Alt.  
  If it sends a scancode, you can remap it.  
    
- Powerful modfier combos  
  Can do keycombos with all 15 modifiers with a single one-liner rule.  
  Combine Modifier-Down, Modifier-NOT-down and Modifier-tapped in one combo.  
  
- Simple, fast and pretty alpha key mapping, to define Workman, Colemak, Dvorak, or play with your own layout.  
  Changing a key position is one character in the .ini file, [ESC]+[R] to reload and you're live.  

- Unlimited sequences (key macros) with configurable delay between keys.  

- Fast. Low-fat C/C++ code. 1 exe 1 dll 1 ini. Never writes, only reads inside its folder.
    
### Features of the default config 
This is the config I use myself.  

- Hold CapsLock + right hand -> Cursor control layer. I LOVE this!!  
    I J K L = Cursor  
    Z U = Home/End   
    H = Backspace  
    etc.  

- Hold TAB + right hand -> NumPad layer  
    U I O = 7 8 9  etc.  
    
- ALT + letter keys-> all regular symbol characters.
  ALT + Q for '!' is an easier combo than Shift + 1, when you get used to it.  
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

### What it doesn't do (today)
- No modifier-only combos (Shift+Alt -> X). I don't like these, they cause accidents.
- No combos-to-modifier (Ctrl+X -> Alt). Useless?
- No dead keys / composing (Alt+U, then O -> Ö). Could be done but what for?
- ALT-Numpad combos for special characters ╠═ö€Σε═╣ don't work in Linux VMs.  
  If you need this, you have to create your own config for Linux special chars.  

## About Interception  
This is a signed driver ("keyboard driver upper filter"), another project on github. It must be installed for capsicain to work. It provides a DLL to interface with the driver.  
The filter driver does nothing (just forwards all key events from the keyboard driver to the next higher driver) unless a client wants to hook into the keyboard events.    
The DLL is free and open source, the driver is free but closed source (sources available for $1000. The guy wants to make some money from commercial projects - I hope he does because he did some really good work here).  

Musings: Capsicain is a normal userspace app, which means you can simply start and stop it anytime. It also means it cannot talk to the keyboard driver directly, so it needs the Interception driver. This is an unavoidable complication in Windows 10, but I actually see it is a good thing: because it is not that easy, no normal application or game will do this - and this means that Capsicain is always #1 in the keyboard processing chain.  

But but rootkit keylogger exposing all my sekrits? True that. I didn't see the source, I don't know the guy, but I sniffed around a bit and it all smells legit to me. Well, everytime you run any binary with admin privileges, it can do all this and more.   

## Notes
v1..12 was created in capsicain_interception repo. This was an experimental non-VS project, now obsolete, except for the history.

Why the name? Beer made me do it. I like chilis. This tool defines a lot of CapsLock Hot keys. 'Capsaicin' is the chemical stuff that makes chilis hot, Capsicain just has a better flow to it (and is a unique name)

# Installation
1. download the latest capsicain.zip here: https://github.com/cajhin/capsicain/releases 
2. extract to any folder you like. You can move it anytime; there are no registry entries for Capsicain.  
3. if it is not in the .zip file, get the Interception installer from here:  
    https://github.com/oblitum/Interception
4. open an Administrator dosbox (Start menu > type 'cmd' > right-click on Command prompt > select 'Run as administrator'  
5. navigate to the Interception installer folder  
6. run `install-interception /install`
7. reboot
8. start capsicain.exe

## Your first config
Out of the box, capsicain.ini is the complete config that I use myself.  
If you want to play around with something simpler, rename capsicain.example.ini to capsicain.ini.  
[ESC]+[R] to reload the config.  

REMEMBER:  
[ESC]+[X] Exits, always, in case your config makes the keyboard unusable. Capsicain doesn't have to be in the foreground to see ESC command combos.   
[ESC]+[0] is the softer method (alpha layer 0); it tells Capsicain to not alter any keys - except for [ESC] combos, so you can switch back to your layer 1 later.

Feel free to open an issue to ask questions.  
