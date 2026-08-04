# 00103 Adding automation support to the audio engine

Date: 2026-08-04  
Tags: audio, automation
Maintainers: Roman Pudashkin, Igor Korsukov

## Status: Accepted

## Context

We're developing an automation feature in the app. It's possible to automate changes to various parameters, including volume and pan.   

## Decision

We'll be passing a volume and pan map to the audio engine via RPC. The key will be the playback position.   
We'll add a new `AutomationControlNode` that will either apply values ​​from the value map if they're specified, or accept and apply simple volume parameters, like a regular `ControlNode`. We will replace the `ControlNode` in the audio graph with the `AutomationControlNode`.

## Consequences

* We can simply pass values ​​to automate volume and pan changes.
* Our audio graph will not change depending on whether automation is enabled or not.
* We will be able to use simple `ControlNode` in other cases where there is no automation.

