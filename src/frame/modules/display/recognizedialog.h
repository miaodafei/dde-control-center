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

#ifndef RECOGNIZEDIALOG_H
#define RECOGNIZEDIALOG_H

#include <QDialog>

namespace dcc {

namespace display {

class Monitor;
class DisplayModel;
class RecognizeDialog : public QDialog
{
    Q_OBJECT

public:
    enum DialogModel { TouchRecognizeDialog , DisplayRecognizeDialog };
    explicit RecognizeDialog(DisplayModel *model, DialogModel dialogModel, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *);

private Q_SLOTS:
    void onScreenRectChanged();

private:
    void paintMonitorMark(QPainter &painter, const QRect &rect, const QString &name);
    void paintMonitorMark(QPainter &painter, const QRect &rect, const QString &name, const QString &manufacturer);
    const QScreen *screenForGeometry(const QRect &rect) const;

private:
    DisplayModel *m_model;
    DialogModel m_dialogModel;
};

} // namespace display

} // namespace dcc

#endif // RECOGNIZEDIALOG_H
