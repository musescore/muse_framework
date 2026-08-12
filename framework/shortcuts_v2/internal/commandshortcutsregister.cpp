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
#include "commandshortcutsregister.h"

#include "containers.h"
#include "global/io/file.h"
#include "global/serialization/json.h"

#include "multiwindows/resourcelockguard.h"

using namespace muse;
using namespace muse::shortcuts;
using namespace muse::async;

static const std::string COMMAND_SHORTCUTS_TAG("CommandShortcuts");
static const std::string DEFAULT_SHORTCUTS_NAME("shortcuts");

namespace muse::shortcuts::command {
static const Shortcut& findShortcut(const ShortcutList& shortcuts, const std::string& command)
{
    for (const Shortcut& shortcut : shortcuts) {
        if (shortcut.command == command) {
            return shortcut;
        }
    }

    static Shortcut null;
    return null;
}
}

void CommandShortcutsRegister::init()
{
    multiwindowsProvider()->resourceChanged().onReceive(this, [this](const std::string& resourceName) {
        if (resourceName == COMMAND_SHORTCUTS_TAG) {
            reload();
        }
    });

    configuration()->currentShortcutsPresetNameChanged().onReceive(this, [this](const std::string&) {
        reload();
    });

    reload();
}

void CommandShortcutsRegister::reload(bool onlyDef)
{
    TRACEFUNC;

    m_shortcuts.clear();
    m_defaultShortcuts.clear();

    io::path_t defPath = configuration()->commandShortcutsAppDataPath(DEFAULT_SHORTCUTS_NAME);
    io::path_t userPath = userShortcutsPath();

    bool ok = readFromFile(m_defaultShortcuts, defPath);

    if (ok) {
        //! NOTE Platform and preset shortcuts files are diffs from the base one
        std::string defaultName = configuration()->defaultShortcutsName();
        if (defaultName != DEFAULT_SHORTCUTS_NAME) {
            applyShortcutsDiff(defaultName, m_defaultShortcuts);
        }

        std::string presetName = configuration()->currentShortcutsPresetName();
        if (!presetName.empty()) {
            applyShortcutsDiff(presetName, m_defaultShortcuts);
        }
    }

    if (ok) {
        if (!onlyDef) {
            if (!io::File::exists(userPath)) {
                ok = false;
            } else {
                //! NOTE The user shortcut file may change, so we need to lock it
                mi::ReadResourceLockGuard guard(multiwindowsProvider(), COMMAND_SHORTCUTS_TAG);
                ok = readFromFile(m_shortcuts, userPath);
            }
        } else {
            ok = false;
        }

        if (!ok) {
            m_shortcuts = m_defaultShortcuts;
        } else {
            mergeShortcuts(m_shortcuts, m_defaultShortcuts);
            mergeAdditionalShortcuts(m_shortcuts);
        }

        ok = true;
    }

    if (ok) {
        makeUnique(m_shortcuts);
        m_shortcutsChanged.notify();
    }
}

std::string CommandShortcutsRegister::activeShortcutsName() const
{
    std::string presetName = configuration()->currentShortcutsPresetName();
    return presetName.empty() ? configuration()->defaultShortcutsName() : presetName;
}

io::path_t CommandShortcutsRegister::userShortcutsPath() const
{
    return configuration()->commandShortcutsUserAppDataPath(activeShortcutsName());
}

void CommandShortcutsRegister::applyShortcutsDiff(const std::string& shortcutsName, ShortcutList& shortcuts) const
{
    ShortcutList diff;
    if (!readFromFile(diff, configuration()->commandShortcutsAppDataPath(shortcutsName))) {
        return;
    }

    mergeShortcuts(diff, shortcuts);
    shortcuts = diff;
}

//! NOTE Scope and autoRepeat always take default values on merge, so only sequences are compared
ShortcutList CommandShortcutsRegister::makeDiff(const ShortcutList& shortcuts, const ShortcutList& defaultShortcuts) const
{
    ShortcutList diff;

    for (const Shortcut& sc : shortcuts) {
        auto it = std::find_if(defaultShortcuts.begin(), defaultShortcuts.end(), [&sc](const Shortcut& defSc) {
            return defSc.command == sc.command;
        });

        if (it == defaultShortcuts.end() || it->sequences != sc.sequences) {
            diff.push_back(sc);
        }
    }

    return diff;
}

bool CommandShortcutsRegister::removeUserFile()
{
    io::path_t path = userShortcutsPath();
    if (path.empty()) {
        return false;
    }

    bool ok = false;
    {
        mi::WriteResourceLockGuard guard(multiwindowsProvider(), COMMAND_SHORTCUTS_TAG);
        ok = io::File::remove(path);
    }

    if (!ok) {
        LOGE() << "failed remove file: " << path;
    }

    return ok;
}

void CommandShortcutsRegister::mergeShortcuts(ShortcutList& shortcuts, const ShortcutList& defaultShortcuts) const
{
    TRACEFUNC;

    ShortcutList needadd;
    for (const Shortcut& defSc : defaultShortcuts) {
        Shortcut scForAdd = defSc;
        bool found = false;

        for (Shortcut& sc : shortcuts) {
            //! NOTE If a user shortcut is found, set scope & auto repeat (always use default values)
            if (sc.command == defSc.command) {
                sc.scope = defSc.scope;
                sc.autoRepeat = defSc.autoRepeat;
                found = true;
            } else if (sc.scope == defSc.scope) {
                for (const std::string& seq : sc.sequences) {
                    //! NOTE If user shortcut has sequence from default shortcut, remove the sequence from default shortcut
                    muse::remove_if(scForAdd.sequences, [&seq](const std::string& cmp){
                        return cmp == seq;
                    });
                }
            }
        }

        //! NOTE If no default shortcut is found in user shortcuts add def
        if (!found) {
            needadd.push_back(scForAdd);
        }
    }

    if (!needadd.empty()) {
        shortcuts.splice(shortcuts.end(), needadd);
    }
}

void CommandShortcutsRegister::mergeAdditionalShortcuts(ShortcutList& shortcuts)
{
    for (const auto& [context, additionalShortcuts] : m_additionalShortcutsMap) {
        mergeShortcuts(shortcuts, additionalShortcuts);
    }
}

void CommandShortcutsRegister::makeUnique(ShortcutList& shortcuts)
{
    TRACEFUNC;

    const ShortcutList all = shortcuts;

    shortcuts.clear();

    for (const Shortcut& sc : all) {
        const std::string& command = sc.command;

        auto it = std::find_if(shortcuts.begin(), shortcuts.end(), [command](const Shortcut& s) {
            return s.command == command;
        });

        if (it == shortcuts.end()) {
            shortcuts.push_back(sc);
            continue;
        }

        Shortcut& foundSc = *it;

        IF_ASSERT_FAILED(foundSc.scope == sc.scope) {
        }

        foundSc.sequences.insert(foundSc.sequences.end(), sc.sequences.begin(), sc.sequences.end());
    }
}

ShortcutList CommandShortcutsRegister::filterAndUpdateAdditionalShortcuts(const ShortcutList& shortcuts)
{
    ShortcutList noAdditionalShortcuts = shortcuts;

    for (auto& [context, additionalShortcuts] : m_additionalShortcutsMap) {
        for (Shortcut& shortcut : additionalShortcuts) {
            auto it = std::find(shortcuts.begin(), shortcuts.end(), shortcut.action);
            if (it != shortcuts.end()) {
                shortcut = *it;
                noAdditionalShortcuts.remove(shortcut);
            }
        }
    }

    return noAdditionalShortcuts;
}

bool CommandShortcutsRegister::readFromFile(ShortcutList& shortcuts, const io::path_t& path) const
{
    TRACEFUNC;

    ByteArray data;
    Ret ret = io::File::readFile(path, data);
    if (!ret) {
        LOGE() << "failed read file: " << path << ", err: " << ret.toString();
        return false;
    }

    JsonDocument doc = JsonDocument::fromJson(data);
    if (!doc.isObject()) {
        LOGE() << "failed parse json: " << path;
        return false;
    }

    JsonObject rootObj = doc.rootObject();

    for (const std::string& key : rootObj.keys()) {
        JsonValue scope = rootObj.value(key);
        if (!scope.isArray()) {
            LOGE() << "failed parse json: " << path << ", key: " << key;
            continue;
        }

        JsonArray arr = scope.toArray();
        for (size_t i = 0; i < arr.size(); ++i) {
            JsonValue value = arr.at(i);
            if (!value.isObject()) {
                LOGE() << "failed parse json: " << path << ", key: " << key << ", i: " << i;
                continue;
            }

            JsonObject obj = value.toObject();
            Shortcut shortcut;
            shortcut.scope = key;
            shortcut.command = obj.value("command").toString().toStdString();
            shortcut.autoRepeat = obj.value("autoRepeat").toBool();

            JsonValue sequences = obj.value("sequences");
            if (!sequences.isArray()) {
                LOGE() << "failed parse json: " << path << ", key: " << key << ", i: " << i;
                continue;
            }

            JsonArray sequencesArr = sequences.toArray();
            for (size_t j = 0; j < sequencesArr.size(); ++j) {
                JsonValue sequence = sequencesArr.at(j);
                if (!sequence.isString()) {
                    LOGE() << "failed parse json: " << path << ", key: " << key << ", i: " << i << ", j: " << j;
                    continue;
                }
                shortcut.sequences.push_back(sequence.toString().toStdString());
            }

            shortcuts.push_back(shortcut);
        }
    }

    return true;
}

bool CommandShortcutsRegister::writeToFile(const ShortcutList& shortcuts, const io::path_t& path) const
{
    TRACEFUNC;

    if (path.empty()) {
        LOGE() << "failed write shortcuts: empty path";
        return false;
    }

    JsonObject root;
    for (const Shortcut& shortcut : shortcuts) {
        JsonObject shortcutObj;
        shortcutObj["command"] = shortcut.command;
        shortcutObj["autoRepeat"] = shortcut.autoRepeat;
        JsonArray sequencesArr;
        for (const std::string& sequence : shortcut.sequences) {
            sequencesArr.append(JsonValue(sequence));
        }
        shortcutObj["sequences"] = sequencesArr;

        JsonValue scopeValue = root.value(shortcut.scope);
        if (!scopeValue.isArray()) {
            scopeValue = JsonArray();
        }
        JsonArray scopeValueArr = scopeValue.toArray();
        scopeValueArr.append(shortcutObj);
        root[shortcut.scope] = scopeValueArr;
    }

    ByteArray data = JsonDocument(root).toJson();

    Ret ret;
    {
        mi::WriteResourceLockGuard guard(multiwindowsProvider(), COMMAND_SHORTCUTS_TAG);
        ret = io::File::writeFile(path, data);
    }

    LOGD() << "write shortcuts to file: " << path;

    if (!ret) {
        LOGE() << ret.toString();
    }

    return ret;
}

const ShortcutList& CommandShortcutsRegister::shortcuts() const
{
    return m_shortcuts;
}

Ret CommandShortcutsRegister::setShortcuts(const ShortcutList& shortcuts)
{
    TRACEFUNC;

    if (shortcuts == m_shortcuts) {
        return true;
    }

    ShortcutList needToWrite = filterAndUpdateAdditionalShortcuts(shortcuts);
    ShortcutList diff = makeDiff(needToWrite, m_defaultShortcuts);

    bool ok = false;
    if (diff.empty()) {
        ok = removeUserFile();
    } else {
        ok = writeToFile(diff, userShortcutsPath());
    }

    if (ok) {
        m_shortcuts = diff;
        mergeShortcuts(m_shortcuts, m_defaultShortcuts);
        mergeAdditionalShortcuts(m_shortcuts);
        m_shortcutsChanged.notify();
    }

    return ok;
}

void CommandShortcutsRegister::resetShortcuts()
{
    //! NOTE Even if the removal failed, reload to keep the state in sync with the file
    removeUserFile();
    reload();
}

Notification CommandShortcutsRegister::shortcutsChanged() const
{
    return m_shortcutsChanged;
}

Ret CommandShortcutsRegister::setAdditionalShortcuts(const std::string& context, const ShortcutList& shortcuts)
{
    m_additionalShortcutsMap[context] = shortcuts;

    mergeShortcuts(m_shortcuts, m_additionalShortcutsMap[context]);
    m_shortcutsChanged.notify();

    return muse::make_ok();
}

ShortcutList CommandShortcutsRegister::shortcutsForSequence(const std::string& sequence) const
{
    ShortcutList list;
    for (const Shortcut& sh : m_shortcuts) {
        auto it = std::find(sh.sequences.cbegin(), sh.sequences.cend(), sequence);
        if (it != sh.sequences.cend()) {
            list.push_back(sh);
        }
    }
    return list;
}

const Shortcut& CommandShortcutsRegister::defaultShortcut(const std::string& command) const
{
    return command::findShortcut(m_defaultShortcuts, command);
}

Ret CommandShortcutsRegister::importFromFile(const io::path_t& filePath)
{
    io::path_t userPath = userShortcutsPath();
    if (userPath.empty()) {
        LOGE() << "failed import file: invalid user shortcuts path";
        return make_ret(Ret::Code::UnknownError);
    }

    {
        mi::WriteResourceLockGuard guard(multiwindowsProvider(), COMMAND_SHORTCUTS_TAG);
        Ret ret = io::File::copy(filePath, userPath, true);
        if (!ret) {
            LOGE() << "failed import file: " << ret.toString();
            return ret;
        }
    }

    reload();

    return make_ret(Ret::Code::Ok);
}

Ret CommandShortcutsRegister::exportToFile(const io::path_t& filePath) const
{
    return writeToFile(m_shortcuts, filePath);
}

std::vector<std::string> CommandShortcutsRegister::availablePresets() const
{
    return configuration()->availableShortcutsPresets();
}

std::string CommandShortcutsRegister::currentPresetName() const
{
    return configuration()->currentShortcutsPresetName();
}

void CommandShortcutsRegister::setCurrentPresetName(const std::string& presetName)
{
    configuration()->setCurrentShortcutsPresetName(presetName);
}

async::Channel<std::string> CommandShortcutsRegister::currentPresetNameChanged() const
{
    return configuration()->currentShortcutsPresetNameChanged();
}

bool CommandShortcutsRegister::isPresetEdited(const std::string& presetName) const
{
    std::string name = presetName.empty() ? configuration()->defaultShortcutsName() : presetName;
    return io::File::exists(configuration()->commandShortcutsUserAppDataPath(name));
}

bool CommandShortcutsRegister::canDeletePreset(const std::string& presetName) const
{
    if (presetName.empty()) {
        return false;
    }

    //! NOTE Built-in presets cannot be deleted
    return !muse::contains(configuration()->availableShortcutsPresets(), presetName);
}

void CommandShortcutsRegister::deletePreset(const std::string& presetName)
{
    if (!canDeletePreset(presetName)) {
        return;
    }

    io::path_t path = configuration()->commandShortcutsUserAppDataPath(presetName);
    if (path.empty()) {
        return;
    }

    bool ok = false;
    {
        mi::WriteResourceLockGuard guard(multiwindowsProvider(), COMMAND_SHORTCUTS_TAG);
        ok = io::File::remove(path);
    }

    if (!ok) {
        LOGE() << "failed remove file: " << path;
        return;
    }

    if (currentPresetName() == presetName) {
        setCurrentPresetName(std::string());
    }
}
