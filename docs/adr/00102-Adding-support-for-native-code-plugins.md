# 00102 Adding support for native code plugins

Date: 2026-07-15  
Tags: extensions, api, native
Maintainers: Dmitry Makarenko, Igor Korsukov

## Status: Accepted

## Context

Audacity requires the use of plugins with native code (for example, using OpenVino, libraries for audio processing, effects).      
The framework has an extension system that assumes the use of Qml for creating user interfaces and Js for creating logic. There is no support for calling native code.  

## Decision

We decided to add support for loading dynamic libraries and calling native code from JS.  

## Consequences

Pros:  
* We have one main extension system with all its capabilities and documentation    
* We are adding the ability to add dynamic libraries to the extension and call code from them.

Cons:   
* To develop a simple native plugin without UI, the developer will have to write the gluing code in JS  

## Alternatives

The option of creating a separate system of native plugins was considered.
However, we saw the following drawbacks:  
* It could gradually replicate the framework's extension system. We'd end up with two duplicate systems with different APIs, but doing the same thing and addressing the same need. 
* It's difficult to create UI in native plugins. To create UI, we'd still have to use the framework's extension system.


## Implementation 

effect.js
```js
const native = require("MuseApi.Native")


    const library = native.open("gain")
    if (!library) {
        throw new Error("Could not load the native library")
    }    

    const gain = library.bind("process")

    ...

    for (const channel of chunk.channels)
        gain(channel, values.gain)
```

gain.c 
```c
#include "extension.h"

#include <string.h>

EXT_EXPORT int32_t extension_dispatch_v0(const char* call, const ext_value* args, uint32_t arg_count, ext_value* result)
{
    if (!call || !result || strcmp(call, "process") != 0) {
        return EXT_STATUS_UNKNOWN_CALL;
    }
    if (!args || arg_count != 2 || args[0].type != EXT_VALUE_BUFFER || args[1].type != EXT_VALUE_NUMBER
        || args[0].as_buffer.size % sizeof(float) != 0) {
        return EXT_STATUS_INVALID_ARGUMENT;
    }

    float* samples = args[0].as_buffer.data;
    const uint64_t count = args[0].as_buffer.size / sizeof(float);
    if (!samples && count) {
        return EXT_STATUS_INVALID_ARGUMENT;
    }
    for (uint64_t i = 0; i < count; ++i) {
        samples[i] *= (float)args[1].as_number;
    }

    result->type = EXT_VALUE_NONE;
    return EXT_STATUS_OK;
}
```

