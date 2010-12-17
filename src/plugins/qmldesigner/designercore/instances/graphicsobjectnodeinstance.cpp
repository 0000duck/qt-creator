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

#include "graphicsobjectnodeinstance.h"

#include "invalidnodeinstanceexception.h"
#include <QGraphicsObject>
#include "private/qgraphicsitem_p.h"
#include <QStyleOptionGraphicsItem>
#include "nodemetainfo.h"

namespace QmlDesigner {
namespace Internal {

GraphicsObjectNodeInstance::GraphicsObjectNodeInstance(QGraphicsObject *graphicsObject, bool hasContent)
   : ObjectNodeInstance(graphicsObject),
   m_hasContent(hasContent),
   m_isMovable(true)
{
}

QGraphicsObject *GraphicsObjectNodeInstance::graphicsObject() const
{
    if (object() == 0)
        return 0;

    Q_ASSERT(qobject_cast<QGraphicsObject*>(object()));
    return static_cast<QGraphicsObject*>(object());
}

bool GraphicsObjectNodeInstance::hasContent() const
{
    return m_hasContent;
}

QPointF GraphicsObjectNodeInstance::position() const
{
    return graphicsObject()->pos();
}

QSizeF GraphicsObjectNodeInstance::size() const
{
    return graphicsObject()->boundingRect().size();
}

QTransform GraphicsObjectNodeInstance::transform() const
{
    if (!nodeInstanceView()->hasInstanceForNode(modelNode()))
        return sceneTransform();

    NodeInstance nodeInstanceParent = nodeInstanceView()->instanceForNode(modelNode()).parent();

    if (!nodeInstanceParent.isValid())
        return sceneTransform();

    QGraphicsObject *graphicsObjectParent = qobject_cast<QGraphicsObject*>(nodeInstanceParent.internalObject());
    if (graphicsObjectParent)
        return graphicsObject()->itemTransform(graphicsObjectParent);
    else
        return sceneTransform();
}

QTransform GraphicsObjectNodeInstance::customTransform() const
{
    return graphicsObject()->transform();
}

QTransform GraphicsObjectNodeInstance::sceneTransform() const
{
    return graphicsObject()->sceneTransform();
}

double GraphicsObjectNodeInstance::rotation() const
{
    return graphicsObject()->rotation();
}

double GraphicsObjectNodeInstance::scale() const
{
    return graphicsObject()->scale();
}

QList<QGraphicsTransform *> GraphicsObjectNodeInstance::transformations() const
{
    return graphicsObject()->transformations();
}

QPointF GraphicsObjectNodeInstance::transformOriginPoint() const
{
    return graphicsObject()->transformOriginPoint();
}

double GraphicsObjectNodeInstance::zValue() const
{
    return graphicsObject()->zValue();
}

double GraphicsObjectNodeInstance::opacity() const
{
    return graphicsObject()->opacity();
}

QObject *GraphicsObjectNodeInstance::parent() const
{
    if (!graphicsObject() || !graphicsObject()->parentItem())
        return 0;

    return graphicsObject()->parentItem()->toGraphicsObject();
}

QRectF GraphicsObjectNodeInstance::boundingRect() const
{
    return graphicsObject()->boundingRect();
}

bool GraphicsObjectNodeInstance::isTopLevel() const
{
    Q_ASSERT(graphicsObject());
    return !graphicsObject()->parentItem();
}

bool GraphicsObjectNodeInstance::isGraphicsObject() const
{
    return true;
}

void GraphicsObjectNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    ObjectNodeInstance::setPropertyVariant(name, value);
}

QVariant GraphicsObjectNodeInstance::property(const QString &name) const
{
    return ObjectNodeInstance::property(name);
}

bool GraphicsObjectNodeInstance::equalGraphicsItem(QGraphicsItem *item) const
{
    return item == graphicsObject();
}

void initOption(QGraphicsItem *item, QStyleOptionGraphicsItem *option, const QTransform &transform)
{
    QGraphicsItemPrivate *privateItem = QGraphicsItemPrivate::get(item);
    privateItem->initStyleOption(option, transform, QRegion());
}

void GraphicsObjectNodeInstance::paintRecursively(QGraphicsItem *graphicsItem, QPainter *painter) const
{
    QGraphicsObject *graphicsObject = graphicsItem->toGraphicsObject();
    if (graphicsObject) {
        if (nodeInstanceView()->hasInstanceForObject(graphicsObject))
            return; //we already keep track of this object elsewhere
    }

    if (graphicsItem->isVisible()) {
        painter->save();
        painter->setTransform(graphicsItem->itemTransform(graphicsItem->parentItem()), true);
        painter->setOpacity(graphicsItem->opacity() * painter->opacity());
        QStyleOptionGraphicsItem option;
        initOption(graphicsItem, &option, painter->transform());
        graphicsItem->paint(painter, &option);
        foreach(QGraphicsItem *childItem, graphicsItem->childItems()) {
            paintRecursively(childItem, painter);
        }
        painter->restore();
    }
}

void GraphicsObjectNodeInstance::paint(QPainter *painter) const
{
    if (graphicsObject()) {
        painter->save();
        if (hasContent()) {
            QStyleOptionGraphicsItem option;
            initOption(graphicsObject(), &option, painter->transform());
            graphicsObject()->paint(painter, &option);

        }

        foreach(QGraphicsItem *graphicsItem, graphicsObject()->childItems())
            paintRecursively(graphicsItem, painter);

        painter->restore();
    }
}

QPair<QGraphicsObject*, bool> GraphicsObjectNodeInstance::createGraphicsObject(const NodeMetaInfo &metaInfo, QDeclarativeContext *context)
{
    QObject *object = ObjectNodeInstance::createObject(metaInfo, context);
    QGraphicsObject *graphicsObject = qobject_cast<QGraphicsObject*>(object);

    if (graphicsObject == 0)
        throw InvalidNodeInstanceException(__LINE__, __FUNCTION__, __FILE__);

//    graphicsObject->setCacheMode(QGraphicsItem::ItemCoordinateCache);
    bool hasContent = !graphicsObject->flags().testFlag(QGraphicsItem::ItemHasNoContents) || metaInfo.isComponent();
    graphicsObject->setFlag(QGraphicsItem::ItemHasNoContents, false);

    return qMakePair(graphicsObject, hasContent);
}

void GraphicsObjectNodeInstance::paintUpdate()
{
    graphicsObject()->update();
}

bool GraphicsObjectNodeInstance::isMovable() const
{
    return m_isMovable && graphicsObject() && graphicsObject()->parentItem();
}

void GraphicsObjectNodeInstance::setMovable(bool movable)
{
    m_isMovable = movable;
}

} // namespace Internal
} // namespace QmlDesigner
