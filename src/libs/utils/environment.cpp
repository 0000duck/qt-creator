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

#include "environment.h"

#include <QtCore/QProcess>
#include <QtCore/QDir>
#include <QtCore/QString>

using namespace Utils;

QList<EnvironmentItem> EnvironmentItem::fromStringList(QStringList list)
{
    QList<EnvironmentItem> result;
    foreach (const QString &string, list) {
        int pos = string.indexOf(QLatin1Char('='));
        if (pos == -1) {
            EnvironmentItem item(string, QString());
            item.unset = true;
            result.append(item);
        } else {
            EnvironmentItem item(string.left(pos), string.mid(pos+1));
            result.append(item);
        }
    }
    return result;
}

QStringList EnvironmentItem::toStringList(QList<EnvironmentItem> list)
{
    QStringList result;
    foreach (const EnvironmentItem &item, list) {
        if (item.unset)
            result << QString(item.name);
        else
            result << QString(item.name + '=' + item.value);
    }
    return result;
}

Environment::Environment()
{
}

Environment::Environment(QStringList env)
{
    foreach (const QString &s, env) {
        int i = s.indexOf("=");
        if (i >= 0) {
#ifdef Q_OS_WIN
            m_values.insert(s.left(i).toUpper(), s.mid(i+1));
#else
            m_values.insert(s.left(i), s.mid(i+1));
#endif
        }
    }
}

QStringList Environment::toStringList() const
{
    QStringList result;
    const QMap<QString, QString>::const_iterator end = m_values.constEnd();
    for (QMap<QString, QString>::const_iterator it = m_values.constBegin(); it != end; ++it) {
        QString entry = it.key();
        entry += QLatin1Char('=');
        entry += it.value();
        result.push_back(entry);
    }
    return result;
}

void Environment::set(const QString &key, const QString &value)
{
#ifdef Q_OS_WIN
    QString _key = key.toUpper();
#else
    const QString &_key = key;
#endif
    m_values.insert(_key, value);
}

void Environment::unset(const QString &key)
{
#ifdef Q_OS_WIN
    QString _key = key.toUpper();
#else
    const QString &_key = key;
#endif
    m_values.remove(_key);
}

void Environment::appendOrSet(const QString &key, const QString &value, const QString &sep)
{
#ifdef Q_OS_WIN
    QString _key = key.toUpper();
#else
    const QString &_key = key;
#endif
    QMap<QString, QString>::iterator it = m_values.find(key);
    if (it == m_values.end()) {
        m_values.insert(_key, value);
    } else {
        // Append unless it is already there
        const QString toAppend = sep + value;
        if (!it.value().endsWith(toAppend))
            it.value().append(toAppend);
    }
}

void Environment::prependOrSet(const QString&key, const QString &value, const QString &sep)
{
#ifdef Q_OS_WIN
    QString _key = key.toUpper();
#else
    const QString &_key = key;
#endif
    QMap<QString, QString>::iterator it = m_values.find(key);
    if (it == m_values.end()) {
        m_values.insert(_key, value);
    } else {
        // Prepend unless it is already there
        const QString toPrepend = value + sep;
        if (!it.value().startsWith(toPrepend))
            it.value().prepend(toPrepend);
    }
}

void Environment::appendOrSetPath(const QString &value)
{
#ifdef Q_OS_WIN
    const QChar sep = QLatin1Char(';');
#else
    const QChar sep = QLatin1Char(':');
#endif
    appendOrSet(QLatin1String("PATH"), QDir::toNativeSeparators(value), QString(sep));
}

void Environment::prependOrSetPath(const QString &value)
{
#ifdef Q_OS_WIN
    const QChar sep = QLatin1Char(';');
#else
    const QChar sep = QLatin1Char(':');
#endif
    prependOrSet(QLatin1String("PATH"), QDir::toNativeSeparators(value), QString(sep));
}

Environment Environment::systemEnvironment()
{
    return Environment(QProcess::systemEnvironment());
}

void Environment::clear()
{
    m_values.clear();
}

QString Environment::searchInPath(const QString &executable,
                                  const QStringList & additionalDirs) const
{
    QString exec = expandVariables(executable);

    if (exec.isEmpty() || QFileInfo(exec).isAbsolute())
        return QDir::toNativeSeparators(exec);

    // Check in directories:
    foreach (const QString &dir, additionalDirs) {
        if (dir.isEmpty())
            continue;
        QFileInfo fi(dir + QLatin1Char('/') + exec);
        if (fi.isFile() && fi.isExecutable())
            return fi.absoluteFilePath();
    }

    // Check in path:
    if (exec.indexOf(QChar('/')) != -1)
        return QString();
    const QChar slash = QLatin1Char('/');
    foreach (const QString &p, path()) {
        QString fp = p;
        fp += slash;
        fp += exec;
        const QFileInfo fi(fp);
        if (fi.exists())
            return fi.absoluteFilePath();
    }

#ifdef Q_OS_WIN
    if (!exec.endsWith(QLatin1String(".exe"))) {
        exec.append(QLatin1String(".exe"));
        return searchInPath(exec, additionalDirs);
    }
#endif
    return QString();
}

QStringList Environment::path() const
{
#ifdef Q_OS_WIN
    const QChar sep = QLatin1Char(';');
#else
    const QChar sep = QLatin1Char(':');
#endif
    return m_values.value(QLatin1String("PATH")).split(sep, QString::SkipEmptyParts);
}

QString Environment::value(const QString &key) const
{
    return m_values.value(key);
}

QString Environment::key(Environment::const_iterator it) const
{
    return it.key();
}

QString Environment::value(Environment::const_iterator it) const
{
    return it.value();
}

Environment::const_iterator Environment::constBegin() const
{
    return m_values.constBegin();
}

Environment::const_iterator Environment::constEnd() const
{
    return m_values.constEnd();
}

Environment::const_iterator Environment::find(const QString &name)
{
    QMap<QString, QString>::const_iterator it = m_values.constFind(name);
    if (it == m_values.constEnd())
        return constEnd();
    else
        return it;
}

int Environment::size() const
{
    return m_values.size();
}

void Environment::modify(const QList<EnvironmentItem> & list)
{
    Environment resultEnvironment = *this;
    foreach (const EnvironmentItem &item, list) {
        if (item.unset) {
            resultEnvironment.unset(item.name);
        } else {
            // TODO use variable expansion
            QString value = item.value;
            for (int i=0; i < value.size(); ++i) {
                if (value.at(i) == QLatin1Char('$')) {
                    if ((i + 1) < value.size()) {
                        const QChar &c = value.at(i+1);
                        int end = -1;
                        if (c == '(')
                            end = value.indexOf(')', i);
                        else if (c == '{')
                            end = value.indexOf('}', i);
                        if (end != -1) {
                            const QString &name = value.mid(i+2, end-i-2);
                            Environment::const_iterator it = find(name);
                            if (it != constEnd())
                                value.replace(i, end-i+1, it.value());
                        }
                    }
                }
            }
            resultEnvironment.set(item.name, value);
        }
    }
    *this = resultEnvironment;
}

bool Environment::hasKey(const QString &key)
{
    return m_values.contains(key);
}

bool Environment::operator!=(const Environment &other) const
{
    return !(*this == other);
}

bool Environment::operator==(const Environment &other) const
{
    return m_values == other.m_values;
}

QStringList Environment::parseCombinedArgString(const QString &program)
{
    QStringList args;
    QString tmp;
    int quoteCount = 0;
    bool inQuote = false;

    // handle quoting. tokens can be surrounded by double quotes
    // "hello world". three consecutive double quotes represent
    // the quote character itself.
    for (int i = 0; i < program.size(); ++i) {
        if (program.at(i) == QLatin1Char('"')) {
            ++quoteCount;
            if (quoteCount == 3) {
                // third consecutive quote
                quoteCount = 0;
                tmp += program.at(i);
            }
            continue;
        }
        if (quoteCount) {
            if (quoteCount == 1)
                inQuote = !inQuote;
            quoteCount = 0;
        }
        if (!inQuote && program.at(i).isSpace()) {
            if (!tmp.isEmpty()) {
                args += tmp;
                tmp.clear();
            }
        } else {
            tmp += program.at(i);
        }
    }
    if (!tmp.isEmpty())
        args += tmp;
    return args;
}

QString Environment::joinArgumentList(const QStringList &arguments)
{
    QString result;
    const QChar doubleQuote = QLatin1Char('"');
    foreach (QString arg, arguments) {
        if (!result.isEmpty())
            result += QLatin1Char(' ');
        arg.replace(QString(doubleQuote), QLatin1String("\"\"\""));
        if (arg.contains(QLatin1Char(' '))) {
            arg.insert(0, doubleQuote);
            arg += doubleQuote;
        }
        result += arg;
    }
    return result;
}

enum State { BASE, VARIABLE, OPTIONALVARIABLEBRACE, STRING };

/** Expand environment variables in a string.
 *
 * Environment variables are accepted in the following forms:
 * $SOMEVAR, ${SOMEVAR} and %SOMEVAR%.
 *
 * Strings enclosed in '"' characters do not get varaibles
 * substituted.
 */
QString Environment::expandVariables(const QString &input) const
{
    QString result;
    QString variable;
    QChar endVariable;
    State state = BASE;

    int length = input.count();
    for (int i = 0; i < length; ++i) {
        QChar c = input.at(i);
        if (state == BASE) {
            if (c == '$') {
                state = OPTIONALVARIABLEBRACE;
                variable.clear();
                endVariable = QChar(0);
            } else if (c == '%') {
                state = VARIABLE;
                variable.clear();
                endVariable = '%';
            } else if (c == '\"') {
                state = STRING;
                result += c;
            } else {
                result += c;
            }
        } else if (state == VARIABLE) {
            if (c == endVariable) {
                result += value(variable);
                state = BASE;
            } else if (c.isLetterOrNumber() || c == '_') {
                variable += c;
            } else {
                result += value(variable);
                result += c;
                state = BASE;
            }
        } else if (state == OPTIONALVARIABLEBRACE) {
            if (c == '{')
                endVariable = '}';
            else
                variable = c;
            state = VARIABLE;
        } else if (state == STRING) {
            if (c == '\"') {
                state = BASE;
                result += c;
            } else {
                result += c;
            }
        }
    }
    if (state == VARIABLE)
        result += value(variable);
    return result;
}

QStringList Environment::expandVariables(const QStringList &variables) const
{
    QStringList results;
    foreach (const QString & i, variables)
        results << expandVariables(i);
    return results;
}
