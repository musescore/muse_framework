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
#include "navigationapi.h"

#include <QElapsedTimer>
#include <QEventLoop>
#include <QTimer>

#include "navigationcommands.h"

#include "log.h"

using namespace muse::api;
using namespace muse::ui;

NavigationApi::NavigationApi(api::IApiEngine* e)
    : api::ApiObject(e)
{
}

NavigationApi::~NavigationApi()
{
}

void NavigationApi::nextPanel()
{
    dispatcher()->dispatch(NEXT_PANEL_COMMAND);
}

void NavigationApi::prevPanel()
{
    dispatcher()->dispatch(PREV_PANEL_COMMAND);
}

void NavigationApi::right()
{
    dispatcher()->dispatch(RIGHT_COMMAND);
}

void NavigationApi::left()
{
    dispatcher()->dispatch(LEFT_COMMAND);
}

void NavigationApi::up()
{
    dispatcher()->dispatch(UP_COMMAND);
}

void NavigationApi::down()
{
    dispatcher()->dispatch(DOWN_COMMAND);
}

void NavigationApi::escape()
{
    dispatcher()->dispatch(ESCAPE_COMMAND);
}

static constexpr int WAIT_POLL_INTERVAL_MSEC = 50;

bool NavigationApi::waitFor(const std::function<bool()>& condition, int timeoutMsec) const
{
    QElapsedTimer elapsed;
    elapsed.start();

    while (true) {
        if (condition()) {
            return true;
        }

        if (elapsed.elapsed() >= timeoutMsec) {
            return false;
        }

        //! NOTE Let the UI update (load QML, fill models) while waiting
        QEventLoop loop;
        bool polled = false;
        QTimer::singleShot(WAIT_POLL_INTERVAL_MSEC, &loop, [&loop, &polled]() {
            polled = true;
            loop.quit();
        });
        loop.exec();

        //! NOTE The loop was interrupted, for example, the application is exiting
        if (!polled) {
            return false;
        }
    }
}

bool NavigationApi::waitControl(const QString& section, const QString& panel, const QString& controlName, int timeoutMsec)
{
    const std::string sectionName = section.toStdString();
    const std::string panelName = panel.toStdString();
    const std::string name = controlName.toStdString();

    return waitFor([this, &sectionName, &panelName, &name]() {
        const INavigationControl* control = navigation()->findControl(sectionName, panelName, name);
        return control && control->enabled();
    }, timeoutMsec);
}

bool NavigationApi::goToControl(const QString& section, const QString& panel, const QJSValue& controlNameOrIndex, int timeoutMsec)
{
    if (timeoutMsec > 0) {
        if (controlNameOrIndex.isString()) {
            waitControl(section, panel, controlNameOrIndex.toString(), timeoutMsec);
        } else {
            const std::string sectionName = section.toStdString();
            const std::string panelName = panel.toStdString();
            waitFor([this, &sectionName, &panelName]() {
                const INavigationPanel* p = navigation()->findPanel(sectionName, panelName);
                return p && p->enabled();
            }, timeoutMsec);
        }
    }

    bool ok = false;
    if (controlNameOrIndex.isString()) {
        ok = navigation()->requestActivateByName(section.toStdString(), panel.toStdString(), controlNameOrIndex.toString().toStdString());
    } else if (controlNameOrIndex.isArray()) {
        if (controlNameOrIndex.property("length").toInt() == 2) {
            INavigation::Index idx;
            idx.row = controlNameOrIndex.property(0).toInt();
            idx.column = controlNameOrIndex.property(1).toInt();
            ok = navigation()->requestActivateByIndex(section.toStdString(), panel.toStdString(), idx);
        } else {
            LOGE() << "bad argument `control`: " << controlNameOrIndex.toString();
            ok = false;
        }
    } else {
        LOGE() << "bad argument `control`: " << controlNameOrIndex.toString();
        ok = false;
    }

    return ok;
}

void NavigationApi::trigger()
{
    dispatcher()->dispatch(TRIGGER_CONTROL_COMMAND);
}

bool NavigationApi::triggerControl(const QString& section, const QString& panel, const QJSValue& controlNameOrIndex,
                                   int timeoutMsec)
{
    bool ok = goToControl(section, panel, controlNameOrIndex, timeoutMsec);
    if (ok) {
        trigger();
    }
    return ok;
}

QString NavigationApi::activeSection() const
{
    INavigationSection* sec = navigation()->activeSection();
    return sec ? sec->name() : QString();
}

QString NavigationApi::activePanel() const
{
    INavigationPanel* p = navigation()->activePanel();
    return p ? p->name() : QString();
}

QString NavigationApi::activeControl() const
{
    INavigationControl* c = navigation()->activeControl();
    return c ? c->name() : QString();
}

static QJSValue toQJSValue(const INavigation* n, IApiEngine* e)
{
    QJSValue o = e->newObject();
    o.setProperty("name", n->name());

    QJSValue io = e->newObject();
    io.setProperty("column", n->index().column);
    io.setProperty("row", n->index().row);
    o.setProperty("index", io);

    o.setProperty("enabled", n->enabled());
    o.setProperty("active", n->active());

    return o;
}

QJSValue NavigationApi::sections() const
{
    const std::set<INavigationSection*>& sects = navigation()->sections();
    QJSValue arr = engine()->newArray(sects.size());
    quint32 i = 0;
    for (const INavigationSection* s : sects) {
        arr.setProperty(i, toQJSValue(s, engine()));
        ++i;
    }

    return arr;
}

QJSValue NavigationApi::panels(const QString& sectionName) const
{
    const INavigationSection* s = navigation()->findSection(sectionName.toStdString());
    if (!s) {
        return engine()->newArray(0);
    }

    QJSValue arr = engine()->newArray(s->panels().size());
    quint32 i = 0;
    for (const INavigationPanel* p : s->panels()) {
        arr.setProperty(i, toQJSValue(p, engine()));
        ++i;
    }

    return arr;
}

QJSValue NavigationApi::controls(const QString& sectionName, const QString& panelName) const
{
    const INavigationPanel* p = navigation()->findPanel(sectionName.toStdString(), panelName.toStdString());
    if (!p) {
        return engine()->newArray(0);
    }

    QJSValue arr = engine()->newArray(p->controls().size());
    quint32 i = 0;
    for (const INavigationControl* c : p->controls()) {
        arr.setProperty(i, toQJSValue(c, engine()));
        ++i;
    }

    return arr;
}

void NavigationApi::dump() const
{
    navigation()->dump();
}
