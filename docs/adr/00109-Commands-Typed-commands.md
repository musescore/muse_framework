# 00109 Commands: Typed commands

Date: 2026-10-09   
Tags: commands, dispatch, schema, menu, toolbar   
Maintainers: Paul Martin   

## Status: Proposed

## Context

A command is identified by a string, and its parameters are a `Params` map keyed by strings. The parameter names and
types are written several times by hand: in the `InputSchema` of the `CommandInfo`, in every sender that fills a
`CommandQuery` or a `Params`, and in the handler that reads them back with `params.at("...")`. Nothing ties these copies
together, so they drift, and a typo or a wrong type is only found at runtime, if it is found at all.

About 90% of our commands take no parameters, and they are fine as they are. The remaining 10% carry parameters, and those are the ones that are error-prone to set up. We hardly ever dispatch a command by hand: almost all of them are sent by a menu item or a toolbar item, so that is where the parameters get written.   

Commands are becoming an API (shortcuts, extensions API, MCP server, command palette; see ADR 00107). An API wants a single, checked definition of each command.   

## Decision

A command may be declared as a struct, the typed command. The struct owns everything about the command: its `id`, its `title` and `description`, optionally its `decoration` and `availabilities`, and its parameters as plain members listed once in `fields()`. Everything else is generated from it:   

* `makeCommandInfo<C>()` builds the `CommandInfo`, including the `InputSchema`.   
* `toParams(c)` / `fromParams(params, c, err)` convert to and from the untyped `Params`; `fromParams` is the single validation point for parameters that arrive untyped (shortcut, API, MCP, `CommandQuery`): missing keys, wrong types and unknown keys are rejected with `BadArgs` before the handler runs.   
* `dispatcher()->dispatch(C { ... })` sends the struct; `dispatcher()->onRequest<C>(client, handler)` receives it already validated.   
* `makeMenuItem(C { ... })` and `makeItem(C { ... })` build a menu or toolbar item that carries the typed parameters; the item itself stays untyped runtime data, since it is consumed by QML and dispatched by the framework.   

A `TypedCommand` concept defines what a typed command is; the generated functions and the dispatcher templates are constrained by it, so a struct missing a piece does not compile, and so does a parameter whose C++ type has no `ParamTraits`. Domain types are made usable as parameters by specializing `ParamTraits`.   

Typed commands do not replace the existing `CommandInfo` / `Params` machinery; they generate it. A module register is still a list of `CommandInfo`, in which hand-written entries and `makeCommandInfo<C>()` entries coexist, and a typed handler is registered through the same `CallBack` as any other. Typed is the way to declare a command that takes parameters, and the recommended way for new commands. Parameterless commands keep working as they are and migrate when their module is touched; a parameterless typed command is a struct with an empty `fields()`, which keeps its id and texts in one place.   

## Consequences

* The id, the texts and the parameters of a command are declared once; a wrong parameter name or type is a compile error on both the sending and the receiving side.   
* The input schema is always complete and always matches the handler, which is what the API, the MCP server and the documentation consume.   
* Handlers receive a struct instead of a map and lose their hand-written argument checks.   
* The legacy `action://` bridge rows of a converted command become unnecessary as soon as its senders are converted.   
* Two ways of declaring a command exist during the transition; both produce the same `CommandInfo`, so nothing downstream needs to know the difference.   
* C++20 concepts are now used in the framework.   
* Known gaps, to address separately: a field cannot yet be declared optional with a default value (only `std::optional<T>` may be absent), `Arg` has no `required` flag to expose that in the schema, and parameter descriptions are not translatable because `Arg::description` is a `String`.   

## Alternatives

* Keep string ids and parameters, and only add runtime validation of `Params` against the hand-written schema. This catches wrong values but not the drift between schema, sender and handler, and still finds typos at runtime.   
* Make typed commands mandatory for every command. It gives one way to declare commands but means converting the 90% that have no parameters and no problem; the register accepts both shapes, so this can happen gradually instead.   
* Make `MenuItem` and `ToolBarItem` themselves template classes. The items are QML-facing runtime data and are dispatched by the framework; typing them would not catch anything that typing their construction does not already catch.   

## Implementation

```
struct ChangePlayRegion {
    static inline const Command id { "command://playback/play-region/change" };
    //: Action title: shown as a menu item or a button label; keep it short
    static inline const TranslatableString title = TranslatableString("action", "Change play region");
    //: Action description: shown as a tooltip; can be a full sentence
    static inline const TranslatableString description = TranslatableString("action_description", "Change play region");
    static inline const Availabilities availabilities = Availability::All;

    double start = 0.0;
    double end = 0.0;

    static constexpr auto fields()
    {
        return std::tuple {
            Field { "start", &ChangePlayRegion::start, u"Region start in seconds", 0.0, 3600.0 },
            Field { "end",   &ChangePlayRegion::end,   u"Region end in seconds" },
        };
    }
};
```

```
// register
const std::vector<CommandInfo> s_commandInfos = {
    makeCommandInfo<ChangePlayRegion>(),
};

// receive
dispatcher()->onRequest<ChangePlayRegion>(this, [this](const ChangePlayRegion& c) {
    return playback()->setPlayRegion(c.start, c.end);
});

// send
dispatcher()->dispatch(ChangePlayRegion { .start = 1.0, .end = 5.0 });

// menu item
items << makeMenuItem(ChangePlayRegion { .start = 1.0, .end = 5.0 });
```

```
template<typename C>
concept TypedCommand = requires {
    { C::id } -> std::convertible_to<const Command&>;
    { C::title } -> std::convertible_to<const TranslatableString&>;
    { C::description } -> std::convertible_to<const TranslatableString&>;
    C::fields();
};
```