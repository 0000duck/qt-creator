/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qnxplugin.h"

#include "blackberrydeviceconfigurationfactory.h"
#include "qnxconstants.h"
#include "blackberryqtversionfactory.h"
#include "blackberrydeployconfigurationfactory.h"
#include "blackberrycreatepackagestepfactory.h"
#include "blackberrydeploystepfactory.h"
#include "blackberryrunconfigurationfactory.h"
#include "blackberryruncontrolfactory.h"
#include "qnxdeviceconfigurationfactory.h"
#include "qnxruncontrolfactory.h"
#include "qnxdeploystepfactory.h"
#include "qnxdeployconfigurationfactory.h"
#include "qnxrunconfigurationfactory.h"
#include "qnxqtversionfactory.h"
#include "blackberryndksettingspage.h"
#include "bardescriptoreditorfactory.h"
#include "bardescriptormagicmatcher.h"
#include "blackberrykeyspage.h"
#include "blackberrycheckdebugtokenstepfactory.h"
#include "blackberrydeviceconnectionmanager.h"
#include "blackberryconfigurationmanager.h"
#include "blackberryconfiguration.h"
#include "cascadesimport/cascadesimportwizard.h"
#include "qnxtoolchain.h"


#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/kitmanager.h>

#include <QtPlugin>

using namespace Qnx::Internal;

QNXPlugin::QNXPlugin()
{
}

QNXPlugin::~QNXPlugin()
{
    delete BlackBerryDeviceConnectionManager::instance();
}

bool QNXPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    // Handles BlackBerry
    addAutoReleasedObject(new BlackBerryQtVersionFactory);
    addAutoReleasedObject(new BlackBerryDeployConfigurationFactory);
    addAutoReleasedObject(new BlackBerryDeviceConfigurationFactory);
    addAutoReleasedObject(new BlackBerryCreatePackageStepFactory);
    addAutoReleasedObject(new BlackBerryDeployStepFactory);
    addAutoReleasedObject(new BlackBerryRunConfigurationFactory);
    addAutoReleasedObject(new BlackBerryRunControlFactory);
    addAutoReleasedObject(new BlackBerryNDKSettingsPage);
    addAutoReleasedObject(new BlackBerryKeysPage);
    addAutoReleasedObject(new BlackBerryCheckDebugTokenStepFactory);
    addAutoReleasedObject(new CascadesImportWizard);
    BlackBerryDeviceConnectionManager::instance()->initialize();

    // Handles QNX
    addAutoReleasedObject(new QnxQtVersionFactory);
    addAutoReleasedObject(new QnxDeviceConfigurationFactory);
    addAutoReleasedObject(new QnxRunControlFactory);
    addAutoReleasedObject(new QnxDeployStepFactory);
    addAutoReleasedObject(new QnxDeployConfigurationFactory);
    addAutoReleasedObject(new QnxRunConfigurationFactory);

    // Handle Qcc Compiler
    addAutoReleasedObject(new QnxToolChainFactory);

    // bar-descriptor.xml editor
    Core::MimeGlobPattern barDescriptorGlobPattern(QLatin1String("*.xml"), Core::MimeGlobPattern::MinWeight + 1);
    Core::MimeType barDescriptorMimeType;
    barDescriptorMimeType.setType(QLatin1String(Constants::QNX_BAR_DESCRIPTOR_MIME_TYPE));
    barDescriptorMimeType.setComment(tr("Bar descriptor file (BlackBerry)"));
    barDescriptorMimeType.setGlobPatterns(QList<Core::MimeGlobPattern>() << barDescriptorGlobPattern);
    barDescriptorMimeType.addMagicMatcher(QSharedPointer<Core::IMagicMatcher>(new BarDescriptorMagicMatcher));
    barDescriptorMimeType.setSubClassesOf(QStringList() << QLatin1String("application/xml"));

    if (!Core::MimeDatabase::addMimeType(barDescriptorMimeType)) {
        *errorString = tr("Could not add mime-type for bar-descriptor.xml editor.");
        return false;
    }
    addAutoReleasedObject(new BarDescriptorEditorFactory);

    connect(ProjectExplorer::KitManager::instance(), SIGNAL(kitsLoaded()), &BlackBerryConfigurationManager::instance(), SLOT(loadSettings()));

    return true;
}

void QNXPlugin::extensionsInitialized()
{
    ProjectExplorer::TaskHub::addCategory(Constants::QNX_TASK_CATEGORY_BARDESCRIPTOR,
                                          tr("Bar Descriptor"));
}

ExtensionSystem::IPlugin::ShutdownFlag QNXPlugin::aboutToShutdown()
{
    return SynchronousShutdown;
}

#ifdef WITH_TESTS
#include <QTest>

#include "bardescriptordocument.h"

void QNXPlugin::testBarDescriptorDocumentSetValue_data()
{
    QTest::addColumn<BarDescriptorDocument::Tag>("tag");
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QString>("baseXml");
    QTest::addColumn<QString>("xml");
    QTest::addColumn<bool>("compareResultValue");

    QTest::newRow("new-id") << BarDescriptorDocument::id
                            << QVariant(QString::fromLatin1("my-application-id"))
                            << QString::fromLatin1("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
                                                   "<qnx xmlns=\"http://www.qnx.com/schemas/application/1.0\"/>\n")
                            << QString::fromLatin1("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
                                                   "<qnx xmlns=\"http://www.qnx.com/schemas/application/1.0\">\n"
                                                   "    <id>my-application-id</id>\n"
                                                   "</qnx>\n")
                            << true;

    QTest::newRow("changed-id") << BarDescriptorDocument::id
                                << QVariant(QString::fromLatin1("my-application-id"))
                                << QString::fromLatin1("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
                                                       "<qnx xmlns=\"http://www.qnx.com/schemas/application/1.0\">\n"
                                                       "    <id>some-application-id</id>\n"
                                                       "</qnx>\n")
                                << QString::fromLatin1("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
                                                       "<qnx xmlns=\"http://www.qnx.com/schemas/application/1.0\">\n"
                                                       "    <id>my-application-id</id>\n"
                                                       "</qnx>\n")
                                << true;


    QTest::newRow("removed-id") << BarDescriptorDocument::id
                                << QVariant(QString::fromLatin1(""))
                                << QString::fromLatin1("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
                                                       "<qnx xmlns=\"http://www.qnx.com/schemas/application/1.0\">\n"
                                                       "    <id>some-application-id</id>\n"
                                                       "</qnx>\n")
                                << QString::fromLatin1("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
                                                       "<qnx xmlns=\"http://www.qnx.com/schemas/application/1.0\"/>\n")
                                << true;

    QStringList splashScreens;
    splashScreens << QLatin1String("image1.png")
                  << QLatin1String("image2.png");
    QTest::newRow("new-splashScreens") << BarDescriptorDocument::splashScreens
                                       << QVariant(splashScreens)
                                       << QString::fromLatin1("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
                                                              "<qnx xmlns=\"http://www.qnx.com/schemas/application/1.0\"/>\n")
                                       << QString::fromLatin1("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
                                                              "<qnx xmlns=\"http://www.qnx.com/schemas/application/1.0\">\n"
                                                              "    <splashScreens>\n"
                                                              "        <image>image1.png</image>\n"
                                                              "        <image>image2.png</image>\n"
                                                              "    </splashScreens>\n"
                                                              "</qnx>\n")
                                       << true;

    QTest::newRow("changed-splashScreens") << BarDescriptorDocument::splashScreens
                                           << QVariant(splashScreens)
                                           << QString::fromLatin1("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
                                                                  "<qnx xmlns=\"http://www.qnx.com/schemas/application/1.0\">\n"
                                                                  "    <splashScreens>\n"
                                                                  "        <image>image3.png</image>\n"
                                                                  "        <image>image4.png</image>\n"
                                                                  "    </splashScreens>\n"
                                                                  "</qnx>\n")
                                           << QString::fromLatin1("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
                                                                  "<qnx xmlns=\"http://www.qnx.com/schemas/application/1.0\">\n"
                                                                  "    <splashScreens>\n"
                                                                  "        <image>image1.png</image>\n"
                                                                  "        <image>image2.png</image>\n"
                                                                  "    </splashScreens>\n"
                                                                  "</qnx>\n")
                                           << true;

    QTest::newRow("removed-splashScreens") << BarDescriptorDocument::splashScreens
                                           << QVariant(QStringList())
                                           << QString::fromLatin1("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
                                                                  "<qnx xmlns=\"http://www.qnx.com/schemas/application/1.0\">\n"
                                                                  "    <splashScreens>\n"
                                                                  "        <image>image1.png</image>\n"
                                                                  "        <image>image2.png</image>\n"
                                                                  "    </splashScreens>\n"
                                                                  "</qnx>\n")
                                           << QString::fromLatin1("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
                                                                  "<qnx xmlns=\"http://www.qnx.com/schemas/application/1.0\"/>\n")
                                           << true;

    BarDescriptorAsset asset1;
    asset1.source = QLatin1String("/path/to/file");
    asset1.destination = QLatin1String("file");
    asset1.entry = false;

    BarDescriptorAsset asset2;
    asset2.source = QLatin1String("/path/to/file2");
    asset2.destination = QLatin1String("file2");
    asset2.entry = false; // Cannot test "true", as "type" and "entry" attributes show up in seemingly arbitrary order

    BarDescriptorAssetList assetList1;
    assetList1 << asset1 << asset2;

    QVariant assets;
    assets.setValue(assetList1);

    QTest::newRow("new-assets") << BarDescriptorDocument::asset
                                << assets
                                << QString::fromLatin1("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
                                                       "<qnx xmlns=\"http://www.qnx.com/schemas/application/1.0\"/>\n")
                                << QString::fromLatin1("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
                                                       "<qnx xmlns=\"http://www.qnx.com/schemas/application/1.0\">\n"
                                                       "    <asset path=\"/path/to/file\">file</asset>\n"
                                                       "    <asset path=\"/path/to/file2\">file2</asset>\n"
                                                       "</qnx>\n")
                                << false;

    asset2.destination = QLatin1String("file3");
    BarDescriptorAssetList assetList2;
    assetList2 << asset1 << asset2;
    assets.setValue(assetList2);

    QTest::newRow("changed-assets") << BarDescriptorDocument::asset
                                    << assets
                                    << QString::fromLatin1("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
                                                           "<qnx xmlns=\"http://www.qnx.com/schemas/application/1.0\">\n"
                                                           "    <asset path=\"/path/to/file\">file</asset>\n"
                                                           "    <asset path=\"/path/to/file2\">file2</asset>\n"
                                                           "</qnx>\n")
                                    << QString::fromLatin1("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
                                                           "<qnx xmlns=\"http://www.qnx.com/schemas/application/1.0\">\n"
                                                           "    <asset path=\"/path/to/file\">file</asset>\n"
                                                           "    <asset path=\"/path/to/file2\">file3</asset>\n"
                                                           "</qnx>\n")
                                    << false;

    QTest::newRow("maintain-position") << BarDescriptorDocument::id
                                       << QVariant(QString::fromLatin1("my-application-id"))
                                       << QString::fromLatin1("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
                                                              "<qnx xmlns=\"http://www.qnx.com/schemas/application/1.0\">\n"
                                                              "    <asset path=\"/path/to/file\">file</asset>\n"
                                                              "    <asset path=\"/path/to/file2\">file2</asset>\n"
                                                              "    <id>some-application-id</id>\n"
                                                              "    <splashScreens>\n"
                                                              "        <image>image1.png</image>\n"
                                                              "        <image>image2.png</image>\n"
                                                              "    </splashScreens>\n"
                                                              "</qnx>\n")
                                       << QString::fromLatin1("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
                                                              "<qnx xmlns=\"http://www.qnx.com/schemas/application/1.0\">\n"
                                                              "    <asset path=\"/path/to/file\">file</asset>\n"
                                                              "    <asset path=\"/path/to/file2\">file2</asset>\n"
                                                              "    <id>my-application-id</id>\n"
                                                              "    <splashScreens>\n"
                                                              "        <image>image1.png</image>\n"
                                                              "        <image>image2.png</image>\n"
                                                              "    </splashScreens>\n"
                                                              "</qnx>\n")
                                       << true;

    QTest::newRow("removed-icon") << BarDescriptorDocument::icon
                                  << QVariant(QString())
                                  << QString::fromLatin1("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
                                                         "<qnx xmlns=\"http://www.qnx.com/schemas/application/1.0\">\n"
                                                         "    <icon>\n"
                                                         "        <image>icon1.png</image>\n"
                                                         "    </icon>\n"
                                                         "</qnx>\n")
                                  << QString::fromLatin1("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
                                                         "<qnx xmlns=\"http://www.qnx.com/schemas/application/1.0\"/>\n")
                                  << true;
}

void QNXPlugin::testBarDescriptorDocumentSetValue()
{
    QFETCH(BarDescriptorDocument::Tag, tag);
    QFETCH(QVariant, value);
    QFETCH(QString, baseXml);
    QFETCH(QString, xml);
    QFETCH(bool, compareResultValue);

    BarDescriptorDocument doc;
    doc.loadContent(baseXml, false);
    QCOMPARE(doc.xmlSource(), baseXml);

    doc.setValue(tag, value);
    QCOMPARE(doc.xmlSource(), xml);
    QCOMPARE(doc.isModified(), true);
    if (compareResultValue)
        QCOMPARE(doc.value(tag), value);
}

void QNXPlugin::testBarDescriptorDocumentSetBannerComment_data()
{
    QTest::addColumn<QString>("comment");
    QTest::addColumn<QString>("baseXml");
    QTest::addColumn<QString>("xml");

    QString procInstr = QString::fromLatin1("<?xml version='1.0' encoding='UTF-8' standalone='no'?>");
    QString comment = QString::fromLatin1("This file is autogenerated, any change will be ...");
    QString xmlComment = QString::fromLatin1("<!--%1-->").arg(comment);
    QString oldXmlComment = QString::fromLatin1("<!-- Some old banner comment -->");
    QString docRoot = QString::fromLatin1("<qnx xmlns=\"http://www.qnx.com/schemas/application/1.0\"/>");
    QChar lf = QChar::fromLatin1('\n');

    QTest::newRow("new-comment")
        << comment
        << QString(procInstr + lf + docRoot + lf)
        << QString(procInstr + lf + xmlComment + lf + docRoot + lf);

    QTest::newRow("new-comment-noproc")
        << comment
        << QString(docRoot + lf)
        << QString(xmlComment + lf + docRoot + lf);

    QTest::newRow("replace-comment")
        << comment
        << QString(procInstr + lf + oldXmlComment + lf + docRoot + lf)
        << QString(procInstr + lf + xmlComment + lf + docRoot + lf);

    QTest::newRow("replace-comment-noproc")
        << comment
        << QString(oldXmlComment + lf + docRoot + lf)
        << QString(xmlComment + lf + docRoot + lf);

    QTest::newRow("remove-comment")
        << QString()
        << QString(procInstr + lf + oldXmlComment + lf + docRoot + lf)
        << QString(procInstr + lf + docRoot + lf);

    QTest::newRow("remove-comment-noproc")
        << QString()
        << QString(oldXmlComment + lf + docRoot + lf)
        << QString(docRoot + lf);

}

void QNXPlugin::testBarDescriptorDocumentSetBannerComment()
{
    QFETCH(QString, comment);
    QFETCH(QString, baseXml);
    QFETCH(QString, xml);

    BarDescriptorDocument doc;
    doc.loadContent(baseXml, false);
    QCOMPARE(doc.xmlSource(), baseXml);

    doc.setBannerComment(comment);
    QCOMPARE(doc.xmlSource(), xml);
    QCOMPARE(doc.isModified(), true);
    QCOMPARE(doc.bannerComment(), comment);
}

void QNXPlugin::testConfigurationManager_data()
{
    const QLatin1String NDKEnvFileKey("NDKEnvFile");
    const QLatin1String NDKPathKey("NDKPath");
    const QLatin1String NDKDisplayNameKey("NDKDisplayName");
    const QLatin1String NDKTargetKey("NDKTarget");
    const QLatin1String NDKHostKey("NDKHost");
    const QLatin1String NDKVersionKey("NDKVersion");
    const QLatin1String NDKAutoDetectionSourceKey("NDKAutoDetectionSource");
    const QLatin1String NDKAutoDetectedKey("NDKAutoDetectedKey");

    QTest::addColumn<QVariantMap>("newerConfiguration");
    QTest::addColumn<QVariantMap>("olderConfiguration");

    QVariantMap newerConfiguration;
    newerConfiguration.insert(NDKEnvFileKey, QLatin1String("bbndk-env.sh"));
    newerConfiguration.insert(NDKPathKey, QLatin1String("NDKPath"));
    newerConfiguration.insert(NDKDisplayNameKey, QLatin1String("NDKDisplayName"));
    newerConfiguration.insert(NDKTargetKey, QLatin1String("NDKTarget"));
    newerConfiguration.insert(NDKHostKey, QLatin1String("NDKHost"));
    newerConfiguration.insert(NDKVersionKey, QLatin1String("10.1.0.1008"));
    newerConfiguration.insert(NDKAutoDetectionSourceKey, QLatin1String("NDKAutoDetectionSource"));
    newerConfiguration.insert(NDKAutoDetectedKey, QLatin1String("NDKAutoDetectedKey"));

    QVariantMap olderConfiguration;
    olderConfiguration.insert(NDKEnvFileKey, QLatin1String("bbndk-env2.sh"));
    olderConfiguration.insert(NDKPathKey, QLatin1String("NDKPath"));
    olderConfiguration.insert(NDKDisplayNameKey, QLatin1String("NDKDisplayName"));
    olderConfiguration.insert(NDKTargetKey, QLatin1String("NDKTarget"));
    olderConfiguration.insert(NDKHostKey, QLatin1String("NDKHost"));
    newerConfiguration.insert(NDKVersionKey, QLatin1String("10.2.0.1008"));
    olderConfiguration.insert(NDKAutoDetectionSourceKey, QLatin1String("NDKAutoDetectionSource"));
    olderConfiguration.insert(NDKAutoDetectedKey, QLatin1String("NDKAutoDetectedKey"));

    QTest::newRow("configurations") << newerConfiguration << olderConfiguration;;
}

void QNXPlugin::testConfigurationManager()
{
    BlackBerryConfigurationManager &manager = BlackBerryConfigurationManager::instance();

    QCOMPARE(manager.configurations().count(), 0);
    QCOMPARE(manager.activeConfigurations().count(), 0);
    QCOMPARE(manager.defaultConfiguration(), static_cast<BlackBerryConfiguration*>(0));
    QVERIFY(manager.newestConfigurationEnabled());

    QFETCH(QVariantMap, newerConfiguration);
    QFETCH(QVariantMap, olderConfiguration);

    BlackBerryConfiguration::setFakeConfig(true);
    BlackBerryConfiguration *newerConfig = new BlackBerryConfiguration(newerConfiguration);
    BlackBerryConfiguration *oldConfig = new BlackBerryConfiguration(olderConfiguration);

    QVERIFY(manager.addConfiguration(oldConfig));
    QVERIFY(manager.newestConfigurationEnabled());
    QCOMPARE(manager.defaultConfiguration(), oldConfig);

    manager.setDefaultConfiguration(oldConfig);

    QCOMPARE(manager.defaultConfiguration(), oldConfig);
    QCOMPARE(manager.configurations().first(), oldConfig);
    QVERIFY(!manager.newestConfigurationEnabled());

    QVERIFY(manager.addConfiguration(newerConfig));
    QCOMPARE(manager.configurations().first(), newerConfig);
    QCOMPARE(manager.defaultConfiguration(), oldConfig);

    manager.setDefaultConfiguration(0);
    QVERIFY(manager.newestConfigurationEnabled());
    QCOMPARE(manager.defaultConfiguration(), newerConfig);

    manager.setDefaultConfiguration(oldConfig);
    manager.removeConfiguration(oldConfig);
    QCOMPARE(manager.defaultConfiguration(), newerConfig);
    QVERIFY(manager.newestConfigurationEnabled());

    manager.removeConfiguration(newerConfig);
    QCOMPARE(manager.defaultConfiguration(), static_cast<BlackBerryConfiguration*>(0));
    QVERIFY(manager.newestConfigurationEnabled());
}

#endif

Q_EXPORT_PLUGIN2(QNX, QNXPlugin)
