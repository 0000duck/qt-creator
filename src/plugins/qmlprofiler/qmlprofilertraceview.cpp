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

#include "qmlprofilertraceview.h"
#include "qmlprofilertool.h"
#include "qmlprofilerstatemanager.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilertimelinemodelproxy.h"
#include "timelinemodelaggregator.h"

// Needed for the load&save actions in the context menu
#include <analyzerbase/ianalyzertool.h>

// Communication with the other views (limit events to range)
#include "qmlprofilerviewmanager.h"
#include "timelinezoomcontrol.h"

#include <utils/styledbar.h>

#include <QQmlContext>
#include <QToolButton>
#include <QEvent>
#include <QVBoxLayout>
#include <QGraphicsObject>
#include <QScrollBar>
#include <QSlider>
#include <QMenu>
#include <QQuickItem>
#include <QApplication>

#include <math.h>

using namespace QmlDebug;

namespace QmlProfiler {
namespace Internal {

/////////////////////////////////////////////////////////
class QmlProfilerTraceView::QmlProfilerTraceViewPrivate
{
public:
    QmlProfilerTraceViewPrivate(QmlProfilerTraceView *qq) : q(qq) {}
    ~QmlProfilerTraceViewPrivate()
    {
        delete m_mainView;
    }

    QmlProfilerTraceView *q;

    QmlProfilerStateManager *m_profilerState;
    Analyzer::IAnalyzerTool *m_profilerTool;
    QmlProfilerViewManager *m_viewContainer;

    QSize m_sizeHint;

    QQuickView *m_mainView;
    QmlProfilerModelManager *m_modelManager;
    TimelineModelAggregator *m_modelProxy;


    TimelineZoomControl *m_zoomControl;
};

QmlProfilerTraceView::QmlProfilerTraceView(QWidget *parent, Analyzer::IAnalyzerTool *profilerTool, QmlProfilerViewManager *container, QmlProfilerModelManager *modelManager, QmlProfilerStateManager *profilerState)
    : QWidget(parent), d(new QmlProfilerTraceViewPrivate(this))
{
    setObjectName(QLatin1String("QML Profiler"));

    d->m_zoomControl = new TimelineZoomControl(this);
    connect(modelManager->traceTime(), &QmlProfilerTraceTime::timeChanged,
            d->m_zoomControl, &TimelineZoomControl::setTrace);

    QVBoxLayout *groupLayout = new QVBoxLayout;
    groupLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->setSpacing(0);

    d->m_mainView = new QmlProfilerQuickView(this);
    d->m_mainView->setResizeMode(QQuickView::SizeRootObjectToView);
    QWidget *mainViewContainer = QWidget::createWindowContainer(d->m_mainView);
    mainViewContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    enableToolbar(false);

    groupLayout->addWidget(mainViewContainer);
    setLayout(groupLayout);

    d->m_profilerTool = profilerTool;
    d->m_viewContainer = container;
    d->m_modelManager = modelManager;
    d->m_modelProxy = new TimelineModelAggregator(this);
    d->m_modelProxy->setModelManager(modelManager);
    connect(d->m_modelManager, SIGNAL(stateChanged()),
            this, SLOT(profilerDataModelStateChanged()));
    d->m_mainView->rootContext()->setContextProperty(QLatin1String("qmlProfilerModelProxy"),
                                                     d->m_modelProxy);
    d->m_profilerState = profilerState;

    // Minimum height: 5 rows of 20 pixels + scrollbar of 50 pixels + 20 pixels margin
    setMinimumHeight(170);
}

QmlProfilerTraceView::~QmlProfilerTraceView()
{
    delete d;
}

/////////////////////////////////////////////////////////
// Initialize widgets
void QmlProfilerTraceView::reset()
{
    d->m_mainView->rootContext()->setContextProperty(QLatin1String("zoomControl"), d->m_zoomControl);
    d->m_mainView->setSource(QUrl(QLatin1String("qrc:/qmlprofiler/MainView.qml")));

    QQuickItem *rootObject = d->m_mainView->rootObject();
    connect(rootObject, SIGNAL(updateCursorPosition()), this, SLOT(updateCursorPosition()));
}

/////////////////////////////////////////////////////////
bool QmlProfilerTraceView::hasValidSelection() const
{
    QQuickItem *rootObject = d->m_mainView->rootObject();
    if (rootObject)
        return rootObject->property("selectionRangeReady").toBool();
    return false;
}

void QmlProfilerTraceView::enableToolbar(bool enable)
{
    QMetaObject::invokeMethod(d->m_mainView->rootObject(), "enableButtonsBar",
                              Q_ARG(QVariant,QVariant(enable)));
}

qint64 QmlProfilerTraceView::selectionStart() const
{
    QQuickItem *rootObject = d->m_mainView->rootObject();
    if (rootObject)
        return rootObject->property("selectionRangeStart").toLongLong();
    return 0;
}

qint64 QmlProfilerTraceView::selectionEnd() const
{
    QQuickItem *rootObject = d->m_mainView->rootObject();
    if (rootObject)
        return rootObject->property("selectionRangeEnd").toLongLong();
    return 0;
}

void QmlProfilerTraceView::clear()
{
    QMetaObject::invokeMethod(d->m_mainView->rootObject(), "clear");
}

void QmlProfilerTraceView::selectByTypeId(int typeId)
{
    QQuickItem *rootObject = d->m_mainView->rootObject();
    if (!rootObject)
        return;

    for (int modelIndex = 0; modelIndex < d->m_modelProxy->modelCount(); ++modelIndex) {
        if (d->m_modelProxy->isSelectionIdValid(modelIndex, typeId)) {
            QMetaObject::invokeMethod(rootObject, "selectBySelectionId",
                                      Q_ARG(QVariant,QVariant(modelIndex)),
                                      Q_ARG(QVariant,QVariant(typeId)));
            return;
        }
    }
}

void QmlProfilerTraceView::selectBySourceLocation(const QString &filename, int line, int column)
{
    QQuickItem *rootObject = d->m_mainView->rootObject();
    if (!rootObject)
        return;

    for (int modelIndex = 0; modelIndex < d->m_modelProxy->modelCount(); ++modelIndex) {
        int typeId = d->m_modelProxy->selectionIdForLocation(modelIndex, filename, line, column);
        if (typeId != -1) {
            QMetaObject::invokeMethod(rootObject, "selectBySelectionId",
                                      Q_ARG(QVariant,QVariant(modelIndex)),
                                      Q_ARG(QVariant,QVariant(typeId)));
            return;
        }
    }
}

/////////////////////////////////////////////////////////
// Goto source location
void QmlProfilerTraceView::updateCursorPosition()
{
    QQuickItem *rootObject = d->m_mainView->rootObject();
    QString file = rootObject->property("fileName").toString();
    if (!file.isEmpty())
        emit gotoSourceLocation(file, rootObject->property("lineNumber").toInt(),
                                rootObject->property("columnNumber").toInt());

    emit typeSelected(rootObject->property("typeId").toInt());
}

////////////////////////////////////////////////////////
void QmlProfilerTraceView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    emit resized();
}

void QmlProfilerTraceView::mousePressEvent(QMouseEvent *event)
{
    d->m_zoomControl->setWindowLocked(true);
    QWidget::mousePressEvent(event);
}

void QmlProfilerTraceView::mouseReleaseEvent(QMouseEvent *event)
{
    d->m_zoomControl->setWindowLocked(false);
    QWidget::mouseReleaseEvent(event);
}

////////////////////////////////////////////////////////////////
// Context menu
void QmlProfilerTraceView::contextMenuEvent(QContextMenuEvent *ev)
{
    showContextMenu(ev->globalPos());
}

void QmlProfilerTraceView::showContextMenu(QPoint position)
{
    QMenu menu;
    QAction *viewAllAction = 0;

    QmlProfilerTool *profilerTool = qobject_cast<QmlProfilerTool *>(d->m_profilerTool);

    if (profilerTool)
        menu.addActions(profilerTool->profilerContextMenuActions());

    menu.addSeparator();

    QAction *getLocalStatsAction = menu.addAction(tr("Limit Events Pane to Current Range"));
    if (!d->m_viewContainer->hasValidSelection())
        getLocalStatsAction->setEnabled(false);

    QAction *getGlobalStatsAction = menu.addAction(tr("Show Full Range in Events Pane"));
    if (d->m_viewContainer->hasGlobalStats())
        getGlobalStatsAction->setEnabled(false);

    if (!d->m_modelProxy->isEmpty()) {
        menu.addSeparator();
        viewAllAction = menu.addAction(tr("Reset Zoom"));
    }

    QAction *selectedAction = menu.exec(position);

    if (selectedAction) {
        if (selectedAction == viewAllAction) {
            d->m_zoomControl->setRange(d->m_zoomControl->traceStart(),
                                       d->m_zoomControl->traceEnd());
        }
        if (selectedAction == getLocalStatsAction) {
            d->m_viewContainer->getStatisticsInRange(
                        d->m_viewContainer->selectionStart(),
                        d->m_viewContainer->selectionEnd());
        }
        if (selectedAction == getGlobalStatsAction)
            d->m_viewContainer->getStatisticsInRange(-1, -1);
    }
}

////////////////////////////////////////////////////////////////
// Profiler State
void QmlProfilerTraceView::profilerDataModelStateChanged()
{
    switch (d->m_modelManager->state()) {
        case QmlProfilerDataState::Empty: break;
        case QmlProfilerDataState::ClearingData:
            d->m_mainView->hide();
            enableToolbar(false);
        break;
        case QmlProfilerDataState::AcquiringData: break;
        case QmlProfilerDataState::ProcessingData: break;
        case QmlProfilerDataState::Done:
            enableToolbar(true);
            d->m_mainView->show();
        break;
    default:
        break;
    }
}

bool QmlProfilerQuickView::event(QEvent *ev)
{
    // We assume context menus can only be triggered by mouse press, mouse release, or
    // pre-synthesized events from the window system.

    bool relayed = false;
    switch (ev->type()) {
    case QEvent::ContextMenu:
        // In the case of mouse clicks the active popup gets automatically closed before they show
        // up here. That's not necessarily the case with keyboard triggered context menu events, so
        // we just ignore them if there is a popup already. Also, the event's pos() and globalPos()
        // don't make much sense in this case, so we just put the menu in the upper left corner.
        if (QApplication::activePopupWidget() == 0) {
            ev->accept();
            parent->showContextMenu(parent->mapToGlobal(QPoint(0,0)));
            relayed = true;
        }
        break;
    case QEvent::MouseMove:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease: {
        QMouseEvent *orig = static_cast<QMouseEvent *>(ev);
        QCoreApplication::instance()->postEvent(parent->window()->windowHandle(),
                new QMouseEvent(orig->type(), parent->window()->mapFromGlobal(orig->globalPos()),
                                orig->button(), orig->buttons(), orig->modifiers()));
        relayed = true;
        break;
    }
    default:
        break;
    }

    // QQuickView will eat mouse events even if they're not accepted by any QML construct. So we
    // ignore the return value of event() above.
    return QQuickView::event(ev) || relayed;
}

} // namespace Internal
} // namespace QmlProfiler
