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

#ifndef DEBUGGER_DEBUGGERENGINE_H
#define DEBUGGER_DEBUGGERENGINE_H

#include "debugger_global.h"
#include "debuggerconstants.h"
#include "moduleshandler.h" // For 'Symbols'

#include <coreplugin/ssh/sshconnection.h> 

#include <QtCore/QObject>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QDebug;
class QPoint;
class QMessageBox;
QT_END_NAMESPACE

namespace TextEditor {
class ITextEditor;
}

namespace Core {
class IOptionsPage;
}

namespace Debugger {

class DebuggerEnginePrivate;
class DebuggerPlugin;
class DebuggerPluginPrivate;
class DebuggerRunControl;
class QmlCppEngine;

class DEBUGGER_EXPORT DebuggerStartParameters
{
public:
    DebuggerStartParameters();
    void clear();

    QString executable;
    QString displayName;
    QString coreFile;
    bool isSnapshot; // set if created internally
    QStringList processArgs;
    QStringList environment;
    QString workingDirectory;
    qint64 attachPID;
    bool useTerminal;
    bool breakAtMain;
    QString crashParameter; // for AttachCrashedExternal

    // for qml debugging
    QString qmlServerAddress;
    quint16 qmlServerPort;
    QString projectBuildDir;
    QString projectDir;
    // for cpp+qml debugging
    DebuggerEngineType cppEngineType;

    // for remote debugging
    QString remoteChannel;
    QString remoteArchitecture;
    QString gnuTarget;
    QString symbolFileName;
    bool useServerStartScript;
    QString serverStartScript;
    QString sysRoot;
    QByteArray remoteDumperLib;
    QByteArray remoteSourcesDir;
    QString remoteMountPoint;
    QString localMountDir;
    Core::SshConnectionParameters connParams;

    QString debuggerCommand;
    int toolChainType;
    QString qtInstallPath;

    QString dumperLibrary;
    QStringList dumperLibraryLocations;
    DebuggerStartMode startMode;

    // for SymbianOS debugging
    quint32 executableUid;
};

DEBUGGER_EXPORT QDebug operator<<(QDebug str, const DebuggerStartParameters &);
DEBUGGER_EXPORT QDebug operator<<(QDebug str, DebuggerState state);

namespace Internal {

class DisassemblerViewAgent;
class MemoryViewAgent;
class Symbol;
class WatchData;
class BreakHandler;
class ModulesHandler;
class RegisterHandler;
class StackHandler;
class StackFrame;
class SourceFilesHandler;
class ThreadsHandler;
class WatchHandler;
class BreakpointData;

struct WatchUpdateFlags
{
    WatchUpdateFlags() : tryIncremental(false) {}
    bool tryIncremental;
};

} // namespace Internal


// FIXME: DEBUGGER_EXPORT?
class DEBUGGER_EXPORT DebuggerEngine : public QObject
{
    Q_OBJECT

public:
    explicit DebuggerEngine(const DebuggerStartParameters &sp);
    virtual ~DebuggerEngine();

    virtual void setToolTipExpression(const QPoint & mousePos,
        TextEditor::ITextEditor *editor, int cursorPos);
    void initializeFromTemplate(DebuggerEngine *other);

    virtual void updateWatchData(const Internal::WatchData &data,
        const Internal::WatchUpdateFlags & flags = Internal::WatchUpdateFlags());
    void startDebugger(DebuggerRunControl *runControl);

    virtual void watchPoint(const QPoint &);
    virtual void fetchMemory(Internal::MemoryViewAgent *, QObject *,
                             quint64 addr, quint64 length);
    virtual void fetchDisassembler(Internal::DisassemblerViewAgent *);
    virtual void activateFrame(int index);

    virtual void reloadModules();
    virtual void examineModules();
    virtual void loadSymbols(const QString &moduleName);
    virtual void loadAllSymbols();
    virtual void requestModuleSymbols(const QString &moduleName);

    virtual void reloadRegisters();
    virtual void reloadSourceFiles();
    virtual void reloadFullStack();

    virtual void setRegisterValue(int regnr, const QString &value);
    virtual void addOptionPages(QList<Core::IOptionsPage*> *) const;
    virtual unsigned debuggerCapabilities() const;

    virtual bool isSynchronous() const;
    virtual QByteArray qtNamespace() const;

    virtual void createSnapshot();
    virtual void updateAll();

    virtual void attemptBreakpointSynchronization();
    virtual bool acceptsBreakpoint(const Internal::BreakpointData *);

    virtual void addBreakpoint(const Internal::BreakpointData &bp);
    virtual void notifyAddBreakpointOk(quint64 id);
    virtual void notifyAddBreakpointFailed(quint64 id);
    virtual void removeBreakpoint(quint64 id);
    virtual void notifyRemoveBreakpointOk(quint64 id);
    virtual void notifyRemoveBreakpointFailed(quint64 id);
    virtual void changeBreakpoint(const Internal::BreakpointData &bp);
    virtual void notifyChangeBreakpointOk(quint64 id);
    virtual void notifyChangeBreakpointFailed(quint64 id);
    virtual void notifyBreakpointAdjusted(const  Internal::BreakpointData &bp);

    virtual void selectThread(int index);

    virtual void assignValueInDebugger(const Internal::WatchData *data,
        const QString &expr, const QVariant &value);
    virtual void removeTooltip();

    // Convenience
    static QMessageBox *showMessageBox
        (int icon, const QString &title, const QString &text, int buttons = 0);

protected:
    friend class DebuggerPluginPrivate;
    virtual void detachDebugger();
    virtual void exitDebugger();
    virtual void executeStep();
    virtual void executeStepOut() ;
    virtual void executeNext();
    virtual void executeStepI();
    virtual void executeNextI();
    virtual void executeReturn();

    virtual void continueInferior();
    virtual void interruptInferior();
    virtual void requestInterruptInferior();

    virtual void executeRunToLine(const QString &fileName, int lineNumber);
    virtual void executeRunToFunction(const QString &functionName);
    virtual void executeJumpToLine(const QString &fileName, int lineNumber);
    virtual void executeDebuggerCommand(const QString &command);

    virtual void frameUp();
    virtual void frameDown();

public:
    static DebuggerPlugin *plugin();
    const DebuggerStartParameters &startParameters() const;
    DebuggerStartParameters &startParameters();

    Internal::ModulesHandler *modulesHandler() const;
    Internal::RegisterHandler *registerHandler() const;
    Internal::StackHandler *stackHandler() const;
    Internal::ThreadsHandler *threadsHandler() const;
    Internal::WatchHandler *watchHandler() const;
    Internal::SourceFilesHandler *sourceFilesHandler() const;
    Internal::BreakHandler *breakHandler() const;

    virtual QAbstractItemModel *modulesModel() const;
    virtual QAbstractItemModel *registerModel() const;
    virtual QAbstractItemModel *stackModel() const;
    virtual QAbstractItemModel *threadsModel() const;
    virtual QAbstractItemModel *localsModel() const;
    virtual QAbstractItemModel *watchersModel() const;
    virtual QAbstractItemModel *returnModel() const;
    virtual QAbstractItemModel *sourceFilesModel() const;

    void progressPing();
    void handleFinished();
    void handleStartFailed();
    bool debuggerActionsEnabled() const;
    static bool debuggerActionsEnabled(DebuggerState state);
    void showModuleSymbols(const QString &moduleName, const Internal::Symbols &symbols);

    void breakByFunction(const QString &functionName);
    void breakByFunctionMain();
    
    DebuggerState state() const;
    DebuggerState lastGoodState() const;
    DebuggerState targetState() const;
    bool isDying() const;

    // Dumper stuff (common to cdb and gdb).
    bool qtDumperLibraryEnabled() const;
    QString qtDumperLibraryName() const;
    QStringList qtDumperLibraryLocations() const;
    void showQtDumperLibraryWarning(const QString &details);

    static const char *stateName(int s);

    void notifyInferiorPid(qint64 pid);
    qint64 inferiorPid() const;
    bool isReverseDebugging() const;
    bool isActive() const; // True if this is the currently active engine;
    void setActive(bool on); // FIXME: private?
    void handleCommand(int role, const QVariant &value);

    // Convenience
    Q_SLOT void showMessage(const QString &msg, int channel = LogDebug, int timeout = -1) const;
    Q_SLOT void showStatusMessage(const QString &msg, int timeout = -1) const;

    void resetLocation();
    virtual void gotoLocation(const QString &fileName, int lineNumber, bool setMarker);
    virtual void gotoLocation(const Internal::StackFrame &frame, bool setMarker);
    virtual void quitDebugger(); // called by DebuggerRunControl

    virtual void updateViews();
    bool isSlaveEngine() const;

signals:
    void stateChanged(const DebuggerState &state);
    void updateViewsRequested();

protected:
    // The base notify*() function implementation should be sufficient
    // in most cases, but engines are free to override them to do some
    // engine specific cleanup like stopping timers etc.
    virtual void notifyEngineSetupOk();
    virtual void notifyEngineSetupFailed();
    virtual void notifyEngineRunFailed();

    virtual void notifyInferiorSetupOk();
    virtual void notifyInferiorSetupFailed();

    virtual void notifyEngineRunAndInferiorRunOk();
    virtual void notifyEngineRunAndInferiorStopOk();
    virtual void notifyInferiorUnrunnable(); // Called by CoreAdapter.

    // Use notifyInferiorRunRequested() plus notifyInferiorRunOk() instead.
    //virtual void notifyInferiorSpontaneousRun();

    virtual void notifyInferiorRunRequested();
    virtual void notifyInferiorRunOk();
    virtual void notifyInferiorRunFailed();

    virtual void notifyInferiorStopOk();
    virtual void notifyInferiorSpontaneousStop();
    virtual void notifyInferiorStopFailed();
    virtual void notifyInferiorExited();

    virtual void notifyInferiorShutdownOk();
    virtual void notifyInferiorShutdownFailed();

    virtual void notifyEngineSpontaneousShutdown();
    virtual void notifyEngineShutdownOk();
    virtual void notifyEngineShutdownFailed();

    virtual void notifyInferiorIll();
    virtual void notifyEngineIll();

    virtual void setupEngine() = 0;
    virtual void setupInferior() = 0;
    virtual void runEngine() = 0;
    virtual void shutdownInferior() = 0;
    virtual void shutdownEngine() = 0;

    void setState(DebuggerState state, bool forced = false);
    void setSlaveEngine(bool value);

protected:
    DebuggerRunControl *runControl() const;

private:
    // wrapper engine needs access to state of its subengines
    friend class QmlCppEngine;

    friend class DebuggerEnginePrivate;
    DebuggerEnginePrivate *d;
};

} // namespace Debugger

#endif // DEBUGGER_DEBUGGERENGINE_H
