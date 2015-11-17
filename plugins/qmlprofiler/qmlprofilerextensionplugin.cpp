/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise Qt Quick Profiler Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#include "qmlprofilerextensionplugin.h"
#include "qmlprofilerextensionconstants.h"
#include <qmlprofiler/qmlprofilertimelinemodelfactory.h>

#ifdef LICENSECHECKER
#  include <licensechecker/licensecheckerplugin.h>
#endif

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>

#include <extensionsystem/pluginmanager.h>

#include <QAction>
#include <QDebug>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>

#include <QtPlugin>

#include "scenegraphtimelinemodel.h"
#include "pixmapcachemodel.h"
#include "memoryusagemodel.h"
#include "inputeventsmodel.h"
#include "debugmessagesmodel.h"

using namespace QmlProfilerExtension::Internal;

class ModelFactory : public QmlProfiler::QmlProfilerTimelineModelFactory {
    Q_OBJECT
public:
    QList<QmlProfiler::QmlProfilerTimelineModel *> create(
            QmlProfiler::QmlProfilerModelManager *manager)
    {
        QList<QmlProfiler::QmlProfilerTimelineModel *> models;
        models << new PixmapCacheModel(manager, this)
               << new SceneGraphTimelineModel(manager, this)
               << new MemoryUsageModel(manager, this)
               << new InputEventsModel(manager, this)
               << new DebugMessagesModel(manager, this);
        return models;
    }
};

QmlProfilerExtensionPlugin::QmlProfilerExtensionPlugin()
{
    // Create your members
}

QmlProfilerExtensionPlugin::~QmlProfilerExtensionPlugin()
{
    // Unregister objects from the plugin manager's object pool
    // Delete members
}

bool QmlProfilerExtensionPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    // Register objects in the plugin manager's object pool
    // Load settings
    // Add actions to menus
    // Connect to other plugins' signals
    // In the initialize method, a plugin can be sure that the plugins it
    // depends on have initialized their members.

    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

#ifdef LICENSECHECKER
    LicenseChecker::LicenseCheckerPlugin *licenseChecker
            = ExtensionSystem::PluginManager::getObject<LicenseChecker::LicenseCheckerPlugin>();

    if (licenseChecker && licenseChecker->hasValidLicense()) {
        if (licenseChecker->enterpriseFeatures())
            addAutoReleasedObject(new ModelFactory);
    } else {
        qWarning() << "Invalid license, disabling QML Profiler Enterprise features";
    }
#else // LICENSECHECKER
    addAutoReleasedObject(new ModelFactory);
#endif

    return true;
}

void QmlProfilerExtensionPlugin::extensionsInitialized()
{
    // Retrieve objects from the plugin manager's object pool
    // In the extensionsInitialized method, a plugin can be sure that all
    // plugins that depend on it are completely initialized.
}

ExtensionSystem::IPlugin::ShutdownFlag QmlProfilerExtensionPlugin::aboutToShutdown()
{
    // Save settings
    // Disconnect from signals that are not needed during shutdown
    // Hide UI (if you add UI that is not in the main window directly)
    return SynchronousShutdown;
}

void QmlProfilerExtensionPlugin::triggerAction()
{
    QMessageBox::information(Core::ICore::mainWindow(),
                             tr("Action triggered"),
                             tr("This is an action from QmlProfilerExtension."));
}

#include "qmlprofilerextensionplugin.moc"
