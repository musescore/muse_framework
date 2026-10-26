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
#include "macosshortcutsinstancemodel.h"

#import <Carbon/Carbon.h>

#include <QGuiApplication>
#include <QKeyEvent>

#include "log.h"

using namespace muse::shortcuts;

static const std::map<UInt32, QString> specialKeysMap = {
    { kVK_F1, "F1" },
    { kVK_F2, "F2" },
    { kVK_F3, "F3" },
    { kVK_F4, "F4" },
    { kVK_F5, "F5" },
    { kVK_F6, "F6" },
    { kVK_F7, "F7" },
    { kVK_F8, "F8" },
    { kVK_F9, "F9" },
    { kVK_F10, "F10" },
    { kVK_F11, "F11" },
    { kVK_F12, "F12" },
    { kVK_F13, "F13" },
    { kVK_F14, "F14" },
    { kVK_F15, "F15" },
    { kVK_F16, "F16" },
    { kVK_F17, "F17" },
    { kVK_F18, "F18" },
    { kVK_F19, "F19" },
    { kVK_F20, "F20" },
    { kVK_Space, "Space" },
    { kVK_Escape, "Esc" },
    { kVK_Delete, "Backspace" },
    { kVK_ForwardDelete, "Delete" },
    { kVK_LeftArrow, "Left" },
    { kVK_RightArrow, "Right" },
    { kVK_UpArrow, "Up" },
    { kVK_DownArrow, "Down" },
    { kVK_Help, "" },
    { kVK_PageUp, "PgUp" },
    { kVK_PageDown, "PgDown" },
    { kVK_Tab, "Tab" },
    { kVK_Return, "Return" },
    { kVK_Home, "Home" },
    { kVK_End, "End" },
    { kVK_ANSI_Keypad0, "0" },
    { kVK_ANSI_Keypad1, "1" },
    { kVK_ANSI_Keypad2, "2" },
    { kVK_ANSI_Keypad3, "3" },
    { kVK_ANSI_Keypad4, "4" },
    { kVK_ANSI_Keypad5, "5" },
    { kVK_ANSI_Keypad6, "6" },
    { kVK_ANSI_Keypad7, "7" },
    { kVK_ANSI_Keypad8, "8" },
    { kVK_ANSI_Keypad9, "9" },
    { kVK_ANSI_KeypadDecimal, "." },
    { kVK_ANSI_KeypadMultiply, "*" },
    { kVK_ANSI_KeypadPlus, "+" },
    { kVK_ANSI_KeypadClear, "Clear" },
    { kVK_ANSI_KeypadDivide, "/" },
    { kVK_ANSI_KeypadEnter, "Enter" },
    { kVK_ANSI_KeypadMinus, "-" },
    { kVK_ANSI_KeypadEquals, "=" }
};

//! NOTE Returns the current keyboard layout if inputSourceId is empty.
//! The returned input source owns the layout data and must be released by the caller
static TISInputSourceRef copyKeyboardLayoutInputSource(const QString& inputSourceId)
{
    static TISInputSourceRef (* TISCopyCurrentKeyboardLayoutInputSource)(void);
    static CFArrayRef (* TISCreateInputSourceList)(CFDictionaryRef properties, Boolean includeAllInstalled);

    CFBundleRef bundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.Carbon"));

    if (bundle) {
        *(void**)& TISCopyCurrentKeyboardLayoutInputSource
            = CFBundleGetFunctionPointerForName(bundle, CFSTR("TISCopyCurrentKeyboardLayoutInputSource"));
        *(void**)& TISCreateInputSourceList = CFBundleGetFunctionPointerForName(bundle, CFSTR("TISCreateInputSourceList"));
    }

    if (!TISCopyCurrentKeyboardLayoutInputSource || !TISCreateInputSourceList) {
        LOGE() << "Error getting functions from Carbon framework";
        return nullptr;
    }

    if (inputSourceId.isEmpty()) {
        return TISCopyCurrentKeyboardLayoutInputSource();
    }

    CFStringRef id = inputSourceId.toCFString();
    const void* keys[] = { CFSTR("TISPropertyInputSourceID") };
    const void* values[] = { id };
    CFDictionaryRef properties = CFDictionaryCreate(kCFAllocatorDefault, keys, values, 1,
                                                    &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    CFArrayRef sources = TISCreateInputSourceList(properties, true);

    CFRelease(properties);
    CFRelease(id);

    TISInputSourceRef source = nullptr;
    if (sources) {
        if (CFArrayGetCount(sources) > 0) {
            source = (TISInputSourceRef)CFRetain(CFArrayGetValueAtIndex(sources, 0));
        }
        CFRelease(sources);
    }

    return source;
}

static UCKeyboardLayout* keyboardLayout(TISInputSourceRef inputSource)
{
    static void*(* TISGetInputSourceProperty)(TISInputSourceRef inputSource, CFStringRef propertyKey);

    CFBundleRef bundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.Carbon"));

    if (bundle) {
        *(void**)& TISGetInputSourceProperty = CFBundleGetFunctionPointerForName(bundle, CFSTR("TISGetInputSourceProperty"));
    }

    if (!TISGetInputSourceProperty) {
        LOGE() << "Error getting functions from Carbon framework";
        return nullptr;
    }

    CFDataRef uchr = (CFDataRef)TISGetInputSourceProperty(inputSource, CFSTR("TISPropertyUnicodeKeyLayoutData"));
    if (!uchr) {
        return nullptr;
    }

    return (UCKeyboardLayout*)CFDataGetBytePtr(uchr);
}

static UInt32 keyboardType()
{
    static UInt8 (* LMGetKbdType)(void);

    CFBundleRef bundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.Carbon"));

    if (bundle) {
        *(void**)& LMGetKbdType = CFBundleGetFunctionPointerForName(bundle, CFSTR("LMGetKbdType"));
    }

    if (!LMGetKbdType) {
        LOGE() << "Error getting functions from Carbon framework";
        return 0;
    }

    return LMGetKbdType();
}

static UInt32 nativeKeycode(UCKeyboardLayout* keyboard, Qt::Key keyCode, bool& found)
{
    found = true;

    switch (keyCode) {
    case Qt::Key_Return:
        return kVK_Return;
    case Qt::Key_Enter:
        return kVK_ANSI_KeypadEnter;
    case Qt::Key_Tab:
        return kVK_Tab;
    case Qt::Key_Space:
        return kVK_Space;
    case Qt::Key_Backspace:
        return kVK_Delete;
    case Qt::Key_Escape:
        return kVK_Escape;
    case Qt::Key_CapsLock:
        return kVK_CapsLock;
    case Qt::Key_Option:
        return kVK_Option;
    case Qt::Key_F17:
        return kVK_F17;
    case Qt::Key_VolumeUp:
        return kVK_VolumeUp;
    case Qt::Key_VolumeDown:
        return kVK_VolumeDown;
    case Qt::Key_F18:
        return kVK_F18;
    case Qt::Key_F19:
        return kVK_F19;
    case Qt::Key_F20:
        return kVK_F20;
    case Qt::Key_F5:
        return kVK_F5;
    case Qt::Key_F6:
        return kVK_F6;
    case Qt::Key_F7:
        return kVK_F7;
    case Qt::Key_F3:
        return kVK_F3;
    case Qt::Key_F8:
        return kVK_F8;
    case Qt::Key_F9:
        return kVK_F9;
    case Qt::Key_F11:
        return kVK_F11;
    case Qt::Key_F13:
        return kVK_F13;
    case Qt::Key_F16:
        return kVK_F16;
    case Qt::Key_F14:
        return kVK_F14;
    case Qt::Key_F10:
        return kVK_F10;
    case Qt::Key_F12:
        return kVK_F12;
    case Qt::Key_F15:
        return kVK_F15;
    case Qt::Key_Help:
        return kVK_Help;
    case Qt::Key_Home:
        return kVK_Home;
    case Qt::Key_PageUp:
        return kVK_PageUp;
    case Qt::Key_Delete:
        return kVK_ForwardDelete;
    case Qt::Key_F4:
        return kVK_F4;
    case Qt::Key_End:
        return kVK_End;
    case Qt::Key_F2:
        return kVK_F2;
    case Qt::Key_PageDown:
        return kVK_PageDown;
    case Qt::Key_F1:
        return kVK_F1;
    case Qt::Key_Left:
        return kVK_LeftArrow;
    case Qt::Key_Right:
        return kVK_RightArrow;
    case Qt::Key_Down:
        return kVK_DownArrow;
    case Qt::Key_Up:
        return kVK_UpArrow;
    default:
        break;
    }

    if (keyCode < 0 || keyCode > 0xFFFF) {
        LOGD() << "Unhandled key code: " << keyCode;
        found = false;
        return 0;
    }

    const UTF16Char keyCodeChar = keyCode;
    // Letters are matched regardless of case: Qt::Key_A is `A`, while the unmodified key produces `a`
    const UTF16Char keyCodeCharLower = QChar(keyCodeChar).toLower().unicode();

    auto matches = [keyCodeChar, keyCodeCharLower](UTF16Char ch) {
        return ch == keyCodeChar || ch == keyCodeCharLower;
    };

    uint8_t* data = (uint8_t*)keyboard;

    // Use the same keyboard type that UCKeyTranslate (see keyCodeToString) will use
    const UInt32 kbdType = keyboardType();
    const UCKeyboardTypeHeader* header = &keyboard->keyboardTypeList[0];
    for (UInt32 i = 0; i < keyboard->keyboardTypeCount; i++) {
        if (keyboard->keyboardTypeList[i].keyboardTypeFirst <= kbdType && kbdType <= keyboard->keyboardTypeList[i].keyboardTypeLast) {
            header = &keyboard->keyboardTypeList[i];
            break;
        }
    }

    UCKeyStateRecordsIndex* stateRec = 0;
    if (header->keyStateRecordsIndexOffset != 0) {
        stateRec = reinterpret_cast<UCKeyStateRecordsIndex*>(data + header->keyStateRecordsIndexOffset);
        if (stateRec->keyStateRecordsIndexFormat != kUCKeyStateRecordsIndexFormat) {
            stateRec = 0;
        }
    }

    UCKeyToCharTableIndex* charTable = reinterpret_cast<UCKeyToCharTableIndex*>(data + header->keyToCharTableIndexOffset);
    if (charTable->keyToCharTableIndexFormat != kUCKeyToCharTableIndexFormat) {
        found = false;
        return 0;
    }

    // Only search the tables for the unmodified state and for Command (the latter holds e.g. the Latin characters
    // of non-Latin layouts). keyCodeToString translates the key without modifiers, so a character that requires
    // e.g. Shift (`:` is `Shift+;` on QWERTY) would be translated to a different character (`;`) and clash with
    // the shortcuts for that character. The tables for other modifiers can't be used either: e.g. the Control table
    // may have a QWERTY-like layout, and Command+CapsLock may select the Shift table
    std::vector<bool> allowedTables(charTable->keyToCharTableCount, false);
    const UCKeyModifiersToTableNum* modifiersToTable
        = reinterpret_cast<const UCKeyModifiersToTableNum*>(data + header->keyModifiersToTableNumOffset);
    if (modifiersToTable->keyModifiersToTableNumFormat == kUCKeyModifiersToTableNumFormat) {
        for (UInt32 modifiers : { UInt32(0), UInt32((cmdKey >> 8) & 0xFF) }) {
            const UInt32 tableNum = modifiers < modifiersToTable->modifiersCount
                                    ? modifiersToTable->tableNum[modifiers] : modifiersToTable->defaultTableNum;
            if (tableNum < allowedTables.size()) {
                allowedTables[tableNum] = true;
            }
        }
    } else if (!allowedTables.empty()) {
        allowedTables[0] = true;
    }

    for (UInt32 j = 0; j < charTable->keyToCharTableCount; j++) {
        if (!allowedTables[j]) {
            continue;
        }

        UCKeyOutput* keyToChar = reinterpret_cast<UCKeyOutput*>(data + charTable->keyToCharTableOffsets[j]);
        for (UInt32 k = 0; k < charTable->keyToCharTableSize; k++) {
            if (keyToChar[k] & kUCKeyOutputTestForIndexMask) {
                long idx = keyToChar[k] & kUCKeyOutputGetIndexMask;
                if (stateRec && idx < stateRec->keyStateRecordCount) {
                    UCKeyStateRecord* rec = reinterpret_cast<UCKeyStateRecord*>(data + stateRec->keyStateRecordOffsets[idx]);
                    if (matches(rec->stateZeroCharData)) {
                        return k;
                    }
                }
            } else if (!(keyToChar[k] & kUCKeyOutputSequenceIndexMask) && keyToChar[k] < 0xFFFE) {
                if (matches(keyToChar[k])) {
                    return k;
                }
            }
        }
    }

    found = false;
    return 0;
}

static QString keyCodeToString(UCKeyboardLayout* keyboard, UInt32 keyNativeCode)
{
    if (muse::contains(specialKeysMap, keyNativeCode)) {
        return specialKeysMap.at(keyNativeCode);
    }

    UInt32 deadKeyState = 0;
    UniCharCount actualLength = 0;
    const int maxLength = 4;

    UniChar actualString[maxLength] = { 0 };

    OSStatus error = UCKeyTranslate(keyboard,
                                    UInt16(keyNativeCode),
                                    UInt16(kUCKeyActionDisplay),
                                    UInt32((0 >> 8) & 0xFF),
                                    keyboardType(),
                                    OptionBits(kUCKeyTranslateNoDeadKeysBit),
                                    &deadKeyState,
                                    maxLength,
                                    &actualLength,
                                    actualString);
    if (error == 0) {
        NSString* nsString = [NSString stringWithCharacters:actualString length:(NSUInteger)actualLength];
        return QString::fromNSString(nsString).toUpper();
    }

    return "";
}

static QKeySequence translateToKeyboardLayout(UCKeyboardLayout* keyboard, const QKeySequence& sequence)
{
    if (!keyboard) {
        return {};
    }

    const QKeyCombination keyCombination = sequence[0];

    const Qt::Key qKey = keyCombination.key();

    bool found = false;
    UInt32 keyNativeCode = nativeKeycode(keyboard, qKey, found);
    if (!found) {
        LOGD() << "Key " << qKey << " not found in the keyboard layout";
        return {};
    }

    QString keyStr = keyCodeToString(keyboard, keyNativeCode);
    Qt::Key translatedQKey = QKeySequence::fromString(keyStr, QKeySequence::PortableText)[0].key();

    return QKeyCombination(keyCombination.keyboardModifiers(), translatedQKey);
}

MacOSShortcutsInstanceModel::MacOSShortcutsInstanceModel(QObject* parent)
    : ShortcutsInstanceModel(parent)
{
    connect(qApp->inputMethod(), &QInputMethod::localeChanged, this, [this]() {
        doLoadShortcuts();
    });
}

void MacOSShortcutsInstanceModel::doLoadShortcuts()
{
    m_shortcuts.clear();

    // RULE: If a sequence is used for several shortcuts but the values for autoRepeat vary depending on
    // the context, then we should force autoRepeat to false for all shortcuts sharing the sequence in
    // question. This prevents the creation of ambiguous shortcuts (see QShortcutEvent::isAmbiguous)
    auto recordAutoRepeat = [this](const QString& sequence, bool autoRepeat) {
        auto search = m_shortcuts.find(sequence);
        if (search == m_shortcuts.end()) {
            // Sequence not found, add it...
            m_shortcuts.insert(sequence, QVariant(autoRepeat));
        } else if (search.value().toBool() && !autoRepeat) {
            // Sequence already exists, but we need to enforce the above rule...
            search.value() = false;
        }
    };

    m_macSequenceMap.clear();

    // Suppose the user uses for example a Cyrillic keyboard layout. When they try to press Cmd+A,
    // Qt will see Cmd+Ф instead, so the QML `Shortcut` object for Cmd+A will not be triggered.
    // We will a workaround: we treat the shortcuts from the shortcuts file as "a combination of
    // physical keys" rather than "a character that would be typed"; i.e. `Shift+,` rather than `<`.
    // We use native macOS APIs to translate from the combination of keys to the character that would
    // be typed. We create a QML `Shortcut` object for e.g. `Cmd+Ф`; that will be triggered by Qt.
    // We store a reverse mapping here, so that we can look up the original sequence when the shortcut
    // is triggered, and then trigger the corresponding action.
    //
    // There is one catch; there are two cases where the untranslated sequence is not a combination of
    // physical keys:
    // - When the current keyboard layout is different from what the shortcuts file was designed for;
    //   for example, the AZERTY layout does not have a `.` key, but instead a `.` is typed as `Shift+;`.
    // - When the sequence was recorded as a character, e.g. `>` rather than `Shift+.`, which is the case
    //   for all custom shortcuts.
    // In these cases, the translation will produce unexpected results. Therefore, in our reverse map,
    // we also store a mapping from the untranslated sequence to the untranslated sequence itself. This
    // takes precedence over mapping from translated sequences to untranslated sequences.
    //
    // The resulting behaviour when a shortcut is pressed by the user is the following:
    // - If there is an action that is bound to exactly the sequence as Qt sees it, then that action is
    //   triggered.
    // - Otherwise, we look up the received sequence in the reverse map, to see by which combination of
    //   physical keys it would be produced, and trigger the action that is bound to that combination.
    auto recordMapping = [this](const QString& translatedSequence, const QString& rawSequence, bool overrideExisting) {
        auto search = m_macSequenceMap.find(translatedSequence);
        if (search == m_macSequenceMap.end()) {
            m_macSequenceMap.insert(translatedSequence, rawSequence);
        } else if (overrideExisting) {
            search.value() = rawSequence;
        }
    };

    TISInputSourceRef inputSource = copyKeyboardLayoutInputSource(keyboardLayoutId());
    UCKeyboardLayout* keyboard = inputSource ? keyboardLayout(inputSource) : nullptr;
    if (!keyboard) {
        LOGE() << "The keyboard layout is not valid";
    }

    ShortcutList shortcuts = commandShortcutsRegister()->shortcuts();

    for (const Shortcut& sc : shortcuts) {
        for (const std::string& seq : sc.sequences) {
            QString untranslatedSequenceStr = QString::fromStdString(seq);

            // Ensure standard order of modifiers by converting to/from QKeySequence
            QKeySequence untranslatedSequence = QKeySequence::fromString(untranslatedSequenceStr, QKeySequence::PortableText);

            // Attempt to translate from combination of keys to character, e.g., `Shift+.` becomes `>`, in the case of a QWERTY layout
            QKeySequence translatedSequence
                = translateToKeyboardLayout(keyboard, untranslatedSequence);
            if (translatedSequence.isEmpty() || !(untranslatedSequence[0].key() & 0xff) || untranslatedSequence[0].key() == Qt::Key_A) {
                QString untranslatedSequenceStrNormalised = untranslatedSequence.toString(QKeySequence::PortableText);

                // Record the untranslated sequence
                // Map to non-normalised, because that's what ShortcutsInstanceModel::doActivate expects
                recordMapping(untranslatedSequenceStrNormalised, untranslatedSequenceStr, true);
                recordAutoRepeat(untranslatedSequenceStrNormalised, sc.autoRepeat);
            }

            if (translatedSequence.isEmpty()) {
                LOGD() << "Failed to translate sequence " << untranslatedSequenceStr;
            } else {
                QString translatedSequenceStrNormalised = translatedSequence.toString(QKeySequence::PortableText);

                // If it was successful, record the translated sequence too, and map it to the untranslated sequence
                // Again, map to non-normalised
                recordMapping(translatedSequenceStrNormalised, untranslatedSequenceStr, false);
                recordAutoRepeat(translatedSequenceStrNormalised, sc.autoRepeat);
            }
        }
    }

    if (inputSource) {
        CFRelease(inputSource);
    }

    assert(m_shortcuts.size() == m_macSequenceMap.size());

    emit shortcutsChanged();
}

QString MacOSShortcutsInstanceModel::keyboardLayoutId() const
{
    return QString();
}

void MacOSShortcutsInstanceModel::doActivate(const QString& seq)
{
    // `seq` will always be in the map, because it comes from one of the QML `Shortcut` objects
    // and for every such object, we have also created an entry in the map, in `doLoadShortcuts`.
    ShortcutsInstanceModel::doActivate(m_macSequenceMap.value(seq));
}
