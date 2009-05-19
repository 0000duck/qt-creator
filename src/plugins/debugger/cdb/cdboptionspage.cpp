/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "cdboptionspage.h"
#include "cdboptions.h"
#include "debuggerconstants.h"

#include <coreplugin/icore.h>

#include <QtCore/QCoreApplication>
#include <QtGui/QMessageBox>

const char * const CDB_SETTINGS_ID = QT_TRANSLATE_NOOP("Debugger::Internal::CdbOptionsPageWidget", "Cdb");

namespace Debugger {
namespace Internal {

CdbOptionsPageWidget::CdbOptionsPageWidget(QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.pathChooser->setExpectedKind(Core::Utils::PathChooser::Directory);
    m_ui.pathChooser->addButton(tr("Autodetect"), this, SLOT(autoDetect()));
    m_ui.failureLabel->setVisible(false);
}


void CdbOptionsPageWidget::setOptions(CdbOptions &o)
{
    m_ui.pathChooser->setPath(o.path);
    m_ui.cdbOptionsGroupBox->setChecked(o.enabled);
    m_ui.symbolPathListEditor->setPathList(o.symbolPaths);
    m_ui.sourcePathListEditor->setPathList(o.sourcePaths);
}

CdbOptions CdbOptionsPageWidget::options() const
{
    CdbOptions  rc;
    rc.path = m_ui.pathChooser->path();
    rc.enabled = m_ui.cdbOptionsGroupBox->isChecked();
    rc.symbolPaths = m_ui.symbolPathListEditor->pathList();
    rc.sourcePaths = m_ui.sourcePathListEditor->pathList();
    return rc;
}

void CdbOptionsPageWidget::autoDetect()
{
    QString path;
    QStringList checkedDirectories;
    const bool ok = CdbOptions::autoDetectPath(&path, &checkedDirectories);
    m_ui.cdbOptionsGroupBox->setChecked(ok);
    if (ok) {
        m_ui.pathChooser->setPath(path);
    } else {
        const QString msg = tr("\"Debugging Tools for Windows\" could not be found.");
        const QString details = tr("Checked:\n%1").arg(checkedDirectories.join(QString(QLatin1Char('\n'))));
        QMessageBox msbBox(QMessageBox::Information, tr("Autodetection"), msg, QMessageBox::Ok, this);
        msbBox.setDetailedText(details);
        msbBox.exec();
    }
}

void CdbOptionsPageWidget::setFailureMessage(const QString &msg)
{
    m_ui.failureLabel->setText(msg);
    m_ui.failureLabel->setVisible(!msg.isEmpty());
}

// ---------- CdbOptionsPage
CdbOptionsPage::CdbOptionsPage(const QSharedPointer<CdbOptions> &options) :
        m_options(options)
{
}

CdbOptionsPage::~CdbOptionsPage()
{
}

QString CdbOptionsPage::settingsId()
{
    return QLatin1String(CDB_SETTINGS_ID);
}

QString CdbOptionsPage::trName() const
{
    return tr(CDB_SETTINGS_ID);
}

QString CdbOptionsPage::category() const
{
    return QLatin1String(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY);
}

QString CdbOptionsPage::trCategory() const
{
    return QCoreApplication::translate("Debugger", Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY);
}

QWidget *CdbOptionsPage::createPage(QWidget *parent)
{
    m_widget = new CdbOptionsPageWidget(parent);
    m_widget->setOptions(*m_options);
    m_widget->setFailureMessage(m_failureMessage);
    return m_widget;
}

void CdbOptionsPage::apply()
{
    if (!m_widget)
        return;
    const CdbOptions newOptions = m_widget->options();
    if (const unsigned changedMask = m_options->compare(newOptions)) {
        *m_options = newOptions;
        m_options->toSettings(Core::ICore::instance()->settings());
        if (changedMask & CdbOptions::DebuggerPathsChanged)
            emit debuggerPathsChanged();
    }
}

void CdbOptionsPage::finish()
{
}

} // namespace Internal
} // namespace Debugger
