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

#include <QAbstractListModel>
#include <QItemSelection>
#include <qqmlintegration.h>
#include <QList>

#include "modularity/ioc.h"
#include "ishortcutsregister.h"
#include "icommandshortcutsregister.h"
#include "ui/iuiactionsregister.h"
#include "async/asyncable.h"
#include "interactive/iinteractive.h"
#include "iglobalconfiguration.h"
#include "rcommand/icommandsregister.h"

class QItemSelection;

namespace muse::shortcuts {
class ShortcutsModel : public QAbstractListModel, public Contextable, public async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(QItemSelection selection READ selection WRITE setSelection NOTIFY selectionChanged)
    Q_PROPERTY(QVariant currentShortcut READ currentShortcut NOTIFY selectionChanged)

    Q_PROPERTY(QVariantList presets READ presets NOTIFY presetsChanged)
    Q_PROPERTY(QString currentPresetName READ currentPresetName WRITE setCurrentPresetName NOTIFY currentPresetNameChanged)
    Q_PROPERTY(bool isCurrentPresetEdited READ isCurrentPresetEdited NOTIFY presetsChanged)
    Q_PROPERTY(bool canDeleteCurrentPreset READ canDeleteCurrentPreset NOTIFY presetsChanged)

    QML_ELEMENT

    GlobalInject<IGlobalConfiguration> globalConfiguration;
    GlobalInject<rcommand::ICommandsRegister> commandsRegister;
    GlobalInject<ICommandShortcutsRegister> commandShortcutsRegister;
    ContextInject<IShortcutsRegister> shortcutsRegister = { this };
    ContextInject<ui::IUiActionsRegister> uiactionsRegister = { this };
    ContextInject<IInteractive> interactive = { this };

public:
    explicit ShortcutsModel(QObject* parent = nullptr);

    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    QItemSelection selection() const;
    QVariant currentShortcut() const;

    QVariantList presets() const;
    QString currentPresetName() const;
    void setCurrentPresetName(const QString& name);

    bool isCurrentPresetEdited() const;
    bool canDeleteCurrentPreset() const;

    Q_INVOKABLE void resetCurrentPreset();
    Q_INVOKABLE void deleteCurrentPreset();

    Q_INVOKABLE void load();
    Q_INVOKABLE bool apply();
    Q_INVOKABLE void reset();

    Q_INVOKABLE void importShortcutsFromFile();
    Q_INVOKABLE void exportShortcutsToFile();

    Q_INVOKABLE void applySequenceToCurrentShortcut(const QString& newSequence, int conflictShortcutIndex = -1);

    Q_INVOKABLE void clearSelectedShortcuts();
    Q_INVOKABLE void resetToDefaultSelectedShortcuts();

    Q_INVOKABLE QVariantList shortcuts() const;

public slots:
    void setSelection(const QItemSelection& selection);

signals:
    void selectionChanged();
    void presetsChanged();
    void currentPresetNameChanged();

private:
    QString presetTitle(const std::string& presetName) const;
    bool isPresetEdited(const std::string& presetName) const;
    bool isPresetEditedOrHasUnsavedChanges(const std::string& presetName) const;
    void markUnsavedChanges();

    const muse::ui::UiAction& action(const std::string& actionCode) const;
    QString actionText(const std::string& actionCode) const;

    QModelIndex currentShortcutIndex() const;
    void notifyAboutShortcutChanged(const QModelIndex& index);

    enum Roles {
        RoleTitle = Qt::UserRole + 1,
        RoleGroup,
        RoleIcon,
        RoleIconColor,
        RoleSequence,
        RoleSearchKey
    };

    struct Item {
        Shortcut shortcut;
        QString group;
        QString title;
        int icon = 0;
        QString iconColor;
        QString sequence;
        QString searchKey;
    };

    QVariant shortcutToObject(const Item& item) const;

    QList<Item> m_items;
    QItemSelection m_selection;
    bool m_hasUnsavedChanges = false;
};
}
