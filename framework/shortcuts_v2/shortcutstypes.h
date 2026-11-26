/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
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

#pragma once

#include <algorithm>
#include <set>
#include <string>
#include <vector>
#include <utility>
#include <vector>

#include <QKeySequence>

#include "global/stringutils.h"
#include "rcommand/commandtypes.h"

#include "global/logstream.h"

namespace muse::shortcuts {
struct Shortcut
{
    rcommand::Command command;
    std::string scope;
    std::vector<std::string> sequences;
    QKeySequence::StandardKey standardKey = QKeySequence::UnknownKey;
    bool autoRepeat = true;
    bool modifiersOnly = false; // held rather than pressed: its sequences are modifier keys alone, e.g. "Ctrl+Alt+Shift"

    Shortcut() = default;

    bool isValid() const
    {
        return command.isValid();
    }

    bool operator ==(const Shortcut& sc) const
    {
        return command == sc.command
               && scope == sc.scope
               && sequences == sc.sequences
               && standardKey == sc.standardKey
               && autoRepeat == sc.autoRepeat
               && modifiersOnly == sc.modifiersOnly;
    }

    std::string sequencesAsString() const { return sequencesToString(sequences); }

    static std::string sequencesToString(const std::vector<std::string>& seqs)
    {
        return muse::strings::join(seqs, ", ");
    }

    static std::vector<std::string> sequencesFromString(const std::string& str)
    {
        std::vector<std::string> seqs;
        muse::strings::split(str, seqs, ", ");
        return seqs;
    }

    void clear()
    {
        sequences.clear();
        standardKey = QKeySequence::StandardKey::UnknownKey;
    }
};

using ShortcutList = std::vector<Shortcut>;

inline bool needIgnoreKey(Qt::Key key)
{
    static const std::set<Qt::Key> ignoredKeys {
        Qt::Key_Shift,
        Qt::Key_Control,
        Qt::Key_Meta,
        Qt::Key_Alt,
        Qt::Key_AltGr,
        Qt::Key_CapsLock,
        Qt::Key_NumLock,
        Qt::Key_ScrollLock,
        Qt::Key_unknown
    };

    return ignoredKeys.find(key) != ignoredKeys.end();
}

inline std::pair<Qt::Key, Qt::KeyboardModifiers> correctKeyInput(Qt::Key key, Qt::KeyboardModifiers modifiers)
{
    // replace Backtab with Shift+Tab
    if (key == Qt::Key_Backtab && modifiers == Qt::ShiftModifier) {
        key = Qt::Key_Tab;
    }

    modifiers &= ~Qt::KeypadModifier;

    return { key, modifiers };
}

//! The modifier a key is (Meta for the Super/Windows keys), or NoModifier if it isn't one
inline Qt::KeyboardModifier modifierOfKey(int key)
{
    switch (key) {
    case Qt::Key_Control: return Qt::ControlModifier;
    case Qt::Key_Alt: return Qt::AltModifier;
    case Qt::Key_Shift: return Qt::ShiftModifier;
    case Qt::Key_Meta:
    case Qt::Key_Super_L:
    case Qt::Key_Super_R: return Qt::MetaModifier;
    default: return Qt::NoModifier;
    }
}

//! The modifiers a modifiers-only shortcut can use, in the order its sequence lists them
inline const std::vector<std::pair<Qt::KeyboardModifier, std::string> >& shortcutModifierNames()
{
    static const std::vector<std::pair<Qt::KeyboardModifier, std::string> > names {
        { Qt::ControlModifier, "Ctrl" },
        { Qt::AltModifier, "Alt" },
        { Qt::ShiftModifier, "Shift" },
        { Qt::MetaModifier, "Meta" },
    };

    return names;
}

//! A modifiers-only sequence, e.g. "Ctrl+Alt+Shift"; empty for no modifiers
inline std::string modifiersToSequence(Qt::KeyboardModifiers modifiers)
{
    std::vector<std::string> parts;

    for (const auto& [modifier, name] : shortcutModifierNames()) {
        if (modifiers.testFlag(modifier)) {
            parts.push_back(name);
        }
    }

    return muse::strings::join(parts, "+");
}

//! The modifiers of a modifiers-only sequence; NoModifier if any part of it isn't a modifier
inline Qt::KeyboardModifiers modifiersFromSequence(const std::string& sequence)
{
    std::vector<std::string> parts;
    muse::strings::split(sequence, parts, "+");

    Qt::KeyboardModifiers modifiers;

    for (const std::string& part : parts) {
        const auto& names = shortcutModifierNames();
        const auto it = std::find_if(names.begin(), names.end(), [&part](const auto& entry) {
            return entry.second == part;
        });

        if (it == names.end()) {
            return Qt::NoModifier;
        }

        modifiers |= it->first;
    }

    return modifiers;
}

//! How the platform writes the modifiers, e.g. "Ctrl+Alt+Shift", or "⌥⇧⌘" on macOS
inline QString modifiersToNativeText(Qt::KeyboardModifiers modifiers)
{
    if (modifiers == Qt::NoModifier) {
        return QString();
    }

    // Qt only writes modifiers before a key: write them before "A", then take it off
    QString text = QKeySequence(QKeyCombination(modifiers, Qt::Key_A)).toString(QKeySequence::NativeText);
    text.chop(1);

    if (text.endsWith(u'+')) {
        text.chop(1);
    }

    return text;
}

inline QString sequencesToNativeText(const std::vector<std::string>& sequences)
{
    std::vector<std::string> seqs;

    for (const std::string& sequence : sequences) {
        if (const Qt::KeyboardModifiers modifiers = modifiersFromSequence(sequence); modifiers != Qt::NoModifier) {
            seqs.push_back(modifiersToNativeText(modifiers).toStdString());
            continue;
        }

        seqs.push_back(QKeySequence(QString::fromStdString(sequence)).toString(QKeySequence::NativeText).toStdString());
    }

    return QString::fromStdString(Shortcut::sequencesToString(seqs));
}

inline bool canShortcutsConflict(const std::string& scope1, const std::string& scope2)
{
    return scope1 == scope2;
}
}

inline muse::logger::Stream& operator<<(muse::logger::Stream& s, const muse::shortcuts::Shortcut& sc)
{
    s << "{" << sc.scope << ", " << sc.command.toString() << ", `" << sc.sequencesAsString() << "`}";
    return s;
}

inline muse::logger::Stream& operator<<(muse::logger::Stream& s, const std::vector<muse::shortcuts::Shortcut>& v)
{
    s << '[';
    for (size_t i = 0; i < v.size(); ++i) {
        s << v.at(i);
        if (i < v.size() - 1) {
            s << ',';
        }
    }
    s << ']';
    return s;
}
