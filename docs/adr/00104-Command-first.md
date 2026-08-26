# ADR-00104: Command-First Architecture

Date: 2026-08-21
Tags: command, query, menu, toolbar, mcp
Maintainers: Igor Korsukov, Dmitry Makarenko

## Status: Accepted

## Context
The current architecture has a command infrastructure consisting of:
* Command registry (command information: name, description, icon)
* Command state (enabled, checked)
* Command dispatcher (register handler, sending commands and queries)
* Shortcuts (assigning hotkeys)
* Menu/Toolbar (displaying commands with their state)
   
In the current implementation of commands infrastructure, query (uri + params) is not a standalone entity.   
Query is treated as a command with parameters and does not have its own info, state, or shortcut.     
This can be a problem for dynamic commands such as calling extensions, workspaces, effects.    
Before the commands, when we had actions, we solved the problem with dynamic actions through the formation of a query, for example:   
* action://some/extensions?action=code
* action://select-workspace?name=code 
* action://effect/open?effectId=code

But for each action in extensions, for each workspace, for each effect, we need separate info (title), state, shortcut... Therefore, in some places, ActionQuery became the key (for example, we can get info for a Query), and in other places we did not have support (for example, we cannot assign a shortcut to a Query).    
   
When switching to commands, we faced the question of whether to make it so that only commands were the key (info, state, shortcuts), and dynamic calls were exactly commands, or to make it so that command query could be the key (info, state, shortcuts) and dynamic calls were like queries with a dynamic parameter.   

## Decision
    
We make it so that only commands can be the key to getting info, state, shortcuts, etc.
Instead of a query with a dynamic parameter, we should form dynamic commands, for example:   
* action://some/extensions?action=code   ->    command://some/extensions/action 
* action://select-workspace?name=code    ->    command://workspace/select/code 
* action://effect/open?effectId=code     ->    command://effect/code 

Query (parameters) are used ONLY to pass data, NOT to identify:
* command://move?x=100&y=200
* command://search?text=hello
* command://effect/reverb?room_size=0.5
...
    
This approach also aligns well with the MCP, where each command is a tool that may or may not have parameters.
With the query approach (for example, effect/open?), it's difficult to describe all possible effects, especially if they also have parameters.   

## Consequences

* We get a simpler and more unambiguous architecture.   
* We get the same infrastructure and approaches for static commands (mostly built-in) and dynamic ones (extensions, workspaces, effects).  
* We have no problem defining what is an identifier and what is just a parameter (for example effect/open?effectId=reverb&room_size=0.5).  
* It is much easier for us to create an MCP so that the AI ​​understands what it can do.
* It will be easier for us to make Command Palette (like in VS Code) (future). 
* But we need to rework the mental picture and the old code.  

##  Alternatives

Make it possible for the command query to be a key - it would be possible to get info, state, and assign a shortcut.

Why rejected: 
* There is ambiguity: somewhere the command is the key (mostly), somewhere the query  
* It is not clear how to determine which part of the query is the key (effect/open?effectId=reverb) and which is just a parameter (room_size=0.5). We would have to complicate the infrastructure, add new entities, for example CommandPreset or CommandKey, and explicitly indicate in the specification which parameters are part of the key - this looks like overengineering.  
* It's difficult to implement an MCP. Instead of a list of commands (tools), there will be one tool with a large, arbitrary description of what parameters it can accept. Everything becomes significantly more complicated if, for example, each effect has its own additional parameters.

## Implementation  

Using extensions as an example

### Make a command 

Instead of forming a query, we should form a command  
```
inline rcommand::Command makeCommand(const ExtensionUri& extensionUri, const ExtensionActionCode& action)
{
    // extension://some/uri + action -> command://extension/some/uri/action
    rcommand::Command c;
    c.setScheme(std::string(rcommand::COMMAND_SCHEME));
    c.addPath(std::string(EXTENSION_SCHEME)); // scheme as first segment of path
    c.addPath(extensionUri.path());
    c.addPath(action);
    return c;
}
```

Adding `extension` as the first segment in the path only solves the problem of not conflicting with the built-in command, as the extension URI may match the built-in command's URI. If there is no such problem in your area, then you don’t need to solve it.   

### Register 

For each extension and action, instead of a query like: 
* `command://extension/perform?uri=extension://some/uri&action=code`
* `command://extension/some/uri?action=code` 

We make commands like: `command://extension/some/uri/action`   
And for each command we create a Info (title, description, icon)

```
void ExtensionsCommandsRegister::reload()
{
    m_commandInfos.clear();
    m_commands.clear();

    const auto manifests = extensionsRegister()->manifestList();

    m_commandInfos.reserve(manifests.size() + s_commandInfos.size());
    m_commandInfos.insert(m_commandInfos.end(), s_commandInfos.begin(), s_commandInfos.end());

    for (const auto& manifest : manifests) {
        for (const auto& action : manifest.actions) {
            CommandInfo info = {
                makeCommand(manifest.uri, action.code),
                TranslatableString::untranslatable(action.title.empty() ? manifest.title : action.title),
                TranslatableString::untranslatable(manifest.description),
                InputSchema(),
                Decoration(action.icon)
            };
            m_commandInfos.push_back(std::move(info));
        }
    }

    m_commandListChanged.notify();
}
```

### State 

Extensions have a common state for the entire extension (all actions), so we only receive the extension's identifier component (URI) from the command and use it to determine the state. More often, the state will be different for each command (for example, for each effect). 

```
CommandState ExtensionsCommandsState::commandState(const Command& command) const
{
    ExtensionUri extensionUri = extensionUriByCommand(command);
    const Manifest& manifest = extensionsRegister()->manifest(extensionUri);
    IF_ASSERT_FAILED(manifest.isValid()) {
        return CommandState(false, false);
    }

    if (!contextResolver()->isContextAllowed(manifest.context)) {
        return CommandState(false, false);
    }

    return CommandState(true, false);
}

```

### Subscribe 

We subscribe to commands and call the handler.
(The command forming is duplicated in several places; in other cases, it might be better to create a service that returns a list of commands.)

```
for (const Manifest& m : provider()->manifestList()) {
    for (const Action& a : m.actions) {
        ExtensionUri uri = m.uri;
        ExtensionActionCode actionCode = a.code;
        commandDispatcher()->onRequest(this, makeCommand(uri, actionCode), [this, uri, actionCode]() {
            return onExtensionTriggered(uri, actionCode);
        });
    }
}
```    

### Make a menu 

Creating a menu for extensions is a bit complicated because there are categories, and actions need to be implemented as submenus. But the gist is that we create items as usual, specify the command, and inside it, we'll do the following:

```
MenuItem* item = makeMenuItem(makeCommand(m.uri, a.code));

MenuItem makeMenuItem(const Command& command)
{
    MenuItem* item = new MenuItem();

    CommandInfo info = commandRegister()->info(command); // <--- info by command
    item->setTitle(info.title);
    ...
  
    Shortcut sc = shortcutRegister()->shortcut(command); // <--- shortcut by command
    item->setShortcut(sc);

    CommandState state = commandState()->state(command); // <--- state by command
    item->setState(state);
}

```