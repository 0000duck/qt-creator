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

#ifndef DEBUGGER_DBG_WINUTILS_H
#define DEBUGGER_DBG_WINUTILS_H

#include <QtCore/QList>

QT_FORWARD_DECLARE_CLASS(QString)

namespace Debugger {
namespace Internal {

struct ProcData; // debuggerdialogs, used by the process listing dialogs

QList<ProcData> winProcessList();

// Resume a suspended thread by id.
bool winResumeThread(unsigned long dwThreadId, QString *errorMessage);

// Open a process by PID and break into it.
bool winDebugBreakProcess(unsigned long  pid, QString *errorMessage);

unsigned long winGetCurrentProcessId();

/* Helper for (case-)normalizing file names:
 * Determine normalized case of a Windows file name (camelcase.cpp -> CamelCase.cpp)
 * as the debugger reports lower case file names.
 * Restriction: File needs to exist and be non-empty and will be to be opened/mapped.
 * The result should be cached as the function can be extensive. */

QString winNormalizeFileName(const QString &f);

bool isWinProcessBeingDebugged(unsigned long pid);

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_DBG_WINUTILS_H
