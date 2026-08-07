# 00005 Interact workflow
   
Date: 2020-05-30   
Tags: actions, commands       
Maintainers: Igor Korsukov    

## Status: Accepted

## Context
  
The user clicks on the UI controls (e.g. toolbar buttons, menu items) and waits for the corresponding actions from the application. Moreover, the same action can be triggered in different ways, for example, by clicking on a toolbar button and by clicking in the menu or in any other way, including a programmatic call. Also, the same actions can be triggered by keyboard shortcuts. In this case, the application's response should be the same regardless of how the action was called.   
When performing actions, an app can change its state, which could be something as simple as "function enabled" or something more complex. This state may need to be reflected in different places of UI, for example, by showing a toolbar button as "enabled" or a menu item as "checked."   

## Decision

We will use the ~~action~~ command* system, the command dispatcher as an intermediary between the command trigger and the handler, and state change notifications.     
   
*We replaced actions to commands. See ADR 00101   
   
Interact workflow looks like this   
* Show: [UIControl] load -> [CommandsRegister]commandInfo 
* Action: [UIControl] onTriggered -> [ViewModel] dispatch(command) -> [CommandsRegister] -> [CommandsController]onRequest -> [Service]doSomeThing   
* State changed: 
1. [Service]somethingChanged -> [CommandsState]updateCommandsState -> [ViewModel]onNotify -> [UIControl]updateView 
2. [Service]somethingChanged -> [ViewModel]onNotify -> [UIControl]updateView  

### CommandsRegister   

This is a registry of all commands that provides information on commands - title, description, parameter scheme, icon.      
Each module registers its commands in a common register.   

### Trigger

Command triggers can be UI control (buttons, menus, etc.), keyboard shortcuts, MIDI remotes, AI integrations (MCP server), extensions, programs calls...    
The trigger of a command should not know who will execute it and should not know what will happen, should not rely on expectations.
    
### CommandsRegister

We can dispatch just command or command query - this is a command with parameters.   
The command execution is asynchronous, the dispatcher returns a Promise, we can subscribe and receive the command result.   

### CommandsController  

Each module has a command controller. Ideally, only this one should receive all commands that the module can process. Avoid registering with the dispatcher and processing commands elsewhere, especially in view models.     
Ideally, the command controller itself does not implement anything; it only calls methods of other services.   

### Service  

Services don't know anything about commands and don't accept them. Services have methods that the command controller can call (usually through an interface).    
When performing actions, services typically change state and must notify of this change. Various consumers can be subscribed to these notifications - viewmodels, other services. The state of the commands themselves (enabled, checked) is also updated. To update the command state in the UI, it's best to use the command state.      

### CommandsState

This is a service that provides state for each command - enabled, checked.    
Each module registers its implementation in it, which updates the state of commands when the state of some objects is changed.       

## Consequences

* We can send commands from a variety of places without worrying about how exactly they will be processed.   
* We don't need to access services that perform actions and call their methods in the required sequence.   
* We can be sure that actions will be performed in the same way, regardless of who triggered them (button, shortcut, MCP server...)
* We can respond to change regardless of who is the source of the change.  
* But we need to perform actions via command dispatch, not by calling service methods, and we still need to subscribe to notifications for updates, even if we are the only ones giving the commands.   

## Alternatives

### Action/UiAction   

Initially, there was the concept of Action/UiAction - it's essentially the same as commands, except commands can now return a result, have a parameter scheme, and have several technical implementation fixes. We could say that Commands are an evolution of Actions.    

### Direct method calls  

* Experience shows that we either get duplicate code or slightly different behavior when called from different places.   
* Action trigger places become complex because they must access many different services to call methods on them.   
* Domain (business) logic begins to leak into action call places (UI), because often, to perform an action, not one method is called, but several according to some logic.   
