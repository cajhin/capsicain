### February 2024: new release v97: new global to deactivate the standard Windows shortcut "LWIN tapped -> open Start menu"
### July 2023: v95 new option to include/exclude specific keyboards, see <a href="https://github.com/cajhin/capsicain/wiki/Keyword%3A-OPTION#option-includedeviceid-searchstring">IncludeDeviceId</a>
### December 2022: Windows 11 update 22H2 breaks *minimize to tray*, see Wiki for a <a href="https://github.com/cajhin/capsicain/wiki/X-doesn't-work#windows-11-version-22h2-cannot-minimize-to-tray">fix</a>

# capsicain

Keyboard configuration tool that re-maps keys and modifier-key-combos at a very low level.

Created for productivity and fast keyboard layout prototyping, but also works for gaming.  
(Might be useful to work around handicaps, but I have no experience. Open an issue if you need a special feature).

Uses the [Interception driver](https://github.com/oblitum/Interception) to receive keys before almost everyone else.  

## Quick links

- <a href="https://github.com/cajhin/capsicain/releases">Latest release</a>
- <a href="https://github.com/cajhin/capsicain/wiki/Installation">Install guide</a>
- <a href="https://github.com/cajhin/capsicain/wiki">Manual</a>
- Readme contents
  - [Why?](#why)
  - [Features](#features)
    - [Features of the default capsicain.ini](#features-of-the-default-capsicainini)
    - [Additional AutoHotKey Features](#additional-autohotkey-features)
  - [Core commands](#core-commands)
  - [Help!](#help-ive-broken-something-and-cant-use-my-keyboard)
  - [Frequently asked questions](#frequently-asked-questions)
  - [Getting involved](#getting-involved)

## Why?

I touch type. I code. I need a keyboard setup where I can keep my fingers on the home row most of the time.  
Also, my layout cured my RSI, and I'm a seriously fast coder/editor on any keyboard (and I never need to learn non-standard laptop keyboards).

Earlier versions were very focused on my own configuration. Latest versions are fully configurable.

## Features

- It just works. Everywhere.  
    - Like a hardware programmable keyboard, Capsicain can view and remap key strokes before anyone else does.

- Can remap every key that is sent out by the keyboard. (Excptions: 'media keys' that are not registered as keypresses, and the ESC key).

- Almost everything is configurable via config file.   
  - Nine separate configs, switch with `ESC` + `<1-9>`.   
  - Modular config with `INCLUDE moduleXY`.  

- Modifier remapping   
  - Examples: `CapsLock` to `MOD9`, `LCtrl` to `Return`, Rotate `Alt`>`Shift`>`Ctrl`>`Alt`.

  - If it sends a scancode, you can remap it.  
    
- Powerful modfier combos  
    - Can do keycombos with all 15 modifiers with a single one-liner rule.  
    - Combine Modifier-Down, Modifier-NOT-down, Modifier-A-OR-B-down, Modifier-tapped in one combo.  
  
- Simple, fast and pretty alpha key mapping, to define Workman, Colemak, Dvorak, or play with your own layout.  
  - Changing a key position is one character in the .ini file, [ESC]+[R] to reload and you're live.  

- Dead key system to define your own äççéñtèd characters.

- Sequences (key macros) with configurable delay between keys.  
  - On the fly recording and playback of macros.  
  - Secret macros that are only stored, obfuscated, in memory

- Fast. Low-fat C/C++ code. 1 exe 1 dll 1 ini. Never writes, only reads inside its folder.

  ### Features of the default capsicain.ini

  This is the config I use myself. I call it the King Configuration, because, like the King in Chess, fingers must never move more than one key from their base position into any direction to write and edit any text. (Exceptions: Escape, Enter, app-specific combos that I have not considered).
  <details>

    - Hold `CapsLock` + right hand keys -> Cursor control layer. I LOVE this!!  
        Key Pressed     | Result
        ----------------| ------
        `I` `J` `K` `L` | (Cursor) `↑` `←` `↓` `→`
        `Z` `U`         | `Home` `End`  
        `H`             | `Backspace`
        etc             | etc

    - Hold `CapsLock` + left hand keys -> Standard Ctrl-Combos
        Key Pressed         | Result
        --------------------| ------
        `A` `S` `D` `F` `G` | `Undo` `Cut` `Copy` `Paste` `Redo`  
        `Q` `W` `E` `R`     | `SelectAll` `GotoTop` `Find` `FindNext`
        `Z` `X` `C` `V`     | `NewFile` `NewTab` `Open` `Save` `CloseTab`

    - Hold `TAB` + right hand -> NumPad layer
        Key Pressed | Result
        ------------| ------
        `U` `I` `O` | `7` `8` `9`
        `J` `K` `L` | `4` `5` `6`
        etc         | etc

    - `ALT` + letter keys-> all regular symbol characters.  
        - `ALT` + `Q` for '!' is an easier combo than Shift + 1, when you get used to it.
        - Layout equivalent:
        ```
        ! @ # $ % ^ & ( ) ü ß
        - + * / = \ { } ö ä
        ` ~ | _ … < > [ ] ...
        ```

    - Tap `ALT`, `<key>` -> Special character layer  
        `€ © ° ¹²³ ...`

    - Tap `ALT`, `<deadkey>`, `<basekey>` -> Special deadkey sequences
        Keystrokes | Result
        -----------| ------
        `~`, `n`   | `ñ` 
        `~`, `a`   | `ã`
        `^`, `a`   | `â`

    - Tap `Caps`, Tap `ALT`, `Shift` + `<key>` -> Uppercase greek characters  
        - Σ   (just because I can)
        
    - `TAB` (NumPad) + `Ctrl` + `Number` -> "Table" characters  
      ```
      ┌────────────────────────┐  
      │ I like these things :) │  
      └────────────────────────┘  
      ```
    - TAB (NumPad) + Ctrl + Shift + Number -> "Fat Table" characters  
      ```
      ╔═════════════════════╦═══╦══╗  
      ║     MOAR TABELS!!   ╠═══╬══╣  
      ╚═════════════════════╩═══╩══╝  
      ```
  </details>

  ### Additional AutoHotKey features

  An AHK script must run that catches `F14` / `F15` key combos.

  - 10 Clipboards
      `Caps` + `<0-9>` copies to clipboard #`n`
      <br />
      Tap `Caps` , `<0-9>` pastes from clipboard #`n`
      
  - Start my apps with centrally configured hotkeys  
  
  - Windows control shortcuts: maximize, minimize, restore, close

## Core commands
 - These talk directly to Capsicain, trigger them with `ESC` + `<key>`

    Key Pressed| Result
    -----------| ------
    `X`        | eXit capsicain  
    `H`        | Help - list available commands
    `<1-9>`    | Switch configs
    `0`        | Switch to empty (disabled) config
 - There are various other options, like "flip Z/Y", "flip WIN/ALT on Apple keyboards", timing for macros, status, more.

## Help! I've broken something and can't use my keyboard! 
  - `ESC`+`X` Exits, always, in case your config makes the keyboard unusable. Capsicain doesn't have to be in the foreground to see `ESC` command combos.   
  - `ESC`+`0` Switch to Config 0 is the softer 'disable' method; it tells Capsicain to not do anything - except listen for `ESC` combos, so you can switch back to your Config 1 later.
  - The [wiki](https://github.com/cajhin/capsicain/wiki) has information on most edits you can do

## Frequently Asked Questions
  * ### Why I don't use tool X instead?
    * **AutoHotkey** is nice, did it for 10 years, but it runs in userspace and fails whenever the target gets key input from low level (VMs, RDP fullscreen, security boxes, games). It is limited when comes to multi-modifier combos. I still use it as a deputy for capsicain, doing Windowsy userspace tasks.  

    * **Windows Keyboard Layout Creator** works more reliable, but it supports only very basic key remapping, and requires a reboot for every change.  

    * **Karabiner** is really good, but Mac-only.

    * **Tmk / Qmk with Hasu's Usb-to-Usb stick** is very cool, but it cannot do laptop keyboards.  

    * **Capsicain** does everything I want, the way I want it.
    
  * ### What's with the weird name?
    * Beer made me do it. I like chilis. This tool defines a lot of CapsLock Hot keys. 'Capsaicin' is the chemical stuff that makes chilis hot, Capsicain just has a better flow to it (and is a unique name. Although that was a mistake - google search still says You Fool! You Spelled capsaicin wrong!)
  * ### What's the old_capsicain_interception repo?
    * v1..12 was created in the capsicain_interception repo. This was an experimental non-VS project, now obsolete, except for the history.
  * ### Why is my keyboard suddenly QWERTZ?!
    * This is the layout I use by default, you can either use `ESC` + `Z` to flip Y/Z, or edit the config to disable this switch  
      (in capsicain.ini section `[CONFIG_1]`, comment out `INCLUDE LAYOUT_QWERTZJ`)
  * ### What is Interception?
    * Interception is the driver that allows capsicain to intercept and modify keyboard events.
      <details>
        <summary>Long version</summary>
        This is a signed driver ("keyboard driver upper filter"), another project on github. It must be installed for capsicain to work. It provides a DLL to interface with the driver.  
  
        The filter driver does nothing (just forwards all key events from the keyboard driver to the next driver in the chain) unless a client wants to hook into the keyboard events.    
        
        The DLL is free and open source, the driver is free but closed source (sources available for $1000. The guy wants to make some money from commercial projects - I hope he does because he did some really good work here).  

        Musings: Capsicain is a normal userspace app, which means you can simply start and stop it anytime. It also means it cannot talk to the keyboard driver directly, so it needs the Interception driver. This is an unavoidable complication in Windows 10, but I actually see it is a good thing: because it is not that easy, no normal application or game will do this - and this means that Capsicain is always #1 in the keyboard processing chain.  

      </details>
  * ### How do I know this isn't a keylogger?
    1. capsicain is completely open source. It does not write to disk, it does not do any networking.
    1. The Interception driver is closed (pay for) source, but it is an established project, and after looking into it, it all seems legit to me.
    1. Reading keyboard input is possible with any binary you run with admin privileges, this caution should be applied universally.
  * ### What are the current limitations?
    - Double-taps are not configurable
    - No combos-to-modifier (Ctrl+X -> Alt). Useless?
    - Windows ALT-Numpad combos for special characters ╠═ö€Σε═╣ don't work in Linux VMs.  
      - If you need this, you have to create your own config for Linux special chars.
    - Cannot be triggered by other software. Capsicain listens to the keyboard hardware and nothing else.

# Getting Involved

Feel free to open an [issue](https://github.com/cajhin/capsicain/issues) to ask questions.  
I'm willing to help, and interested in ideas.  
You're also welcome to start a [discussion](https://github.com/cajhin/capsicain/discussions)

