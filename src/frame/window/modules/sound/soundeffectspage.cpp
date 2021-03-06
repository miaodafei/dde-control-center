/*
 * Copyright (C) 2011 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     lq <longqi_cm@deepin.com>
 *
 * Maintainer: lq <longqi_cm@deepin.com>
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

#include "soundeffectspage.h"
#include "window/utils.h"

#include "modules/sound/soundmodel.h"
#include "widgets/switchwidget.h"
#include "widgets/titlelabel.h"

#include <DIconButton>

#include <QTimer>
#include <QVBoxLayout>
#include <QListView>
#include <QLabel>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QSound>
#include <QScroller>

using namespace dcc::sound;
using namespace dcc::widgets;
using namespace DCC_NAMESPACE::sound;
DWIDGET_USE_NAMESPACE

const int AnimationDuration = 5000;

SoundEffectsPage::SoundEffectsPage(QWidget *parent)
    : QWidget(parent)
    , m_layout(new QVBoxLayout)
    , m_effectList(new DListView)
    , m_sound(nullptr)
{
    m_layout->setContentsMargins(ThirdPageContentsMargins);

    TitleLabel *lblTitle = new TitleLabel(tr("Sound Effects"));
    DFontSizeManager::instance()->bind(lblTitle, DFontSizeManager::T5, QFont::DemiBold);
    m_sw = new SwitchWidget(nullptr, lblTitle);
    m_sw->switchButton()->setAccessibleName(lblTitle->text());
    m_layout->addWidget(m_sw, 0, Qt::AlignTop);

    m_effectList->setAccessibleName("List_effectlist");
    m_effectList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_effectList->setSelectionMode(QListView::SelectionMode::NoSelection);
    m_effectList->setEditTriggers(DListView::NoEditTriggers);
    m_effectList->setFrameShape(DListView::NoFrame);
    m_effectList->setViewportMargins(5, 0, 10, 0);
    m_layout->addWidget(m_effectList, 1);
    m_layout->addStretch();
    m_effectList->setMinimumWidth(350);

    m_effectList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    QScroller::grabGesture(m_effectList->viewport(), QScroller::LeftMouseButtonGesture);
    QScroller *scroller = QScroller::scroller(m_effectList->viewport());
    QScrollerProperties sp;
    sp.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    scroller->setScrollerProperties(sp);

    m_aniTimer = new QTimer(this);
    m_aniTimer->setSingleShot(false);

    setLayout(m_layout);
}

SoundEffectsPage::~SoundEffectsPage()
{
    QScroller *scroller = QScroller::scroller(m_effectList->viewport());
    if (scroller) {
        scroller->stop();
    }
}

void SoundEffectsPage::setModel(dcc::sound::SoundModel *model)
{
    m_model = model;

    initList();
    m_effectList->setVisible(m_model->enableSoundEffect());

    connect(m_model, &SoundModel::enableSoundEffectChanged, this, [this](bool on) {
        m_sw->blockSignals(true);
        m_sw->setChecked(on);
        m_sw->blockSignals(false);
        m_effectList->setVisible(on);
    });
    connect(m_sw, &SwitchWidget::checkedChanged,
    this, [ = ](bool on) {
        this->requestSwitchSoundEffects(on);
    });
}

void SoundEffectsPage::startPlay(const QModelIndex &index)
{
    if (m_playIdx.isValid()) {
        auto item = static_cast<DStandardItem *>(m_listModel->itemFromIndex(m_playIdx));
        auto aniAction = item->actionList(Qt::Edge::RightEdge)[0];
        aniAction->setVisible(false);
        m_effectList->update(m_playIdx);
    }
    m_playIdx = index;

    auto eff = m_model->soundEffectMap()[index.row()].second;
    m_sound.reset(new QSound(m_model->soundEffectPathByType(eff)));
    m_sound->stop();
    m_sound->play();

    m_aniTimer->disconnect();
    auto item = static_cast<DStandardItem *>(m_listModel->itemFromIndex(index));
    auto aniAction = item->actionList(Qt::Edge::RightEdge)[0];
    int intervalal = 300;
    m_aniTimer->setInterval(intervalal);
    aniAction->setVisible(true);
    connect(m_aniTimer, &QTimer::timeout, this, [ = ] {
        auto aniIdx = (m_aniDuration / intervalal) % 3 + 1;
        auto icon = QIcon::fromTheme("dcc_volume" + QString::number(aniIdx));
        aniAction->setIcon(icon);

        m_aniDuration += intervalal;
        if (m_aniDuration > AnimationDuration)
        {
            aniAction->setVisible(false);
            m_aniTimer->stop();
            m_aniDuration = 0;
        }
        m_effectList->update(index);
    });

    m_aniTimer->start();
}

void SoundEffectsPage::initList()
{
    m_sw->setChecked(m_model->enableSoundEffect());

    m_listModel = new QStandardItemModel(this);
    m_effectList->setModel(m_listModel);

    DStandardItem *item = nullptr;
    for (auto se : m_model->soundEffectMap()) {
        item = new DStandardItem(se.first);
        auto action = new DViewItemAction(Qt::AlignVCenter, QSize(), QSize(), true);
        auto checkstatus = m_model->queryEffectData(se.second) ?
                           DStyle::SP_IndicatorChecked : DStyle::SP_IndicatorUnchecked ;
        auto icon = qobject_cast<DStyle *>(style())->standardIcon(checkstatus);
        action->setIcon(icon);
        auto aniAction = new DViewItemAction(Qt::AlignVCenter);
        aniAction->setVisible(false);
        item->setActionList(Qt::Edge::RightEdge, {aniAction, action});
        m_listModel->appendRow(item);

        connect(action, &DViewItemAction::triggered, this, [ = ] {
            auto isSelected = m_model->queryEffectData(se.second);
            this->requestSetEffectAble(se.second, !isSelected);
            requestRefreshList();
        });
    }

    connect(m_effectList, &DListView::clicked, this, &SoundEffectsPage::startPlay);
    connect(m_effectList, &DListView::activated, m_effectList, &QListView::clicked);
    connect(m_model, &SoundModel::soundEffectDataChanged, this,
    [ = ](DDesktopServices::SystemSoundEffect effect, const bool enable) {
        for (int idx = 0; idx < m_model->soundEffectMap().size(); ++idx) {
            auto ite = m_model->soundEffectMap().at(idx);
            if (ite.second != effect) {
                continue;
            }

auto items = static_cast<DStandardItem *>(m_listModel->item(idx));
            auto action = items->actionList(Qt::Edge::RightEdge)[1];
            auto checkstatus = enable ?
                               DStyle::SP_IndicatorChecked : DStyle::SP_IndicatorUnchecked ;
            auto icon = qobject_cast<DStyle *>(style())->standardIcon(checkstatus);
            action->setIcon(icon);
            m_effectList->update(items->index());
            break;
        }
    });
}
