/*
 * Copyright (C) 2011 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     andywang <andywang_cm@deepin.com>
 *
 * Maintainer: andywang <andywang_cm@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "widgets/comboxwidget.h"
#include "widgets/labels/normallabel.h"

#include <QHBoxLayout>
#include <QComboBox>
#include <QDebug>
#include <QStringList>
#include <QMouseEvent>

namespace dcc {
namespace widgets {

ComboxWidget::ComboxWidget(QFrame *parent)
    : ComboxWidget(new NormalLabel, parent)
{

}

ComboxWidget::ComboxWidget(const QString &title, QFrame *parent)
    : ComboxWidget(new NormalLabel(title), parent)
{

}

ComboxWidget::ComboxWidget(QWidget *widget, QFrame *parent)
    : SettingsItem(parent)
    , m_leftWidget(widget)
    , m_switchComboBox(new QComboBox)
{
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);

    //set a vertical margin instead of a fixed height,
    //as we are now dealing with multi-line labels as well.
    int vertical_margin = 0;
    QLabel *label = qobject_cast<QLabel *>(m_leftWidget);
    if (label) {
        label->setWordWrap(true);
        vertical_margin = (36 - label->fontMetrics().height()) / 2;
    }

    mainLayout->setContentsMargins(20, vertical_margin, 10, vertical_margin);

    mainLayout->addWidget(m_leftWidget, 0, Qt::AlignVCenter);
    mainLayout->addStretch();
    mainLayout->addWidget(m_switchComboBox, 0, Qt::AlignVCenter);

    m_leftWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    setLayout(mainLayout);

    connect(m_switchComboBox, &QComboBox::currentTextChanged, this, &ComboxWidget::onSelectChanged);
}

void ComboxWidget::setComboxOption(const QStringList &options)
{
    m_switchComboBox->blockSignals(true);
    for (QString item : options) {
        m_switchComboBox->addItem(item);
    }
    m_switchComboBox->blockSignals(false);
}

void ComboxWidget::setTitle(const QString &title)
{
    QLabel *label = qobject_cast<QLabel *>(m_leftWidget);
    if (label) {
        label->setWordWrap(true);
        label->setText(title);
    }

    setAccessibleName(title);
}

void ComboxWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_switchComboBox->geometry().contains(event->pos())) {
        Q_EMIT clicked();
    }

    return SettingsItem::mouseReleaseEvent(event);
}

}
}