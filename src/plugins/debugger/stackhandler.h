/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef DEBUGGER_STACKHANDLER_H
#define DEBUGGER_STACKHANDLER_H

#include "stackframe.h"

#include <QtCore/QAbstractItemModel>

namespace Debugger {
namespace Internal {

////////////////////////////////////////////////////////////////////////
//
// StackCookie
//
////////////////////////////////////////////////////////////////////////

struct StackCookie
{
    StackCookie() : isFull(true), gotoLocation(false) {}
    StackCookie(bool full, bool jump) : isFull(full), gotoLocation(jump) {}
    bool isFull;
    bool gotoLocation;
};


////////////////////////////////////////////////////////////////////////
//
// StackModel
//
////////////////////////////////////////////////////////////////////////

/*! A model to represent the stack in a QTreeView. */
class StackHandler : public QAbstractTableModel
{
    Q_OBJECT

public:
    StackHandler();
    ~StackHandler();

    void setFrames(const StackFrames &frames, bool canExpand = false);
    const StackFrames &frames() const;
    void setCurrentIndex(int index);
    int currentIndex() const { return m_currentIndex; }
    StackFrame currentFrame() const;
    const StackFrame &frameAt(int index) const { return m_stackFrames.at(index); }
    int stackSize() const { return m_stackFrames.size(); }
    quint64 topAddress() const { return m_stackFrames.at(0).address; }

    // Called from StackHandler after a new stack list has been received
    void removeAll();
    QAbstractItemModel *model() { return this; }
    bool isContentsValid() const { return m_contentsValid; }
    void scheduleResetLocation();
    void resetLocation();

private:
    // QAbstractTableModel
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    Q_SLOT void resetModel() { reset(); }

    StackFrames m_stackFrames;
    int m_currentIndex;
    const QVariant m_positionIcon;
    const QVariant m_emptyIcon;
    bool m_canExpand;
    bool m_resetLocationScheduled;
    bool m_contentsValid;
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::StackCookie)


#endif // DEBUGGER_STACKHANDLER_H
