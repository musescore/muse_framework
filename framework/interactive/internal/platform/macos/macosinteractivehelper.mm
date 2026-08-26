/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2022 MuseScore Limited and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "macosinteractivehelper.h"

#include <QUrl>
#include <QStandardPaths>

#include <Cocoa/Cocoa.h>
#include <vector>

#include "types/uri.h"

#include "log.h"

using namespace muse;
using namespace muse::async;

bool MacOSInteractiveHelper::revealInFinder(const io::path_t& filePath)
{
    NSURL* fileUrl = QUrl::fromLocalFile(filePath.toQString()).toNSURL();

    [[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:@[fileUrl]];

    return true;
}

Ret MacOSInteractiveHelper::isAppExists(const std::string& appIdentifier)
{
    NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
    NSURL* appURL = [workspace URLForApplicationWithBundleIdentifier:@(appIdentifier.c_str())];
    return appURL != nil;
}

Ret MacOSInteractiveHelper::canOpenApp(const UriQuery& uri)
{
    NSString* nsUrlString = [NSString stringWithUTF8String:uri.toString().c_str()];
    if (nsUrlString == nil) {
        return make_ret(Ret::Code::InternalError, std::string("Invalid UTF-8 string passed as URI"));
    }

    NSURL* nsUrl = [NSURL URLWithString:nsUrlString];
    if (nsUrl == nil) {
        return make_ret(Ret::Code::InternalError, std::string("Invalid URI"));
    }

    NSURL* appURL = [[NSWorkspace sharedWorkspace] URLForApplicationToOpenURL:nsUrl];
    return appURL != nil;
}

async::Promise<Ret> MacOSInteractiveHelper::openApp(const UriQuery& uri)
{
    return Promise<Ret>([uri](auto resolve, auto reject) {
        NSString* nsUrlString = [NSString stringWithUTF8String:uri.toString().c_str()];
        if (nsUrlString == nil) {
            return reject(int(Ret::Code::InternalError), "Invalid UTF-8 string passed as URI");
        }

        NSURL* nsUrl = [NSURL URLWithString:nsUrlString];
        if (nsUrl == nil) {
            return reject(int(Ret::Code::InternalError), "Invalid URI");
        }

        auto configuration = [NSWorkspaceOpenConfiguration configuration];
        [configuration setPromptsUserIfNeeded:NO];
        [[NSWorkspace sharedWorkspace]
         openURL: nsUrl
         configuration: configuration
         completionHandler: ^(NSRunningApplication*, NSError* error) {
             if (error) {
                 std::string errorStr = [[error description] UTF8String];
                 (void)reject(int(Ret::Code::InternalError), errorStr);
             } else {
                 (void)resolve(make_ok());
             }
         }
        ];

        return Promise<Ret>::Result::unchecked();
    });
}

#import <objc/message.h>

struct SavedMenuItemState {
    NSMenuItem* item;
    id target;
    SEL action;
    NSString* keyEquivalent;
    NSEventModifierFlags modifierMask;
    BOOL isEnabled;
    NSImage* image;
    NSInteger preferredImageVisibility;
    BOOL hasActionImage;
    NSImage* actionImage;
    bool wasCreated;
    SEL scopeAction;
};

struct ActionSpec {
    MacOSInteractiveHelper::EditAction editAction;
    SEL action;
    NSString* key;
    NSEventModifierFlags mod;
    NSString* fallbackTitle;
};

static const std::vector<ActionSpec> s_actionSpecs = {
    { MacOSInteractiveHelper::EditAction::Undo,      @selector(undo:),      @"z", NSEventModifierFlagCommand,
      @"Undo" },
    { MacOSInteractiveHelper::EditAction::Redo,      @selector(redo:),      @"Z", (NSEventModifierFlagCommand | NSEventModifierFlagShift),
      @"Redo" },
    { MacOSInteractiveHelper::EditAction::Cut,       @selector(cut:),       @"x", NSEventModifierFlagCommand,
      @"Cut" },
    { MacOSInteractiveHelper::EditAction::Copy,      @selector(copy:),      @"c", NSEventModifierFlagCommand,
      @"Copy" },
    { MacOSInteractiveHelper::EditAction::Paste,     @selector(paste:),     @"v", NSEventModifierFlagCommand,
      @"Paste" },
    { MacOSInteractiveHelper::EditAction::SelectAll, @selector(selectAll:), @"a", NSEventModifierFlagCommand,
      @"Select All" },
};

static std::vector<SavedMenuItemState> s_savedStates;
static NSMenu* s_editMenu = nil;
static std::vector<id> s_menuObservers;
static BOOL s_savedAutoenables = YES;
static int s_nativeDialogCount = 0;
static bool s_isTransforming = false;
static int s_customEditMenuIndex = -1;
static std::map<MacOSInteractiveHelper::EditAction, int> s_customEditMenuStructure;

void MacOSInteractiveHelper::setEditMenuIndex(int menuIndex)
{
    s_customEditMenuIndex = menuIndex;
}

void MacOSInteractiveHelper::setEditMenuStructure(const std::map<EditAction, int>& structure)
{
    s_customEditMenuStructure = structure;
}

static NSMenuItem* matchByStructure(const ActionSpec& spec)
{
    if (!s_editMenu) {
        return nil;
    }

    int index = -1;
    auto it = s_customEditMenuStructure.find(spec.editAction);
    if (it != s_customEditMenuStructure.end()) {
        index = it->second;
    } else if (s_customEditMenuStructure.empty()) {
        switch (spec.editAction) {
        case MacOSInteractiveHelper::EditAction::Undo:      index = 0;
            break;
        case MacOSInteractiveHelper::EditAction::Redo:      index = 1;
            break;
        case MacOSInteractiveHelper::EditAction::Cut:       index = 2;
            break;
        case MacOSInteractiveHelper::EditAction::Copy:      index = 3;
            break;
        case MacOSInteractiveHelper::EditAction::Paste:     index = 4;
            break;
        case MacOSInteractiveHelper::EditAction::SelectAll: index = 5;
            break;
        }
    }

    if (index >= 0) {
        NSArray<NSMenuItem*>* items = [s_editMenu itemArray];
        if (index < (int)[items count]) {
            return items[index];
        }
    }
    return nil;
}

static void ensureFallbackItemsExist()
{
    if (!s_editMenu) {
        return;
    }

    for (const auto& spec : s_actionSpecs) {
        bool isPresentInMenu = false;
        for (const auto& s : s_savedStates) {
            if (s.scopeAction == spec.action && [s.item menu] == s_editMenu) {
                isPresentInMenu = true;
                break;
            }
        }
        if (!isPresentInMenu) {
            NSMenuItem* newItem = [[NSMenuItem alloc] initWithTitle:spec.fallbackTitle
                                   action:spec.action
                                   keyEquivalent:spec.key];
            [newItem setKeyEquivalentModifierMask:spec.mod];
            [newItem setTarget:nil];
            [newItem setEnabled:YES];
            [newItem setHidden:YES];
            [newItem setImage:nil];
            if ([newItem respondsToSelector:@selector(setPreferredImageVisibility:)]) {
                ((void (*)(id, SEL, NSInteger)) objc_msgSend)(newItem, @selector(setPreferredImageVisibility:), 1);
            }
            if ([newItem respondsToSelector:@selector(_setHasActionImage:)]) {
                ((void (*)(id, SEL, BOOL)) objc_msgSend)(newItem, @selector(_setHasActionImage:), NO);
            }
            if ([newItem respondsToSelector:@selector(_setActionImage:)]) {
                ((void (*)(id, SEL, id)) objc_msgSend)(newItem, @selector(_setActionImage:), nil);
            }
            if ([newItem respondsToSelector:@selector(_setActionImageName:)]) {
                ((void (*)(id, SEL, id)) objc_msgSend)(newItem, @selector(_setActionImageName:), nil);
            }
            if ([newItem respondsToSelector:@selector(setAllowsKeyEquivalentWhenHidden:)]) {
                [newItem setAllowsKeyEquivalentWhenHidden:YES];
            }
            [s_editMenu addItem:newItem];
            s_savedStates.push_back({ newItem, nil, nil, nil, 0, YES, nil, 0, NO, nil, true, spec.action });
        }
    }
}

static void applyScopeTransformations()
{
    if (s_isTransforming || s_nativeDialogCount <= 0) {
        return;
    }

    NSMenu* mainMenu = [NSApp mainMenu];
    if (mainMenu && s_customEditMenuIndex >= 0 && s_customEditMenuIndex < (int)[[mainMenu itemArray] count]) {
        NSMenu* liveEditMenu = [[mainMenu itemArray][s_customEditMenuIndex] submenu];
        if (liveEditMenu != s_editMenu) {
            if (s_editMenu) {
                [s_editMenu release];
            }
            s_editMenu = [liveEditMenu retain];
            if (s_editMenu) {
                s_savedAutoenables = [s_editMenu autoenablesItems];
            }
        }
    }

    if (!s_editMenu) {
        return;
    }

    s_isTransforming = true;

    [s_editMenu setAutoenablesItems:NO];

    auto isAlreadySaved = [](NSMenuItem* candidate) {
        for (const auto& s : s_savedStates) {
            if (s.item == candidate) {
                return true;
            }
        }
        return false;
    };

    for (const auto& spec : s_actionSpecs) {
        NSMenuItem* matchedItem = matchByStructure(spec);
        if (matchedItem) {
            if (!isAlreadySaved(matchedItem)) {
                NSImage* img = [matchedItem image];
                NSInteger visibility = 0;
                if ([matchedItem respondsToSelector:@selector(preferredImageVisibility)]) {
                    visibility = ((NSInteger (*)(id, SEL)) objc_msgSend)(matchedItem, @selector(preferredImageVisibility));
                }
                BOOL hasActionImg = NO;
                if ([matchedItem respondsToSelector:@selector(_hasActionImage)]) {
                    hasActionImg = ((BOOL (*)(id, SEL)) objc_msgSend)(matchedItem, @selector(_hasActionImage));
                }
                NSImage* actImg = nil;
                if ([matchedItem respondsToSelector:@selector(_actionImage)]) {
                    actImg = ((NSImage * (*)(id, SEL)) objc_msgSend)(matchedItem, @selector(_actionImage));
                }
                s_savedStates.push_back({ [matchedItem retain], [matchedItem target], [matchedItem action],
                                          [[matchedItem keyEquivalent] copy], [matchedItem keyEquivalentModifierMask],
                                          [matchedItem isEnabled], [img retain], visibility, hasActionImg,
                                          [actImg retain], false, spec.action });
            }
            [matchedItem setTarget:nil];
            [matchedItem setAction:spec.action];
            [matchedItem setKeyEquivalent:spec.key];
            [matchedItem setKeyEquivalentModifierMask:spec.mod];
            [matchedItem setEnabled:YES];
            [matchedItem setImage:nil];
            if ([matchedItem respondsToSelector:@selector(setPreferredImageVisibility:)]) {
                ((void (*)(id, SEL, NSInteger)) objc_msgSend)(matchedItem, @selector(setPreferredImageVisibility:), 1);
            }
            if ([matchedItem respondsToSelector:@selector(_setHasActionImage:)]) {
                ((void (*)(id, SEL, BOOL)) objc_msgSend)(matchedItem, @selector(_setHasActionImage:), NO);
            }
            if ([matchedItem respondsToSelector:@selector(_setActionImage:)]) {
                ((void (*)(id, SEL, id)) objc_msgSend)(matchedItem, @selector(_setActionImage:), nil);
            }
            if ([matchedItem respondsToSelector:@selector(_setActionImageName:)]) {
                ((void (*)(id, SEL, id)) objc_msgSend)(matchedItem, @selector(_setActionImageName:), nil);
            }
        }
    }

    ensureFallbackItemsExist();

    s_isTransforming = false;
}

MacOSInteractiveHelper::NativeDialogScope::NativeDialogScope()
{
    NSCAssert([NSThread isMainThread], @"NativeDialogScope must run on main thread");

    s_nativeDialogCount++;
    if (s_nativeDialogCount > 1) {
        return;
    }

    s_savedStates.clear();
    s_editMenu = nil;

    // Initial transformation of current menu items and creation of any missing fallback items
    applyScopeTransformations();

    // Register notification observers to safely protect against Qt/QML menu updates during modal dialog.
    if (s_menuObservers.empty()) {
        id beginObs = [[NSNotificationCenter defaultCenter] addObserverForName:NSMenuDidBeginTrackingNotification
                       object:nil
                       queue:[NSOperationQueue mainQueue]
                       usingBlock:^(NSNotification* _Nonnull /*note*/) {
                           dispatch_async(dispatch_get_main_queue(), ^{
                                              applyScopeTransformations();
                                          });
                       }];
        s_menuObservers.push_back(beginObs);

        id endObs = [[NSNotificationCenter defaultCenter] addObserverForName:NSMenuDidEndTrackingNotification
                     object:nil
                     queue:[NSOperationQueue mainQueue]
                     usingBlock:^(NSNotification* _Nonnull /*note*/) {
                         applyScopeTransformations();
                     }];
        s_menuObservers.push_back(endObs);
    }
}

MacOSInteractiveHelper::NativeDialogScope::~NativeDialogScope()
{
    NSCAssert([NSThread isMainThread], @"~NativeDialogScope must run on main thread");

    s_nativeDialogCount--;
    if (s_nativeDialogCount <= 0) {
        s_nativeDialogCount = 0;

        for (id obs : s_menuObservers) {
            [[NSNotificationCenter defaultCenter] removeObserver:obs];
        }
        s_menuObservers.clear();

        for (const auto& saved : s_savedStates) {
            if (saved.wasCreated) {
                if ([saved.item menu]) {
                    [[saved.item menu] removeItem:saved.item];
                }
                [saved.item release];
            } else {
                [saved.item setTarget:saved.target];
                [saved.item setAction:saved.action];
                [saved.item setKeyEquivalent:saved.keyEquivalent ? saved.keyEquivalent : @""];
                [saved.item setKeyEquivalentModifierMask:saved.modifierMask];
                [saved.item setEnabled:saved.isEnabled];
                [saved.item setImage:saved.image];
                if ([saved.item respondsToSelector:@selector(setPreferredImageVisibility:)]) {
                    ((void (*)(id, SEL, NSInteger)) objc_msgSend)(saved.item, @selector(setPreferredImageVisibility:),
                                                                  saved.preferredImageVisibility);
                }
                if ([saved.item respondsToSelector:@selector(_setHasActionImage:)]) {
                    ((void (*)(id, SEL, BOOL)) objc_msgSend)(saved.item, @selector(_setHasActionImage:),
                                                             saved.hasActionImage);
                }
                if ([saved.item respondsToSelector:@selector(_setActionImage:)]) {
                    ((void (*)(id, SEL, id)) objc_msgSend)(saved.item, @selector(_setActionImage:),
                                                           saved.actionImage);
                }
                if (saved.actionImage) {
                    [saved.actionImage release];
                }
                if (saved.image) {
                    [saved.image release];
                }
                [saved.keyEquivalent release];
                [saved.item release];
            }
        }
        s_savedStates.clear();
        if (s_editMenu) {
            [s_editMenu setAutoenablesItems:s_savedAutoenables];
            [s_editMenu release];
            s_editMenu = nil;
        }
    }
}
