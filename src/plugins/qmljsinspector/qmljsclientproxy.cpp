/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmljsclientproxy.h"
#include "qmljsprivateapi.h"
#include "qmljsobserverclient.h"
#include "qmljsinspector.h"

#include <debugger/debuggerplugin.h>
#include <debugger/debuggerrunner.h>
#include <debugger/qml/qmlengine.h>
#include <debugger/qml/qmladapter.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>
#include <projectexplorer/project.h>

#include <QUrl>
#include <QAbstractSocket>
#include <QDebug>

enum {
    debug = false
};

using namespace QmlJSInspector::Internal;

ClientProxy::ClientProxy(Debugger::QmlAdapter *adapter, QObject *parent)
    : QObject(parent)
    , m_adapter(adapter)
    , m_client(m_adapter->client())
    , m_designClient(0)
    , m_engineQuery(0)
    , m_contextQuery(0)
    , m_isConnected(false)
{
    m_requestObjectsTimer.setSingleShot(true);
    m_requestObjectsTimer.setInterval(3000);
    connect(m_client, SIGNAL(newObjects()), this, SLOT(newObjects()));
    connect(&m_requestObjectsTimer, SIGNAL(timeout()), this, SLOT(refreshObjectTree()));
    connectToServer();
}

void ClientProxy::connectToServer()
{
    m_designClient = new QmlJSObserverClient(m_adapter->connection(), this);


    m_adapter->logServiceStatusChange(m_designClient->name(), m_designClient->status());

    connect(m_designClient, SIGNAL(connectedStatusChanged(QDeclarativeDebugClient::Status)),
             this, SLOT(clientStatusChanged(QDeclarativeDebugClient::Status)));
    connect(m_designClient, SIGNAL(currentObjectsChanged(QList<int>)),
        SLOT(onCurrentObjectsChanged(QList<int>)));
    connect(m_designClient, SIGNAL(colorPickerActivated()),
        SIGNAL(colorPickerActivated()));
    connect(m_designClient, SIGNAL(zoomToolActivated()),
        SIGNAL(zoomToolActivated()));
    connect(m_designClient, SIGNAL(selectToolActivated()),
        SIGNAL(selectToolActivated()));
    connect(m_designClient, SIGNAL(selectMarqueeToolActivated()),
        SIGNAL(selectMarqueeToolActivated()));
    connect(m_designClient, SIGNAL(animationSpeedChanged(qreal)),
        SIGNAL(animationSpeedChanged(qreal)));
    connect(m_designClient, SIGNAL(designModeBehaviorChanged(bool)),
        SIGNAL(designModeBehaviorChanged(bool)));
    connect(m_designClient, SIGNAL(reloaded()), this,
        SIGNAL(serverReloaded()));
    connect(m_designClient, SIGNAL(selectedColorChanged(QColor)),
        SIGNAL(selectedColorChanged(QColor)));
    connect(m_designClient, SIGNAL(contextPathUpdated(QStringList)),
        SIGNAL(contextPathUpdated(QStringList)));

    updateConnected();
}

void ClientProxy::clientStatusChanged(QDeclarativeDebugClient::Status status)
{
    QString serviceName;
    if (QDeclarativeDebugClient *client = qobject_cast<QDeclarativeDebugClient*>(sender())) {
        serviceName = client->name();
    }

    m_adapter->logServiceStatusChange(serviceName, status);

    updateConnected();
}

void ClientProxy::disconnectFromServer()
{
    if (m_designClient) {
        disconnect(m_designClient, SIGNAL(connectedStatusChanged(QDeclarativeDebugClient::Status)),
                 this, SLOT(clientStatusChanged(QDeclarativeDebugClient::Status)));
        disconnect(m_designClient, SIGNAL(currentObjectsChanged(QList<int>)),
            this, SLOT(onCurrentObjectsChanged(QList<int>)));
        disconnect(m_designClient, SIGNAL(colorPickerActivated()),
            this, SIGNAL(colorPickerActivated()));
        disconnect(m_designClient, SIGNAL(zoomToolActivated()),
            this, SIGNAL(zoomToolActivated()));
        disconnect(m_designClient, SIGNAL(selectToolActivated()),
            this, SIGNAL(selectToolActivated()));
        disconnect(m_designClient, SIGNAL(selectMarqueeToolActivated()),
            this, SIGNAL(selectMarqueeToolActivated()));
        disconnect(m_designClient, SIGNAL(animationSpeedChanged(qreal)),
            this, SIGNAL(animationSpeedChanged(qreal)));
        disconnect(m_designClient, SIGNAL(designModeBehaviorChanged(bool)),
            this, SIGNAL(designModeBehaviorChanged(bool)));
        disconnect(m_designClient, SIGNAL(selectedColorChanged(QColor)),
            this, SIGNAL(selectedColorChanged(QColor)));
        disconnect(m_designClient, SIGNAL(contextPathUpdated(QStringList)),
            this, SIGNAL(contextPathUpdated(QStringList)));

        delete m_designClient;
        m_designClient = 0;
    }

    if (m_engineQuery)
        delete m_engineQuery;
    m_engineQuery = 0;

    if (m_contextQuery)
        delete m_contextQuery;
    m_contextQuery = 0;

    qDeleteAll(m_objectTreeQuery);
    m_objectTreeQuery.clear();

    updateConnected();
}

void ClientProxy::refreshObjectTree()
{
    if (!m_contextQuery) {
        m_requestObjectsTimer.stop();
        qDeleteAll(m_objectTreeQuery);
        m_objectTreeQuery.clear();
        queryEngineContext(m_engines.value(0).debugId());
    }
}

void ClientProxy::onCurrentObjectsChanged(const QList< int >& debugIds, bool requestIfNeeded)
{
    QList<QDeclarativeDebugObjectReference> selectedItems;

    foreach (int debugId, debugIds) {
        QDeclarativeDebugObjectReference ref = objectReferenceForId(debugId);
        if (ref.debugId() != -1) {
            selectedItems << ref;
        } else if (requestIfNeeded) {
            // ### FIXME right now, there's no way in the protocol to
            // a) get some item and know its parent (although that's possible
            //    by adding it to a separate plugin)
            // b) add children to part of an existing tree.
            // So the only choice that remains is to update the complete
            // tree when we have an unknown debug id.
            // break;
        }
    }

    emit selectedItemsChanged(selectedItems);
}

void ClientProxy::setSelectedItemsByObjectId(const QList<QDeclarativeDebugObjectReference> &objectRefs)
{
    if (isConnected())
        m_designClient->setSelectedItemsByObjectId(objectRefs);
}

QDeclarativeDebugObjectReference ClientProxy::objectReferenceForId(int debugId) const
{
    foreach (const QDeclarativeDebugObjectReference& it, m_rootObjects) {
        QDeclarativeDebugObjectReference result = objectReferenceForId(debugId, it);
        if (result.debugId() == debugId)
            return result;
    }
    return QDeclarativeDebugObjectReference();
}

QList<QDeclarativeDebugObjectReference> QmlJSInspector::Internal::ClientProxy::rootObjectReference() const
{
    return m_rootObjects;
}

QDeclarativeDebugObjectReference ClientProxy::objectReferenceForId(int debugId,
                                                                   const QDeclarativeDebugObjectReference &objectRef) const
{
    if (objectRef.debugId() == debugId)
        return objectRef;

    foreach(const QDeclarativeDebugObjectReference &child, objectRef.children()) {
        QDeclarativeDebugObjectReference result = objectReferenceForId(debugId, child);
        if (result.debugId() == debugId)
            return result;
    }

    return QDeclarativeDebugObjectReference();
}

QDeclarativeDebugObjectReference ClientProxy::objectReferenceForId(const QString &objectId) const
{
    if (!objectId.isEmpty() && objectId[0].isLower()) {
        const QList<QDeclarativeDebugObjectReference> refs = objectReferences();
        foreach (const QDeclarativeDebugObjectReference &ref, refs) {
            if (ref.idString() == objectId)
                return ref;
        }
    }
    return QDeclarativeDebugObjectReference();
}

QDeclarativeDebugObjectReference ClientProxy::objectReferenceForLocation(const int line, const int column) const
{
    const QList<QDeclarativeDebugObjectReference> refs = objectReferences();
    foreach (const QDeclarativeDebugObjectReference &ref, refs) {
        if (ref.source().lineNumber() == line && ref.source().columnNumber() == column)
            return ref;
    }

    return QDeclarativeDebugObjectReference();
}

QList<QDeclarativeDebugObjectReference> ClientProxy::objectReferences() const
{
    QList<QDeclarativeDebugObjectReference> result;
    foreach(const QDeclarativeDebugObjectReference &it, m_rootObjects) {
        result.append(objectReferences(it));
    }
    return result;
}

QList<QDeclarativeDebugObjectReference> ClientProxy::objectReferences(const QDeclarativeDebugObjectReference &objectRef) const
{
    QList<QDeclarativeDebugObjectReference> result;
    result.append(objectRef);

    foreach(const QDeclarativeDebugObjectReference &child, objectRef.children()) {
        result.append(objectReferences(child));
    }

    return result;
}

bool ClientProxy::setBindingForObject(int objectDebugId,
                                      const QString &propertyName,
                                      const QVariant &value,
                                      bool isLiteralValue)
{
    if (debug)
        qDebug() << "setBindingForObject():" << objectDebugId << propertyName << value;
    if (objectDebugId == -1)
        return false;

    if (propertyName == QLatin1String("id"))
        return false; // Crashes the QMLViewer.

    bool result = m_client->setBindingForObject(objectDebugId, propertyName, value.toString(), isLiteralValue);

    return result;
}

bool ClientProxy::setMethodBodyForObject(int objectDebugId, const QString &methodName, const QString &methodBody)
{
    if (debug)
        qDebug() << "setMethodBodyForObject():" << objectDebugId << methodName << methodBody;
    if (objectDebugId == -1)
        return 0;
    return m_client->setMethodBody(objectDebugId, methodName, methodBody);
}

bool ClientProxy::resetBindingForObject(int objectDebugId, const QString& propertyName)
{
    if (debug)
        qDebug() << "resetBindingForObject():" << objectDebugId << propertyName;
    if (objectDebugId == -1)
        return false;
    //    if (propertyName == QLatin1String("id"))  return false;
    return m_client->resetBindingForObject(objectDebugId, propertyName);
}

QDeclarativeDebugExpressionQuery *ClientProxy::queryExpressionResult(int objectDebugId, const QString &expr, QObject *parent)
{
    if (debug)
        qDebug() << "queryExpressionResult():" << objectDebugId << expr << parent;
    if (objectDebugId != -1)
        return m_client->queryExpressionResult(objectDebugId,expr,parent);
    return 0;
}

void ClientProxy::clearComponentCache()
{
    if (isConnected())
        m_designClient->clearComponentCache();
}

void ClientProxy::queryEngineContext(int id)
{
    if (id < 0)
        return;

    if (m_contextQuery) {
        delete m_contextQuery;
        m_contextQuery = 0;
    }

    m_contextQuery = m_client->queryRootContexts(QDeclarativeDebugEngineReference(id), this);
    if (!m_contextQuery->isWaiting())
        contextChanged();
    else
        connect(m_contextQuery, SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
                this, SLOT(contextChanged()));
}

void ClientProxy::contextChanged()
{
    if (m_contextQuery) {
        m_rootObjects.clear();
        QDeclarativeDebugContextReference rootContext = m_contextQuery->rootContext();
        delete m_contextQuery;
        m_contextQuery = 0;

        qDeleteAll(m_objectTreeQuery);
        m_objectTreeQuery.clear();
        m_requestObjectsTimer.stop();

        fetchContextObjectRecusrsive(rootContext);
    }
}

void ClientProxy::fetchContextObjectRecusrsive(const QDeclarativeDebugContextReference& context)
{
    foreach (const QDeclarativeDebugObjectReference & obj, context.objects()) {
        QDeclarativeDebugObjectQuery* query = m_client->queryObjectRecursive(obj, this);
        if (!query->isWaiting()) {
            query->deleteLater(); //ignore errors;
        } else {
            m_objectTreeQuery << query;
            connect(query,
                    SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
                    SLOT(objectTreeFetched(QDeclarativeDebugQuery::State)));
        }
    }
    foreach (const QDeclarativeDebugContextReference& child, context.contexts()) {
        fetchContextObjectRecusrsive(child);
    }
}


void ClientProxy::objectTreeFetched(QDeclarativeDebugQuery::State state)
{
    QDeclarativeDebugObjectQuery *query = qobject_cast<QDeclarativeDebugObjectQuery *>(sender());
    if (!query || state == QDeclarativeDebugQuery::Error) {
        delete query;
        return;
    }

    m_rootObjects.append(query->object());

    int removed = m_objectTreeQuery.removeAll(query);
    Q_ASSERT(removed == 1);
    Q_UNUSED(removed);
    delete query;

    if (m_objectTreeQuery.isEmpty()) {
        int old_count = m_debugIdHash.count();
        m_debugIdHash.clear();
        m_debugIdHash.reserve(old_count + 1);
        foreach(const QDeclarativeDebugObjectReference &it, m_rootObjects)
            buildDebugIdHashRecursive(it);
        emit objectTreeUpdated();

        if (isConnected()) {
            if (!m_designClient->selectedItemIds().isEmpty())
                onCurrentObjectsChanged(m_designClient->selectedItemIds(), false);

            m_designClient->setObjectIdList(m_rootObjects);
        }
    }
}

void ClientProxy::buildDebugIdHashRecursive(const QDeclarativeDebugObjectReference& ref)
{
    QString filename = ref.source().url().toLocalFile();
    int lineNum = ref.source().lineNumber();
    int colNum = ref.source().columnNumber();
    int rev = 0;

    // handle the case where the url contains the revision number encoded. (for object created by the debugger)
    static QRegExp rx("^file://(.*)_(\\d+):(\\d+)$");
    if (rx.exactMatch(filename)) {
        filename = rx.cap(1);
        rev = rx.cap(2).toInt();
        lineNum += rx.cap(3).toInt() - 1;
    }

    //convert the filename to a canonical filename in case of shadow build.
    bool isShadowBuild = InspectorUi::instance()->isShadowBuildProject();
    if (isShadowBuild && rev == 0) {
        QString shadowBuildDir = InspectorUi::instance()->debugProjectBuildDirectory();

        if (filename.startsWith(shadowBuildDir)) {
            ProjectExplorer::Project *debugProject = InspectorUi::instance()->debugProject();
            filename = debugProject->projectDirectory() + filename.mid(shadowBuildDir.length());
        }
    }

    // append the debug ids in the hash
    m_debugIdHash[qMakePair<QString, int>(filename, rev)][qMakePair<int, int>(lineNum, colNum)].append(ref.debugId());

    foreach(const QDeclarativeDebugObjectReference &it, ref.children())
        buildDebugIdHashRecursive(it);
}


void ClientProxy::reloadQmlViewer()
{
    if (isConnected())
        m_designClient->reloadViewer();
}

void ClientProxy::setDesignModeBehavior(bool inDesignMode)
{
    if (isConnected())
        m_designClient->setDesignModeBehavior(inDesignMode);
}

void ClientProxy::setAnimationSpeed(qreal slowdownFactor)
{
    if (isConnected())
        m_designClient->setAnimationSpeed(slowdownFactor);
}

void ClientProxy::changeToColorPickerTool()
{
    if (isConnected())
        m_designClient->changeToColorPickerTool();
}

void ClientProxy::changeToZoomTool()
{
    if (isConnected())
        m_designClient->changeToZoomTool();
}
void ClientProxy::changeToSelectTool()
{
    if (isConnected())
        m_designClient->changeToSelectTool();
}

void ClientProxy::changeToSelectMarqueeTool()
{
    if (isConnected())
        m_designClient->changeToSelectMarqueeTool();
}

void ClientProxy::createQmlObject(const QString &qmlText, int parentDebugId,
                                  const QStringList &imports, const QString &filename)
{
    if (isConnected())
        m_designClient->createQmlObject(qmlText, parentDebugId, imports, filename);
}

void ClientProxy::destroyQmlObject(int debugId)
{
    if (isConnected())
        m_designClient->destroyQmlObject(debugId);
}

void ClientProxy::reparentQmlObject(int debugId, int newParent)
{
    if (isConnected())
        m_designClient->reparentQmlObject(debugId, newParent);
}

void ClientProxy::setContextPathIndex(int contextIndex)
{
    if (isConnected())
        m_designClient->setContextPathIndex(contextIndex);
}

void ClientProxy::updateConnected()
{
    bool isConnected = m_designClient && m_designClient->status() == QDeclarativeDebugClient::Enabled
            && m_client && m_client->status() == QDeclarativeEngineDebug::Enabled;

    if (isConnected != m_isConnected) {
        m_isConnected = isConnected;
        if (isConnected) {
            emit connected();
            reloadEngines();
        } else {
            emit disconnected();
        }
    }
}

void ClientProxy::reloadEngines()
{
    if (m_engineQuery) {
        emit connectionStatusMessage("[Inspector] Waiting for response to previous engine query");
        return;
    }

    emit aboutToReloadEngines();

    m_engineQuery = m_client->queryAvailableEngines(this);
    if (!m_engineQuery->isWaiting())
        updateEngineList();
    else
        connect(m_engineQuery, SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
                         this, SLOT(updateEngineList()));
}

QList<QDeclarativeDebugEngineReference> ClientProxy::engines() const
{
    return m_engines;
}

void ClientProxy::updateEngineList()
{
    m_engines = m_engineQuery->engines();
    delete m_engineQuery;
    m_engineQuery = 0;

    emit enginesChanged();
}

Debugger::QmlAdapter *ClientProxy::qmlAdapter() const
{
    return m_adapter;
}

bool ClientProxy::isConnected() const
{
    return m_isConnected;
}

void ClientProxy::newObjects()
{
    if (!m_requestObjectsTimer.isActive())
        m_requestObjectsTimer.start();
}
