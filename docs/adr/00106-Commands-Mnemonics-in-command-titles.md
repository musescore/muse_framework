# 00106 Commands: Mnemonics in command titles

Date: 2026-09-29   
Tags: command, menu, mnemonic   
Maintainers: Igor Korsukov, Elnur Ismailzada   

## Status: Accepted

## Context

We have commands, and each command has information containing the title, description, and so on. We typically display the command title from the info in the locations where the command is triggered—primarily in the application menu and context menus. Menu item labels use mnemonics `&` for hotkeys and the `…` marker to indicate that an action involving a transition, such as opening a new page or dialog will follow.
We need to decide where to specify `&` and `…`. Strictly speaking, these symbols are not part of the command title; they apply only to a specific menu and may differ or be absent elsewhere.   

## Decision

We decided to add the `&` and `…` directly to the command title itself. We realize this isn't strictly part of the command title, but it is a sensible choice. Since we have only one menu and even if there were several, the mnemonics would likely overlap. This approach primarily allows us to avoid title duplication (i.e., having the command title be identical to the menu label containing the mnemonic).   
Where mnemonics are not required, we can obtain a version of the title without the mnemonic by calling a method on the class (`MnemonicString`) used to store the title.   

Typical places and usage of the command title:   
* App Menu - with `&` and `…`  
* Context Menu - with `&` and `…`  
* Toolbars - without `&` and `…`  (icons are usually used, with some exceptions)
* Shortcuts - without `&` and with `…` 

## Consequences

* We eliminate duplicate of command title if they match the menu item labels (which applies to the majority of cases).  
* We can easily obtain the title version without the mnemonic where necessary.   
* We can use the title without the marker `…` where necessary.
* We will have a single set of mnemonic assignments for the entire application (primarily for the menus); we don't have any other places for them at the moment anyway. 
* We are slightly blurring the lines of responsibility in the interest of common sense.  

## Alternatives

We could have avoided including mnemonics in the command titles itself and added them only where necessary when building the menu. However, that would mean duplicating all the titles and translations of them (a couple of hundred of them). That wasn't practical, so we decided against it.   
   
## Implementation 

```
    CommandInfo{
        PROJECT_NEW_COMMAND,
        TranslatableString("project", "&New…"),
        TranslatableString("project", "Create a new project"),
        InputSchema(),
        Decoration(IconCode::Code::NEW_FILE)
    },

    CommandInfo{
        PROJECT_SAVE_COMMAND,
        TranslatableString("project", "&Save"),
        TranslatableString("project", "Save the project"),
        InputSchema(),
        Decoration(IconCode::Code::SAVE)
    },
```    
   
