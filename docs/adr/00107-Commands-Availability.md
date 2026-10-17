# 00107 Commands: Availability

Date: 2026-10-05   
Tags: commands, dispatch, shortcuts, api, mcp   
Maintainers: Igor Korsukov, Dmitry Makarenko    

## Status: Accepted

## Context

We have commands, and we have big plans for them - shortcuts, access via the extensions API, the MCP server, the command palette...   
Commands are becoming an API. Any API must be explicitly specified as public; in other words, we shouldn't simply expose everything—instead, we must take full responsibility when explicitly making something public.   

## Decision

We are adding an availability flag to the command info.   
Even though most commands are available for all, the command is disabled by default, even for dispatch - to ensure that the availability flag is specified explicitly and responsibly.   

## Consequences

* We clearly and responsibly specify the availability of the command; this can also be discussed during the review.   
* We can control command availability at a granular level.  
* But we will clearly need to specify an availability flag for all existing and future commands.   

## Alternatives

We can control command availability at the level of specific functionality (for example, the keyboard shortcut configuration file). However, this results in information being scattered across different places, making it difficult to form a complete picture, and leading to either duplication or inconsistent implementations in each place.

## Implementation 

```
enum class Availability {
    Disabled = 0,
    Dispatch = 1 << 0,  // If only dispatch is available, then the command is essentially internal.
    Shortcut = 1 << 1,
    API = 1 << 2,
    MCP = 1 << 3,

    All = Dispatch | Shortcut | API | MCP
};
DECLARE_FLAGS(Availabilities, Availability)
DECLARE_OPERATORS_FOR_FLAGS(Availabilities)

struct CommandInfo
{
    Command command;
    MnemonicString title;
    TranslatableString description;
    InputSchema inputSchema;
    Decoration decoration;
    Availabilities availabilities = Availability::Disabled;

    bool isValid() const { return command.isValid(); }
};
```

```
    CommandInfo{
        DIAGNOSTICS_SAVE_FILES_COMMAND,
        TranslatableString("diagnostics", "Save diagnostic files"),
        TranslatableString("diagnostics", "Save diagnostic files"),
        InputSchema(),
        Decoration(),
        Availability::All
    },
    CommandInfo{
        DIAGNOSTICS_SHOW_PATHS_COMMAND,
        TranslatableString("diagnostics", "Show paths…"),
        TranslatableString("diagnostics", "Show paths"),
        InputSchema(),
        Decoration(),
        Availability::Dispatch
    },
```