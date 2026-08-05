# 00002 Dependency injection

Date: 2020-05-30   
Tags: modularity, ioc, injection      
Maintainers: Igor Korsukov     

## Status: Accepted

## Context

We divide the application into modules. Modules must interact with each other - provide data, provide services, notify, etc.   
We want modules to be decoupled at the link level, so that we can use a module without its dependencies (replace dependencies with stubs or implement them differently), and also be able to disable modules.  
   
We also want to be able to unit test classes by replacing their dependencies with mocks.   

## Decision

We will use the Inversion of Control pattern. To do this, we will create an IoC container and a Dependency Injection system.   
Each module must register in the IoC container the services it provides, which can be used by other modules or by the module itself.   
The dependency injection system will by default get the dependency from the IoC container, but it will also be possible to set the dependency using a setter for unit test purposes.  

## Consequences

* We get a system of interconnections between services and modules that is not linked at the linking level.   
* We clearly show which interfaces and methods in them are public, and which are private to the service implementation.   
* We have the ability to replace any service with a stub, or change it to a different implementation, or replace it with a mock in unit tests.   
* We will clearly see what dependencies a class has.   
* But we'll need to explicitly create public interfaces and implement them. And register these implementations in the IoC container.  

## Alternatives

Other alternatives are either more complex or do not allow us to achieve our goals.   

## Implementation 
   
   
Public interface   

```cpp
#pragma once

#include "global/modularity/imoduleinterface.h"

namespace muse::somemodule {
class ISomeService : MODULE_GLOBAL_INTERFACE
{
    INTERFACE_ID(ISomeService)
public:
    virtual ~ISomeService() = default;

    virtual int someValue() const = 0;
    virtual void doSomeThing() = 0;
};
}
```
    
Implementation   

```cpp
#pragma once

#include "isomeservice.h"

namespace muse::somemodule {
class SomeService : public ISomeService
{
public:

    int someValue() const override;
    void doSomeThing() override;
};
}
```

Registration   

```cpp
void SomeModule::registerExports()
{
    globalIoc()->registerExport<ISomeService>(mname, new SomeService());
}
```

Usage  

```cpp
#pragma once

#include "global/modularity/ioc.h"
#include "somemodule/isomeservice.h"

namespace muse::anothermodule {
class AnotherService 
{
public:    
    GlobalInject<somemodule::ISomeService> someService;
    
public:

    void doSomeThing() 
    {
        int value = someService()->someValue();
        someService()->doSomeThing();  
    }
};
}
```

Replacement in unit tests   

```cpp

AnotherService anotherService;
anotherService.someService.set(std::make_shared<SomeServiceMock>());
```
