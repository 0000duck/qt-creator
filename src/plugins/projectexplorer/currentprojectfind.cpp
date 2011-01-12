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

#include "currentprojectfind.h"

#include "projectexplorer.h"
#include "project.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QRegExp>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>

using namespace Find;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
using namespace TextEditor;

CurrentProjectFind::CurrentProjectFind(ProjectExplorerPlugin *plugin, SearchResultWindow *resultWindow)
  : BaseFileFind(resultWindow),
    m_plugin(plugin),
    m_configWidget(0)
{
    connect(m_plugin, SIGNAL(currentProjectChanged(ProjectExplorer::Project*)),
            this, SIGNAL(changed()));
}

QString CurrentProjectFind::id() const
{
    return QLatin1String("Current Project");
}

QString CurrentProjectFind::displayName() const
{
    return tr("Current Project");
}

bool CurrentProjectFind::isEnabled() const
{
    return m_plugin->currentProject() != 0 && BaseFileFind::isEnabled();
}

Utils::FileIterator *CurrentProjectFind::files()
{
    Project *project = m_plugin->currentProject();
    Q_ASSERT(project);
    QList<QRegExp> filterRegs;
    QStringList nameFilters = fileNameFilters();
    foreach (const QString &filter, nameFilters) {
        filterRegs << QRegExp(filter, Qt::CaseInsensitive, QRegExp::Wildcard);
    }
    QStringList files;
    if (!filterRegs.isEmpty()) {
        foreach (const QString &file, project->files(Project::AllFiles)) {
            foreach (const QRegExp &reg, filterRegs) {
                if (reg.exactMatch(file) || reg.exactMatch(QFileInfo(file).fileName())) {
                    files.append(file);
                    break;
                }
            }
        }
    } else {
        files += project->files(Project::AllFiles);
    }
    files.removeDuplicates();
    return new Utils::FileIterator(files);
}

QWidget *CurrentProjectFind::createConfigWidget()
{
    if (!m_configWidget) {
        m_configWidget = new QWidget;
        QGridLayout * const layout = new QGridLayout(m_configWidget);
        layout->setMargin(0);
        m_configWidget->setLayout(layout);
        QLabel * const filePatternLabel = new QLabel(tr("File &pattern:"));
        filePatternLabel->setMinimumWidth(80);
        filePatternLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        filePatternLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        QWidget *patternWidget = createPatternWidget();
        filePatternLabel->setBuddy(patternWidget);
        layout->addWidget(filePatternLabel, 0, 0, Qt::AlignRight);
        layout->addWidget(patternWidget, 0, 1);
        m_configWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    }
    return m_configWidget;
}

void CurrentProjectFind::writeSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("CurrentProjectFind"));
    writeCommonSettings(settings);
    settings->endGroup();
}

void CurrentProjectFind::readSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("CurrentProjectFind"));
    readCommonSettings(settings, QString(QLatin1Char('*')));
    settings->endGroup();
}
