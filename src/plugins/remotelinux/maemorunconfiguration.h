/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef MAEMORUNCONFIGURATION_H
#define MAEMORUNCONFIGURATION_H

#include "maemoconstants.h"
#include "maemodeviceconfigurations.h"
#include "maemodeployable.h"

#include <utils/environment.h>

#include <projectexplorer/runconfiguration.h>

#include <QtCore/QDateTime>
#include <QtCore/QStringList>

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace Qt4ProjectManager {
class Qt4BuildConfiguration;
class Qt4Project;
class Qt4BaseTarget;
class Qt4ProFileNode;
} // namespace Qt4ProjectManager

namespace RemoteLinux {
namespace Internal {

class AbstractLinuxDeviceDeployStep;
class MaemoDeviceConfigListModel;
class MaemoRemoteMountsModel;
class MaemoRunConfigurationFactory;
class MaemoToolChain;
class Qt4MaemoDeployConfiguration;

class MaemoRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    friend class MaemoRunConfigurationFactory;

public:
    enum BaseEnvironmentBase {
        CleanEnvironmentBase = 0,
        SystemEnvironmentBase = 1
    };

    enum DebuggingType { DebugCppOnly, DebugQmlOnly, DebugCppAndQml };

    MaemoRunConfiguration(Qt4ProjectManager::Qt4BaseTarget *parent,
        const QString &proFilePath);
    virtual ~MaemoRunConfiguration();

    bool isEnabled() const;
    QString disabledReason() const;
    QWidget *createConfigurationWidget();
    Utils::OutputFormatter *createOutputFormatter() const;
    Qt4ProjectManager::Qt4BaseTarget *qt4Target() const;
    Qt4ProjectManager::Qt4BuildConfiguration *activeQt4BuildConfiguration() const;

    Qt4MaemoDeployConfiguration *deployConfig() const;
    MaemoRemoteMountsModel *remoteMounts() const { return m_remoteMounts; }

    QString localExecutableFilePath() const;
    QString remoteExecutableFilePath() const;
    const QString targetRoot() const;
    const QString arguments() const;
    void setArguments(const QString &args);
    QSharedPointer<const MaemoDeviceConfig> deviceConfig() const;
    MaemoPortList freePorts() const;
    bool useRemoteGdb() const;
    void setUseRemoteGdb(bool useRemoteGdb) { m_useRemoteGdb = useRemoteGdb; }
    void updateFactoryState() { emit isEnabledChanged(isEnabled()); }
    DebuggingType debuggingType() const;

    const QString gdbCmd() const;
    QString localDirToMountForRemoteGdb() const;
    QString remoteProjectSourcesMountPoint() const;

    virtual QVariantMap toMap() const;

    QString baseEnvironmentText() const;
    BaseEnvironmentBase baseEnvironmentBase() const;
    void setBaseEnvironmentBase(BaseEnvironmentBase env);

    Utils::Environment environment() const;
    Utils::Environment baseEnvironment() const;

    QList<Utils::EnvironmentItem> userEnvironmentChanges() const;
    void setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff);

    Utils::Environment systemEnvironment() const;
    void setSystemEnvironment(const Utils::Environment &environment);

    int portsUsedByDebuggers() const;
    bool hasEnoughFreePorts(const QString &mode) const;

    QString proFilePath() const;

signals:
    void deviceConfigurationChanged(ProjectExplorer::Target *target);
    void targetInformationChanged() const;

    void baseEnvironmentChanged();
    void systemEnvironmentChanged();
    void userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &diff);

protected:
    MaemoRunConfiguration(Qt4ProjectManager::Qt4BaseTarget *parent,
        MaemoRunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);
    QString defaultDisplayName();

private slots:
    void proFileUpdate(Qt4ProjectManager::Qt4ProFileNode *pro, bool success);
    void proFileInvalidated(Qt4ProjectManager::Qt4ProFileNode *pro);
    void updateDeviceConfigurations();
    void handleDeployConfigChanged();

private:
    void init();
    void handleParseState(bool success);
    AbstractLinuxDeviceDeployStep *deployStep() const;

    QString m_proFilePath;
    mutable QString m_gdbPath;
    MaemoRemoteMountsModel *m_remoteMounts;
    QString m_arguments;
    bool m_useRemoteGdb;

    BaseEnvironmentBase m_baseEnvironmentBase;
    Utils::Environment m_systemEnvironment;
    QList<Utils::EnvironmentItem> m_userEnvironmentChanges;
    bool m_validParse;
    mutable QString m_disabledReason;
};

    } // namespace Internal
} // namespace RemoteLinux

#endif // MAEMORUNCONFIGURATION_H
