#ifndef QMLGDBENGINE_H
#define QMLGDBENGINE_H

#include "debuggerengine.h"

#include <QtCore/QScopedPointer>

namespace Core {
class IEditor;
}

namespace Debugger {

struct QmlCppEnginePrivate;

class DEBUGGER_EXPORT QmlCppEngine : public DebuggerEngine
{
    Q_OBJECT
public:
    explicit QmlCppEngine(const DebuggerStartParameters &sp);
    virtual ~QmlCppEngine();

    void setActiveEngine(DebuggerLanguage language);

    virtual void setToolTipExpression(const QPoint &mousePos,
        TextEditor::ITextEditor * editor, int cursorPos);
    virtual void updateWatchData(const Internal::WatchData &data,
        const Internal::WatchUpdateFlags &flags);

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
    virtual unsigned debuggerCapabilities() const;

    virtual bool isSynchronous() const;
    virtual QByteArray qtNamespace() const;

    virtual void createSnapshot();
    virtual void updateAll();

    virtual void attemptBreakpointSynchronization();
    virtual bool acceptsBreakpoint(const Internal::BreakpointData *br);
    virtual void selectThread(int index);

    virtual void assignValueInDebugger(const Internal::WatchData *w,
        const QString &expr, const QVariant &value);

    QAbstractItemModel *modulesModel() const;
    QAbstractItemModel *registerModel() const;
    QAbstractItemModel *stackModel() const;
    QAbstractItemModel *threadsModel() const;
    QAbstractItemModel *localsModel() const;
    QAbstractItemModel *watchersModel() const;
    QAbstractItemModel *returnModel() const;
    QAbstractItemModel *sourceFilesModel() const;

    DebuggerEngine *cppEngine() const;

protected:
    virtual void detachDebugger();
    virtual void executeStep();
    virtual void executeStepOut();
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

    virtual void notifyInferiorRunOk();

protected:
    virtual void setupEngine();
    virtual void setupInferior();
    virtual void runEngine();
    virtual void shutdownInferior();
    virtual void shutdownEngine();

private slots:
    void masterEngineStateChanged(const DebuggerState &state);
    void slaveEngineStateChanged(const DebuggerState &state);
    void setupSlaveEngine();
    void editorChanged(Core::IEditor *editor);

private:
    void setupSlaveEngineOnTimer();
    void finishDebugger();
    void handleSlaveEngineStateChange(const DebuggerState &newState);
    void handleSlaveEngineStateChangeAsActive(const DebuggerState &newState);

private:
    QScopedPointer<QmlCppEnginePrivate> d;
};

} // namespace Debugger

#endif // QMLGDBENGINE_H
