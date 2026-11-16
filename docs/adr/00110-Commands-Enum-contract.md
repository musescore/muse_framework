# 00110 Commands: Enum contract

Date: 2026-11-13   
Tags: commands, enums  
Maintainers: Igor Korsukov  

## Status: Accepted

## Context

We need to pass enum values ​​as command parameters.   
   
For example
```
enum class BracketType : unsigned char {
    AUTO_BRACKET, SHOW_BRACKET, SHOW_NO_BRACKET, DEFAULT /*from style*/
};
```

## Decision

We will pass enum values ​​as strings; these strings may differ from the actual enum value names, as they will be named in a way that best suits the API. To achieve this, we will create conversion contracts between the enum and the string representation.

## Consequences

* Values ​​that are easier for the consumer to understand
* It is easier to maintain backward compatibility and versioning. 
* The contract describes only what is used.
* But, we need to add a conversion contract for each enum.

## Alternatives

We could pass it as an `int`, but that wouldn't be clear to anyone; we’d need documentation, and maintaining backward compatibility would be difficult if the enum were to change.

## Implementation 

In each module, next to the commands, you need to add a file: `modulecommandsemums.h`
```
#pragma once

#include "rcommand/enumcontract.h"

#include "moduletypes.h"

namespace app::module {

// an alias for convenience    
template<typename E, std::size_t N>
using EnumTable = muse::rcommand::EnumTable<E, N>;

struct BracketTypeContract {
    static constexpr EnumTable<BracketType, 4> table { {
        { "auto", BracketType::AUTO_BRACKET, "auto bracket" },
        { "show", BracketType::SHOW_BRACKET, "show bracket" },
        { "none", BracketType::SHOW_NO_BRACKET, "no bracket" },
        { "default", BracketType::DEFAULT, "default (from style)" },
    } };
};
}

// Important! You must declare it in namespace muse::rcommand 
namespace muse::rcommand {
DECLARE_ENUM_CONTRACT(app::module::BracketTypeContract);
}
```

**Important!**   
You must declare it in `namespace muse::rcommand`    
Because the conversion functions are called from within the `muse::rcommand` module, which cannot "see" the application's modules.
  
Once the contract is declared, it will be used for automatic conversion within typed commands.   
```
struct AddBracketCommand {
    ...
    BracketType bracketType = BracketType::DEFAULT;

    cprop_decl input = FieldsScheme {
        Field{ "type", &AddBracketCommand::bracketType, "Bracket type" },
    };
```

You can also call conversion functions manually, if needed.   
```
{
    using namespace muse::rcommand;
    std::string str = EnumContract<BracketType>::str_val(BracketType::SHOW_BRACKET); 
    std::optional<BracketType> eval = EnumContract<BracketType>::enum_val(str); 
}
```

**Important!**   
Avoid using this conversion outside the context of commands! For example, when writing to a file... This is not enum serialization; it is a command API!