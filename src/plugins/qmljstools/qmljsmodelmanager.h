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

#ifndef QMLJSMODELMANAGER_H
#define QMLJSMODELMANAGER_H

#include "qmljstools_global.h"

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsdocument.h>

#include <QFuture>
#include <QFutureSynchronizer>
#include <QMutex>
#include <QProcess>

namespace Core {
class ICore;
class MimeType;
}

namespace QmlJSTools {
namespace Internal {

class QMLJSTOOLS_EXPORT ModelManager: public QmlJS::ModelManagerInterface
{
    Q_OBJECT

public:
    ModelManager(QObject *parent = 0);

    virtual WorkingCopy workingCopy() const;
    virtual QmlJS::Snapshot snapshot() const;

    virtual void updateSourceFiles(const QStringList &files,
                                   bool emitDocumentOnDiskChanged);
    virtual void fileChangedOnDisk(const QString &path);
    virtual void removeFiles(const QStringList &files);

    virtual QList<ProjectInfo> projectInfos() const;
    virtual ProjectInfo projectInfo(ProjectExplorer::Project *project) const;
    virtual void updateProjectInfo(const ProjectInfo &pinfo);

    void updateDocument(QmlJS::Document::Ptr doc);
    void updateLibraryInfo(const QString &path, const QmlJS::LibraryInfo &info);
    void emitDocumentChangedOnDisk(QmlJS::Document::Ptr doc);

    virtual QStringList importPaths() const;

    virtual void loadPluginTypes(const QString &libraryPath, const QString &importPath, const QString &importUri);

Q_SIGNALS:
    void projectPathChanged(const QString &projectPath);

private Q_SLOTS:
    void onLoadPluginTypes(const QString &libraryPath, const QString &importPath, const QString &importUri);
    void qmlPluginTypeDumpDone(int exitCode);
    void qmlPluginTypeDumpError(QProcess::ProcessError error);

protected:
    QFuture<void> refreshSourceFiles(const QStringList &sourceFiles,
                                     bool emitDocumentOnDiskChanged);

    static void parse(QFutureInterface<void> &future,
                      WorkingCopy workingCopy,
                      QStringList files,
                      ModelManager *modelManager,
                      bool emitDocChangedOnDisk);

    void loadQmlTypeDescriptions();
    void loadQmlTypeDescriptions(const QString &path);

    void updateImportPaths();

private:
    static bool matchesMimeType(const Core::MimeType &fileMimeType, const Core::MimeType &knownMimeType);

    mutable QMutex m_mutex;
    Core::ICore *m_core;
    QmlJS::Snapshot _snapshot;
    QStringList m_allImportPaths;
    QStringList m_defaultImportPaths;
    QHash<QProcess *, QString> m_runningQmldumps;

    QFutureSynchronizer<void> m_synchronizer;

    // project integration
    QMap<ProjectExplorer::Project *, ProjectInfo> m_projects;
};

} // namespace Internal
} // namespace QmlJSTools

#endif // QMLJSMODELMANAGER_H
