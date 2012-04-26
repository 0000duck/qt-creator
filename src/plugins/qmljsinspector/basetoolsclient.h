/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef BASETOOLSCLIENT_H
#define BASETOOLSCLIENT_H

#include <qmldebug/qmldebugclient.h>
#include <qmldebug/baseenginedebugclient.h>

namespace QmlJSInspector {
namespace Internal {

class BaseToolsClient : public QmlDebug::QmlDebugClient
{
    Q_OBJECT
public:
    BaseToolsClient(QmlDebug::QmlDebugConnection* client, QLatin1String clientName);

    virtual void setCurrentObjects(const QList<int> &debugIds) = 0;
    virtual void reloadViewer() = 0;
    virtual void setDesignModeBehavior(bool inDesignMode) = 0;
    virtual void setAnimationSpeed(qreal slowDownFactor) = 0;
    virtual void setAnimationPaused(bool paused) = 0;
    virtual void changeToSelectTool() = 0;
    virtual void changeToSelectMarqueeTool() = 0;
    virtual void changeToZoomTool() = 0;
    virtual void showAppOnTop(bool showOnTop) = 0;

    virtual void createQmlObject(const QString &qmlText, int parentDebugId,
                         const QStringList &imports, const QString &filename,
                         int order) = 0;
    virtual void destroyQmlObject(int debugId) = 0;
    virtual void reparentQmlObject(int debugId, int newParent) = 0;

    virtual void applyChangesToQmlFile() = 0;
    virtual void applyChangesFromQmlFile() = 0;

    virtual QList<int> currentObjects() const = 0;

    // ### Qt 4.8: remove if we can have access to qdeclarativecontextdata or id's
    virtual void setObjectIdList(
            const QList<QmlDebug::QmlDebugObjectReference> &objectRoots) = 0;

    virtual void clearComponentCache() = 0;

signals:
    void connectedStatusChanged(QmlDebugClient::Status status);

    void currentObjectsChanged(const QList<int> &debugIds);
    void selectToolActivated();
    void selectMarqueeToolActivated();
    void zoomToolActivated();
    void animationSpeedChanged(qreal slowdownFactor);
    void animationPausedChanged(bool paused);
    void designModeBehaviorChanged(bool inDesignMode);
    void showAppOnTopChanged(bool showAppOnTop);
    void reloaded(); // the server has reloadetd he document

    void logActivity(QString client, QString message);

protected:
    void statusChanged(Status);

    void recurseObjectIdList(const QmlDebug::QmlDebugObjectReference &ref,
                             QList<int> &debugIds, QList<QString> &objectIds);
protected:
    enum LogDirection {
        LogSend,
        LogReceive
    };
};

} // namespace Internal
} // namespace QmlJSInspector

#endif // BASETOOLSCLIENT_H
