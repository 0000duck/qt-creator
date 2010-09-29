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

#ifndef QMLJSMODELMANAGERINTERFACE_H
#define QMLJSMODELMANAGERINTERFACE_H

#include "qmljs_global.h"
#include "qmljsdocument.h"

#include <QObject>
#include <QStringList>
#include <QSharedPointer>
#include <QPointer>

namespace ProjectExplorer {
    class Project;
}

QT_QML_BEGIN_NAMESPACE

namespace QmlJS {

class Snapshot;

class QMLJS_EXPORT ModelManagerInterface: public QObject
{
    Q_OBJECT

public:
    class ProjectInfo
    {
    public:
        ProjectInfo()
        { }

        ProjectInfo(QPointer<ProjectExplorer::Project> project)
            : project(project)
        { }

        operator bool() const
        { return ! project.isNull(); }

        bool isValid() const
        { return ! project.isNull(); }

        bool isNull() const
        { return project.isNull(); }

    public: // attributes
        QPointer<ProjectExplorer::Project> project;
        QStringList sourceFiles;
        QStringList importPaths;
        QString qmlDumpPath;
    };

    class WorkingCopy
    {
    public:
        typedef QHash<QString, QPair<QString, int> > Table;

        void insert(const QString &fileName, const QString &source, int revision = 0)
        { _elements.insert(fileName, qMakePair(source, revision)); }

        bool contains(const QString &fileName) const
        { return _elements.contains(fileName); }

        QString source(const QString &fileName) const
        { return _elements.value(fileName).first; }

        QPair<QString, int> get(const QString &fileName) const
        { return _elements.value(fileName); }

        Table all() const
        { return _elements; }

    private:
        Table _elements;
    };

public:
    ModelManagerInterface(QObject *parent = 0);
    virtual ~ModelManagerInterface();

    static ModelManagerInterface *instance();

    virtual WorkingCopy workingCopy() const = 0;
    virtual QmlJS::Snapshot snapshot() const = 0;

    virtual void updateSourceFiles(const QStringList &files,
                                   bool emitDocumentOnDiskChanged) = 0;
    virtual void fileChangedOnDisk(const QString &path) = 0;
    virtual void removeFiles(const QStringList &files) = 0;

    virtual QList<ProjectInfo> projectInfos() const = 0;
    virtual ProjectInfo projectInfo(ProjectExplorer::Project *project) const = 0;
    virtual void updateProjectInfo(const ProjectInfo &pinfo) = 0;

    virtual QStringList importPaths() const = 0;

    virtual void loadPluginTypes(const QString &libraryPath, const QString &importPath, const QString &importUri) = 0;

signals:
    void documentUpdated(QmlJS::Document::Ptr doc);
    void documentChangedOnDisk(QmlJS::Document::Ptr doc);
    void aboutToRemoveFiles(const QStringList &files);
};

} // namespace QmlJS

QT_QML_END_NAMESPACE

#endif // QMLJSMODELMANAGERINTERFACE_H
