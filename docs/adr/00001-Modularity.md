# 00001 Modularity

Date: 2020-05-30   
Tags: modularity    
Maintainers: Igor Korsukov      

## Status: Accepted

## Context

### Software’s Primary Technical Imperative: Managing Complexity
> Managing complexity is the most important technical topic in software development. In my view, it’s so important that Software’s Primary Technical Imperative has to be managing complexity.
> Dijkstra pointed out that no one’s skull is really big enough to contain a modern computer program (Dijkstra 1972), which means that we as software developers shouldn’t try to cram whole programs into our skulls at once; we should try to organize our programs in such a way that we can safely focus on one part of it at a time. The goal is to minimize the amount of a program you have to think about at any one time. You might think of this as mental juggling—the more mental balls the program requires you to keep in the air at once, the more likely you’ll drop one of the balls, leading to a design or coding error.
> At the software-architecture level, the complexity of a problem is reduced by dividing the system into subsystems. Humans have an easier time comprehending several simple pieces of information than one complicated piece. The goal of all software-design techniques is to break a complicated problem into simple pieces. The more independent the subsystems are, the more you make it safe to focus on one bit of complexity at a time. Carefully defined objects separate concerns so that you can focus on one thing at a time. Packages provide the same benefit at a higher level of aggregation.
> Keeping routines short helps reduce your mental workload. Writing programs in terms of the problem domain, rather than in terms of low-level implementation details, and working at the highest level of abstraction reduce the load on your brain.
> The bottom line is that programmers who compensate for inherent human limitations write code that’s easier for themselves and others to understand and that has fewer errors.

(c) Steven C. McConnell "Code Complete" Chapter 5

## Decision

We divide the entire application into a modules. A module is a functional unit that, on the one hand, contains independent value, on the other hand, an application can work without this module.

## Consequences

* We can develop and manage the application at a high level, discuss modules 
* We can configure the application by enabling and disabling modules for different cases. 
* We can reuse modules in other applications
* But we will need to organize interaction between modules 

## Alternatives

The alternative is monolithic applications. Over time, they become very complex and bulky. We can't change them piecemeal (only by adhering to the public interface, the contract); any change could have a cascading effect. The developments of such an application can't be used in other cases or in other applications.

## Implementation 

Use this files structure

```
modulename/
  CMakeList.txt - module cmake project
  {name}module.cpp/h - module setup
  isomeinterface.h - public interfaces
  {name}types.h - public types
  {name}errors.h - error codes
  {name}commands.h - commands of module
  internal/ - dir with private implementations
  qml/ - dir with qml files and view models
```

The module must implement the module setup (muse::modularity::IModuleSetup),    
add the module to the application's CMakeLists.txt   
and add to the application creation fabric. 
See examples of modules in the code.   
