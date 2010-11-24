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

#ifndef DEBUGGER_AGENTS_H
#define DEBUGGER_AGENTS_H

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QPointer>
#include <QtCore/QVector>

#include "disassemblerlines.h"

namespace Core {
class IEditor;
}

namespace Debugger {

class DebuggerEngine;

namespace Internal {

class StackFrame;
class DisassemblerViewAgent;
class DisassemblerViewAgentPrivate;

class MemoryViewAgent : public QObject
{
    Q_OBJECT

public:
    // Called from Gui
    explicit MemoryViewAgent(Debugger::DebuggerEngine *engine, quint64 startaddr);
    explicit MemoryViewAgent(Debugger::DebuggerEngine *engine, const QString &startaddr);
    ~MemoryViewAgent();

    enum { BinBlockSize = 1024 };

public slots:
    // Called from Engine
    void addLazyData(QObject *editorToken, quint64 addr, const QByteArray &data);

private:
    Q_SLOT void createBinEditor(quint64 startAddr);
    Q_SLOT void fetchLazyData(Core::IEditor *, quint64 block, bool sync);
    Q_SLOT void provideNewRange(Core::IEditor *editor, quint64 address);
    Q_SLOT void handleStartOfFileRequested(Core::IEditor *editor);
    Q_SLOT void handleEndOfFileRequested(Core::IEditor *editor);

    QList<QPointer<Core::IEditor> > m_editors;
    QPointer<Debugger::DebuggerEngine> m_engine;
};

class DisassemblerViewAgent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString mimeType READ mimeType WRITE setMimeType)
public:
    // Called from Gui
    explicit DisassemblerViewAgent(Debugger::DebuggerEngine *engine);
    ~DisassemblerViewAgent();

    void setFrame(const StackFrame &frame, bool tryMixed, bool setMarker);
    const StackFrame &frame() const;
    void resetLocation();
    void setContents(const DisassemblerLines &contents);

    // Mimetype: "text/a-asm" or some specialized architecture
    QString mimeType() const;
    Q_SLOT void setMimeType(const QString &mt);

    quint64 address() const;
    bool contentsCoversAddress(const QString &contents) const;
    void cleanup();
    bool isMixed() const;

    // Return address of an assembly line "0x0dfd  bla"
    static quint64 addressFromDisassemblyLine(const QString &data);
private:
    DisassemblerViewAgentPrivate *d;
};


} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_WATCHWINDOW_H
