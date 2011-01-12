/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "rvcttoolchain.h"
#include "rvctparser.h"

#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>

#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager::Internal;

static const char rvctBinaryC[] = "armcc";

RVCTToolChain::RVCTToolChain(const S60Devices::Device &device, ToolChain::ToolChainType type) :
    m_mixin(device),
    m_type(type),
    m_versionUpToDate(false),
    m_major(0),
    m_minor(0),
    m_build(0)
{
}

// Return the environment variable indicating the RVCT version
// 'RVCT<major><minor>BIN'
QByteArray RVCTToolChain::rvctBinEnvironmentVariable()
{
    static QByteArray binVar;
    // Grep the environment list
    if (binVar.isEmpty()) {
        const QRegExp regex(QLatin1String("^(RVCT\\d\\dBIN)=.*$"));
        QTC_ASSERT(regex.isValid(), return QByteArray());
        foreach(const QString &v, QProcessEnvironment::systemEnvironment().toStringList()) {
            if (regex.exactMatch(v)) {
                binVar = regex.cap(1).toLocal8Bit();
                break;
            }
        }
    }
    return binVar;
}

// Return binary path as pointed to by RVCT<X><X>BIN
QString RVCTToolChain::rvctBinPath()
{
    static QString binPath;
    if (binPath.isEmpty()) {
        const QByteArray binVar = rvctBinEnvironmentVariable();
        if (!binVar.isEmpty()) {
            const QByteArray binPathB = qgetenv(binVar);
            if (!binPathB.isEmpty()) {
                const QFileInfo fi(QString::fromLocal8Bit(binPathB));
                if (fi.isDir())
                    binPath = fi.absoluteFilePath();
            }
        }
    }
    return binPath;
}

// Return binary expanded by path or resort to PATH
QString RVCTToolChain::rvctBinary()
{
    QString executable = QLatin1String(rvctBinaryC);
#ifdef Q_OS_WIN
    executable += QLatin1String(".exe");
#endif
    const QString binPath = rvctBinPath();
    return binPath.isEmpty() ? executable : (binPath + QLatin1Char('/') + executable);
}

ToolChain::ToolChainType RVCTToolChain::type() const
{
    return m_type;
}

void RVCTToolChain::updateVersion()
{
    if (m_versionUpToDate)
        return;

    m_versionUpToDate = true;
    m_major = 0;
    m_minor = 0;
    m_build = 0;
    QProcess armcc;
    Utils::Environment env = Utils::Environment::systemEnvironment();
    addToEnvironment(env);
    armcc.setEnvironment(env.toStringList());
    const QString binary = rvctBinary();
    armcc.start(rvctBinary(), QStringList());
    if (!armcc.waitForStarted()) {
        qWarning("Unable to run rvct binary '%s' when trying to determine version.", qPrintable(binary));
        return;
    }
    armcc.closeWriteChannel();
    if (!armcc.waitForFinished()) {
        Utils::SynchronousProcess::stopProcess(armcc);
        qWarning("Timeout running rvct binary '%s' trying to determine version.", qPrintable(binary));
        return;
    }
    if (armcc.exitStatus() != QProcess::NormalExit) {
        qWarning("A crash occurred when running rvct binary '%s' trying to determine version.", qPrintable(binary));
        return;
    }
    QString versionLine = QString::fromLocal8Bit(armcc.readAllStandardOutput());
    versionLine += QString::fromLocal8Bit(armcc.readAllStandardError());
    const QRegExp versionRegExp(QLatin1String("RVCT(\\d*)\\.(\\d*).*\\[Build.(\\d*)\\]"),
                                Qt::CaseInsensitive);
    QTC_ASSERT(versionRegExp.isValid(), return);
    if (versionRegExp.indexIn(versionLine) != -1) {
        m_major = versionRegExp.cap(1).toInt();
	m_minor = versionRegExp.cap(2).toInt();
	m_build = versionRegExp.cap(3).toInt();
    }
}

QByteArray RVCTToolChain::predefinedMacros()
{
    // see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0205f/Babbacdb.html
    updateVersion();
    QByteArray ba = QString::fromLatin1(
        "#define __arm__arm__\n"
        "#define __ARMCC_VERSION %1%2%3%4\n"
        "#define __ARRAY_OPERATORS\n"
        "#define _BOOL\n"
        "#define c_plusplus\n"
        "#define __cplusplus\n"
        "#define __CC_ARM\n"
        "#define __EDG__\n"
        "#define __STDC__\n"
        "#define __STDC_VERSION__\n"
        "#define __TARGET_FEATURE_DOUBLEWORD\n"
        "#define __TARGET_FEATURE_DSPMUL\n"
        "#define __TARGET_FEATURE_HALFWORD\n"
        "#define __TARGET_FEATURE_THUMB\n"
        "#define _WCHAR_T\n"
        "#define __SYMBIAN32__\n"
        ).arg(m_major, 1, 10, QLatin1Char('0'))
        .arg(m_minor, 1, 10, QLatin1Char('0'))
        .arg("0")
        .arg(m_build, 3, 10, QLatin1Char('0')).toLatin1();
    return ba;
}

QList<HeaderPath> RVCTToolChain::systemHeaderPaths()
{
    if (m_systemHeaderPaths.isEmpty()) {
        updateVersion();
        Utils::Environment env = Utils::Environment::systemEnvironment();
        QString rvctInclude = env.value(QString::fromLatin1("RVCT%1%2INC").arg(m_major).arg(m_minor));
        if (!rvctInclude.isEmpty())
            m_systemHeaderPaths.append(HeaderPath(rvctInclude, HeaderPath::GlobalHeaderPath));
        switch (m_type) {
        case ProjectExplorer::ToolChain::RVCT_ARMV5_GNUPOC:
            m_systemHeaderPaths += m_mixin.gnuPocRvctHeaderPaths(m_major, m_minor);
            break;
        default:
            m_systemHeaderPaths += m_mixin.epocHeaderPaths();
            break;
        }
    }
    return m_systemHeaderPaths;
}

static inline QStringList headerPathToStringList(const QList<ProjectExplorer::HeaderPath> &hl)
{
    QStringList rc;
    foreach(const ProjectExplorer::HeaderPath &hp, hl)
        rc.push_back(hp.path());
    return rc;
}

// Expand an RVCT variable, such as RVCT22BIN, by some new values
void RVCTToolChain::addToRVCTPathVariable(const QString &postfix, const QStringList &values,
                                  Utils::Environment &env) const
{
    // get old values
    const QChar separator = QLatin1Char(',');
    const QString variable = QString::fromLatin1("RVCT%1%2%3").arg(m_major).arg(m_minor).arg(postfix);
    const QString oldValueS = env.value(variable);
    const QStringList oldValue = oldValueS.isEmpty() ? QStringList() : oldValueS.split(separator);
    // merge new values
    QStringList newValue = oldValue;
    foreach(const QString &v, values) {
        const QString normalized = QDir::toNativeSeparators(v);
        if (!newValue.contains(normalized))
            newValue.push_back(normalized);
    }
    if (newValue != oldValue)
        env.set(variable, newValue.join(QString(separator)));
}

// Figure out lib path via
QStringList RVCTToolChain::libPaths()
{
    const QByteArray binLocation = qgetenv(rvctBinEnvironmentVariable());
    if (binLocation.isEmpty())
        return QStringList();
    const QString pathRoot = QFileInfo(QString::fromLocal8Bit(binLocation)).path();
    QStringList rc;
    rc.push_back(pathRoot + QLatin1String("/lib"));
    rc.push_back(pathRoot + QLatin1String("/lib/armlib"));
    return rc;
}

void RVCTToolChain::addToEnvironment(Utils::Environment &env)
{
    updateVersion();
    switch (m_type) {
    case ProjectExplorer::ToolChain::RVCT_ARMV5_GNUPOC: {
        m_mixin.addGnuPocToEnvironment(&env);
        // setup RVCT22INC, LIB
        addToRVCTPathVariable(QLatin1String("INC"),
                      headerPathToStringList(m_mixin.gnuPocRvctHeaderPaths(m_major, m_minor)),
                      env);
        addToRVCTPathVariable(QLatin1String("LIB"),
                              libPaths() + m_mixin.gnuPocRvctLibPaths(5, true),
                              env);
        // Add rvct to path and set locale to 'C'
        const QString binPath = rvctBinPath();
        if (!binPath.isEmpty())
            env.prependOrSetPath(binPath);
        env.set(QLatin1String("LANG"), QString(QLatin1Char('C')));
    }
        break;
    default:
        m_mixin.addEpocToEnvironment(&env);
        break;
    }
    // we currently support RVCT v2.2 only:
    env.set(QLatin1String("QT_RVCT_VERSION"), QLatin1String("2.2"));
}

QString RVCTToolChain::makeCommand() const
{
    return QLatin1String("make");
}

ProjectExplorer::IOutputParser *RVCTToolChain::outputParser() const
{
    return new RvctParser;
}

bool RVCTToolChain::equals(const ToolChain *otherIn) const
{
    if (otherIn->type() != type())
        return false;
    const RVCTToolChain *other = static_cast<const RVCTToolChain *>(otherIn);
    return other->m_mixin == m_mixin;
}

