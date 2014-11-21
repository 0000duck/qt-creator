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

#ifndef TIMELINERENDERER_H
#define TIMELINERENDERER_H

#include <QQuickPaintedItem>
#include <QJSValue>
#include "qmlprofilertimelinemodelproxy.h"
#include "timelinezoomcontrol.h"
#include "timelinemodelaggregator.h"

namespace QmlProfiler {
namespace Internal {

class TimelineRenderer : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QObject *profilerModelProxy READ profilerModelProxy WRITE setProfilerModelProxy NOTIFY profilerModelProxyChanged)
    Q_PROPERTY(QObject *zoomer READ zoomer WRITE setZoomer NOTIFY zoomerChanged)
    Q_PROPERTY(bool selectionLocked READ selectionLocked WRITE setSelectionLocked NOTIFY selectionLockedChanged)
    Q_PROPERTY(int selectedItem READ selectedItem NOTIFY selectedItemChanged)
    Q_PROPERTY(int selectedModel READ selectedModel NOTIFY selectedModelChanged)
    Q_PROPERTY(int startDragArea READ startDragArea WRITE setStartDragArea NOTIFY startDragAreaChanged)
    Q_PROPERTY(int endDragArea READ endDragArea WRITE setEndDragArea NOTIFY endDragAreaChanged)

public:
    explicit TimelineRenderer(QQuickPaintedItem *parent = 0);

    bool selectionLocked() const
    {
        return m_selectionLocked;
    }

    int selectedItem() const
    {
        return m_selectedItem;
    }

    int selectedModel() const
    {
        return m_selectedModel;
    }

    int startDragArea() const
    {
        return m_startDragArea;
    }

    int endDragArea() const
    {
        return m_endDragArea;
    }

    TimelineModelAggregator *profilerModelProxy() const { return m_profilerModelProxy; }
    void setProfilerModelProxy(QObject *profilerModelProxy);

    TimelineZoomControl *zoomer() const { return m_zoomer; }
    void setZoomer(QObject *zoomer);

    Q_INVOKABLE int getYPosition(int modelIndex, int index) const;

    Q_INVOKABLE void selectNext();
    Q_INVOKABLE void selectPrev();
    Q_INVOKABLE int nextItemFromSelectionId(int modelIndex, int selectionId) const;
    Q_INVOKABLE int prevItemFromSelectionId(int modelIndex, int selectionId) const;
    Q_INVOKABLE void selectFromEventIndex(int modelIndex, int index);
    Q_INVOKABLE void selectNextFromSelectionId(int modelIndex, int selectionId);
    Q_INVOKABLE void selectPrevFromSelectionId(int modelIndex, int selectionId);

signals:
    void profilerModelProxyChanged(TimelineModelAggregator *list);
    void zoomerChanged(TimelineZoomControl *zoomer);
    void selectionLockedChanged(bool locked);
    void selectedItemChanged(int itemIndex);
    void selectedModelChanged(int modelIndex);
    void selectionChanged(int modelIndex, int itemIndex);
    void startDragAreaChanged(int startDragArea);
    void endDragAreaChanged(int endDragArea);
    void itemPressed(int modelIndex, int pressedItem);

public slots:
    void clearData();
    void requestPaint();
    void swapSelections(int modelIndex1, int modelIndex2);

    void setSelectionLocked(bool locked)
    {
        if (m_selectionLocked != locked) {
            m_selectionLocked = locked;
            update();
            emit selectionLockedChanged(locked);
        }
    }

    void setStartDragArea(int startDragArea)
    {
        if (m_startDragArea != startDragArea) {
            m_startDragArea = startDragArea;
            emit startDragAreaChanged(startDragArea);
        }
    }

    void setEndDragArea(int endDragArea)
    {
        if (m_endDragArea != endDragArea) {
            m_endDragArea = endDragArea;
            emit endDragAreaChanged(endDragArea);
        }
    }

protected:
    virtual void paint(QPainter *);
    virtual void componentComplete();
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void hoverMoveEvent(QHoverEvent *event);

private:
    void drawItemsToPainter(QPainter *p, int modelIndex, int fromIndex, int toIndex);
    void drawSelectionBoxes(QPainter *p, int modelIndex, int fromIndex, int toIndex);
    void drawBindingLoopMarkers(QPainter *p, int modelIndex, int fromIndex, int toIndex);
    void drawNotes(QPainter *p);

    int modelFromPosition(int y);
    int rowFromPosition(int y);

    void manageClicked();
    void manageHovered(int mouseX, int mouseY);

    void setSelectedItem(int itemIndex)
    {
        if (m_selectedItem != itemIndex) {
            m_selectedItem = itemIndex;
            update();
            emit selectedItemChanged(itemIndex);
        }
    }

    void setSelectedModel(int modelIndex)
    {
        if (m_selectedModel != modelIndex) {
            m_selectedModel = modelIndex;
            update();
            emit selectedModelChanged(modelIndex);
        }
    }

private:
    static const int OutOfScreenMargin = 3; // margin to make sure the rectangles stay invisible
    static const int MinimumItemWidth = 3;

    inline void getItemXExtent(int modelIndex, int i, int &currentX, int &itemWidth);
    void resetCurrentSelection();

    qreal m_spacing;
    qreal m_spacedDuration;

    TimelineModelAggregator *m_profilerModelProxy;
    TimelineZoomControl *m_zoomer;

    struct {
        qint64 startTime;
        qint64 endTime;
        int row;
        int eventIndex;
        int modelIndex;
    } m_currentSelection;

    int m_selectedItem;
    int m_selectedModel;
    bool m_selectionLocked;
    int m_startDragArea;
    int m_endDragArea;
};

} // namespace Internal
} // namespace QmlProfiler

QML_DECLARE_TYPE(QmlProfiler::Internal::TimelineRenderer)

#endif // TIMELINERENDERER_H
