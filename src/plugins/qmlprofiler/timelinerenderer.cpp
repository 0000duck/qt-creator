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

#include "timelinerenderer.h"
#include "qmlprofilernotesmodel.h"

#include <QQmlContext>
#include <QQmlProperty>
#include <QTimer>
#include <QPixmap>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QVarLengthArray>

#include <math.h>

using namespace QmlProfiler::Internal;

TimelineRenderer::TimelineRenderer(QQuickPaintedItem *parent) :
    QQuickPaintedItem(parent), m_spacing(0), m_spacedDuration(0), m_profilerModelProxy(0),
    m_zoomer(0), m_selectedItem(-1), m_selectedModel(-1), m_selectionLocked(true),
    m_startDragArea(-1), m_endDragArea(-1)
{
    resetCurrentSelection();
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
}

void TimelineRenderer::setProfilerModelProxy(QObject *profilerModelProxy)
{
    if (m_profilerModelProxy) {
        disconnect(m_profilerModelProxy, SIGNAL(expandedChanged()), this, SLOT(requestPaint()));
        disconnect(m_profilerModelProxy, SIGNAL(hiddenChanged()), this, SLOT(requestPaint()));
        disconnect(m_profilerModelProxy, SIGNAL(rowHeightChanged()), this, SLOT(requestPaint()));
        disconnect(m_profilerModelProxy, SIGNAL(modelsChanged(int,int)),
                   this, SLOT(swapSelections(int,int)));
        disconnect(m_profilerModelProxy, SIGNAL(notesChanged()), this, SLOT(requestPaint()));
    }
    m_profilerModelProxy = qobject_cast<TimelineModelAggregator *>(profilerModelProxy);

    if (m_profilerModelProxy) {
        connect(m_profilerModelProxy, SIGNAL(expandedChanged()), this, SLOT(requestPaint()));
        connect(m_profilerModelProxy, SIGNAL(hiddenChanged()), this, SLOT(requestPaint()));
        connect(m_profilerModelProxy, SIGNAL(rowHeightChanged()), this, SLOT(requestPaint()));
        connect(m_profilerModelProxy, SIGNAL(modelsChanged(int,int)),
                this, SLOT(swapSelections(int,int)));
        connect(m_profilerModelProxy, SIGNAL(notesChanged(int,int,int)),
                this, SLOT(requestPaint()));
    }
    emit profilerModelProxyChanged(m_profilerModelProxy);
}

void TimelineRenderer::setZoomer(QObject *zoomControl)
{
    TimelineZoomControl *zoomer = qobject_cast<TimelineZoomControl *>(zoomControl);
    if (zoomer != m_zoomer) {
        if (m_zoomer != 0)
            disconnect(m_zoomer, SIGNAL(rangeChanged(qint64,qint64)), this, SLOT(requestPaint()));
        m_zoomer = zoomer;
        if (m_zoomer != 0)
            connect(m_zoomer, SIGNAL(rangeChanged(qint64,qint64)), this, SLOT(requestPaint()));
        emit zoomerChanged(zoomer);
        update();
    }
}

void TimelineRenderer::componentComplete()
{
    const QMetaObject *metaObject = this->metaObject();
    int propertyCount = metaObject->propertyCount();
    int requestPaintMethod = metaObject->indexOfMethod("requestPaint()");
    for (int ii = TimelineRenderer::staticMetaObject.propertyCount(); ii < propertyCount; ++ii) {
        QMetaProperty p = metaObject->property(ii);
        if (p.hasNotifySignal())
            QMetaObject::connect(this, p.notifySignalIndex(), this, requestPaintMethod, 0, 0);
    }
    QQuickItem::componentComplete();
}

void TimelineRenderer::requestPaint()
{
    update();
}

void TimelineRenderer::swapSelections(int modelIndex1, int modelIndex2)
{
    // Any hovered event is most likely useless now. Reset it.
    resetCurrentSelection();

    // Explicitly selected events can be tracked in a useful way.
    if (m_selectedModel == modelIndex1)
        setSelectedModel(modelIndex2);
    else if (m_selectedModel == modelIndex2)
        setSelectedModel(modelIndex1);

    update();
}

inline void TimelineRenderer::getItemXExtent(int modelIndex, int i, int &currentX, int &itemWidth)
{
    qint64 start = m_profilerModelProxy->startTime(modelIndex, i) - m_zoomer->rangeStart();

    // avoid integer overflows by using floating point for width calculations. m_spacing is qreal,
    // too, so for some intermediate calculations we have to use floats anyway.
    qreal rawWidth;
    if (start > 0) {
        currentX = static_cast<int>(start * m_spacing);
        rawWidth = m_profilerModelProxy->duration(modelIndex, i) * m_spacing;
    } else {
        currentX = -OutOfScreenMargin;
        // Explicitly round the "start" part down, away from 0, to match the implicit rounding of
        // currentX in the > 0 case. If we don't do that we get glitches where a pixel is added if
        // the element starts outside the screen and subtracted if it starts inside the screen.
        rawWidth = m_profilerModelProxy->duration(modelIndex, i) * m_spacing +
                floor(start * m_spacing) + OutOfScreenMargin;
    }
    if (rawWidth < MinimumItemWidth) {
        currentX -= static_cast<int>((MinimumItemWidth - rawWidth) / 2);
        itemWidth = MinimumItemWidth;
    } else if (rawWidth > m_spacedDuration) {
        itemWidth = static_cast<int>(m_spacedDuration);
    } else {
        itemWidth = static_cast<int>(rawWidth);
    }
}

void TimelineRenderer::resetCurrentSelection()
{
    m_currentSelection.startTime = -1;
    m_currentSelection.endTime = -1;
    m_currentSelection.row = -1;
    m_currentSelection.eventIndex = -1;
    m_currentSelection.modelIndex = -1;
}

void TimelineRenderer::paint(QPainter *p)
{
    if (m_zoomer->rangeDuration() <= 0)
        return;

    m_spacing = width() / m_zoomer->rangeDuration();
    m_spacedDuration = width() + 2 * OutOfScreenMargin;

    p->setPen(Qt::transparent);

    for (int modelIndex = 0; modelIndex < m_profilerModelProxy->modelCount(); modelIndex++) {
        if (m_profilerModelProxy->hidden(modelIndex))
            continue;
        int lastIndex = m_profilerModelProxy->lastIndex(modelIndex, m_zoomer->rangeEnd());
        if (lastIndex >= 0 && lastIndex < m_profilerModelProxy->count(modelIndex)) {
            int firstIndex = m_profilerModelProxy->firstIndex(modelIndex, m_zoomer->rangeStart());
            if (firstIndex >= 0) {
                drawItemsToPainter(p, modelIndex, firstIndex, lastIndex);
                if (m_selectedModel == modelIndex)
                    drawSelectionBoxes(p, modelIndex, firstIndex, lastIndex);
                drawBindingLoopMarkers(p, modelIndex, firstIndex, lastIndex);
            }
        }
    }
    drawNotes(p);
}

void TimelineRenderer::drawItemsToPainter(QPainter *p, int modelIndex, int fromIndex, int toIndex)
{
    p->save();
    p->setPen(Qt::transparent);
    int modelRowStart = 0;
    for (int mi = 0; mi < modelIndex; mi++)
        modelRowStart += m_profilerModelProxy->model(mi)->height();

    for (int i = fromIndex; i <= toIndex; i++) {
        int currentX, currentY, itemWidth, itemHeight;

        int rowNumber = m_profilerModelProxy->row(modelIndex, i);
        currentY = modelRowStart + m_profilerModelProxy->rowOffset(modelIndex, rowNumber) - y();
        if (currentY >= height())
            continue;

        itemHeight = m_profilerModelProxy->rowHeight(modelIndex, rowNumber) *
                m_profilerModelProxy->relativeHeight(modelIndex, i);

        currentY += m_profilerModelProxy->rowHeight(modelIndex, rowNumber) - itemHeight;
        if (currentY + itemHeight < 0)
            continue;

        getItemXExtent(modelIndex, i, currentX, itemWidth);

        // normal events
        p->setBrush(m_profilerModelProxy->color(modelIndex, i));
        p->drawRect(currentX, currentY, itemWidth, itemHeight);
    }
    p->restore();
}

void TimelineRenderer::drawSelectionBoxes(QPainter *p, int modelIndex, int fromIndex, int toIndex)
{
    const uint strongLineWidth = 3;
    const uint lightLineWidth = 2;
    static const QColor strongColor = Qt::blue;
    static const QColor lockedStrongColor = QColor(96,0,255);
    static const QColor lightColor = strongColor.lighter(130);
    static const QColor lockedLightColor = lockedStrongColor.lighter(130);

    if (m_selectedItem == -1)
        return;


    int id = m_profilerModelProxy->selectionId(modelIndex, m_selectedItem);

    int modelRowStart = 0;
    for (int mi = 0; mi < modelIndex; mi++)
        modelRowStart += m_profilerModelProxy->model(mi)->height();

    p->save();

    QPen strongPen(m_selectionLocked ? lockedStrongColor : strongColor, strongLineWidth);
    strongPen.setJoinStyle(Qt::MiterJoin);
    QPen lightPen(m_selectionLocked ? lockedLightColor : lightColor, lightLineWidth);
    lightPen.setJoinStyle(Qt::MiterJoin);
    p->setPen(lightPen);
    p->setBrush(Qt::transparent);

    int currentX, currentY, itemWidth;
    for (int i = fromIndex; i <= toIndex; i++) {
        if (m_profilerModelProxy->selectionId(modelIndex, i) != id)
            continue;

        int row = m_profilerModelProxy->row(modelIndex, i);
        int rowHeight = m_profilerModelProxy->rowHeight(modelIndex, row);
        int itemHeight = rowHeight * m_profilerModelProxy->relativeHeight(modelIndex, i);

        currentY = modelRowStart + m_profilerModelProxy->rowOffset(modelIndex, row) + rowHeight -
                itemHeight - y();
        if (currentY + itemHeight < 0 || height() < currentY)
            continue;

        getItemXExtent(modelIndex, i, currentX, itemWidth);

        if (i == m_selectedItem)
            p->setPen(strongPen);

        // Draw the lines at the right offsets. The lines have a width and we don't want them to
        // bleed into the previous or next row as that may belong to a different model and get cut
        // off.
        int lineWidth = p->pen().width();
        itemWidth -= lineWidth;
        itemHeight -= lineWidth;
        currentX += lineWidth / 2;
        currentY += lineWidth / 2;

        // If it's only a line or point, draw it left/top aligned.
        if (itemWidth > 0) {
            if (itemHeight > 0) {
                p->drawRect(currentX, currentY, itemWidth, itemHeight);
            } else {
                p->drawLine(currentX, currentY + itemHeight, currentX + itemWidth,
                            currentY + itemHeight);
            }
        } else if (itemHeight > 0) {
            p->drawLine(currentX + itemWidth, currentY, currentX + itemWidth,
                        currentY + itemHeight);
        } else {
            p->drawPoint(currentX + itemWidth, currentY + itemHeight);
        }

        if (i == m_selectedItem)
            p->setPen(lightPen);
    }

    p->restore();
}

void TimelineRenderer::drawBindingLoopMarkers(QPainter *p, int modelIndex, int fromIndex, int toIndex)
{
    int destindex;
    int xfrom, xto, width;
    int yfrom, yto;
    int radius = 10;
    QPen shadowPen = QPen(QColor("grey"),2);
    QPen markerPen = QPen(QColor("orange"),2);
    QBrush shadowBrush = QBrush(QColor("grey"));
    QBrush markerBrush = QBrush(QColor("orange"));

    p->save();
    for (int i = fromIndex; i <= toIndex; i++) {
        destindex = m_profilerModelProxy->bindingLoopDest(modelIndex, i);
        if (destindex >= 0) {
            // to
            getItemXExtent(modelIndex, destindex, xto, width);
            xto += width / 2;
            yto = getYPosition(modelIndex, destindex) + m_profilerModelProxy->rowHeight(modelIndex,
                    m_profilerModelProxy->row(modelIndex, destindex)) / 2 - y();

            // from
            getItemXExtent(modelIndex, i, xfrom, width);
            xfrom += width / 2;
            yfrom = getYPosition(modelIndex, i) + m_profilerModelProxy->rowHeight(modelIndex,
                    m_profilerModelProxy->row(modelIndex, i)) / 2 - y();

            // radius (derived from width of origin event)
            radius = 5;
            if (radius * 2 > width)
                radius = width / 2;
            if (radius < 2)
                radius = 2;

            int shadowoffset = 2;
            if ((yfrom + radius + shadowoffset < 0 && yto + radius + shadowoffset < 0) ||
                    (yfrom - radius >= height() && yto - radius >= height()))
                continue;

            // shadow
            p->setPen(shadowPen);
            p->setBrush(shadowBrush);
            p->drawEllipse(QPoint(xfrom, yfrom + shadowoffset), radius, radius);
            p->drawEllipse(QPoint(xto, yto + shadowoffset), radius, radius);
            p->drawLine(QPoint(xfrom, yfrom + shadowoffset), QPoint(xto, yto + shadowoffset));


            // marker
            p->setPen(markerPen);
            p->setBrush(markerBrush);
            p->drawEllipse(QPoint(xfrom, yfrom), radius, radius);
            p->drawEllipse(QPoint(xto, yto), radius, radius);
            p->drawLine(QPoint(xfrom, yfrom), QPoint(xto, yto));
        }
    }
    p->restore();
}

void TimelineRenderer::drawNotes(QPainter *p)
{
    static const QColor shadowBrush("grey");
    static const QColor markerBrush("orange");
    static const int annotationWidth = 4;
    static const int annotationHeight1 = 16;
    static const int annotationHeight2 = 4;
    static const int annotationSpace = 4;
    static const int shadowOffset = 2;

    QmlProfilerNotesModel *notes = m_profilerModelProxy->notes();
    for (int i = 0; i < notes->count(); ++i) {
        int managerIndex = notes->timelineModel(i);
        if (managerIndex == -1)
            continue;
        int modelIndex = m_profilerModelProxy->modelIndexFromManagerIndex(managerIndex);
        if (m_profilerModelProxy->hidden(modelIndex))
            continue;
        int eventIndex = notes->timelineIndex(i);
        int row = m_profilerModelProxy->row(modelIndex, eventIndex);
        int rowHeight = m_profilerModelProxy->rowHeight(modelIndex, row);
        int currentY = m_profilerModelProxy->rowOffset(modelIndex, row) - y();
        for (int mi = 0; mi < modelIndex; mi++)
            currentY += m_profilerModelProxy->model(mi)->height();
        if (currentY + rowHeight < 0 || height() < currentY)
            continue;
        int currentX;
        int itemWidth;
        getItemXExtent(modelIndex, eventIndex, currentX, itemWidth);

        // shadow
        int annoX = currentX + (itemWidth - annotationWidth) / 2;
        int annoY = currentY + rowHeight / 2 -
                (annotationHeight1 + annotationHeight2 + annotationSpace) / 2;

        p->setBrush(shadowBrush);
        p->drawRect(annoX, annoY + shadowOffset, annotationWidth, annotationHeight1);
        p->drawRect(annoX, annoY + annotationHeight1 + annotationSpace + shadowOffset,
                annotationWidth, annotationHeight2);

        // marker
        p->setBrush(markerBrush);
        p->drawRect(annoX, annoY, annotationWidth, annotationHeight1);
        p->drawRect(annoX, annoY + annotationHeight1 + annotationSpace,
                annotationWidth, annotationHeight2);
    }
}

int TimelineRenderer::rowFromPosition(int y)
{
    int ret = 0;
    for (int modelIndex = 0; modelIndex < m_profilerModelProxy->modelCount(); modelIndex++) {
        int modelHeight = m_profilerModelProxy->model(modelIndex)->height();
        if (y < modelHeight) {
            for (int row = 0; row < m_profilerModelProxy->rowCount(modelIndex); ++row) {
                y -= m_profilerModelProxy->rowHeight(modelIndex, row);
                if (y < 0) return ret;
                ++ret;
            }
        } else {
            y -= modelHeight;
            ret += m_profilerModelProxy->rowCount(modelIndex);
        }
    }
    return ret;
}

int TimelineRenderer::modelFromPosition(int y)
{
    for (int modelIndex = 0; modelIndex < m_profilerModelProxy->modelCount(); modelIndex++) {
        y -= m_profilerModelProxy->model(modelIndex)->height();
        if (y < 0)
            return modelIndex;
    }
    return 0;
}

void TimelineRenderer::mousePressEvent(QMouseEvent *event)
{
    // special case: if there is a drag area below me, don't accept the
    // events unless I'm actually clicking inside an item
    if (m_currentSelection.eventIndex == -1 &&
            event->pos().x()+x() >= m_startDragArea &&
            event->pos().x()+x() <= m_endDragArea)
        event->setAccepted(false);

}

void TimelineRenderer::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if (!m_profilerModelProxy->isEmpty())
        manageClicked();
}

void TimelineRenderer::mouseMoveEvent(QMouseEvent *event)
{
    event->setAccepted(false);
}


void TimelineRenderer::hoverMoveEvent(QHoverEvent *event)
{
    Q_UNUSED(event);
    manageHovered(event->pos().x(), event->pos().y());
    if (m_currentSelection.eventIndex == -1)
        event->setAccepted(false);
}

void TimelineRenderer::manageClicked()
{
    if (m_currentSelection.eventIndex != -1) {
        if (m_currentSelection.eventIndex == m_selectedItem && m_currentSelection.modelIndex == m_selectedModel)
            setSelectionLocked(!m_selectionLocked);
        else
            setSelectionLocked(true);

        // itemPressed() will trigger an update of the events and JavaScript views. Make sure the
        // correct event is already selected when that happens, to prevent confusion.
        selectFromEventIndex(m_currentSelection.modelIndex, m_currentSelection.eventIndex);
        emit itemPressed(m_currentSelection.modelIndex, m_currentSelection.eventIndex);
    } else {
        setSelectionLocked(false);
        selectFromEventIndex(m_currentSelection.modelIndex, m_currentSelection.eventIndex);
    }
}

void TimelineRenderer::manageHovered(int mouseX, int mouseY)
{
    qint64 duration = m_zoomer->rangeDuration();
    if (duration <= 0)
        return;

    // Make the "selected" area 3 pixels wide by adding/subtracting 1 to catch very narrow events.
    qint64 startTime = (mouseX - 1) * duration / width() + m_zoomer->rangeStart();
    qint64 endTime = (mouseX + 1) * duration / width() + m_zoomer->rangeStart();
    int row = rowFromPosition(mouseY + y());
    int modelIndex = modelFromPosition(mouseY + y());

    // already covered? nothing to do
    if (m_currentSelection.eventIndex != -1 &&
            endTime >= m_currentSelection.startTime &&
            startTime <= m_currentSelection.endTime &&
            row == m_currentSelection.row) {
        return;
    }

    // find if there's items in the time range
    int eventFrom = m_profilerModelProxy->firstIndex(modelIndex, startTime);
    int eventTo = m_profilerModelProxy->lastIndex(modelIndex, endTime);
    if (eventFrom == -1 ||
            eventTo < eventFrom || eventTo >= m_profilerModelProxy->count(modelIndex)) {
        m_currentSelection.eventIndex = -1;
        return;
    }

    int modelRowStart = 0;
    for (int mi = 0; mi < modelIndex; mi++)
        modelRowStart += m_profilerModelProxy->rowCount(mi);

    // find if we are in the right column
    int itemRow;
    for (int i=eventTo; i>=eventFrom; --i) {
        itemRow = modelRowStart + m_profilerModelProxy->row(modelIndex, i);

        if (itemRow == row) {
            // There can be small events that don't reach the cursor position after large events
            // that do but are in a different row.
            qint64 itemEnd = m_profilerModelProxy->endTime(modelIndex, i);
            if (itemEnd < startTime)
                continue;

            // match
            m_currentSelection.eventIndex = i;
            m_currentSelection.startTime = m_profilerModelProxy->startTime(modelIndex, i);
            m_currentSelection.endTime = itemEnd;
            m_currentSelection.row = row;
            m_currentSelection.modelIndex = modelIndex;
            if (!m_selectionLocked)
                selectFromEventIndex(modelIndex, i);
            return;
        }
    }

    m_currentSelection.eventIndex = -1;
    return;
}

void TimelineRenderer::clearData()
{
    m_spacing = 0;
    m_spacedDuration = 0;
    resetCurrentSelection();
    setSelectedItem(-1);
    setSelectedModel(-1);
    setSelectionLocked(true);
    setStartDragArea(-1);
    setEndDragArea(-1);
}

int TimelineRenderer::getYPosition(int modelIndex, int index) const
{
    Q_ASSERT(m_profilerModelProxy);
    if (index >= m_profilerModelProxy->count(modelIndex))
        return 0;

    int modelRowStart = 0;
    for (int mi = 0; mi < modelIndex; mi++)
        modelRowStart += m_profilerModelProxy->model(mi)->height();

    return modelRowStart + m_profilerModelProxy->rowOffset(modelIndex,
            m_profilerModelProxy->row(modelIndex, index));
}

void TimelineRenderer::selectFromEventIndex(int modelIndex, int eventIndex)
{
    if (modelIndex != m_selectedModel || eventIndex != m_selectedItem) {
        setSelectedModel(modelIndex);
        setSelectedItem(eventIndex);
        emit selectionChanged(modelIndex, eventIndex);
    }
}

void TimelineRenderer::selectNextFromSelectionId(int modelIndex, int selectionId)
{
    selectFromEventIndex(modelIndex, m_profilerModelProxy->model(modelIndex)->nextItemBySelectionId(
                             selectionId, m_zoomer->rangeStart(), m_selectedItem));
}

void TimelineRenderer::selectPrevFromSelectionId(int modelIndex, int selectionId)
{
    selectFromEventIndex(modelIndex, m_profilerModelProxy->model(modelIndex)->prevItemBySelectionId(
                             selectionId, m_zoomer->rangeStart(), m_selectedItem));
}
