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

#include "microphonepage.h"
#include "modules/sound/soundmodel.h"
#include "window/utils.h"

#include <com_deepin_daemon_audio_source.h>

#include "widgets/switchwidget.h"
#include "widgets/titlelabel.h"
#include "widgets/titledslideritem.h"
#include "widgets/dccslider.h"
#include "widgets/comboxwidget.h"
#include "widgets/settingsgroup.h"
#include "types/audioport.h"

#include <DStandardItem>
#include <DStyle>
#include <DFontSizeManager>
#include <DGuiApplicationHelper>
#include <DApplication>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QDebug>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QSvgRenderer>


using namespace dcc::sound;
using namespace dcc::widgets;
using namespace DCC_NAMESPACE::sound;
using com::deepin::daemon::audio::Source;

Q_DECLARE_METATYPE(const dcc::sound::Port *)

SoundLabel::SoundLabel(QWidget *parent)
    : QLabel(parent)
    , m_mute(false)
{

}

void SoundLabel::mouseReleaseEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
    m_mute = !m_mute;
    Q_EMIT clicked(m_mute);
}

MicrophonePage::MicrophonePage(QWidget *parent)
    : QWidget(parent)
    , m_layout(new QVBoxLayout)
    , m_sw(new SwitchWidget)
    , m_mute(false)
{
    const int titleLeftMargin = 8;
    //~ contents_path /sound/Advanced
    TitleLabel *labelInput = new TitleLabel(tr("Input"));
    DFontSizeManager::instance()->bind(labelInput, DFontSizeManager::T5, QFont::DemiBold);
    labelInput->setContentsMargins(titleLeftMargin, 0, 0, 0);
    labelInput->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    m_inputSoundCbx = new ComboxWidget(tr("Input Device"));
    m_inputSoundCbx->setMaximumHeight(50);

    QHBoxLayout *hlayout = new QHBoxLayout;
    TitleLabel *lblTitle = new TitleLabel(tr("On"));
    DFontSizeManager::instance()->bind(lblTitle, DFontSizeManager::T5, QFont::DemiBold);
    m_sw = new SwitchWidget(nullptr, lblTitle);

    TitleLabel *ndTitle = new TitleLabel(tr("Automatic Noise Suppression"));
    DFontSizeManager::instance()->bind(ndTitle, DFontSizeManager::T5, QFont::DemiBold);
    m_noiseReductionsw = new SwitchWidget(nullptr, ndTitle);

    hlayout->addWidget(m_sw);

    m_inputModel  = new QStandardItemModel(m_inputSoundCbx->comboBox());
    m_inputSoundCbx->comboBox()->setModel(m_inputModel);
    SettingsGroup *inputSoundsGrp = new SettingsGroup(nullptr, SettingsGroup::GroupBackground);
    inputSoundsGrp->appendItem(m_inputSoundCbx);
    if (inputSoundsGrp->layout()) {
        inputSoundsGrp->layout()->setContentsMargins(ThirdPageContentsMargins);
    }

    m_layout->setContentsMargins(ThirdPageContentsMargins);
    m_layout->addWidget(labelInput);
    m_layout->addWidget(inputSoundsGrp);
    m_layout->addLayout(hlayout);
    m_layout->addWidget(m_noiseReductionsw);
    setLayout(m_layout);
}

MicrophonePage::~MicrophonePage()
{
#ifndef DCC_DISABLE_FEEDBACK
    if (m_feedbackSlider)
        m_feedbackSlider->disconnect(m_conn);
    m_feedbackSlider->deleteLater();
#endif
}

/**当用户进入扬声器端口手动切换蓝牙输出端口后，再进入麦克风页面时
 * 会有默认输入端口路径为空或者指定路径下激活端口为空的情况，
 */
void MicrophonePage::resetUi()
{
    QDBusInterface interface("com.deepin.daemon.Audio", "/com/deepin/daemon/Audio", "com.deepin.daemon.Audio", QDBusConnection::sessionBus(), this);
    QDBusObjectPath defaultPath = interface.property("DefaultSource").value<QDBusObjectPath>();
    if (defaultPath.path() == "/") //路径为空
        m_inputSoundCbx->comboBox()->setCurrentIndex(-1);
    else {
        Source defaultSourcer("com.deepin.daemon.Audio", defaultPath.path(), QDBusConnection::sessionBus(), this);
        AudioPort port = defaultSourcer.activePort();
        if (port.name.isEmpty() || port.description.isEmpty()) {
            m_inputSoundCbx->comboBox()->setCurrentIndex(-1);
        }
    }
}

void MicrophonePage::setModel(SoundModel *model)
{
    m_model = model;

    //监听消息设置是否可用
    connect(m_model, &SoundModel::isPortEnableChanged, this, [ = ](bool enable) {
        m_sw->setChecked(enable);
    });
    //发送查询请求消息看是否可用
    connect(m_model, &SoundModel::setPortChanged, this, [ = ](const dcc::sound::Port  * port) {
        m_currentPort = port;
        if (!m_currentPort) return;
        m_sw->setHidden(!m_model->isShow(m_inputModel, m_currentPort));
        Q_EMIT m_model->requestSwitchEnable(port->cardId(), port->id());
    });

    auto ports = m_model->ports();
    for (auto port : ports) {
        addPort(port);
    }
    //这个临时这个判断,直接设置m_noiseReductionsw(m_model->reduceNoise()) 没有效果,待dtk组的同时祥查
    if (m_model->reduceNoise())
        m_noiseReductionsw->setChecked(true);
    else
        m_noiseReductionsw->setChecked(false);

    //连接switch点击信号，发送切换开/关扬声器的请求信号
    connect(m_sw, &SwitchWidget::checkedChanged, this, [ = ] {
        if(m_currentPort != nullptr)
            Q_EMIT m_model->requestSwitchSetEnable(m_currentPort->cardId(), m_currentPort->id(), m_sw->checked());
        else
            m_sw->setChecked(false);
    });

    connect(m_model, &SoundModel::portAdded, this, &MicrophonePage::addPort);

    connect(m_model, &SoundModel::portRemoved, this, &MicrophonePage::removePort);

    connect(m_inputSoundCbx->comboBox(), static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](const int  idx) {
        showDevice();
        if (idx < 0) return;
        auto temp = m_inputModel->index(idx, 0);
        this->requestSetPort(m_inputModel->data(temp, Qt::WhatsThisPropertyRole).value<const dcc::sound::Port *>());
    });

    connect(m_noiseReductionsw, &SwitchWidget::checkedChanged, this, &MicrophonePage::requestReduceNoise);

    connect(m_model, &SoundModel::microphoneOnChanged, this, [ = ](bool flag) {
        m_mute = flag; refreshIcon();
    });

    initSlider();

    if (m_currentPort) m_sw->setHidden(!m_model->isShow(m_inputModel, m_currentPort));

    if (m_inputModel->rowCount() < 2) m_sw->setHidden(true);

}

void MicrophonePage::removePort(const QString &portId, const uint &cardId)
{
    auto rmFunc = [ = ](QStandardItemModel * model) {
        for (int i = 0; i < model->rowCount();) {
            auto item = model->item(i);
            auto port = item->data(Qt::WhatsThisPropertyRole).value<const dcc::sound::Port *>();
            if (port->id() == portId && cardId == port->cardId()) {
                model->removeRow(i);
            } else {
                ++i;
            }
        }
    };

    rmFunc(m_inputModel);
    if (m_currentPort)
        m_sw->setHidden(!m_model->isShow(m_inputModel, m_currentPort));
    showDevice();
}

void MicrophonePage::addPort(const dcc::sound::Port *port)
{
    if (port->In == port->direction()) {
        DStandardItem *pi = new DStandardItem;
        pi->setText(port->name() + "(" + port->cardName() + ")");

        pi->setData(QVariant::fromValue<const dcc::sound::Port *>(port), Qt::WhatsThisPropertyRole);

        connect(port, &dcc::sound::Port::nameChanged, this, [ = ](const QString str) {
            pi->setText(str);
        });
        connect(port, &dcc::sound::Port::isActiveChanged, this, [ = ](bool isActive) {
            pi->setCheckState(isActive ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
        });

        m_inputModel->appendRow(pi);
        if (port->isActive()) {
            m_inputSoundCbx->comboBox()->setCurrentText(port->name() + "(" + port->cardName() + ")");
            m_currentPort = port;
            Q_EMIT m_model->requestSwitchEnable(port->cardId(), port->id());
        }
        if (m_currentPort)
            m_sw->setHidden(!m_model->isShow(m_inputModel, m_currentPort));
        showDevice();
    }
}

void MicrophonePage::toggleMute()
{
    Q_EMIT requestMute();
}

void MicrophonePage::initSlider()
{
    //~ contents_path /sound/Microphone
    m_inputSlider = new TitledSliderItem(tr("Input Volume"));
    m_inputSlider->addBackground();
    m_layout->insertWidget(3, m_inputSlider);

    m_volumeBtn = new SoundLabel(this);
    m_volumeBtn->setScaledContents(true);
    m_inputSlider->getbottomlayout()->insertWidget(0,m_volumeBtn);
    m_volumeBtn->setAccessibleName("volume-button");
    m_volumeBtn->setFixedSize(ICON_SIZE, ICON_SIZE);

    DCCSlider *slider = m_inputSlider->slider();
    slider->setRange(0, 100);
    slider->setType(DCCSlider::Vernier);
    slider->setTickPosition(QSlider::NoTicks);
    auto icon_high = qobject_cast<DStyle *>(style())->standardIcon(DStyle::SP_MediaVolumeHighElement);
    slider->setRightIcon(icon_high);
    slider->setIconSize(QSize(24, 24));
    slider->setTickInterval(1);
    slider->setSliderPosition(int(m_model->microphoneVolume() * 100));
    slider->setPageStep(1);

    auto slotfunc1 = [ = ](int pos) {
        double val = pos / 100.0;
        Q_EMIT requestSetMicrophoneVolume(val);
    };
    int val = static_cast<int>(m_model->microphoneVolume() * static_cast<int>(100.0f));
    slider->setValue(val);
    m_inputSlider->setValueLiteral(QString::number(m_model->microphoneVolume() * 100) + "%");
    connect(slider, &DCCSlider::valueChanged, this, slotfunc1);
    connect(slider, &DCCSlider::sliderMoved, this, slotfunc1);
    connect(m_model, &SoundModel::microphoneVolumeChanged, this, [ = ](double v) {
        slider->blockSignals(true);
        slider->setValue(static_cast<int>(v * 100));
        slider->setSliderPosition(static_cast<int>(v * 100));
        slider->blockSignals(false);
        m_inputSlider->setValueLiteral(QString::number(v * 100) + "%");
    });
    connect(m_volumeBtn, &SoundLabel::clicked, this, &MicrophonePage::toggleMute);

#ifndef DCC_DISABLE_FEEDBACK
    //~ contents_path /sound/Microphone
    m_feedbackSlider = (new TitledSliderItem(tr("Input Level")));
    m_feedbackSlider->addBackground();
    DCCSlider *slider2 = m_feedbackSlider->slider();
    slider2->setRange(0, 100);
    slider2->setEnabled(false);
    slider2->setType(DCCSlider::Vernier);
    slider2->setTickPosition(QSlider::NoTicks);
    slider2->setLeftIcon(QIcon::fromTheme("dcc_feedbacklow"));
    slider2->setRightIcon(QIcon::fromTheme("dcc_feedbackhigh"));
    slider2->setIconSize(QSize(24, 24));
    slider2->setTickInterval(1);
    slider2->setPageStep(1);

    connect(m_model, &SoundModel::isPortEnableChanged, m_noiseReductionsw, &ComboxWidget::setVisible);
    m_conn = connect(m_model, &SoundModel::microphoneFeedbackChanged, [ = ](double vol2) {
        slider2->setSliderPosition(int(vol2 * 100));
    });
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &MicrophonePage::refreshIcon);
    connect(qApp, &DApplication::iconThemeChanged, this, &MicrophonePage::refreshIcon);
    m_layout->setSpacing(10);
    m_layout->insertWidget(4, m_feedbackSlider);
    m_layout->addStretch(10);
#endif

    refreshIcon();
    showDevice();
}

void MicrophonePage::refreshIcon()
{
    QString volumeString;
    if (m_mute) {
        volumeString = "muted";
    } else {
        volumeString = "low";
    }

    QString iconLeft = QString("audio-volume-%1-symbolic").arg(volumeString);

    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
        iconLeft.append("-dark");
    }
    const auto ratio = devicePixelRatioF();
    QPixmap  ret = loadSvg(iconLeft, ":/", ICON_SIZE, ratio);
    m_volumeBtn->setPixmap(ret);
}

const QPixmap MicrophonePage::loadSvg(const QString &iconName, const QString &localPath, const int size, const qreal ratio)
{
    QIcon icon = QIcon::fromTheme(iconName);
    if (!icon.isNull()) {
        QPixmap pixmap = icon.pixmap(int(size * ratio), int(size * ratio));
        pixmap.setDevicePixelRatio(ratio);
        return pixmap;
    }

    QPixmap pixmap(int(size * ratio), int(size * ratio));
    QString localIcon = QString("%1%2%3").arg(localPath).arg(iconName).arg(iconName.contains(".svg") ? "" : ".svg");
    QSvgRenderer renderer(localIcon);
    pixmap.fill(Qt::transparent);

    QPainter painter;
    painter.begin(&pixmap);
    renderer.render(&painter);
    painter.end();
    pixmap.setDevicePixelRatio(ratio);

    return pixmap;
}

void MicrophonePage::showDevice()
{
    if (!m_feedbackSlider || !m_inputSlider || !m_noiseReductionsw)
        return;
    if (m_inputModel->rowCount() > 0) {
        m_feedbackSlider->show();
        m_inputSlider->show();
        m_noiseReductionsw->show();
    } else {
        m_feedbackSlider->hide();
        m_inputSlider->hide();
        m_noiseReductionsw->hide();
    }
}
