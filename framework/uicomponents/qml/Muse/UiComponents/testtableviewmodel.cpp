/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2025 MuseScore Limited and others
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
#include "testtableviewmodel.h"

#include "internal/tableviewheader.h"

using namespace muse::uicomponents;

TestTableViewModel::TestTableViewModel(QObject* parent)
    : AbstractTableViewModel(parent)
{
    load();
}

void TestTableViewModel::load()
{
    QVector<QVector<TableViewCell*> > table;

    const ValList listOptions { Val("Item 1"), Val("Item 2"), Val("Item 3") };
    const ValList colorOptions { Val("Red"), Val("Green"), Val("Blue") };

    table.append({
        makeCell(Val("Charlie")),
        makeCell(Val(listOptions), Val("Item 2")),
        makeCell(Val("12h34m56.789 s")),
        makeCell(Val(3.5)),
        makeCell(Val("00h05m12.000 s")),
        makeCell(Val(colorOptions), Val("Red")),
    });

    table.append({
        makeCell(Val("Alpha")),
        makeCell(Val(listOptions), Val("Item 3")),
        makeCell(Val("00h00m00.000 s")),
        makeCell(Val(-1.25)),
        makeCell(Val("01h22m07.500 s")),
        makeCell(Val(colorOptions), Val("Green")),
    });

    table.append({
        makeCell(Val("Echo")),
        makeCell(Val(listOptions), Val("Item 1")),
        makeCell(Val("00h00m00.000 s")),
        makeCell(Val(10.0)),
        makeCell(Val("00h00m45.250 s")),
        makeCell(Val(colorOptions), Val("Blue")),
    });

    table.append({
        makeCell(Val("Bravo")),
        makeCell(Val(listOptions), Val("Item 1")),
        makeCell(Val("00h00m00.000 s")),
        makeCell(Val(0.0)),
        makeCell(Val("02h15m30.000 s")),
        makeCell(Val(colorOptions), Val("Green")),
    });

    table.append({
        makeCell(Val("Foxtrot")),
        makeCell(Val(listOptions), Val("Item 3")),
        makeCell(Val("00h00m00.000 s")),
        makeCell(Val(7.75)),
        makeCell(Val("00h45m00.000 s")),
        makeCell(Val(colorOptions), Val("Blue")),
    });

    table.append({
        makeCell(Val("Delta")),
        makeCell(Val(listOptions), Val("Item 2")),
        makeCell(Val("00h00m00.000 s")),
        makeCell(Val(2.0)),
        makeCell(Val("00h00m00.000 s")),
        makeCell(Val(colorOptions), Val("Red")),
    });

    table.append({
        makeCell(Val("Bravo")),
        makeCell(Val(listOptions), Val("Item 2")),
        makeCell(Val("00h00m00.000 s")),
        makeCell(Val(5.0)),
        makeCell(Val("00h12m00.000 s")),
        makeCell(Val(colorOptions), Val("Blue")),
    });

    table.append({
        makeCell(Val("Golf")),
        makeCell(Val(listOptions), Val("Item 1")),
        makeCell(Val("00h00m00.000 s")),
        makeCell(Val(-3.0)),
        makeCell(Val("03h00m00.000 s")),
        makeCell(Val(colorOptions), Val("Green")),
    });

    QList<TableViewHeader*> hHeaders = {
        makeHorizontalHeader("Column 1", TableViewCellType::Type::String, TableViewCellEditMode::Mode::DoubleClick,
                             100),
        makeHorizontalHeader("Column 2", TableViewCellType::Type::List, TableViewCellEditMode::Mode::DoubleClick, 100),
        makeHorizontalHeader("Column 3", static_cast<TableViewCellType::Type>(TestTableViewCellType::Type::Custom),
                             TableViewCellEditMode::Mode::StartInEdit, 200, makeAvailableFormats()),
        makeHorizontalHeader("Column 4", TableViewCellType::Type::Double, TableViewCellEditMode::Mode::StartInEdit,
                             200),
        makeHorizontalHeader("Column 5", TableViewCellType::Type::String, TableViewCellEditMode::Mode::StartInEdit,
                             300),
        makeHorizontalHeader("Column 6", TableViewCellType::Type::List, TableViewCellEditMode::Mode::DoubleClick, 100),
    };

    setHorizontalHeaders(hHeaders);

    setTable(table);
}

bool TestTableViewModel::doCellValueChangeRequested(int row, int column, const Val& value)
{
    Q_UNUSED(row);
    Q_UNUSED(column);
    Q_UNUSED(value);
    return true;
}

MenuItemList TestTableViewModel::makeAvailableFormats()
{
    MenuItemList items;

    ui::UiActionState enabledState;
    enabledState.enabled = true;

    ui::UiAction action;

    action.code = "default";
    action.title = TranslatableString::untranslatable("Default");
    MenuItem* item1 = new MenuItem(action);
    item1->setState(enabledState);
    item1->setSelectable(true);
    items << item1;

    action.code = "accent";
    action.title = TranslatableString::untranslatable("Accent");
    MenuItem* item2 = new MenuItem(action);
    item2->setState(enabledState);
    item2->setSelectable(true);
    items << item2;

    return items;
}
