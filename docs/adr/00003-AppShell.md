# 00003 AppShell

Date: 2020-05-30   
Tags: appshell, modularity    
Maintainers: Igor Korsukov    

## Status: Accepted

## Context

The application consists of modules. Some modules provide certain views. These views can be shown to the user in different ways: in a sidebar, in a bottom bar, there can be several views on one page, or each view can be in its own tab. The views themselves don't know (and shouldn't know) how they will be shown. This should be determined by the overall application structure, which may vary for different cases.   
   
## Decision

We will use the AppShell pattern.    
AppShell - this is an architectural pattern that separates the UI structure from the content. In order to be able to change the structure of the application and the components separately, independently of each other. It also controls the loading of components.    
AppShell is just a container that defines the UI structure of the application and connects the application components to each other.    
AppShell defines such things as toolbars, panels, central content, status bars, component life cycle, navigation, etc.     
Examples of using AppShell in other technologies:    
* [AppShell for Progressive Web App](https://developers.google.com/web/fundamentals/architecture/app-shell)
* [Xamarin.Forms Shell](https://docs.microsoft.com/en-gb/xamarin/xamarin-forms/app-fundamentals/shell/)
   
## Consequences

* We can develop the view of some components and the structure of the application separately.    
* We can change the application structure for different cases (expert/lite, viewer/editor, phone/tab, etc.)   
* We can avoid mixing the logic of interaction with components and components among themselves with the logic of the components themselves.  
* We can use the same view components in different scenarios within this application.  
* We can optimize the loading and display of components, showing some earlier, some later... 
* But, when developing components, we should try not to assume exactly how they will be shown, but to anticipate different variations of their display.

## Alternatives

The alternative is to not develop view components, but to develop a rigid structure—a left panel, a bottom panel, a central area of ​​the application, etc. But we won't be able to show what was done in any other way, even in the application itself when developing new functionality.   
