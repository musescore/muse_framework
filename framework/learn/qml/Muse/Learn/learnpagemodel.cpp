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

#include "learnpagemodel.h"

#include "translation.h"

using namespace muse::learn;

LearnPageModel::LearnPageModel(QObject* parent)
    : QObject(parent),
    m_gettingStartedModel(new PlaylistModel(this)),
    m_advancedModel(new PlaylistModel(this))
{
}

PlaylistModel* LearnPageModel::startedPlaylist() const
{
    return m_gettingStartedModel;
}

PlaylistModel* LearnPageModel::advancedPlaylist() const
{
    return m_advancedModel;
}

void LearnPageModel::load()
{
    learnService()->refreshPlaylists();

    m_gettingStartedModel->setPlaylist(learnService()->startedPlaylist());
    learnService()->startedPlaylistChanged().onReceive(this, [this] (const Playlist& playlist) {
        m_gettingStartedModel->setPlaylist(playlist);
    });

    m_advancedModel->setPlaylist(learnService()->advancedPlaylist());
    learnService()->advancedPlaylistChanged().onReceive(this, [this] (const Playlist& playlist) {
        m_advancedModel->setPlaylist(playlist);
    });
}

QVariantMap LearnPageModel::classesAuthor() const
{
    QVariantMap author;
    author["name"] = muse::qtrc("learn", "Marc Sabatella");
    author["role"] = muse::qtrc("learn", "Instructor");

    // Rename to "MuseScore Studio" in this description?
    author["position"] = muse::qtrc("learn", "Creator, Mastering MuseScore");
    author["description"] = muse::qtrc("learn", "Welcome to Mastering MuseScore – the most comprehensive resource "
                                                "for learning the world’s most popular music notation software! "
                                                "My name is Marc Sabatella, and I have been helping develop, support, "
                                                "and promote MuseScore since its initial release over ten years ago.\n\n"
                                                "Whether you are just getting started with music notation software, "
                                                "or are a power user eager to explore advanced engraving and playback techniques, "
                                                "my flagship online course Mastering MuseScore "
                                                "covers everything you need to know to get the most out of MuseScore.\n\n"
                                                "In addition, Mastering MuseScore features a supportive community of musicians, "
                                                "with discussion spaces, live streams, "
                                                "and other related courses and services to help you create your best music. "
                                                "Take advantage of this opportunity to learn MuseScore from one of its most recognized experts!\n\n"
                                                "(Note: Mastering MuseScore is available in English only)");
    author["avatarUrl"] = "qrc:/qt/qml/Muse/Learn/resources/marc_sabatella.jpg";
    author["organizationName"] = muse::qtrc("learn", "Mastering MuseScore");
    author["organizationUrl"] = "https://www.masteringmusescore.com/musescore4";

    return author;
}

bool LearnPageModel::classesEnabled()
{
    return learnConfiguration()->classesEnabled();
}
