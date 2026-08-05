/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
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

#include <memory>

#include <gtest/gtest.h>

#include <QAbstractListModel>
#include <QList>
#include <QString>

#include "sectionedmodel.h"

#include "uicomponents/qml/Muse/UiComponents/sectionproxymodel.h"
#include "uicomponents/qml/Muse/UiComponents/sortfilterproxymodel.h"

using namespace Qt::StringLiterals;

namespace muse::uicomponents {
namespace {
QString titleAt(const SectionProxyModel* model, int row)
{
    return model->data(model->index(row, 0), SectionedModel::RoleTitle).toString();
}

bool isSectionAt(const SectionProxyModel* model, int row)
{
    return model->data(model->index(row, 0), SectionProxyModel::RoleIsSection).toBool();
}

QString sectionNameAt(const SectionProxyModel* model, int row)
{
    return model->data(model->index(row, 0), SectionProxyModel::RoleSectionName).toString();
}
}

class UiComponents_SectionProxyModelTests : public ::testing::Test
{
public:
    UiComponents_SectionProxyModelTests()
    {
        m_sortFilterModel->setSourceModel(m_model.get());
        m_sortFilterModel->setSectionRoleName(u"group"_s);

        m_proxyModel->setSourceModel(m_sortFilterModel.get());
        m_proxyModel->setSectionRoleName(u"group"_s);
    }

protected:
    std::unique_ptr<SectionedModel> m_model = std::make_unique<SectionedModel>();
    std::unique_ptr<SortFilterProxyModel> m_sortFilterModel = std::make_unique<SortFilterProxyModel>();
    std::unique_ptr<SectionProxyModel> m_proxyModel = std::make_unique<SectionProxyModel>();
};

TEST_F(UiComponents_SectionProxyModelTests, testDataChangedWithoutRoles)
{
    int resetCount = 0;
    int dataChangedCount = 0;

    QObject::connect(m_proxyModel.get(), &QAbstractItemModel::modelReset, m_proxyModel.get(), [&resetCount]() {
        ++resetCount;
    });
    QObject::connect(m_proxyModel.get(), &QAbstractItemModel::dataChanged, m_proxyModel.get(), [&dataChangedCount]() {
        ++dataChangedCount;
    });

    //! NOTE: The rows are: section A, a2, a1, section B, b1, b2, section "", x
    m_model->setTitle(3 /*a1*/, u"a1 edited"_s);

    EXPECT_EQ(resetCount, 0);
    EXPECT_EQ(dataChangedCount, 1);
    EXPECT_EQ(m_proxyModel->rowCount(), 8);
    EXPECT_EQ(titleAt(m_proxyModel.get(), 2), u"a1 edited"_s);

    m_model->setGroup(3 /*a1*/, u"B"_s);

    EXPECT_EQ(resetCount, 1);
    ASSERT_EQ(m_proxyModel->rowCount(), 8);
    EXPECT_EQ(m_proxyModel->data(m_proxyModel->index(0, 0), SectionProxyModel::RoleSectionItemCount).toInt(), 1);
    EXPECT_EQ(titleAt(m_proxyModel.get(), 1), u"a2"_s);
    EXPECT_EQ(m_proxyModel->data(m_proxyModel->index(2, 0), SectionProxyModel::RoleSectionItemCount).toInt(), 3);
}

TEST_F(UiComponents_SectionProxyModelTests, testNonContiguousSections)
{
    //! NOTE: The source model is not sorted by the section role here
    auto proxyModel = std::make_unique<SectionProxyModel>();
    proxyModel->setSourceModel(m_model.get());
    proxyModel->setSectionRoleName(u"group"_s);

    ASSERT_EQ(proxyModel->rowCount(), 8);

    //! NOTE: The sections follow the order of their first appearance,
    //! the items keep the order of the source model within a section
    EXPECT_TRUE(isSectionAt(proxyModel.get(), 0));
    EXPECT_EQ(sectionNameAt(proxyModel.get(), 0), u"B"_s);
    EXPECT_EQ(proxyModel->data(proxyModel->index(0, 0), SectionProxyModel::RoleSectionItemCount).toInt(), 2);
    EXPECT_EQ(titleAt(proxyModel.get(), 1), u"b1"_s);
    EXPECT_EQ(titleAt(proxyModel.get(), 2), u"b2"_s);

    EXPECT_TRUE(isSectionAt(proxyModel.get(), 3));
    EXPECT_EQ(sectionNameAt(proxyModel.get(), 3), u"A"_s);
    EXPECT_EQ(titleAt(proxyModel.get(), 4), u"a2"_s);
    EXPECT_EQ(titleAt(proxyModel.get(), 5), u"a1"_s);

    EXPECT_TRUE(isSectionAt(proxyModel.get(), 6));
    EXPECT_EQ(sectionNameAt(proxyModel.get(), 6), QString());
    EXPECT_EQ(titleAt(proxyModel.get(), 7), u"x"_s);

    //! NOTE: Collapsing and expanding gives the same rows as the rebuild
    proxyModel->setSectionCollapsed(u"B"_s, true);

    ASSERT_EQ(proxyModel->rowCount(), 6);
    EXPECT_TRUE(isSectionAt(proxyModel.get(), 0));
    EXPECT_TRUE(isSectionAt(proxyModel.get(), 1));
    EXPECT_EQ(sectionNameAt(proxyModel.get(), 1), u"A"_s);

    proxyModel->setSectionCollapsed(u"B"_s, false);

    ASSERT_EQ(proxyModel->rowCount(), 8);
    EXPECT_EQ(titleAt(proxyModel.get(), 1), u"b1"_s);
    EXPECT_EQ(titleAt(proxyModel.get(), 2), u"b2"_s);
}

TEST_F(UiComponents_SectionProxyModelTests, testSectionRows)
{
    ASSERT_EQ(m_proxyModel->rowCount(), 8);

    EXPECT_TRUE(isSectionAt(m_proxyModel.get(), 0));
    EXPECT_EQ(sectionNameAt(m_proxyModel.get(), 0), u"A"_s);
    EXPECT_EQ(m_proxyModel->data(m_proxyModel->index(0, 0), SectionProxyModel::RoleSectionItemCount).toInt(), 2);
    EXPECT_EQ(titleAt(m_proxyModel.get(), 1), u"a2"_s);
    EXPECT_EQ(titleAt(m_proxyModel.get(), 2), u"a1"_s);

    EXPECT_TRUE(isSectionAt(m_proxyModel.get(), 3));
    EXPECT_EQ(sectionNameAt(m_proxyModel.get(), 3), u"B"_s);
    EXPECT_EQ(titleAt(m_proxyModel.get(), 4), u"b1"_s);
    EXPECT_EQ(titleAt(m_proxyModel.get(), 5), u"b2"_s);

    //! NOTE: The items without a section are shown last
    EXPECT_TRUE(isSectionAt(m_proxyModel.get(), 6));
    EXPECT_EQ(sectionNameAt(m_proxyModel.get(), 6), QString());
    EXPECT_EQ(titleAt(m_proxyModel.get(), 7), u"x"_s);
}

TEST_F(UiComponents_SectionProxyModelTests, testIndexInSection)
{
    EXPECT_EQ(m_proxyModel->data(m_proxyModel->index(1, 0), SectionProxyModel::RoleIndexInSection).toInt(), 0);
    EXPECT_EQ(m_proxyModel->data(m_proxyModel->index(2, 0), SectionProxyModel::RoleIndexInSection).toInt(), 1);
    EXPECT_EQ(m_proxyModel->data(m_proxyModel->index(4, 0), SectionProxyModel::RoleIndexInSection).toInt(), 0);
    EXPECT_EQ(m_proxyModel->data(m_proxyModel->index(5, 0), SectionProxyModel::RoleIndexInSection).toInt(), 1);
    EXPECT_EQ(m_proxyModel->data(m_proxyModel->index(7, 0), SectionProxyModel::RoleIndexInSection).toInt(), 0);
}

TEST_F(UiComponents_SectionProxyModelTests, testMapping)
{
    EXPECT_FALSE(m_proxyModel->mapToSource(m_proxyModel->index(0, 0)).isValid());
    EXPECT_EQ(m_proxyModel->sourceRowOf(0), -1);
    EXPECT_TRUE(m_proxyModel->isSectionRow(0));

    const QModelIndex sourceIndex = m_proxyModel->mapToSource(m_proxyModel->index(1, 0));
    ASSERT_TRUE(sourceIndex.isValid());
    EXPECT_EQ(sourceIndex.data(SectionedModel::RoleTitle).toString(), u"a2"_s);
    EXPECT_EQ(m_proxyModel->mapFromSource(sourceIndex).row(), 1);
}

TEST_F(UiComponents_SectionProxyModelTests, testCollapseAndExpand)
{
    m_proxyModel->setSectionCollapsed(u"A"_s, true);

    ASSERT_EQ(m_proxyModel->rowCount(), 6);
    EXPECT_TRUE(isSectionAt(m_proxyModel.get(), 0));
    EXPECT_EQ(sectionNameAt(m_proxyModel.get(), 0), u"A"_s);
    EXPECT_FALSE(m_proxyModel->data(m_proxyModel->index(0, 0), SectionProxyModel::RoleSectionExpanded).toBool());
    EXPECT_EQ(m_proxyModel->data(m_proxyModel->index(0, 0), SectionProxyModel::RoleSectionItemCount).toInt(), 2);

    EXPECT_TRUE(isSectionAt(m_proxyModel.get(), 1));
    EXPECT_EQ(sectionNameAt(m_proxyModel.get(), 1), u"B"_s);
    EXPECT_EQ(titleAt(m_proxyModel.get(), 2), u"b1"_s);

    //! NOTE: The hidden items are not mapped
    const QModelIndex hiddenSourceIndex = m_sortFilterModel->index(0, 0);
    ASSERT_EQ(hiddenSourceIndex.data(SectionedModel::RoleTitle).toString(), u"a2"_s);
    EXPECT_FALSE(m_proxyModel->mapFromSource(hiddenSourceIndex).isValid());

    m_proxyModel->toggleSection(u"A"_s);

    ASSERT_EQ(m_proxyModel->rowCount(), 8);
    EXPECT_EQ(titleAt(m_proxyModel.get(), 1), u"a2"_s);
    EXPECT_EQ(titleAt(m_proxyModel.get(), 2), u"a1"_s);
    EXPECT_TRUE(m_proxyModel->data(m_proxyModel->index(0, 0), SectionProxyModel::RoleSectionExpanded).toBool());
}

TEST_F(UiComponents_SectionProxyModelTests, testCollapseAllAndExpandAll)
{
    m_proxyModel->collapseAll();

    ASSERT_EQ(m_proxyModel->rowCount(), 3);
    for (int row = 0; row < m_proxyModel->rowCount(); ++row) {
        SCOPED_TRACE(row);
        EXPECT_TRUE(isSectionAt(m_proxyModel.get(), row));
    }

    m_proxyModel->expandAll();

    EXPECT_EQ(m_proxyModel->rowCount(), 8);
    EXPECT_TRUE(m_proxyModel->collapsedSections().isEmpty());
}

TEST_F(UiComponents_SectionProxyModelTests, testWithoutSectionRoleName)
{
    m_proxyModel->setSectionRoleName(QString());

    ASSERT_EQ(m_proxyModel->rowCount(), m_sortFilterModel->rowCount());

    for (int row = 0; row < m_proxyModel->rowCount(); ++row) {
        SCOPED_TRACE(row);

        EXPECT_FALSE(isSectionAt(m_proxyModel.get(), row));
        EXPECT_EQ(m_proxyModel->sourceRowOf(row), row);
        EXPECT_EQ(m_proxyModel->mapToSource(m_proxyModel->index(row, 0)), m_sortFilterModel->index(row, 0));
    }
}
}
