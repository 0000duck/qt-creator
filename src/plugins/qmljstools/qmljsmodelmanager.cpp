/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmljsmodelmanager.h"
#include "qmljstoolsconstants.h"
#include "qmljssemanticinfo.h"
#include "qmljsbundleprovider.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <cpptools/cppmodelmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljsfindexportedcpptypes.h>
#include <qmljs/qmljsplugindumper.h>
#include <qtsupport/qmldumptool.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <texteditor/textdocument.h>
#include <utils/hostosinfo.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <utils/runextensions.h>
#include <QTextDocument>
#include <QTextStream>
#include <QTimer>
#include <QRegExp>
#include <QLibraryInfo>
#include <qglobal.h>

using namespace Core;
using namespace QmlJS;
using namespace QmlJSTools;
using namespace QmlJSTools::Internal;


ModelManagerInterface::ProjectInfo QmlJSTools::Internal::ModelManager::defaultProjectInfoForProject(
        ProjectExplorer::Project *project) const
{
    ModelManagerInterface::ProjectInfo projectInfo(project);
    ProjectExplorer::Target *activeTarget = 0;
    if (project) {
        QList<MimeGlobPattern> globs;
        foreach (const MimeType &mimeType, MimeDatabase::mimeTypes())
            if (mimeType.type() == QLatin1String(Constants::QML_MIMETYPE)
                    || mimeType.subClassesOf().contains(QLatin1String(Constants::QML_MIMETYPE)))
                globs << mimeType.globPatterns();
        if (globs.isEmpty()) {
            globs.append(MimeGlobPattern(QLatin1String("*.qbs")));
            globs.append(MimeGlobPattern(QLatin1String("*.qml")));
            globs.append(MimeGlobPattern(QLatin1String("*.qmltypes")));
            globs.append(MimeGlobPattern(QLatin1String("*.qmlproject")));
        }
        foreach (const QString &filePath,
                 project->files(ProjectExplorer::Project::ExcludeGeneratedFiles))
            foreach (const MimeGlobPattern &glob, globs)
                if (glob.matches(filePath))
                    projectInfo.sourceFiles << filePath;
        activeTarget = project->activeTarget();
    }
    ProjectExplorer::Kit *activeKit = activeTarget ? activeTarget->kit() :
                                           ProjectExplorer::KitManager::defaultKit();
    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(activeKit);

    bool preferDebugDump = false;
    bool setPreferDump = false;
    projectInfo.tryQmlDump = false;

    if (activeTarget) {
        if (ProjectExplorer::BuildConfiguration *bc = activeTarget->activeBuildConfiguration()) {
            preferDebugDump = bc->buildType() == ProjectExplorer::BuildConfiguration::Debug;
            setPreferDump = true;
        }
    }
    if (!setPreferDump && qtVersion)
        preferDebugDump = (qtVersion->defaultBuildConfig() & QtSupport::BaseQtVersion::DebugBuild);
    if (qtVersion && qtVersion->isValid()) {
        projectInfo.tryQmlDump = project && (
                    qtVersion->type() == QLatin1String(QtSupport::Constants::DESKTOPQT)
                    || qtVersion->type() == QLatin1String(QtSupport::Constants::SIMULATORQT));
        projectInfo.qtQmlPath = QFileInfo(qtVersion->qmakeProperty("QT_INSTALL_QML")).canonicalFilePath();
        projectInfo.qtImportsPath = QFileInfo(qtVersion->qmakeProperty("QT_INSTALL_IMPORTS")).canonicalFilePath();
        projectInfo.qtVersionString = qtVersion->qtVersionString();
    } else {
        projectInfo.qtQmlPath = QFileInfo(QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath)).canonicalFilePath();
        projectInfo.qtImportsPath = QFileInfo(QLibraryInfo::location(QLibraryInfo::ImportsPath)).canonicalFilePath();
        projectInfo.qtVersionString = QLatin1String(qVersion());
    }

    if (projectInfo.tryQmlDump) {
        ProjectExplorer::ToolChain *toolChain =
                ProjectExplorer::ToolChainKitInformation::toolChain(activeKit);
        QtSupport::QmlDumpTool::pathAndEnvironment(project, qtVersion,
                                                   toolChain,
                                                   preferDebugDump, &projectInfo.qmlDumpPath,
                                                   &projectInfo.qmlDumpEnvironment);
        projectInfo.qmlDumpHasRelocatableFlag = qtVersion->hasQmlDumpWithRelocatableFlag();
    } else {
        projectInfo.qmlDumpPath.clear();
        projectInfo.qmlDumpEnvironment.clear();
        projectInfo.qmlDumpHasRelocatableFlag = true;
    }
    setupProjectInfoQmlBundles(projectInfo);
    return projectInfo;
}

void QmlJSTools::setupProjectInfoQmlBundles(ModelManagerInterface::ProjectInfo &projectInfo)
{
    ProjectExplorer::Target *activeTarget = 0;
    if (projectInfo.project)
        activeTarget = projectInfo.project->activeTarget();
    ProjectExplorer::Kit *activeKit = activeTarget
            ? activeTarget->kit() : ProjectExplorer::KitManager::defaultKit();
    QHash<QString, QString> replacements;
    replacements.insert(QLatin1String("$(QT_INSTALL_IMPORTS)"), projectInfo.qtImportsPath);
    replacements.insert(QLatin1String("$(QT_INSTALL_QML)"), projectInfo.qtQmlPath);

    QList<IBundleProvider *> bundleProviders =
            ExtensionSystem::PluginManager::getObjects<IBundleProvider>();

    foreach (IBundleProvider *bp, bundleProviders) {
        if (bp)
            bp->mergeBundlesForKit(activeKit, projectInfo.activeBundle, replacements);
    }
    projectInfo.extendedBundle = projectInfo.activeBundle;

    if (projectInfo.project) {
        QSet<ProjectExplorer::Kit *> currentKits;
        foreach (const ProjectExplorer::Target *t, projectInfo.project->targets())
            if (t->kit())
                currentKits.insert(t->kit());
        currentKits.remove(activeKit);
        foreach (ProjectExplorer::Kit *kit, currentKits) {
            foreach (IBundleProvider *bp, bundleProviders)
                if (bp)
                    bp->mergeBundlesForKit(kit, projectInfo.extendedBundle, replacements);
        }
    }
}

QHash<QString,QmlJS::Dialect> ModelManager::languageForSuffix() const
{
    QHash<QString,QmlJS::Dialect> res = ModelManagerInterface::languageForSuffix();

    if (ICore::instance()) {
        MimeType jsSourceTy = MimeDatabase::findByType(QLatin1String(Constants::JS_MIMETYPE));
        foreach (const QString &suffix, jsSourceTy.suffixes())
            res[suffix] = Dialect::JavaScript;
        MimeType qmlSourceTy = MimeDatabase::findByType(QLatin1String(Constants::QML_MIMETYPE));
        foreach (const QString &suffix, qmlSourceTy.suffixes())
            res[suffix] = Dialect::Qml;
        MimeType qbsSourceTy = MimeDatabase::findByType(QLatin1String(Constants::QBS_MIMETYPE));
        foreach (const QString &suffix, qbsSourceTy.suffixes())
            res[suffix] = Dialect::QmlQbs;
        MimeType qmlProjectSourceTy = MimeDatabase::findByType(QLatin1String(Constants::QMLPROJECT_MIMETYPE));
        foreach (const QString &suffix, qmlProjectSourceTy.suffixes())
            res[suffix] = Dialect::QmlProject;
        MimeType qmlUiSourceTy = MimeDatabase::findByType(QLatin1String(Constants::QMLUI_MIMETYPE));
        foreach (const QString &suffix, qmlUiSourceTy.suffixes())
            res[suffix] = Dialect::QmlQtQuick2Ui;
        MimeType jsonSourceTy = MimeDatabase::findByType(QLatin1String(Constants::JSON_MIMETYPE));
        foreach (const QString &suffix, jsonSourceTy.suffixes())
            res[suffix] = Dialect::Json;
    }
    return res;
}

ModelManager::ModelManager(QObject *parent):
        ModelManagerInterface(parent)
{
    qRegisterMetaType<QmlJSTools::SemanticInfo>("QmlJSTools::SemanticInfo");
    loadDefaultQmlTypeDescriptions();
}

ModelManager::~ModelManager()
{
}

void ModelManager::delayedInitialization()
{
    CppTools::CppModelManager *cppModelManager =
            CppTools::CppModelManager::instance();
    if (cppModelManager) {
        // It's important to have a direct connection here so we can prevent
        // the source and AST of the cpp document being cleaned away.
        connect(cppModelManager, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
                this, SLOT(maybeQueueCppQmlTypeUpdate(CPlusPlus::Document::Ptr)), Qt::DirectConnection);
    }

    connect(ProjectExplorer::SessionManager::instance(), &ProjectExplorer::SessionManager::projectRemoved,
            this, &ModelManager::removeProjectInfo);
    connect(ProjectExplorer::SessionManager::instance(), &ProjectExplorer::SessionManager::startupProjectChanged,
            this, &ModelManager::updateDefaultProjectInfo);

    QmlJS::ViewerContext qbsVContext;
    qbsVContext.language = Dialect::QmlQbs;
    qbsVContext.maybeAddPath(ICore::resourcePath() + QLatin1String("/qbs"));
    setDefaultVContext(qbsVContext);
}

void ModelManager::loadDefaultQmlTypeDescriptions()
{
    if (ICore::instance()) {
        loadQmlTypeDescriptionsInternal(ICore::resourcePath());
        loadQmlTypeDescriptionsInternal(ICore::userResourcePath());
    }
}

void ModelManager::writeMessageInternal(const QString &msg) const
{
    MessageManager::write(msg, MessageManager::Flash);
}

ModelManagerInterface::WorkingCopy ModelManager::workingCopyInternal() const
{
    WorkingCopy workingCopy;
    foreach (IDocument *document, DocumentModel::openedDocuments()) {
        const QString key = document->filePath().toString();
        if (TextEditor::TextDocument *textDocument = qobject_cast<TextEditor::TextDocument *>(document)) {
            // TODO the language should be a property on the document, not the editor
            if (DocumentModel::editorsForDocument(document).first()->context().contains(ProjectExplorer::Constants::LANG_QMLJS))
                workingCopy.insert(key, textDocument->plainText(), textDocument->document()->revision());
        }
    }

    return workingCopy;
}

void ModelManager::updateDefaultProjectInfo()
{
    // needs to be performed in the ui thread
    ProjectExplorer::Project *currentProject = ProjectExplorer::SessionManager::startupProject();
    ProjectInfo newDefaultProjectInfo = projectInfo(currentProject,
                                                    defaultProjectInfoForProject(currentProject));
    setDefaultProject(projectInfo(currentProject,newDefaultProjectInfo), currentProject);
}


// Check whether fileMimeType is the same or extends knownMimeType
bool ModelManager::matchesMimeType(const MimeType &fileMimeType, const MimeType &knownMimeType)
{
    const QStringList knownTypeNames = QStringList(knownMimeType.type()) + knownMimeType.aliases();

    foreach (const QString &knownTypeName, knownTypeNames)
        if (fileMimeType.matchesType(knownTypeName))
            return true;

    // recursion to parent types of fileMimeType
    foreach (const QString &parentMimeType, fileMimeType.subClassesOf())
        if (matchesMimeType(MimeDatabase::findByType(parentMimeType), knownMimeType))
            return true;

    return false;
}

void ModelManager::addTaskInternal(QFuture<void> result, const QString &msg, const char *taskId) const
{
    ProgressManager::addTask(result, msg, taskId);
}
