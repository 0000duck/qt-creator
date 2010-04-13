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

#include "targetsetuppage.h"

#include "qt4project.h"
#include "qt4projectmanagerconstants.h"
#include "qt4target.h"

#include <utils/pathchooser.h>

#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QTreeWidget>

using namespace Qt4ProjectManager::Internal;

TargetSetupPage::TargetSetupPage(QWidget *parent) :
    QWizardPage(parent),
    m_preferMobile(false)
{
    resize(500, 400);
    setTitle(tr("Set up targets for your project"));

    QVBoxLayout *vbox = new QVBoxLayout(this);

    QLabel * importLabel = new QLabel(this);
    importLabel->setText(tr("Qt Creator can set up the following targets:"));
    importLabel->setWordWrap(true);

    vbox->addWidget(importLabel);

    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setColumnCount(3);
    m_treeWidget->header()->setResizeMode(0, QHeaderView::ResizeToContents);
    m_treeWidget->header()->setResizeMode(1, QHeaderView::ResizeToContents);
    m_treeWidget->setHeaderLabels(QStringList() << tr("Qt Version") << tr("Status") << tr("Directory"));
    vbox->addWidget(m_treeWidget);

    QHBoxLayout *hbox = new QHBoxLayout;
    m_directoryLabel = new QLabel(this);
    m_directoryLabel->setText(tr("Scan for builds"));
    hbox->addWidget(m_directoryLabel);

    m_directoryChooser = new Utils::PathChooser(this);
    m_directoryChooser->setPromptDialogTitle(tr("Directory to import builds from"));
    m_directoryChooser->setExpectedKind(Utils::PathChooser::Directory);
    hbox->addWidget(m_directoryChooser);
    vbox->addLayout(hbox);

    connect(m_directoryChooser, SIGNAL(changed(QString)),
            this, SLOT(importDirectoryAdded(QString)));
}

TargetSetupPage::~TargetSetupPage()
{
    resetInfos();
}

void TargetSetupPage::setImportInfos(const QList<ImportInfo> &infos)
{
    disconnect(m_treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
               this, SLOT(itemWasChanged()));

    // Clean up!
    resetInfos();

    // Find possible targets:
    QStringList targets;
    foreach (const ImportInfo &i, infos) {
        // Make sure we have no duplicate directories:
        bool skip = false;
        foreach (const ImportInfo &j, m_infos) {
            if (j.isExistingBuild && i.isExistingBuild && (j.directory == i.directory)) {
                skip = true;
                break;
            }
        }
        if (skip) {
            if (i.isTemporary)
                delete i.version;
            continue;
        }

        m_infos.append(i);

        QSet<QString> versionTargets = i.version->supportedTargetIds();
        foreach (const QString &t, versionTargets) {
            if (!targets.contains(t))
                targets.append(t);
        }
    }
    qSort(targets.begin(), targets.end());

    Qt4TargetFactory factory;
    foreach (const QString &t, targets) {
        QTreeWidgetItem *targetItem = new QTreeWidgetItem(m_treeWidget);
        targetItem->setText(0, factory.displayNameForId(t));
        targetItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        targetItem->setData(0, Qt::UserRole, t);
        targetItem->setExpanded(true);

        int pos = -1;
        foreach (const ImportInfo &i, m_infos) {
            ++pos;

            if (!i.version->supportsTargetId(t))
                continue;
            QTreeWidgetItem *versionItem = new QTreeWidgetItem(targetItem);
            // Column 0:
            versionItem->setText(0, i.version->displayName());
            versionItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            versionItem->setData(0, Qt::UserRole, pos);
            // Prefer imports to creating new builds, but precheck any
            // Qt that exists (if there is no import with that version)
            bool shouldCheck = true;
            if (!i.isExistingBuild) {
                foreach (const ImportInfo &j, m_infos) {
                    if (j.isExistingBuild && j.version == i.version) {
                        shouldCheck = false;
                        break;
                    }
                }
            }
            shouldCheck = shouldCheck && (m_preferMobile == i.version->supportsMobileTarget());
            shouldCheck = shouldCheck || i.isExistingBuild; // always check imports
            shouldCheck = shouldCheck || m_infos.count() == 1; // always check only option
            versionItem->setCheckState(0, shouldCheck ? Qt::Checked : Qt::Unchecked);

            // Column 1 (status):
            versionItem->setText(1, i.isExistingBuild ? tr("Import", "Is this an import of an existing build or a new one?") :
                                                        tr("New", "Is this an import of an existing build or a new one?"));

            // Column 2 (directory):
            versionItem->setText(2, QDir::toNativeSeparators(i.directory));
        }
    }

    connect(m_treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this, SLOT(itemWasChanged()));

    emit completeChanged();
}

QList<TargetSetupPage::ImportInfo> TargetSetupPage::importInfos() const
{
    return m_infos;
}

bool TargetSetupPage::hasSelection() const
{
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem * current = m_treeWidget->topLevelItem(i);
        for (int j = 0; j < current->childCount(); ++j) {
            QTreeWidgetItem * child = current->child(j);
            if (child->checkState(0) == Qt::Checked)
                return true;
        }
    }
    return false;
}

bool TargetSetupPage::isTargetSelected(const QString &targetid) const
{
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem * current = m_treeWidget->topLevelItem(i);
        if (current->data(0, Qt::UserRole).toString() != targetid)
            continue;
        for (int j = 0; j < current->childCount(); ++j) {
            QTreeWidgetItem * child = current->child(j);
            if (child->checkState(0) == Qt::Checked)
                return true;
        }
    }
    return false;
}

bool TargetSetupPage::setupProject(Qt4ProjectManager::Qt4Project *project)
{
    Q_ASSERT(project->targets().isEmpty());
    QtVersionManager *vm = QtVersionManager::instance();

    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *current = m_treeWidget->topLevelItem(i);
        QString targetId = current->data(0, Qt::UserRole).toString();

        QList<BuildConfigurationInfo> targetInfos;
        for (int j = 0; j < current->childCount(); ++j) {
            QTreeWidgetItem *child = current->child(j);
            if (child->checkState(0) != Qt::Checked)
                continue;

            ImportInfo &info = m_infos[child->data(0, Qt::UserRole).toInt()];

            // Register temporary Qt version
            if (info.isTemporary) {
                vm->addVersion(info.version);
                info.isTemporary = false;
            }

            if ((info.buildConfig | QtVersion::DebugBuild) != info.buildConfig)
                targetInfos.append(BuildConfigurationInfo(info.version, QtVersion::QmakeBuildConfigs(info.buildConfig | QtVersion::DebugBuild),
                                                          info.additionalArguments, info.directory));
            targetInfos.append(BuildConfigurationInfo(info.version, info.buildConfig,
                                                      info.additionalArguments, info.directory));
        }

        // create the target:
        Qt4Target *target = 0;
        if (!targetInfos.isEmpty())
            target = project->targetFactory()->create(project, targetId, targetInfos);

        if (target)
            project->addTarget(target);
    }

    // Create the default target if nothing else was set up:
    if (project->targets().isEmpty()) {
        Qt4Target *target = project->targetFactory()->create(project, Constants::DESKTOP_TARGET_ID);
        if (target)
            project->addTarget(target);
    }

    return !project->targets().isEmpty();
}

void TargetSetupPage::itemWasChanged()
{
    emit completeChanged();
}

bool TargetSetupPage::isComplete() const
{
    return hasSelection();
}

void TargetSetupPage::setImportDirectoryBrowsingEnabled(bool browsing)
{
    m_directoryChooser->setEnabled(browsing);
    m_directoryChooser->setVisible(browsing);
    m_directoryLabel->setVisible(browsing);
}

void TargetSetupPage::setImportDirectoryBrowsingLocation(const QString &directory)
{
    m_directoryChooser->setInitialBrowsePathBackup(directory);
}

void TargetSetupPage::setShowLocationInformation(bool location)
{
    m_treeWidget->setColumnCount(location ? 3 : 1);
}

void TargetSetupPage::setPreferMobile(bool mobile)
{
    m_preferMobile = mobile;
}

void TargetSetupPage::setProFilePath(const QString &path)
{
    m_proFilePath = path;
}

QList<TargetSetupPage::ImportInfo>
TargetSetupPage::importInfosForKnownQtVersions(Qt4ProjectManager::Qt4Project *project)
{
    QList<ImportInfo> results;
    QtVersionManager * vm = QtVersionManager::instance();
    QList<QtVersion *> validVersions = vm->validVersions();
    // Fallback in case no valid versions are found:
    if (validVersions.isEmpty())
        validVersions.append(vm->versions().at(0)); // there is always one!
    foreach (QtVersion *v, validVersions) {
        ImportInfo info;
        if (project) {
            if (v->supportsShadowBuilds())
                info.directory = project->defaultTopLevelBuildDirectory();
            else
                info.directory = project->projectDirectory();
        }
        info.isExistingBuild = false;
        info.isTemporary = false;
        info.version = v;
        results.append(info);
    }
    return results;
}

QList<TargetSetupPage::ImportInfo> TargetSetupPage::filterImportInfos(const QSet<QString> &validTargets,
                                                                      const QList<ImportInfo> &infos)
{
    QList<ImportInfo> results;
    foreach (const ImportInfo &info, infos) {
        Q_ASSERT(info.version);
        foreach (const QString &target, validTargets) {
            if (info.version->supportsTargetId(target))
                results.append(info);
        }
    }
    return results;
}

QList<TargetSetupPage::ImportInfo>
TargetSetupPage::recursivelyCheckDirectoryForBuild(const QString &directory, const QString &proFile, int maxdepth)
{
    QList<ImportInfo> results;

    if (maxdepth <= 0)
        return results;

    // Check for in-source builds first:
    QString qmakeBinary = QtVersionManager::findQMakeBinaryFromMakefile(directory);

    // Recurse into subdirectories:
    if (qmakeBinary.isNull() || !QtVersionManager::makefileIsFor(directory, proFile)) {
        QStringList subDirs = QDir(directory).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        foreach (QString subDir, subDirs)
            results.append(recursivelyCheckDirectoryForBuild(QDir::cleanPath(directory + QChar('/') + subDir),
                                                             proFile, maxdepth - 1));
        return results;
    }

    // Shiny fresh directory with a Makefile...
    QtVersionManager * vm = QtVersionManager::instance();
    TargetSetupPage::ImportInfo info;
    info.directory = directory;

    // This also means we have a build in there
    // First get the qt version
    info.version = vm->qtVersionForQMakeBinary(qmakeBinary);
    info.isExistingBuild = true;

    // Okay does not yet exist, create
    if (!info.version) {
        info.version = new QtVersion(qmakeBinary);
        info.isTemporary = true;
    }

    QPair<QtVersion::QmakeBuildConfigs, QStringList> result =
            QtVersionManager::scanMakeFile(directory, info.version->defaultBuildConfig());
    info.buildConfig = result.first;
    info.additionalArguments = Qt4BuildConfiguration::removeSpecFromArgumentList(result.second);

    QString parsedSpec = Qt4BuildConfiguration::extractSpecFromArgumentList(result.second, directory, info.version);
    QString versionSpec = info.version->mkspec();

    // Compare mkspecs and add to additional arguments
    if (parsedSpec.isEmpty() || parsedSpec == versionSpec || parsedSpec == "default") {
        // using the default spec, don't modify additional arguments
    } else {
        info.additionalArguments.prepend(parsedSpec);
        info.additionalArguments.prepend("-spec");
    }

    results.append(info);
    return results;
}

void TargetSetupPage::importDirectoryAdded(const QString &directory)
{
    QFileInfo dir(directory);
    if (!dir.exists() || !dir.isDir())
        return;
    m_directoryChooser->setPath(QString());
    QList<ImportInfo> tmp = m_infos;
    m_infos.clear(); // Clear m_infos without deleting temporary QtVersions!
    tmp.append(recursivelyCheckDirectoryForBuild(directory, m_proFilePath));
    setImportInfos(tmp);
}

void TargetSetupPage::resetInfos()
{
    m_treeWidget->clear();
    foreach (const ImportInfo &info, m_infos) {
        if (info.isTemporary)
            delete info.version;
    }
    m_infos.clear();
}
