/*
 * Copyright (C) 2011 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     wuchuanfei <wuchuanfei_cm@deepin.com>
 *
 * Maintainer: wuchuanfei <wuchuanfei_cm@deepin.com>
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

#pragma once

#include "window/namespace.h"

#include "widgets/settingsitem.h"

#include <QString>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>

namespace dcc {
namespace widgets {
class SwitchWidget;
}
}

namespace DCC_NAMESPACE {
namespace network {
class NetSwitchWidget : public dcc::widgets::SettingsItem
{
    Q_OBJECT
public:
    explicit NetSwitchWidget(QFrame *parent = nullptr);
    void initNetSwitch();

    void setTitle(const QString title);
    dcc::widgets::SwitchWidget *switchWidget();

private:
    QHBoxLayout *m_swswitchLayout;
    QLabel *m_swithTitle;
    dcc::widgets::SwitchWidget *m_autoConnItem;
};
}
}