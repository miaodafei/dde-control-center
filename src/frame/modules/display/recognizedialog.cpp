/*
 * Copyright (C) 2011 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
 *
 * Maintainer: sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
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

#include "recognizedialog.h"
#include "displaymodel.h"

#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QApplication>

using namespace dcc::display;

const int AUTOHIDE_DELAY = 1000 * 5;
const int HORIZENTAL_MARGIN = 30;
const int VERTICAL_MARGIN = 20;
const int FONT_SIZE = 20;
const int RADIUS = 5;

RecognizeDialog::RecognizeDialog(DisplayModel *model, RecognizeDialog::DialogModel dialogModel, QWidget *parent)
    : QDialog(parent)
    , m_model(model)
    , m_dialogModel(dialogModel)
{
    connect(m_model, &DisplayModel::screenHeightChanged, this, &RecognizeDialog::onScreenRectChanged);
    connect(m_model, &DisplayModel::screenWidthChanged, this, &RecognizeDialog::onScreenRectChanged);

    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::X11BypassWindowManagerHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    onScreenRectChanged();

    // set auto hide timer
    QTimer *autoHideTimer = new QTimer(this);
    autoHideTimer->setInterval(AUTOHIDE_DELAY);
    connect(autoHideTimer, &QTimer::timeout, this, &RecognizeDialog::accept);
    autoHideTimer->start();
}

void RecognizeDialog::paintEvent(QPaintEvent *)
{
    QPen p;
    p.setWidth(1);
    p.setColor(Qt::black);

    QBrush b;
    b.setColor(Qt::white);
    b.setStyle(Qt::SolidPattern);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(p);
    painter.setBrush(b);

    if (m_model->monitorsIsIntersect()) {
        const QRect intersectRect = QRect(0, 0, m_model->monitorList()[0]->w(), m_model->monitorList()[0]->h());
        QString intersectName = m_model->monitorList().first()->name();
        for (int i(1); i != m_model->monitorList().size(); ++i)
        {
            if (m_dialogModel == TouchRecognizeDialog) {
                paintMonitorMark(painter, m_model->monitorList()[i]->rect(),
                                 m_model->monitorList()[i]->name(), m_model->monitorList()[i]->manufacturer());
            } else if (m_dialogModel == DisplayRecognizeDialog) {
                intersectName += "=" + m_model->monitorList()[i]->name();
                paintMonitorMark(painter, intersectRect, intersectName);
            }
        }

    } else {
        for (auto mon : m_model->monitorList()) {
            if (m_dialogModel == TouchRecognizeDialog) {
                paintMonitorMark(painter, mon->rect(), mon->name(), mon->manufacturer());
            } else if (m_dialogModel == DisplayRecognizeDialog) {
                paintMonitorMark(painter, mon->rect(), mon->name());
            }
        }
    }
}

void RecognizeDialog::onScreenRectChanged()
{
    const auto ratio = devicePixelRatioF();

    QRect r(0, 0, m_model->screenWidth(), m_model->screenHeight());

    const QScreen *screen = screenForGeometry(r);

    if (screen) {
        const QRect screenGeo = screen->geometry();
        r.moveTopLeft(screenGeo.topLeft() + (r.topLeft() - screenGeo.topLeft()) / ratio);
    }

    setGeometry(r);

    update();
}

void RecognizeDialog::paintMonitorMark(QPainter &painter, const QRect &rect, const QString &name)
{
    const qreal ratio = devicePixelRatioF();
    const QRect r(rect.topLeft() / ratio, rect.size() / ratio);
    QFont font;
    font.setPixelSize(20);
    QFontMetrics standartFm(font);
    //以宽度的百分之80计算像素大小
    int fontXSize = int(r.width() / standartFm.width(name) * 16);

    //字体太高的，将字体缩小
    if(r.height() / 5  < fontXSize) {
        fontXSize = r.height() / 5;
    }
    font.setPixelSize(fontXSize);
    QFontMetrics fm(font);
    const int x = r.center().x() - fm.width(name) / 2;
    const int y = r.center().y() + fm.height() / 4;
    QPainterPath path;
    path.addText(x, y, font, name);
    painter.drawPath(path);
}

void RecognizeDialog::paintMonitorMark(QPainter &painter, const QRect &rect, const QString &name, const QString &manufacturer)
{
    int line = 2;
    const qreal ratio = devicePixelRatioF();
    const QRect r(rect.topLeft() / ratio, rect.size() / ratio);
    QFont font;
    font.setPixelSize(20);
    const QFontMetrics fm(font);

    int textWidth = fm.width(name) > fm.width(manufacturer) ? fm.width(name) : fm.width(manufacturer);

    QPainterPath path;
    path.addText(r.center().x() - fm.width(manufacturer) / 2, r.center().y() - fm.height() / 2, font, manufacturer);
    path.addText(r.center().x() - fm.width(name) / 2, r.center().y() + fm.height() / 2, font, name);

    QPen backgroundPen(QColor(255, 255, 255, 40));
    painter.setPen(backgroundPen);
    QBrush backgroundBrush(QColor(0, 0, 0, 200), Qt::SolidPattern);
    painter.setBrush(backgroundBrush);

    const int rectX = r.center().x() - textWidth / 2 - HORIZENTAL_MARGIN;
    const int rectY = r.center().y() - fm.height() * line / 2 - VERTICAL_MARGIN;
    QRect rec(rectX, rectY, textWidth + HORIZENTAL_MARGIN * 2, fm.height() * line + VERTICAL_MARGIN * 2);
    painter.drawRoundedRect(rec, RADIUS, RADIUS);

    QPen textPen(QColor(255, 255, 255));
    painter.setPen(textPen);
    QBrush textBrush(QColor(255, 255, 255), Qt::SolidPattern);
    painter.setBrush(textBrush);

    painter.drawPath(path);
}

const QScreen *RecognizeDialog::screenForGeometry(const QRect &rect) const
{
    const qreal ratio = qApp->devicePixelRatio();

    for (const auto *s : qApp->screens())
    {
        const QRect &g(s->geometry());
        const QRect realRect(g.topLeft() / ratio, g.size());

        if (realRect.contains(rect.center()))
            return s;
    }

    return nullptr;
}
