/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef DEBUGGER_PLAINGDBADAPTER_H
#define DEBUGGER_PLAINGDBADAPTER_H

#include "abstractgdbadapter.h"
#include "gdbengine.h"
#include "outputcollector.h"

#include <consoleprocess.h>

#include <QtCore/QDebug>
#include <QtCore/QProcess>

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// PlainGdbAdapter
//
///////////////////////////////////////////////////////////////////////

class PlainGdbAdapter : public AbstractGdbAdapter
{
    Q_OBJECT

public:
    PlainGdbAdapter(GdbEngine *engine, QObject *parent = 0);

    //void kill() { m_gdbProc.kill(); }
    //void terminate() { m_gdbProc.terminate(); }
    QString errorString() const { return m_gdbProc.errorString(); }
    QByteArray readAllStandardError() { return m_gdbProc.readAllStandardError(); }
    QByteArray readAllStandardOutput() { return m_gdbProc.readAllStandardOutput(); }
    qint64 write(const char *data) { return m_gdbProc.write(data); }
    void setWorkingDirectory(const QString &dir) { m_gdbProc.setWorkingDirectory(dir); }
    void setEnvironment(const QStringList &env) { m_gdbProc.setEnvironment(env); }
    bool isAdapter() const { return false; }
    void interruptInferior();

    void startAdapter(const DebuggerStartParametersPtr &sp);
    void prepareInferior();
    void startInferior();
    void shutdownInferior();
    void shutdownAdapter();

private:
    void handleFileExecAndSymbols(const GdbResultRecord &, const QVariant &);
    void handleKill(const GdbResultRecord &, const QVariant &);
    void handleExit(const GdbResultRecord &, const QVariant &);
    void handleStubAttached(const GdbResultRecord &, const QVariant &);
    void handleExecRun(const GdbResultRecord &response, const QVariant &);
    void handleInfoTarget(const GdbResultRecord &response, const QVariant &);

    void debugMessage(const QString &msg) { m_engine->debugMessage(msg); }
    void emitAdapterStartFailed(const QString &msg);
    Q_SLOT void handleGdbFinished(int, QProcess::ExitStatus);
    Q_SLOT void handleGdbStarted();
    Q_SLOT void stubStarted();
    Q_SLOT void stubError(const QString &msg);

    QProcess m_gdbProc;
    DebuggerStartParametersPtr m_startParameters;
    Core::Utils::ConsoleProcess m_stubProc;
    OutputCollector m_outputCollector;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_PLAINGDBADAPTER_H
