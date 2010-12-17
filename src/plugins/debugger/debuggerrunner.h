/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGERRUNNER_H
#define DEBUGGERRUNNER_H

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <projectexplorer/runconfiguration.h>

#include <QtCore/QScopedPointer>

namespace Utils {
class Environment;
}

namespace Debugger {
class DebuggerEngine;
class DebuggerRunControl;
class QmlEngine;
class DebuggerStartParameters;
struct DebuggerRunnerPrivate;

namespace Internal {
class GdbEngine;
class AbstractGdbAdapter;
}

//DEBUGGER_EXPORT QDebug operator<<(QDebug str, const DebuggerStartParameters &);

class DEBUGGER_EXPORT DebuggerRunControlFactory
    : public ProjectExplorer::IRunControlFactory
{
    Q_OBJECT

public:
    DebuggerRunControlFactory(QObject *parent, unsigned enabledEngines);

    // This is used by the "Non-Standard" scenarios, e.g. Attach to Core.
    // FIXME: What to do in case of a 0 runConfiguration?
    typedef ProjectExplorer::RunConfiguration RunConfiguration;
    typedef ProjectExplorer::RunControl RunControl;
    DebuggerRunControl *create(const DebuggerStartParameters &sp,
        RunConfiguration *runConfiguration = 0);

    // ProjectExplorer::IRunControlFactory
    // FIXME: Used by qmljsinspector.cpp:469
    RunControl *create(RunConfiguration *runConfiguration, const QString &mode);
    bool canRun(RunConfiguration *runConfiguration, const QString &mode) const;
private:
    QString displayName() const;
    QWidget *createConfigurationWidget(RunConfiguration *runConfiguration);

    unsigned m_enabledEngines;
};


// This is a job description containing all data "local" to the jobs, including
// the models of the individual debugger views.
class DEBUGGER_EXPORT DebuggerRunControl
    : public ProjectExplorer::RunControl
{
    Q_OBJECT

public:
    typedef ProjectExplorer::RunConfiguration RunConfiguration;
    explicit DebuggerRunControl(RunConfiguration *runConfiguration,
        unsigned enabledEngines, const DebuggerStartParameters &sp);
    ~DebuggerRunControl();

    // ProjectExplorer::RunControl
    virtual void start();
    virtual bool aboutToStop() const;
    virtual StopResult stop();
    virtual bool isRunning() const;
    QString displayName() const;

    void createEngine(const DebuggerStartParameters &startParameters);

    void setCustomEnvironment(Utils::Environment env);

    void startFailed();
    void debuggingFinished();
    RunConfiguration *runConfiguration() const;

    DebuggerState state() const;
    DebuggerEngine *engine();

    void showMessage(const QString &msg, int channel);

    void handleRemoteSetupDone();
    void handleRemoteSetupFailed(const QString &message);

    static bool checkDebugConfiguration(int toolChain,
                                 QString *errorMessage,
                                 QString *settingsCategory = 0,
                                 QString *settingsPage = 0);

signals:
    void engineRequestSetup();

public slots:
    void emitAddToOutputWindow(const QString &line, bool onStdErr);
    void emitAppendMessage(const QString &, bool isError);

private slots:
    void handleFinished();

protected:
    const DebuggerStartParameters &startParameters() const;

private:
    DebuggerEngineType engineForExecutable(unsigned enabledEngineTypes, const QString &executable);
    DebuggerEngineType engineForMode(unsigned enabledEngineTypes, DebuggerStartMode mode);
    void initGdbEngine(Internal::GdbEngine *engine);
    Internal::GdbEngine *gdbEngine() const;
    Internal::AbstractGdbAdapter *gdbAdapter() const;

    QScopedPointer<DebuggerRunnerPrivate> d;
};

} // namespace Debugger

#endif // DEBUGGERRUNNER_H
