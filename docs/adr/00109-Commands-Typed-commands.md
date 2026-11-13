# 00109 Commands: Typed commands

Date: 2026-10-09   
Tags: commands, dispatch   
Maintainers: Paul Martin, Igor Korsukov   

## Status: Accepted

## Context

Initially, actions were simply written as strings in the code—for example, "some-action" or "action://some/action" This was a deliberate decision: we wanted to avoid coupling between modules at the compilation stage, and even more so at the linking stage. Furthermore, we held the view that the component issuing a command shouldn't need to "know" which entity the command belongs to; the command is simply sent into the system, and someone else will process it. As for the parameters, they were either passed in a specific order (which required knowing that order), or we would explicitly write out the parameter key name as a string.   
Some developers disliked this, as it is a potential source of bugs caused by typos. It also makes refactoring difficult (especially if someone decides to generate action names dynamically).   
     
When transitioning to commands, we decided to address this inconvenience by defining the commands as constants. This proved to be a convenient approach: all of a module's commands are listed in a single header file, and these constants are then used throughout the rest of the code. We coupling the modules at compile time, so now each one "knows" which command belongs to whom—but we decided that this isn't a major issue.   
However, the issue of parameter key names remained unresolved; they were still being defined as strings within the code in various places. Another drawback was that a command’s URI and its associated information (title, input schema, etc.) were located in different places.   

## Decision

We decided to add support for typed commands. Specifically, a command is defined as a structure with fields, which can be used both to generate a command registry containing relevant information and for sending/receiving commands.

## Consequences

* Now the entire command is defined in one place—including its URI, meta-information, and parameters with annotations.
* We don’t need to know the name of the parameter keys and their types, we explicitly set/read the values ​​of the fields and that’s it. 
* The command declaration, as a whole, appears simpler and clearer; it includes only what is required.
* The command typing system is built on top of the existing system, so there is no need to change the internal implementation. 
* Typed commands are automatically serialized and deserialized; this is necessary for API and MCP support.  
* The connection between modules remains at the compilation level only.
* But, the header file for the module containing the commands can now become quite large; it’s difficult to see a simple list—you have to scroll through it to take a look.

## Alternatives

Alternative solutions involving only parameters were considered, but they proved inconvenient to use. The approach based on typed constructs turned out to be much more convenient.

## Implementation 

Minimal command without parameters
```
struct SomeCommand {
    cprop_decl id = Command("command://some/command");
    cprop_decl title = MnemonicString("action", "Some");
    cprop_decl description = TranslatableString("action", "Some command");
};
```
   
Command with input parameters   
```
struct SomeCommand {
    cprop_decl id = Command("command://some/command");
    cprop_decl title = MnemonicString("action", "Some");
    cprop_decl description = TranslatableString("action", "Some command");
    cprop_decl availabilities = Availability::API;

    bool bVal = false;
    int iVal = 0;

    cprop_decl input = FieldsScheme {
        Field("bVal", &SomeCommand::bVal, "Boolean value"),
        Field("iVal", &SomeCommand::iVal, "Integer value", -100, 100, Presence::Optional),
    };
};
```

Command with an output result
```
struct SomeCommand {
    cprop_decl id = Command("command://some/command");
    cprop_decl title = MnemonicString("action", "Some");
    cprop_decl description = TranslatableString("action", "Some command");
    cprop_decl annotations = muse::rcommand::Annotation::ReadOnly;

    struct Output {
        bool enabled = false;
    };

    using O = SomeCommand::Output;
    cprop_decl output = FieldsScheme {
        Field("enabled", &O::enabled, "Flag indicating some is enabled"),
    };
};
```

Dispatch 
```
commandDispatcher()->dispatch(SomeCommand {.bVal = true});
```

Receive 
```
commandDispatcher()->onRequest<SomeCommand>(this, [this](const auto& command) {
    LOGD() << command.bVal;
    return muse::make_ok();
});
```