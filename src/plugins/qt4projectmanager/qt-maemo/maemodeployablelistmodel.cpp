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

#include "maemodeployablelistmodel.h"

#include "maemoprofilewrapper.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

namespace Qt4ProjectManager {
namespace Internal {

MaemoDeployableListModel::MaemoDeployableListModel(const Qt4ProFileNode *proFileNode,
    const QSharedPointer<ProFileOption> &proFileOption,
    ProFileUpdateSetting updateSetting, QObject *parent)
    : QAbstractTableModel(parent),
      m_projectType(proFileNode->projectType()),
      m_proFilePath(proFileNode->path()),
      m_projectName(proFileNode->displayName()),
      m_targetInfo(proFileNode->targetInformation()),
      m_modified(false),
      m_proFileWrapper(new MaemoProFileWrapper(m_proFilePath,
          proFileNode->buildDir(), proFileOption)),
      m_proFileUpdateSetting(updateSetting),
      m_hasTargetPath(false)
{
    buildModel();
}

MaemoDeployableListModel::~MaemoDeployableListModel() {}

bool MaemoDeployableListModel::buildModel()
{
    m_deployables.clear();

    const MaemoProFileWrapper::InstallsList &installs = m_proFileWrapper->installs();
    m_hasTargetPath = !installs.targetPath.isEmpty();
    if (!m_hasTargetPath && m_proFileUpdateSetting == UpdateProFile) {
        const QString remoteDirSuffix
            = QLatin1String(m_projectType == LibraryTemplate
                ? "/lib" : "/bin");
        const QString remoteDirMaemo5
            = QLatin1String("/opt/usr") + remoteDirSuffix;
        const QString remoteDirMaemo6
            = QLatin1String("/usr/local") + remoteDirSuffix;
        m_deployables.prepend(MaemoDeployable(localExecutableFilePath(),
            remoteDirMaemo5));
        QFile projectFile(m_proFilePath);
        if (!projectFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
            qWarning("Error updating .pro file.");
            return false;
        }
        QString proFileTemplate = QLatin1String("\nunix:!symbian {\n"
            "    maemo5 {\n        target.path = maemo5path\n    } else {\n"
            "        target.path = maemo6path\n    }\n"
            "    INSTALLS += target\n}");
        proFileTemplate.replace(QLatin1String("maemo5path"), remoteDirMaemo5);
        proFileTemplate.replace(QLatin1String("maemo6path"), remoteDirMaemo6);
        if (!projectFile.write(proFileTemplate.toLocal8Bit())) {
            qWarning("Error updating .pro file.");
            return false;
        }
    } else {
        m_deployables.prepend(MaemoDeployable(localExecutableFilePath(),
            installs.targetPath));
    }
    foreach (const MaemoProFileWrapper::InstallsElem &elem, installs.normalElems) {
        foreach (const QString &file, elem.files)
            m_deployables << MaemoDeployable(file, elem.path);
    }

    m_modified = true; // ???
    return true;
}

MaemoDeployable MaemoDeployableListModel::deployableAt(int row) const
{
    Q_ASSERT(row >= 0 && row < rowCount());
    return m_deployables.at(row);
}

bool MaemoDeployableListModel::addDeployable(const MaemoDeployable &deployable,
    QString *error)
{
    if (m_deployables.contains(deployable)) {
        *error = tr("File already in list.");
        return false;
    }

    if (!m_proFileWrapper->addInstallsElem(deployable.remoteDir,
        deployable.localFilePath)) {
        *error = tr("Failed to update .pro file.");
        return false;
    }

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_deployables << deployable;
    endInsertRows();
    return true;
}

bool MaemoDeployableListModel::removeDeployableAt(int row, QString *error)
{
    Q_ASSERT(row > 0 && row < rowCount());

    const MaemoDeployable &deployable = deployableAt(row);
    if (!m_proFileWrapper->removeInstallsElem(deployable.remoteDir,
        deployable.localFilePath)) {
        *error = tr("Could not update .pro file.");
        return false;
    }

    beginRemoveRows(QModelIndex(), row, row);
    m_deployables.removeAt(row);
    endRemoveRows();
    return true;
}

int MaemoDeployableListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_deployables.count();
}

int MaemoDeployableListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 2;
}

QVariant MaemoDeployableListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount())
        return QVariant();

    const MaemoDeployable &d = deployableAt(index.row());
    if (index.column() == 0 && role == Qt::DisplayRole)
        return QDir::toNativeSeparators(d.localFilePath);
    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return d.remoteDir;
    return QVariant();
}

Qt::ItemFlags MaemoDeployableListModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags parentFlags = QAbstractTableModel::flags(index);
//    if (index.column() == 1)
//        return parentFlags | Qt::ItemIsEditable;
    return parentFlags;
}

bool MaemoDeployableListModel::setData(const QModelIndex &index,
                                   const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= rowCount() || index.column() != 1
        || role != Qt::EditRole)
        return false;

    MaemoDeployable &deployable = m_deployables[index.row()];
    const QString &newRemoteDir = value.toString();
    if (!m_proFileWrapper->replaceInstallPath(deployable.remoteDir,
        deployable.localFilePath, newRemoteDir)) {
        qWarning("Error: Could not update .pro file");
        return false;
    }

    deployable.remoteDir = newRemoteDir;
    emit dataChanged(index, index);
    return true;
}

QVariant MaemoDeployableListModel::headerData(int section,
             Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole)
        return QVariant();
    return section == 0 ? tr("Local File Path") : tr("Remote Directory");
}

QString MaemoDeployableListModel::localExecutableFilePath() const
{
    if (!m_targetInfo.valid)
        return QString();

    const bool isLib = m_projectType == LibraryTemplate;
    bool isStatic = false; // Nonsense init for stupid compilers.
    QString fileName;
    if (isLib) {
        fileName += QLatin1String("lib");
        const QStringList &config
            = m_proFileWrapper->varValues(QLatin1String("CONFIG"));
        isStatic = config.contains(QLatin1String("static"))
            || config.contains(QLatin1String("staticlib"));
    }
    fileName += m_targetInfo.target;
    if (isLib)
        fileName += QLatin1String(isStatic ? ".a" : ".so");
    return QDir::cleanPath(m_targetInfo.workingDir + '/' + fileName);
}

QString MaemoDeployableListModel::remoteExecutableFilePath() const
{
    return m_hasTargetPath
        ? deployableAt(0).remoteDir + '/'
              + QFileInfo(localExecutableFilePath()).fileName()
        : QString();
}

QString MaemoDeployableListModel::projectDir() const
{
    return QFileInfo(m_proFilePath).dir().path();
}

void MaemoDeployableListModel::setProFileUpdateSetting(ProFileUpdateSetting updateSetting)
{
    m_proFileUpdateSetting = updateSetting;
    if (updateSetting == UpdateProFile)
        buildModel();
}

} // namespace Qt4ProjectManager
} // namespace Internal
