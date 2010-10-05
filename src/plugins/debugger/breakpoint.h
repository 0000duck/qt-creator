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

#ifndef DEBUGGER_BREAKPOINT_H
#define DEBUGGER_BREAKPOINT_H

#include <QtCore/QMetaType>
#include <QtCore/QList>
#include <QtCore/QString>

namespace Debugger {
namespace Internal {

class BreakpointMarker;
class BreakHandler;

//////////////////////////////////////////////////////////////////
//
// BreakpointData
//
//////////////////////////////////////////////////////////////////

class BreakpointData
{
public:
    BreakpointData();
    ~BreakpointData();

    void removeMarker();
    void updateMarker();
    QString toToolTip() const;
    BreakHandler *handler() { return m_handler; }
    void reinsertBreakpoint();
    void clear(); // Delete all generated data.

    bool isLocatedAt(const QString &fileName, int lineNumber,
        bool useMarkerPosition) const;
    bool isSimilarTo(const BreakpointData *needle) const;
    bool conditionsMatch() const;

    // This copies only the static data.
    BreakpointData *clone() const;

    // Generic name for function to break on 'throw'
    static const char *throwFunction;
    static const char *catchFunction;

private:
    // Intentionally unimplemented.
    // Making it copyable is tricky because of the markers.
    BreakpointData(const BreakpointData &);
    void operator=(const BreakpointData &);

    // Our owner
    BreakHandler *m_handler; // Not owned.
    friend class BreakHandler;

public:
    enum Type { BreakpointType, WatchpointType };

    bool enabled;            // Should we talk to the debugger engine?
    bool pending;            // Does the debugger engine know about us already?
    Type type;               // Type of breakpoint.

    // This "user requested information" will get stored in the session.
    QString fileName;        // Short name of source file.
    QByteArray condition;    // Condition associated with breakpoint.
    int ignoreCount;         // Ignore count associated with breakpoint.
    int lineNumber;          // Line in source file.
    quint64 address;         // Address for watchpoints.
    QByteArray threadSpec;   // Thread specification.
    // Name of containing function, special values:
    // BreakpointData::throwFunction, BreakpointData::catchFunction
    QString funcName;
    bool useFullPath;        // Should we use the full path when setting the bp?

    // This is what gdb produced in response.
    QByteArray bpNumber;     // Breakpoint number assigned by the debugger engine.
    QByteArray bpCondition;  // Condition acknowledged by the debugger engine.
    int bpIgnoreCount;       // Ignore count acknowledged by the debugger engine.
    QString bpFileName;      // File name acknowledged by the debugger engine.
    QString bpFullName;      // Full file name acknowledged by the debugger engine.
    int bpLineNumber; // Line number acknowledged by the debugger engine.
    int bpCorrectedLineNumber; // Acknowledged by the debugger engine.
    QByteArray bpThreadSpec; // Thread spec acknowledged by the debugger engine.
    QString bpFuncName;      // Function name acknowledged by the debugger engine.
    quint64 bpAddress;       // Address acknowledged by the debugger engine.
    bool bpMultiple;         // Happens in constructors/gdb.
    bool bpEnabled;          // Enable/disable command sent.
    QByteArray bpState;      // gdb: <PENDING>, <MULTIPLE>

    void setMarkerFileName(const QString &fileName);
    QString markerFileName() const { return m_markerFileName; }

    void setMarkerLineNumber(int lineNumber);
    int markerLineNumber() const { return m_markerLineNumber; }

    bool isSetByFunction() const { return !funcName.isEmpty(); }
    bool isSetByFileAndLine() const { return !fileName.isEmpty(); }

private:
    // Taken from either user input or gdb responses.
    QString m_markerFileName; // Used to locate the marker.
    int m_markerLineNumber;

    // Our red blob in the editor.
    BreakpointMarker *marker;
};

typedef QList<BreakpointData *> Breakpoints;

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::BreakpointData *);

#endif // DEBUGGER_BREAKPOINT_H
