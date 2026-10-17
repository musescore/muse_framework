# 00108 Shortcuts: Replacing the context with scope

Date: 2026-09-01   
Tags: shortcuts, context, scope   
Maintainers: Igor Korsukov   

## Status: Accepted

## Context

The application supports a large number of actions, yet the available key combinations are limited; consequently, situations arise where the same combination is assigned to multiple actions, necessitating a determination of which specific action should be executed at any given moment.

When actions were implemented, there was a concept of "context" there. Context refers to the application's current state—specifically, the user's location within the app. There was the context of the action itself (defining when it is available; for instance, if an action operates on a project, that project must be open) and the context of the shortcut (determining which action to dispatch at a given moment based on the user's exact location). At the same time, the current context was just one variable. However, experience showed us that nesting and combinatorics were required.   
Overall, things were fine for most actions. However, certain actions - such as copy - required a more detailed definition of exactly what was happening at that moment. That was when the idea of ​​global actions emerged. The concept was to avoid determining the specific action to send at the shortcut level; instead, a "global" action would always be sent, and a dedicated controller would then determine the current state and the specific action required.    
   
The idea was simple: we have a single variable - the context - that stores the app's current state (e.g., project open, project in focus, another panel in focus), and each shortcut has a context property specifying when its action should be triggered. If the current context matches the shortcut's context, the action can be dispatched. we just need to compare two values.    
But the resulting system turned out to be complex and convoluted:   
* Comparing the current context with the shortcut context  
* Determining the context priority to obtain one of several possible options  
* Contextual combinatorics (this one or that one)  
* "Global" abstract shortcuts and determining the appropriate action in a separate controller.  

And all of this was running simultaneously, scattered across various places, making it difficult to understand how everything worked and what needed to be done to achieve the desired behavior.   

## Decision

When transitioning from actions to commands, we are also changing the way shortcuts are structured.
   
Shortcuts now feature the concept of "scope" - a virtual context that can be anything we like, with any level of detail. For example, PLAYBACK, NAVIGATION, PROJECT... Scopes are no longer tied to any specific application state; they are simply distinct areas for shortcuts - and that’s it.   
The logic for resolving shortcut conflicts has now changed. Previously, the approach was to determine the user's exact location and dispatch the corresponding action. Now, the process involves determining which area has higher priority, provided it is currently active.    
        
The scope itself is now stored in the shortcut configuration file or calculated dynamically (previously, the context was stored within the action/command).         
This solution is intended to replace all previous methods for determining which command to dispatch in the event of a shortcut conflict.  
    
Due to significant changes in the shortcut structure, we are adding a new "shortcut v2" module. We are also changing the format and structure of the shortcut configuration file (switching from xml with flat list to json with nested structure).   
  
The process of resolving the required command upon a key combination trigger now looks like this:  
* We get a list of all shortcuts for the given key combination.  
* We are filtering out unavailable (not enabled) commands; command availability is now determined more rigorously - this is the first level of conflict resolution.  
* If more than one shortcut remains in the list, we determine which scope has higher priority.  

Using the MSS as an example:
We have three areas, three commands, and the same shortcut.

| Scope                 | Command           | Key    |
| --------------------- | ----------------- | -------|
| NAVIGATION            | control-trigger   | Space  |
| PLAYBACK              | toggle-playback   | Space  |
| NOTATION_TEXT_EDIT    | next-word         | Space  |

We are building a priority map:
```
    {
        { NAVIGATION_SCOPE, []() { return 1; } },
        { PLAYBACK_SCOPE, []() { return isPlaybackAllowed() ? 2 : -1; } },
        { NOTATION_SCOPE, [this]() { return isNotationFocused() ? 3 : -1; } },
        { NOTATION_NOTE_INPUT_SCOPE, [this]() { return isNotationFocusedAndNoteInputMode() ? 4 : -1; } },
        { NOTATION_TEXT_EDITING_SCOPE, [this]() { return isNotationFocusedAndTextEditing() ? 5 : -1; } },
    };
```        

Ultimately, we arrive at the following logic:
* if now `isNotationFocusedAndTextEditing` send `next-word`
* if now `isPlaybackAllowed` send `toggle-playback`
* else send `control-trigger`

## Consequences

* We are simplifying shortcut management by replacing several different ways of resolving conflicts with a single one.   
* However, we need to create a new module rather than modify the current one, and break compatibility with the old shortcut configuration file (including the user's). 

## Implementation 

The process of resolving the required command upon a key combination trigger 
```
void ShortcutsController::activate(const std::string& sequence)
{
    ShortcutList allowedShortcuts;
    const ShortcutList& commandShortcuts = commandShortcutsRegister()->shortcutsForSequence(sequence);
    for (const Shortcut& sc : commandShortcuts) {
        if (commandsState()->commandState(sc.command).enabled) {
            allowedShortcuts.push_back(sc);
        }
    }

    Shortcut selectedShortcut;
    if (allowedShortcuts.size() == 1) {
        selectedShortcut = allowedShortcuts.front();
    } else if (allowedShortcuts.size() > 1) {
        if (shortcutsResolver()) {
            selectedShortcut = shortcutsResolver()->selectOne(allowedShortcuts);
        } else {
            selectedShortcut = allowedShortcuts.front();
        }
    }

    if (selectedShortcut.isValid()) {
        commandDispatcher()->dispatch(selectedShortcut.command);
    }
}
```
   
Resolver interface: since scopes can vary and depend on the application, it needs to be implemented at the application level rather than the framework level.     
```
class IShortcutsResolver : MODULE_CONTEXT_INTERFACE
{
    INTERFACE_ID(IShortcutsResolver)
public:
    virtual ~IShortcutsResolver() = default;

    virtual Shortcut selectOne(const ShortcutList& list) const = 0;

    virtual TranslatableString scopeTitle(const std::string& scopeCode) const = 0;
};
```

We construct a priority map based on the current state and determine which command has the highest priority at the moment.
```
int ShortcutResolver::scopePriority(const std::string& scope) const
{
    if (m_scopePriorityMap.empty()) {
        m_scopePriorityMap = {
            { NAVIGATION_SCOPE, []() { return 1; } },
            { PLAYBACK_SCOPE, []() { return isPlaybackAllowed() ? 2 : -1; } },
            { NOTATION_SCOPE, [this]() { return isNotationFocused() ? 3 : -1; } },
            { NOTATION_NOTE_INPUT_SCOPE, [this]() { return isNotationFocusedAndNoteInputMode() ? 4 : -1; } },
            { NOTATION_TEXT_EDITING_SCOPE, [this]() { return isNotationFocusedAndTextEditing() ? 5 : -1; } },
        };
    }

    auto priority = muse::value(m_scopePriorityMap, scope, nullptr);
    if (priority) {
        return priority();
    }

    return 0;
}

Shortcut ShortcutResolver::selectOne(const ShortcutList& list) const
{
    IF_ASSERT_FAILED(list.size() > 0) {
        return Shortcut();
    }

    ShortcutList sorted = list;
    std::sort(sorted.begin(), sorted.end(), [this](const Shortcut& a, const Shortcut& b) {
        return scopePriority(a.scope) > scopePriority(b.scope);
    });

    return sorted.front();
}
```