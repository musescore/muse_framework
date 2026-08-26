/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited and others
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

#include <gtest/gtest.h>

#import <Cocoa/Cocoa.h>
#import <objc/message.h>
#include "interactive/internal/platform/macos/macosinteractivehelper.h"

@interface MockMenuTarget : NSObject
- (void)triggerAction:(id)sender;
- (void)fileSaveAction:(id)sender;
@end

@implementation MockMenuTarget
- (void)triggerAction:(id)sender {}
- (void)fileSaveAction:(id)sender {}
@end

class MacOSInteractiveHelperTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_pool = [[NSAutoreleasePool alloc] init];
        NSApplication* app = [NSApplication sharedApplication];
        if (!app) {
            GTEST_SKIP() << "No macOS GUI / NSApplication session available";
        }

        m_mockTarget = [[MockMenuTarget alloc] init];

        m_mainMenu = [[NSMenu alloc] initWithTitle:@"MainMenu"];
        [NSApp setMainMenu:m_mainMenu];

        // File Menu (non-edit menu)
        NSMenuItem* fileTop = [[NSMenuItem alloc] initWithTitle:@"File" action:nil keyEquivalent:@""];
        NSMenu* fileSub = [[NSMenu alloc] initWithTitle:@"File"];
        m_saveItem = [[NSMenuItem alloc] initWithTitle:@"Save" action:@selector(fileSaveAction:) keyEquivalent:@"s"];
        [m_saveItem setKeyEquivalentModifierMask:NSEventModifierFlagCommand];
        [m_saveItem setTarget:m_mockTarget];
        [fileSub addItem:m_saveItem];
        [fileTop setSubmenu:fileSub];
        [m_mainMenu addItem:fileTop];

        // Edit Menu
        NSMenuItem* editTop = [[NSMenuItem alloc] initWithTitle:@"Edit" action:nil keyEquivalent:@""];
        NSMenu* editSub = [[NSMenu alloc] initWithTitle:@"Edit"];

        m_undoItem = [[NSMenuItem alloc] initWithTitle:@"Undo" action:@selector(triggerAction:) keyEquivalent:@"z"];
        [m_undoItem setKeyEquivalentModifierMask:NSEventModifierFlagCommand];
        [m_undoItem setTarget:m_mockTarget];
        [editSub addItem:m_undoItem];

        m_redoItem = [[NSMenuItem alloc] initWithTitle:@"Redo" action:@selector(triggerAction:) keyEquivalent:@"Z"];
        [m_redoItem setKeyEquivalentModifierMask:(NSEventModifierFlagCommand | NSEventModifierFlagShift)];
        [m_redoItem setTarget:m_mockTarget];
        [editSub addItem:m_redoItem];

        m_cutItem = [[NSMenuItem alloc] initWithTitle:@"Cut" action:@selector(triggerAction:) keyEquivalent:@"x"];
        [m_cutItem setKeyEquivalentModifierMask:NSEventModifierFlagCommand];
        [m_cutItem setTarget:m_mockTarget];
        [editSub addItem:m_cutItem];

        m_copyItem = [[NSMenuItem alloc] initWithTitle:@"Copy" action:@selector(triggerAction:) keyEquivalent:@"c"];
        [m_copyItem setKeyEquivalentModifierMask:NSEventModifierFlagCommand];
        [m_copyItem setTarget:m_mockTarget];
        [editSub addItem:m_copyItem];

        m_pasteItem = [[NSMenuItem alloc] initWithTitle:@"Paste" action:@selector(triggerAction:) keyEquivalent:@"v"];
        [m_pasteItem setKeyEquivalentModifierMask:NSEventModifierFlagCommand];
        [m_pasteItem setTarget:m_mockTarget];
        [editSub addItem:m_pasteItem];

        m_selectAllItem = [[NSMenuItem alloc] initWithTitle:@"Select All" action:@selector(triggerAction:) keyEquivalent:@"a"];
        [m_selectAllItem setKeyEquivalentModifierMask:NSEventModifierFlagCommand];
        [m_selectAllItem setTarget:m_mockTarget];
        [editSub addItem:m_selectAllItem];

        [editTop setSubmenu:editSub];
        [m_mainMenu addItem:editTop];

        std::map<muse::MacOSInteractiveHelper::EditAction, int> defaultStructure = {
            { muse::MacOSInteractiveHelper::EditAction::Undo, 0 },
            { muse::MacOSInteractiveHelper::EditAction::Redo, 1 },
            { muse::MacOSInteractiveHelper::EditAction::Cut, 2 },
            { muse::MacOSInteractiveHelper::EditAction::Copy, 3 },
            { muse::MacOSInteractiveHelper::EditAction::Paste, 4 },
            { muse::MacOSInteractiveHelper::EditAction::SelectAll, 5 },
        };
        muse::MacOSInteractiveHelper::setEditMenuIndex(1);
        muse::MacOSInteractiveHelper::setEditMenuStructure(defaultStructure);
    }

    void TearDown() override
    {
        muse::MacOSInteractiveHelper::setEditMenuIndex(-1);
        muse::MacOSInteractiveHelper::setEditMenuStructure({});
        [NSApp setMainMenu:nil];
        [m_pool drain];
    }

    NSAutoreleasePool* m_pool = nil;
    MockMenuTarget* m_mockTarget = nil;
    NSMenu* m_mainMenu = nil;
    NSMenuItem* m_saveItem = nil;
    NSMenuItem* m_undoItem = nil;
    NSMenuItem* m_redoItem = nil;
    NSMenuItem* m_cutItem = nil;
    NSMenuItem* m_copyItem = nil;
    NSMenuItem* m_pasteItem = nil;
    NSMenuItem* m_selectAllItem = nil;
};

TEST_F(MacOSInteractiveHelperTest, NativeDialogScope_SetsNilTargetAndStandardActions)
{
    // Before scope: All edit items target the mock object with triggerAction:
    EXPECT_EQ([m_undoItem target], m_mockTarget);
    EXPECT_EQ([m_undoItem action], @selector(triggerAction:));

    {
        muse::MacOSInteractiveHelper::NativeDialogScope scope;

        // Inside scope: Edit items must target First Responder (nil) with standard selectors and key equivalents, and image is nil
        EXPECT_EQ([m_undoItem target], nil);
        EXPECT_EQ([m_undoItem action], @selector(undo:));
        EXPECT_STREQ([[m_undoItem keyEquivalent] UTF8String], "z");
        EXPECT_TRUE([m_undoItem isEnabled]);
        if ([m_undoItem respondsToSelector:@selector(preferredImageVisibility)]) {
            NSInteger visibility = ((NSInteger (*)(id, SEL)) objc_msgSend)(m_undoItem, @selector(preferredImageVisibility));
            EXPECT_EQ(visibility, 1); // 1 = NSMenuItemImageVisibilityHidden
        }
        if ([m_copyItem respondsToSelector:@selector(_hasActionImage)]) {
            BOOL hasImg = ((BOOL (*)(id, SEL)) objc_msgSend)(m_copyItem, @selector(_hasActionImage));
            EXPECT_FALSE(hasImg);
        }
        if ([m_copyItem respondsToSelector:@selector(_actionImage)]) {
            NSImage* actImg = ((NSImage * (*)(id, SEL)) objc_msgSend)(m_copyItem, @selector(_actionImage));
            EXPECT_EQ(actImg, nil);
        }
    }
}

TEST_F(MacOSInteractiveHelperTest, NativeDialogScope_RestoresOriginalTargetsAndActions)
{
    {
        muse::MacOSInteractiveHelper::NativeDialogScope scope;
        EXPECT_EQ([m_undoItem target], nil);
    }

    // After scope exits: All items must be restored to original target, action, and keyEquivalent
    EXPECT_EQ([m_undoItem target], m_mockTarget);
    EXPECT_EQ([m_undoItem action], @selector(triggerAction:));
    EXPECT_STREQ([[m_undoItem keyEquivalent] UTF8String], "z");

    EXPECT_EQ([m_redoItem target], m_mockTarget);
    EXPECT_EQ([m_redoItem action], @selector(triggerAction:));
    EXPECT_STREQ([[m_redoItem keyEquivalent] UTF8String], "Z");

    EXPECT_EQ([m_cutItem target], m_mockTarget);
    EXPECT_EQ([m_cutItem action], @selector(triggerAction:));
    EXPECT_STREQ([[m_cutItem keyEquivalent] UTF8String], "x");

    EXPECT_EQ([m_copyItem target], m_mockTarget);
    EXPECT_EQ([m_copyItem action], @selector(triggerAction:));
    EXPECT_STREQ([[m_copyItem keyEquivalent] UTF8String], "c");

    EXPECT_EQ([m_pasteItem target], m_mockTarget);
    EXPECT_EQ([m_pasteItem action], @selector(triggerAction:));
    EXPECT_STREQ([[m_pasteItem keyEquivalent] UTF8String], "v");

    EXPECT_EQ([m_selectAllItem target], m_mockTarget);
    EXPECT_EQ([m_selectAllItem action], @selector(triggerAction:));
    EXPECT_STREQ([[m_selectAllItem keyEquivalent] UTF8String], "a");
}

TEST_F(MacOSInteractiveHelperTest, NativeDialogScope_HandlesNestedScopes)
{
    {
        muse::MacOSInteractiveHelper::NativeDialogScope outerScope;
        EXPECT_EQ([m_pasteItem target], nil);

        {
            muse::MacOSInteractiveHelper::NativeDialogScope innerScope;
            EXPECT_EQ([m_pasteItem target], nil);
        }

        // Still in outer scope
        EXPECT_EQ([m_pasteItem target], nil);
        EXPECT_EQ([m_pasteItem action], @selector(paste:));
    }

    // Outermost scope exited -> restored
    EXPECT_EQ([m_pasteItem target], m_mockTarget);
    EXPECT_EQ([m_pasteItem action], @selector(triggerAction:));
}

TEST_F(MacOSInteractiveHelperTest, NativeDialogScope_PreservesNonEditItems)
{
    {
        muse::MacOSInteractiveHelper::NativeDialogScope scope;

        // Non-edit items (such as Save) must remain completely untouched
        EXPECT_EQ([m_saveItem target], m_mockTarget);
        EXPECT_EQ([m_saveItem action], @selector(fileSaveAction:));
    }

    EXPECT_EQ([m_saveItem target], m_mockTarget);
    EXPECT_EQ([m_saveItem action], @selector(fileSaveAction:));
}

TEST_F(MacOSInteractiveHelperTest, NativeDialogScope_HandlesEmptyKeyEquivalentMenuItems)
{
    // Simulate main branch where Qt constructs menu items without shortcuts (keyEquivalent is empty)
    NSMenuItem* editTop = [m_mainMenu itemArray][1];
    NSMenu* editSub = [editTop submenu];
    [editSub removeAllItems];

    std::map<muse::MacOSInteractiveHelper::EditAction, int> structure = {
        { muse::MacOSInteractiveHelper::EditAction::Undo, 0 },
        { muse::MacOSInteractiveHelper::EditAction::Redo, 1 },
        { muse::MacOSInteractiveHelper::EditAction::Paste, 2 },
    };
    muse::MacOSInteractiveHelper::setEditMenuStructure(structure);

    NSMenuItem* undo = [[NSMenuItem alloc] initWithTitle:@"Undo\t" action:@selector(triggerAction:) keyEquivalent:@""];
    [undo setTarget:m_mockTarget];
    [editSub addItem:undo];

    NSMenuItem* redo = [[NSMenuItem alloc] initWithTitle:@"Redo\t" action:@selector(triggerAction:) keyEquivalent:@""];
    [redo setTarget:m_mockTarget];
    [editSub addItem:redo];

    NSMenuItem* paste = [[NSMenuItem alloc] initWithTitle:@"&Paste\t" action:@selector(triggerAction:) keyEquivalent:@""];
    [paste setTarget:m_mockTarget];
    [editSub addItem:paste];

    {
        muse::MacOSInteractiveHelper::NativeDialogScope scope;

        EXPECT_EQ([undo target], nil);
        EXPECT_EQ([undo action], @selector(undo:));
        EXPECT_STREQ([[undo keyEquivalent] UTF8String], "z");
        EXPECT_TRUE([undo isEnabled]);

        EXPECT_EQ([redo target], nil);
        EXPECT_EQ([redo action], @selector(redo:));
        EXPECT_STREQ([[redo keyEquivalent] UTF8String], "Z");
        EXPECT_TRUE([redo isEnabled]);

        EXPECT_EQ([paste target], nil);
        EXPECT_EQ([paste action], @selector(paste:));
        EXPECT_STREQ([[paste keyEquivalent] UTF8String], "v");
        EXPECT_TRUE([paste isEnabled]);
    }

    // After scope: original empty keyEquivalent and target restored
    EXPECT_EQ([undo target], m_mockTarget);
    EXPECT_STREQ([[undo keyEquivalent] UTF8String], "");

    EXPECT_EQ([redo target], m_mockTarget);
    EXPECT_STREQ([[redo keyEquivalent] UTF8String], "");

    EXPECT_EQ([paste target], m_mockTarget);
    EXPECT_STREQ([[paste keyEquivalent] UTF8String], "");
}

TEST_F(MacOSInteractiveHelperTest, NativeDialogScope_HandlesUppercaseZRedoWithoutShift)
{
    // AppKit uppercase "Z" implies Shift even if modifierMask is only Command
    NSMenuItem* editTop = [m_mainMenu itemArray][1];
    NSMenu* editSub = [editTop submenu];
    [editSub removeAllItems];

    std::map<muse::MacOSInteractiveHelper::EditAction, int> structure = {
        { muse::MacOSInteractiveHelper::EditAction::Redo, 0 },
    };
    muse::MacOSInteractiveHelper::setEditMenuStructure(structure);

    NSMenuItem* redoItem = [[NSMenuItem alloc] initWithTitle:@"Redo" action:@selector(triggerAction:) keyEquivalent:@"Z"];
    [redoItem setKeyEquivalentModifierMask:NSEventModifierFlagCommand]; // no explicit Shift flag
    [redoItem setTarget:m_mockTarget];
    [editSub addItem:redoItem];

    {
        muse::MacOSInteractiveHelper::NativeDialogScope scope;
        EXPECT_EQ([redoItem target], nil);
        EXPECT_EQ([redoItem action], @selector(redo:));
        EXPECT_STREQ([[redoItem keyEquivalent] UTF8String], "Z");
        EXPECT_TRUE([redoItem isEnabled]);
    }

    EXPECT_EQ([redoItem target], m_mockTarget);
    EXPECT_EQ([redoItem action], @selector(triggerAction:));
    EXPECT_STREQ([[redoItem keyEquivalent] UTF8String], "Z");
}

TEST_F(MacOSInteractiveHelperTest, NativeDialogScope_PreservesPasteSpecialWithDifferentModifier)
{
    NSMenuItem* editTop = [m_mainMenu itemArray][1];
    NSMenu* editSub = [editTop submenu];
    [editSub removeAllItems];

    std::map<muse::MacOSInteractiveHelper::EditAction, int> structure = {
        { muse::MacOSInteractiveHelper::EditAction::Paste, 0 },
    };
    muse::MacOSInteractiveHelper::setEditMenuStructure(structure);

    NSMenuItem* normalPaste = [[NSMenuItem alloc] initWithTitle:@"&Paste\t" action:@selector(triggerAction:) keyEquivalent:@""];
    [normalPaste setTarget:m_mockTarget];
    [editSub addItem:normalPaste];

    NSMenuItem* specialPaste = [[NSMenuItem alloc] initWithTitle:@"Paste Special\t" action:@selector(triggerAction:) keyEquivalent:@"v"];
    [specialPaste setKeyEquivalentModifierMask:(NSEventModifierFlagCommand | NSEventModifierFlagOption)];
    [specialPaste setTarget:m_mockTarget];
    [editSub addItem:specialPaste];

    {
        muse::MacOSInteractiveHelper::NativeDialogScope scope;

        EXPECT_EQ([normalPaste target], nil);
        EXPECT_EQ([normalPaste action], @selector(paste:));
        EXPECT_STREQ([[normalPaste keyEquivalent] UTF8String], "v");
        EXPECT_EQ([normalPaste keyEquivalentModifierMask], NSEventModifierFlagCommand);
        EXPECT_TRUE([normalPaste isEnabled]);

        // Paste Special must remain completely untouched
        EXPECT_EQ([specialPaste target], m_mockTarget);
        EXPECT_EQ([specialPaste action], @selector(triggerAction:));
        EXPECT_STREQ([[specialPaste keyEquivalent] UTF8String], "v");
        EXPECT_EQ([specialPaste keyEquivalentModifierMask], (NSEventModifierFlagCommand | NSEventModifierFlagOption));
    }

    EXPECT_EQ([normalPaste target], m_mockTarget);
    EXPECT_STREQ([[normalPaste keyEquivalent] UTF8String], "");
    EXPECT_EQ([specialPaste target], m_mockTarget);
    EXPECT_EQ([specialPaste keyEquivalentModifierMask], (NSEventModifierFlagCommand | NSEventModifierFlagOption));
}

TEST_F(MacOSInteractiveHelperTest, NativeDialogScope_CreatesAndRemovesFallbackItemsWhenActionsMissing)
{
    // Configure menu with only Undo (index 0) provided. Redo, Cut, Copy, Paste, Select All are missing.
    std::map<muse::MacOSInteractiveHelper::EditAction, int> structure = {
        { muse::MacOSInteractiveHelper::EditAction::Undo, 0 },
    };
    muse::MacOSInteractiveHelper::setEditMenuStructure(structure);
    muse::MacOSInteractiveHelper::setEditMenuIndex(1);

    NSMenuItem* editTop = [m_mainMenu itemArray][1];
    NSMenu* editSub = [editTop submenu];
    [editSub removeAllItems];

    NSMenuItem* undo = [[NSMenuItem alloc] initWithTitle:@"Undo" action:@selector(triggerAction:) keyEquivalent:@"z"];
    [undo setTarget:m_mockTarget];
    [editSub addItem:undo];

    NSUInteger initialCount = [[editSub itemArray] count]; // 1 item (Undo)

    {
        muse::MacOSInteractiveHelper::NativeDialogScope scope;

        // 5 fallback items should be created for missing actions (Redo, Cut, Copy, Paste, Select All)
        EXPECT_EQ([[editSub itemArray] count], initialCount + 5);

        // Verify fallback items target First Responder, are enabled, and hidden from view
        for (NSUInteger i = initialCount; i < [[editSub itemArray] count]; ++i) {
            NSMenuItem* fallback = [editSub itemArray][i];
            EXPECT_EQ([fallback target], nil);
            EXPECT_TRUE([fallback isEnabled]);
            EXPECT_TRUE([fallback isHidden]);
        }
    }

    // On scope exit, all 5 fallback items must be removed cleanly
    EXPECT_EQ([[editSub itemArray] count], initialCount);
    EXPECT_EQ([undo target], m_mockTarget);
}

TEST_F(MacOSInteractiveHelperTest, NativeDialogScope_MatchesCustomStructureAndCreatesZeroTemporaryItems)
{
    std::map<muse::MacOSInteractiveHelper::EditAction, int> structure = {
        { muse::MacOSInteractiveHelper::EditAction::Undo, 0 },
        { muse::MacOSInteractiveHelper::EditAction::Redo, 1 },
        { muse::MacOSInteractiveHelper::EditAction::Cut, 4 },
        { muse::MacOSInteractiveHelper::EditAction::Copy, 5 },
        { muse::MacOSInteractiveHelper::EditAction::Paste, 6 },
        { muse::MacOSInteractiveHelper::EditAction::SelectAll, 8 },
    };
    muse::MacOSInteractiveHelper::setEditMenuStructure(structure);
    muse::MacOSInteractiveHelper::setEditMenuIndex(2);

    [m_mainMenu removeAllItems];

    NSMenuItem* appItem = [[NSMenuItem alloc] initWithTitle:@"MuseScore Studio" action:nil keyEquivalent:@""];
    [appItem setSubmenu:[[NSMenu alloc] initWithTitle:@"MuseScore Studio"]];
    [m_mainMenu addItem:appItem];

    NSMenuItem* fileItem = [[NSMenuItem alloc] initWithTitle:@"文件 (F)" action:nil keyEquivalent:@""];
    [fileItem setSubmenu:[[NSMenu alloc] initWithTitle:@"文件 (F)"]];
    [m_mainMenu addItem:fileItem];

    NSMenuItem* editItem = [[NSMenuItem alloc] initWithTitle:@"编辑 (E)" action:nil keyEquivalent:@""];
    NSMenu* editSub = [[NSMenu alloc] initWithTitle:@"编辑 (E)"];

    NSMenuItem* zhUndo = [[NSMenuItem alloc] initWithTitle:@"撤消“粘贴”" action:@selector(triggerAction:) keyEquivalent:@"z"];
    [zhUndo setTarget:m_mockTarget];
    [editSub addItem:zhUndo];

    NSMenuItem* zhRedo = [[NSMenuItem alloc] initWithTitle:@"恢复" action:@selector(triggerAction:) keyEquivalent:@"Z"];
    [zhRedo setTarget:m_mockTarget];
    [editSub addItem:zhRedo];

    NSMenuItem* zhHistory = [[NSMenuItem alloc] initWithTitle:@"历史 (H)" action:@selector(triggerAction:) keyEquivalent:@""];
    [zhHistory setTarget:m_mockTarget];
    [editSub addItem:zhHistory];

    [editSub addItem:[NSMenuItem separatorItem]];

    NSMenuItem* zhCut = [[NSMenuItem alloc] initWithTitle:@"剪切 (T)" action:@selector(triggerAction:) keyEquivalent:@"x"];
    [zhCut setTarget:m_mockTarget];
    [editSub addItem:zhCut];

    NSMenuItem* zhCopy = [[NSMenuItem alloc] initWithTitle:@"拷贝 (C)" action:@selector(triggerAction:) keyEquivalent:@"c"];
    [zhCopy setTarget:m_mockTarget];
    [editSub addItem:zhCopy];

    NSMenuItem* zhPaste = [[NSMenuItem alloc] initWithTitle:@"粘贴 (E)" action:@selector(triggerAction:) keyEquivalent:@"v"];
    [zhPaste setTarget:m_mockTarget];
    [editSub addItem:zhPaste];

    [editSub addItem:[NSMenuItem separatorItem]];

    NSMenuItem* zhSelectAll = [[NSMenuItem alloc] initWithTitle:@"全选 (A)" action:@selector(triggerAction:) keyEquivalent:@"a"];
    [zhSelectAll setTarget:m_mockTarget];
    [editSub addItem:zhSelectAll];

    [editItem setSubmenu:editSub];
    [m_mainMenu addItem:editItem];

    NSUInteger initialCount = [[editSub itemArray] count];

    {
        muse::MacOSInteractiveHelper::NativeDialogScope scope;

        // Verify zero temporary items were created in editSub
        EXPECT_EQ([[editSub itemArray] count], initialCount);

        // Verify existing Chinese items were modified in-place
        EXPECT_EQ([zhUndo target], nil);
        EXPECT_EQ([zhUndo action], @selector(undo:));
        EXPECT_STREQ([[zhUndo keyEquivalent] UTF8String], "z");
        EXPECT_TRUE([zhUndo isEnabled]);

        EXPECT_EQ([zhRedo target], nil);
        EXPECT_EQ([zhRedo action], @selector(redo:));
        EXPECT_STREQ([[zhRedo keyEquivalent] UTF8String], "Z");
        EXPECT_TRUE([zhRedo isEnabled]);

        EXPECT_EQ([zhCut target], nil);
        EXPECT_EQ([zhCut action], @selector(cut:));
        EXPECT_STREQ([[zhCut keyEquivalent] UTF8String], "x");
        EXPECT_TRUE([zhCut isEnabled]);

        EXPECT_EQ([zhCopy target], nil);
        EXPECT_EQ([zhCopy action], @selector(copy:));
        EXPECT_STREQ([[zhCopy keyEquivalent] UTF8String], "c");
        EXPECT_TRUE([zhCopy isEnabled]);

        EXPECT_EQ([zhPaste target], nil);
        EXPECT_EQ([zhPaste action], @selector(paste:));
        EXPECT_STREQ([[zhPaste keyEquivalent] UTF8String], "v");
        EXPECT_TRUE([zhPaste isEnabled]);

        EXPECT_EQ([zhSelectAll target], nil);
        EXPECT_EQ([zhSelectAll action], @selector(selectAll:));
        EXPECT_STREQ([[zhSelectAll keyEquivalent] UTF8String], "a");
        EXPECT_TRUE([zhSelectAll isEnabled]);

        // Unmodified items like 历史 (H) remain untouched
        EXPECT_EQ([zhHistory target], m_mockTarget);
    }

    // Post-scope: original state restored, item count identical
    EXPECT_EQ([[editSub itemArray] count], initialCount);
    EXPECT_EQ([zhUndo target], m_mockTarget);
    EXPECT_STREQ([[zhUndo keyEquivalent] UTF8String], "z");
    EXPECT_EQ([zhPaste target], m_mockTarget);
    EXPECT_STREQ([[zhPaste keyEquivalent] UTF8String], "v");
}

TEST_F(MacOSInteractiveHelperTest, NativeDialogScope_MaintainsTargetWhenEditMenuIsClickedAndTracked)
{
    [m_mainMenu removeAllItems];

    NSMenuItem* appItem = [[NSMenuItem alloc] initWithTitle:@"MuseScore Studio" action:nil keyEquivalent:@""];
    [appItem setSubmenu:[[NSMenu alloc] initWithTitle:@"MuseScore Studio"]];
    [m_mainMenu addItem:appItem];

    NSMenuItem* editItem = [[NSMenuItem alloc] initWithTitle:@"Edit" action:nil keyEquivalent:@""];
    NSMenu* editSub = [[NSMenu alloc] initWithTitle:@"Edit"];
    muse::MacOSInteractiveHelper::setEditMenuIndex(1);

    NSMenuItem* undo = [[NSMenuItem alloc] initWithTitle:@"Undo" action:@selector(triggerAction:) keyEquivalent:@""];
    [undo setTarget:m_mockTarget];
    [editSub addItem:undo];

    NSMenuItem* redo = [[NSMenuItem alloc] initWithTitle:@"Redo" action:@selector(triggerAction:) keyEquivalent:@""];
    [redo setTarget:m_mockTarget];
    [editSub addItem:redo];

    [editItem setSubmenu:editSub];
    [m_mainMenu addItem:editItem];

    {
        muse::MacOSInteractiveHelper::NativeDialogScope scope;

        EXPECT_EQ([undo target], nil);
        EXPECT_EQ([redo target], nil);

        // Simulate QML/Qt resetting the target when onAboutToShow executes
        [undo setTarget:m_mockTarget];
        [redo setTarget:m_mockTarget];
        [editSub setAutoenablesItems:YES];

        EXPECT_EQ([undo target], m_mockTarget);

        // Simulate user clicking the Edit menu -> AppKit posts NSMenuDidEndTrackingNotification when menu interaction completes
        [[NSNotificationCenter defaultCenter] postNotificationName:NSMenuDidEndTrackingNotification object:editSub];
        [[NSOperationQueue mainQueue] waitUntilAllOperationsAreFinished];

        // Verify observer automatically restored target = nil and autoenablesItems = NO
        EXPECT_EQ([undo target], nil);
        EXPECT_EQ([redo target], nil);
        EXPECT_FALSE([editSub autoenablesItems]);
    }

    EXPECT_EQ([undo target], m_mockTarget);
    EXPECT_EQ([redo target], m_mockTarget);
}

TEST_F(MacOSInteractiveHelperTest, NativeDialogScope_HandlesMenuRecreationDuringTracking)
{
    [m_mainMenu removeAllItems];

    NSMenuItem* appItem = [[NSMenuItem alloc] initWithTitle:@"MuseScore Studio" action:nil keyEquivalent:@""];
    [appItem setSubmenu:[[NSMenu alloc] initWithTitle:@"MuseScore Studio"]];
    [m_mainMenu addItem:appItem];

    NSMenuItem* editItem = [[NSMenuItem alloc] initWithTitle:@"Edit" action:nil keyEquivalent:@""];
    NSMenu* editSub = [[NSMenu alloc] initWithTitle:@"Edit"];
    muse::MacOSInteractiveHelper::setEditMenuIndex(1);

    NSMenuItem* undo1 = [[NSMenuItem alloc] initWithTitle:@"Undo" action:@selector(triggerAction:) keyEquivalent:@"z"];
    [undo1 setTarget:m_mockTarget];
    [editSub addItem:undo1];

    [editItem setSubmenu:editSub];
    [m_mainMenu addItem:editItem];

    NSMenuItem* undo2 = nil;

    {
        muse::MacOSInteractiveHelper::NativeDialogScope scope;
        EXPECT_EQ([undo1 target], nil);

        // Simulate Qt completely clearing the menu and adding brand new NSMenuItem objects during menu tracking
        [editSub removeAllItems];
        undo2 = [[NSMenuItem alloc] initWithTitle:@"Undo" action:@selector(triggerAction:) keyEquivalent:@"z"];
        [undo2 setTarget:m_mockTarget];
        [editSub addItem:undo2];
        [editSub setAutoenablesItems:YES];

        EXPECT_EQ([undo2 target], m_mockTarget);

        // End tracking notification fires when user closes menu
        [[NSNotificationCenter defaultCenter] postNotificationName:NSMenuDidEndTrackingNotification object:editSub];
        [[NSOperationQueue mainQueue] waitUntilAllOperationsAreFinished];

        // Observer must dynamically match undo2 and recreate missing fallbacks (Redo, Cut, Copy, Paste, Select All)
        EXPECT_EQ([undo2 target], nil);
        EXPECT_FALSE([editSub autoenablesItems]);
        EXPECT_EQ([[editSub itemArray] count], 6);

        for (NSUInteger i = 1; i < [[editSub itemArray] count]; ++i) {
            NSMenuItem* fallback = [editSub itemArray][i];
            EXPECT_EQ([fallback target], nil);
            EXPECT_TRUE([fallback isEnabled]);
            EXPECT_TRUE([fallback isHidden]);
        }
    }

    // On scope exit, fallbacks must be removed cleanly and undo2 restored to original target
    EXPECT_EQ([[editSub itemArray] count], 1);
    EXPECT_EQ([undo2 target], m_mockTarget);
}

TEST_F(MacOSInteractiveHelperTest, NativeDialogScope_HandlesSubmenuReplacementDuringTracking)
{
    [m_mainMenu removeAllItems];

    NSMenuItem* appItem = [[NSMenuItem alloc] initWithTitle:@"MuseScore Studio" action:nil keyEquivalent:@""];
    [appItem setSubmenu:[[NSMenu alloc] initWithTitle:@"MuseScore Studio"]];
    [m_mainMenu addItem:appItem];

    NSMenuItem* editItem = [[NSMenuItem alloc] initWithTitle:@"Edit" action:nil keyEquivalent:@""];
    NSMenu* oldEditSub = [[NSMenu alloc] initWithTitle:@"Edit"];
    muse::MacOSInteractiveHelper::setEditMenuIndex(1);

    NSMenuItem* undo1 = [[NSMenuItem alloc] initWithTitle:@"Undo" action:@selector(triggerAction:) keyEquivalent:@"z"];
    [undo1 setTarget:m_mockTarget];
    [oldEditSub addItem:undo1];

    [editItem setSubmenu:oldEditSub];
    [m_mainMenu addItem:editItem];

    NSMenuItem* undo2 = nil;
    NSMenu* newEditSub = nil;

    {
        muse::MacOSInteractiveHelper::NativeDialogScope scope;
        EXPECT_EQ([undo1 target], nil);

        // Simulate replacing the entire NSMenu instance of the Edit submenu
        newEditSub = [[NSMenu alloc] initWithTitle:@"Edit"];
        undo2 = [[NSMenuItem alloc] initWithTitle:@"Undo" action:@selector(triggerAction:) keyEquivalent:@"z"];
        [undo2 setTarget:m_mockTarget];
        [newEditSub addItem:undo2];
        [editItem setSubmenu:newEditSub];

        EXPECT_EQ([undo2 target], m_mockTarget);

        // Tracking notification fires -> applyScopeTransformations re-resolves the live submenu
        [[NSNotificationCenter defaultCenter] postNotificationName:NSMenuDidEndTrackingNotification object:newEditSub];
        [[NSOperationQueue mainQueue] waitUntilAllOperationsAreFinished];

        EXPECT_EQ([undo2 target], nil);
        EXPECT_FALSE([newEditSub autoenablesItems]);
        EXPECT_EQ([[newEditSub itemArray] count], 6);
    }

    // On scope exit, fallbacks removed and target restored
    EXPECT_EQ([[newEditSub itemArray] count], 1);
    EXPECT_EQ([undo2 target], m_mockTarget);
}
