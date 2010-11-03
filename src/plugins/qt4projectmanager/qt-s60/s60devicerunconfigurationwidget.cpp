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

#include "s60devicerunconfigurationwidget.h"
#include "s60devicerunconfiguration.h"

#include <utils/detailswidget.h>

#include <QtGui/QLineEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QFormLayout>

namespace Qt4ProjectManager {
namespace Internal {

S60DeviceRunConfigurationWidget::S60DeviceRunConfigurationWidget(
            S60DeviceRunConfiguration *runConfiguration,
            QWidget *parent)
    : QWidget(parent),
    m_runConfiguration(runConfiguration),
    m_detailsWidget(new Utils::DetailsWidget),
    m_argumentsLineEdit(new QLineEdit(m_runConfiguration->commandLineArguments().join(QString(QLatin1Char(' ')))))
{
    m_detailsWidget->setState(Utils::DetailsWidget::NoSummary);
    QVBoxLayout *mainBoxLayout = new QVBoxLayout();
    mainBoxLayout->setMargin(0);
    setLayout(mainBoxLayout);
    mainBoxLayout->addWidget(m_detailsWidget);
    QWidget *detailsContainer = new QWidget;
    m_detailsWidget->setWidget(detailsContainer);

    QVBoxLayout *detailsBoxLayout = new QVBoxLayout();
    detailsBoxLayout->setMargin(0);
    detailsContainer->setLayout(detailsBoxLayout);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setMargin(0);
    detailsBoxLayout->addLayout(formLayout);
    formLayout->addRow(tr("Arguments:"), m_argumentsLineEdit);

    connect(m_argumentsLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(argumentsEdited(QString)));

    connect(m_runConfiguration, SIGNAL(isEnabledChanged(bool)),
            this, SLOT(runConfigurationEnabledChange(bool)));

    setEnabled(m_runConfiguration->isEnabled());
}

void S60DeviceRunConfigurationWidget::argumentsEdited(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        m_runConfiguration->setCommandLineArguments(QStringList());
    } else {
        m_runConfiguration->setCommandLineArguments(trimmed.split(QLatin1Char(' '),
                                                                  QString::SkipEmptyParts));
    }
}

void S60DeviceRunConfigurationWidget::runConfigurationEnabledChange(bool enabled)
{
    setEnabled(enabled);
}

} // namespace Internal
} // namespace Qt4ProjectManager
