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
#include "testflowinteractive.h"

#include "log.h"

using namespace muse;
using namespace muse::testflow;

void TestflowInteractive::setRealInteractive(std::shared_ptr<IInteractive> real)
{
    m_real = real;
}

std::shared_ptr<IInteractive> TestflowInteractive::realInteractive() const
{
    return m_real;
}

IInteractive::Result TestflowInteractive::questionSync(const std::string& contentTitle, const Text& text, const ButtonDatas& buttons,
                                                       int defBtn, const Options& options, const std::string& dialogTitle)
{
    if (m_skipDialogs) {
        return skippedResult("QUESTION", contentTitle);
    }
    return m_real->questionSync(contentTitle, text, buttons, defBtn, options, dialogTitle);
}

async::Promise<IInteractive::Result> TestflowInteractive::question(const std::string& contentTitle, const Text& text,
                                                                   const ButtonDatas& buttons, int defBtn,
                                                                   const Options& options, const std::string& dialogTitle)
{
    if (m_skipDialogs) {
        return resultPromise(skippedResult("QUESTION", contentTitle));
    }
    return m_real->question(contentTitle, text, buttons, defBtn, options, dialogTitle);
}

IInteractive::ButtonData TestflowInteractive::buttonData(Button b) const
{
    return m_real->buttonData(b);
}

IInteractive::Result TestflowInteractive::infoSync(const std::string& contentTitle, const Text& text, const ButtonDatas& buttons,
                                                   int defBtn,
                                                   const Options& options, const std::string& dialogTitle)
{
    if (m_skipDialogs) {
        return skippedResult("INFO", contentTitle);
    }
    if (isUnattended()) {
        return unattendedResult("INFO", contentTitle, text, buttons, defBtn);
    }
    return m_real->infoSync(contentTitle, text, buttons, defBtn, options, dialogTitle);
}

async::Promise<IInteractive::Result> TestflowInteractive::info(const std::string& contentTitle, const Text& text,
                                                               const ButtonDatas& buttons, int defBtn,
                                                               const Options& options, const std::string& dialogTitle)
{
    if (m_skipDialogs) {
        return resultPromise(skippedResult("INFO", contentTitle));
    }
    if (isUnattended()) {
        Result res = unattendedResult("INFO", contentTitle, text, buttons, defBtn);
        return resultPromise(res);
    }
    return m_real->info(contentTitle, text, buttons, defBtn, options, dialogTitle);
}

IInteractive::Result TestflowInteractive::warningSync(const std::string& contentTitle, const Text& text, const ButtonDatas& buttons,
                                                      int defBtn,
                                                      const Options& options, const std::string& dialogTitle)
{
    if (m_skipDialogs) {
        return skippedResult("WARNING", contentTitle);
    }
    if (isUnattended()) {
        return unattendedResult("WARNING", contentTitle, text, buttons, defBtn);
    }
    return m_real->warningSync(contentTitle, text, buttons, defBtn, options, dialogTitle);
}

async::Promise<IInteractive::Result> TestflowInteractive::warning(const std::string& contentTitle, const Text& text,
                                                                  const ButtonDatas& buttons, int defBtn,
                                                                  const Options& options, const std::string& dialogTitle)
{
    if (m_skipDialogs) {
        return resultPromise(skippedResult("WARNING", contentTitle));
    }
    if (isUnattended()) {
        Result res = unattendedResult("WARNING", contentTitle, text, buttons, defBtn);
        return resultPromise(res);
    }
    return m_real->warning(contentTitle, text, buttons, defBtn, options, dialogTitle);
}

IInteractive::Result TestflowInteractive::errorSync(const std::string& contentTitle, const Text& text, const ButtonDatas& buttons,
                                                    int defBtn,
                                                    const Options& options, const std::string& dialogTitle)
{
    if (m_skipDialogs) {
        return skippedResult("ERROR", contentTitle);
    }
    if (isUnattended()) {
        return unattendedResult("ERROR", contentTitle, text, buttons, defBtn);
    }
    return m_real->errorSync(contentTitle, text, buttons, defBtn, options, dialogTitle);
}

async::Promise<IInteractive::Result> TestflowInteractive::error(const std::string& contentTitle, const Text& text,
                                                                const ButtonDatas& buttons, int defBtn,
                                                                const Options& options, const std::string& dialogTitle)
{
    if (m_skipDialogs) {
        return resultPromise(skippedResult("ERROR", contentTitle));
    }
    if (isUnattended()) {
        Result res = unattendedResult("ERROR", contentTitle, text, buttons, defBtn);
        return resultPromise(res);
    }
    return m_real->error(contentTitle, text, buttons, defBtn, options, dialogTitle);
}

void TestflowInteractive::showProgress(const std::string& title, Progress progress)
{
    m_real->showProgress(title, progress);
}

async::Promise<io::path_t> TestflowInteractive::selectOpeningFile(const std::string& title, const io::path_t& dir,
                                                                  const std::vector<std::string>& filter)
{
    if (m_skipDialogs) {
        LOGW() << "[testflow] dialog skipped (select file): " << title;
        return async::make_promise<io::path_t>([](auto, auto reject) {
            Ret ret = muse::make_ret(Ret::Code::Cancel);
            return reject(ret.code(), ret.text());
        });
    }
    return m_real->selectOpeningFile(title, dir, filter);
}

io::path_t TestflowInteractive::selectOpeningFileSync(const std::string& title, const io::path_t& dir,
                                                      const std::vector<std::string>& filter, const int options)
{
    if (m_skipDialogs) {
        LOGW() << "[testflow] dialog skipped (select file): " << title;
        return {};
    }
    return m_real->selectOpeningFileSync(title, dir, filter, options);
}

io::paths_t TestflowInteractive::selectOpeningFilesSync(const std::string& title, const io::path_t& dir,
                                                        const std::vector<std::string>& filter, const int options)
{
    if (m_skipDialogs) {
        LOGW() << "[testflow] dialog skipped (select files): " << title;
        return {};
    }
    return m_real->selectOpeningFilesSync(title, dir, filter, options);
}

async::Promise<io::path_t> TestflowInteractive::selectSavingFile(const std::string& title, const io::path_t& dir,
                                                                 const std::vector<std::string>& filter,
                                                                 bool confirmOverwrite)
{
    if (m_skipDialogs) {
        LOGW() << "[testflow] dialog skipped (save file): " << title;
        return async::make_promise<io::path_t>([](auto, auto reject) {
            Ret ret = muse::make_ret(Ret::Code::Cancel);
            return reject(ret.code(), ret.text());
        });
    }
    return m_real->selectSavingFile(title, dir, filter, confirmOverwrite);
}

io::path_t TestflowInteractive::selectSavingFileSync(const std::string& title, const io::path_t& dir,
                                                     const std::vector<std::string>& filter,
                                                     bool confirmOverwrite)
{
    if (m_skipDialogs) {
        LOGW() << "[testflow] dialog skipped (save file): " << title;
        return {};
    }
    // return m_real->selectSavingFile(title, dir, filter, confirmOverwrite);
    QStringList filterList;
    for (const std::string& fileFilter : filter) {
        filterList << QString::fromStdString(fileFilter);
    }

    LOGD() << title << " dir:" << dir << ", filter: " << filterList << ", confirmOverwrite: " << confirmOverwrite;
    m_real->openSync("muse://testflow/selectfile?filePath=" + dir.toStdString());
    m_selectedFilePath = dir;
    return m_selectedFilePath;
}

io::path_t TestflowInteractive::selectDirectory(const std::string& title, const io::path_t& dir)
{
    if (m_skipDialogs) {
        LOGW() << "[testflow] dialog skipped (select directory): " << title;
        return {};
    }
    return m_real->selectDirectory(title, dir);
}

io::paths_t TestflowInteractive::selectMultipleDirectories(const std::string& title, const io::path_t& dir,
                                                           const io::paths_t& initialDirectories)
{
    if (m_skipDialogs) {
        LOGW() << "[testflow] dialog skipped (select directories): " << title;
        return {};
    }
    return m_real->selectMultipleDirectories(title, dir, initialDirectories);
}

async::Promise<Color> TestflowInteractive::selectColor(const Color& color, const std::string& title, bool allowAlpha)
{
    if (m_skipDialogs) {
        LOGW() << "[testflow] dialog skipped (select color): " << title;
        return async::make_promise<Color>([](auto, auto reject) {
            Ret ret = muse::make_ret(Ret::Code::Cancel);
            return reject(ret.code(), ret.text());
        });
    }
    return m_real->selectColor(color, title, allowAlpha);
}

bool TestflowInteractive::isSelectColorOpened() const
{
    return m_real->isSelectColorOpened();
}

RetVal<Val> TestflowInteractive::openSync(const UriQuery& uri)
{
    if (m_skipDialogs && isDialog(uri.uri())) {
        LOGW() << "[testflow] dialog skipped (open): " << uri.toString();
        return RetVal<Val>(muse::make_ret(Ret::Code::Cancel));
    }
    return m_real->openSync(uri);
}

async::Promise<Val> TestflowInteractive::open(const UriQuery& uri)
{
    if (m_skipDialogs && isDialog(uri.uri())) {
        LOGW() << "[testflow] dialog skipped (open): " << uri.toString();
        return async::make_promise<Val>([](auto, auto reject) {
            Ret ret = muse::make_ret(Ret::Code::Cancel);
            return reject(ret.code(), ret.text());
        });
    }
    return m_real->open(uri);
}

RetVal<bool> TestflowInteractive::isOpened(const UriQuery& uri) const
{
    return m_real->isOpened(uri);
}

RetVal<bool> TestflowInteractive::isOpened(const Uri& uri) const
{
    return m_real->isOpened(uri);
}

async::Channel<Uri> TestflowInteractive::opened() const
{
    return m_real->opened();
}

void TestflowInteractive::raise(const UriQuery& uri)
{
    m_real->raise(uri);
}

async::Promise<Ret> TestflowInteractive::close(const UriQuery& uri)
{
    return m_real->close(uri);
}

async::Promise<Ret> TestflowInteractive::close(const Uri& uri)
{
    return m_real->close(uri);
}

Ret TestflowInteractive::closeSync(const UriQuery& uri)
{
    return m_real->closeSync(uri);
}

async::Promise<Ret> TestflowInteractive::closeAllDialogs()
{
    return m_real->closeAllDialogs();
}

Ret TestflowInteractive::closeAllDialogsSync()
{
    return m_real->closeAllDialogsSync();
}

ValCh<Uri> TestflowInteractive::currentUri() const
{
    return m_real->currentUri();
}

RetVal<bool> TestflowInteractive::isCurrentUriDialog() const
{
    return m_real->isCurrentUriDialog();
}

async::Notification TestflowInteractive::currentUriAboutToBeChanged() const
{
    return m_real->currentUriAboutToBeChanged();
}

std::vector<Uri> TestflowInteractive::stack() const
{
    return m_real->stack();
}

QWindow* TestflowInteractive::topWindow() const
{
    return m_real->topWindow();
}

bool TestflowInteractive::topWindowIsWidget() const
{
    return m_real->topWindowIsWidget();
}

io::path_t TestflowInteractive::selectedFilePath() const
{
    return m_selectedFilePath;
}

void TestflowInteractive::setUnattendedMessageHandler(const MessageHandler& handler)
{
    m_unattendedMessageHandler = handler;
}

void TestflowInteractive::setSkipDialogs(bool skip)
{
    m_skipDialogs = skip;
}

bool TestflowInteractive::isDialog(const Uri& uri) const
{
    //! NOTE Pages (e.g. the startup page) are not dialogs, they don't wait for an answer
    return uriRegister()->meta(uri).type != interactive::ContainerMeta::PrimaryPage;
}

IInteractive::Result TestflowInteractive::skippedResult(const std::string& type, const std::string& title) const
{
    LOGW() << "[testflow] dialog skipped (" << type << "): " << title;
    return Result(int(Button::Cancel));
}

async::Promise<IInteractive::Result> TestflowInteractive::resultPromise(const Result& res)
{
    return async::make_promise<Result>([res](auto resolve) {
        return resolve(res);
    });
}

bool TestflowInteractive::isUnattended() const
{
    return m_unattendedMessageHandler != nullptr;
}

IInteractive::Result TestflowInteractive::unattendedResult(const std::string& type, const std::string& title, const Text& text,
                                                           const ButtonDatas& buttons, int defBtn) const
{
    m_unattendedMessageHandler(type, title, text.text);

    if (defBtn != int(Button::NoButton)) {
        return Result(defBtn);
    }

    if (!buttons.empty()) {
        return Result(buttons.front().btn);
    }

    return Result(int(Button::Ok));
}
