/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#ifndef OUTPUTWINDOW_H
#define OUTPUTWINDOW_H

#include "core_global.h"
#include "icontext.h"

#include <utils/outputformat.h>

#include <QPlainTextEdit>
#include <QTimer>
#include <QTime>

namespace Utils { class OutputFormatter; }

namespace Core {

namespace Internal { class OutputWindowPrivate; }

class CORE_EXPORT OutputWindow : public QPlainTextEdit
{
    Q_OBJECT

public:
    OutputWindow(Context context, QWidget *parent = 0);
    ~OutputWindow();

    Utils::OutputFormatter* formatter() const;
    void setFormatter(Utils::OutputFormatter *formatter);

    void appendMessage(const QString &out, Utils::OutputFormat format);
    /// appends a \p text using \p format without using formater
    void appendText(const QString &text, const QTextCharFormat &format = QTextCharFormat());

    void grayOutOldContent();
    void clear();
    //OPENMV-DIFF//
    void save();
    void setTabSettings(int tabWidth);
    //OPENMV-DIFF//

    void showEvent(QShowEvent *);

    void scrollToBottom();

    void setMaxLineCount(int count);
    int maxLineCount() const;

    void setBaseFont(const QFont &newFont);
    float fontZoom() const;
    void setFontZoom(float zoom);
    void setWheelZoomEnabled(bool enabled);

signals:
    void wheelZoom();
    //OPENMV-DIFF//
    void writeBytes(const QByteArray &data);
    //OPENMV-DIFF//

public slots:
    void setWordWrapEnabled(bool wrap);

protected:
    bool isScrollbarAtBottom() const;

    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void resizeEvent(QResizeEvent *e);
    virtual void keyPressEvent(QKeyEvent *ev);
    virtual void wheelEvent(QWheelEvent *e);

private:
    using QPlainTextEdit::setFont; // call setBaseFont instead, which respects the zoom factor
    QTimer m_scrollTimer;
    QTime m_lastMessage;
    void enableUndoRedo();
    QString doNewlineEnforcement(const QString &out);

    Internal::OutputWindowPrivate *d;
};

} // namespace Core

#endif // OUTPUTWINDOW_H
