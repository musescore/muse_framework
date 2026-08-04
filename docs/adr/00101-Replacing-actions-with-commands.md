# 00101 Replacing actions with commands

Date: 2026-06-12  
Tags: actions, commands, mcp   
Maintainers: Igor Korsukov

## Status: Accepted

## Context

It all started with the idea of ​​creating an MCP server (an integration protocol for AI models).  
Our actions are suitable for implementing an MCP server, but they have several drawbacks:  
* Some actions need to return data (for example, the number of instruments) - our actions don't.   
* We need an input schema, what parameters there are, their name, type, min, and max - our actions don't explicitly provide this.   
* We need an action to return a success result (i.e., the protocol requires responses to requests) - our actions don't return a result (and this was intentional).  
   
There is also internal debt and internal problems with actions:
* We currently have two implementations: the old one with code and action data, and the new actionquery.
* It's very difficult for the team to understand what action context and shortcut context are, and how action availability is calculated (the calculation of action availability is really blurred across different services; we wanted to make it simpler, but it turned out to be complicated). 
* There are also other issues with both actions and shortcuts related to global actions (copy, delete, etc.).

## Decision

Based on these two inputs, I started creating a new command system.   
They're suitable for MCPs (for example, returning data), and their implementation is an opportunity to fix the issues with actions.   
So, MCPs were the pretext, but in reality, I'm now fixing internal issues with actions.   
A well-developed and high-quality command system can also be the basis for any integration with external systems.   

## Consequences

Key differences:
* They return a result and can return data, meaning they can be not only actions but also queries.
* Context have been removed from the commands themselves; their availability is now defined in a single corresponding class, and the shortcut context will be defined in the shortcut configuration file.
* They have an input schema—a description of the available parameters and their types.
* The command register and their state are now two different services (previously, everything was in one).
* Commands are asynchronous.
* A number of other technical and semantic improvements.
* There will be one implementation, one approach, not two.

The new command system will completely replace actions.

## Implementation

* Added the `rcommand` module - it contains the command data type, commands register, commands state and command dispatcher 
* Added the `rcontrol` module - this is integration with external systems, in particular the MCP server

Each module must implement:
* List of command constants (/modulecommands.h) - for convenience   
* Module's commands register (/internal/modulecommandsregister.h/cpp) - is a static list of commands and command information. And register it. 
* Module's commands state (/internal/modulecommandsstate.h/cpp) - is the command state controller (enabled, checked). And register it.
* See examples in the code 
  