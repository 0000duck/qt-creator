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

#include "abstractprocess.h"

#include <windows.h>

namespace Utils {

QStringList AbstractProcess::fixWinEnvironment(const QStringList &env)
{
    QStringList envStrings = env;
    // add PATH if necessary (for DLL loading)
    if (envStrings.filter(QRegExp(QLatin1String("^PATH="),Qt::CaseInsensitive)).isEmpty()) {
        QByteArray path = qgetenv("PATH");
        if (!path.isEmpty())
            envStrings.prepend(QString(QLatin1String("PATH=%1")).arg(QString::fromLocal8Bit(path)));
    }
    // add systemroot if needed
    if (envStrings.filter(QRegExp(QLatin1String("^SystemRoot="),Qt::CaseInsensitive)).isEmpty()) {
        QByteArray systemRoot = qgetenv("SystemRoot");
        if (!systemRoot.isEmpty())
            envStrings.prepend(QString(QLatin1String("SystemRoot=%1")).arg(QString::fromLocal8Bit(systemRoot)));
    }
    return envStrings;
}

QString AbstractProcess::createWinCommandline(const QString &program, const QStringList &args)
{
    const QChar doubleQuote = QLatin1Char('"');
    const QChar blank = QLatin1Char(' ');
    const QChar backSlash = QLatin1Char('\\');

    QString programName = program;
    if (!programName.startsWith(doubleQuote) && !programName.endsWith(doubleQuote) && programName.contains(blank)) {
        programName.insert(0, doubleQuote);
        programName.append(doubleQuote);
    }
    // add the prgram as the first arrg ... it works better
    programName.replace(QLatin1Char('/'), backSlash);
    QString cmdLine = programName;
    if (args.empty())
        return cmdLine;

    cmdLine += blank;
    for (int i = 0; i < args.size(); ++i) {
        QString tmp = args.at(i);
        // in the case of \" already being in the string the \ must also be escaped
        tmp.replace(QLatin1String("\\\""), QLatin1String("\\\\\""));
        // escape a single " because the arguments will be parsed
        tmp.replace(QString(doubleQuote), QLatin1String("\\\""));
        if (tmp.isEmpty() || tmp.contains(blank) || tmp.contains('\t')) {
            // The argument must not end with a \ since this would be interpreted
            // as escaping the quote -- rather put the \ behind the quote: e.g.
            // rather use "foo"\ than "foo\"
            QString endQuote(doubleQuote);
            int i = tmp.length();
            while (i > 0 && tmp.at(i - 1) == backSlash) {
                --i;
                endQuote += backSlash;
            }
            cmdLine += QLatin1String(" \"");
            cmdLine += tmp.left(i);
            cmdLine += endQuote;
        } else {
            cmdLine += blank;
            cmdLine += tmp;
        }
    }
    return cmdLine;
}

QByteArray AbstractProcess::createWinEnvironment(const QStringList &env)
{
    QByteArray envlist;
    int pos = 0;
    foreach (const QString &tmp, env) {
        const uint tmpSize = sizeof(TCHAR) * (tmp.length() + 1);
        envlist.resize(envlist.size() + tmpSize);
        memcpy(envlist.data() + pos, tmp.utf16(), tmpSize);
        pos += tmpSize;
    }
    envlist.resize(envlist.size() + 2);
    envlist[pos++] = 0;
    envlist[pos++] = 0;
    return envlist;
}

} //namespace Utils
