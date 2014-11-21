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

#include "qmlprofilerrangemodel.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerdatamodel.h"
#include "abstracttimelinemodel_p.h"

#include <QCoreApplication>
#include <QVector>
#include <QHash>
#include <QUrl>
#include <QString>
#include <QStack>

#include <QDebug>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerRangeModel::QmlProfilerRangeModelPrivate : public AbstractTimelineModelPrivate
{
public:
    // convenience functions
    void computeNestingContracted();
    void computeExpandedLevels();
    void findBindingLoops();

    QVector<QmlRangeEventStartInstance> data;
    QVector<int> expandedRowTypes;
private:
    Q_DECLARE_PUBLIC(QmlProfilerRangeModel)
};

QmlProfilerRangeModel::QmlProfilerRangeModel(QmlDebug::RangeType rangeType, QObject *parent)
    : AbstractTimelineModel(new QmlProfilerRangeModelPrivate, categoryLabel(rangeType),
                            QmlDebug::MaximumMessage, rangeType, parent)
{
    Q_D(QmlProfilerRangeModel);
    d->expandedRowTypes << -1;
}

quint64 QmlProfilerRangeModel::features() const
{
    Q_D(const QmlProfilerRangeModel);
    return 1ULL << QmlDebug::featureFromRangeType(d->rangeType);
}

void QmlProfilerRangeModel::clear()
{
    Q_D(QmlProfilerRangeModel);
    d->expandedRowTypes.clear();
    d->expandedRowTypes << -1;
    d->data.clear();
    AbstractTimelineModel::clear();
}

void QmlProfilerRangeModel::loadData()
{
    Q_D(QmlProfilerRangeModel);
    clear();
    QmlProfilerDataModel *simpleModel = d->modelManager->qmlModel();
    if (simpleModel->isEmpty())
        return;

    // collect events
    const QVector<QmlProfilerDataModel::QmlEventData> &eventList = simpleModel->getEvents();
    const QVector<QmlProfilerDataModel::QmlEventTypeData> &typesList = simpleModel->getEventTypes();
    foreach (const QmlProfilerDataModel::QmlEventData &event, eventList) {
        const QmlProfilerDataModel::QmlEventTypeData &type = typesList[event.typeIndex];
        if (!accepted(type))
            continue;

        // store starttime-based instance
        d->data.insert(insert(event.startTime, event.duration, event.typeIndex),
                       QmlRangeEventStartInstance());
        d->modelManager->modelProxyCountUpdated(d->modelId, count(), eventList.count() * 6);
    }

    d->modelManager->modelProxyCountUpdated(d->modelId, 2, 6);

    // compute range nesting
    computeNesting();

    // compute nestingLevel - nonexpanded
    d->computeNestingContracted();

    d->modelManager->modelProxyCountUpdated(d->modelId, 3, 6);

    // compute nestingLevel - expanded
    d->computeExpandedLevels();

    d->modelManager->modelProxyCountUpdated(d->modelId, 4, 6);

    d->findBindingLoops();

    d->modelManager->modelProxyCountUpdated(d->modelId, 5, 6);

    d->modelManager->modelProxyCountUpdated(d->modelId, 1, 1);
}

void QmlProfilerRangeModel::QmlProfilerRangeModelPrivate::computeNestingContracted()
{
    Q_Q(QmlProfilerRangeModel);
    int i;
    int eventCount = q->count();

    int nestingLevels = QmlDebug::Constants::QML_MIN_LEVEL;
    collapsedRowCount = nestingLevels + 1;
    QVector<qint64> nestingEndTimes;
    nestingEndTimes.fill(0, nestingLevels + 1);

    for (i = 0; i < eventCount; i++) {
        qint64 st = ranges[i].start;

        // per type
        if (nestingEndTimes[nestingLevels] > st) {
            if (++nestingLevels == nestingEndTimes.size())
                nestingEndTimes << 0;
            if (nestingLevels == collapsedRowCount)
                ++collapsedRowCount;
        } else {
            while (nestingLevels > QmlDebug::Constants::QML_MIN_LEVEL &&
                   nestingEndTimes[nestingLevels-1] <= st)
                nestingLevels--;
        }
        nestingEndTimes[nestingLevels] = st + ranges[i].duration;

        data[i].displayRowCollapsed = nestingLevels;
    }
}

void QmlProfilerRangeModel::QmlProfilerRangeModelPrivate::computeExpandedLevels()
{
    Q_Q(QmlProfilerRangeModel);
    QHash<int, int> eventRow;
    int eventCount = q->count();
    for (int i = 0; i < eventCount; i++) {
        int typeId = ranges[i].typeId;
        if (!eventRow.contains(typeId)) {
            eventRow[typeId] = expandedRowTypes.size();
            expandedRowTypes << typeId;
        }
        data[i].displayRowExpanded = eventRow[typeId];
    }
    expandedRowCount = expandedRowTypes.size();
}

void QmlProfilerRangeModel::QmlProfilerRangeModelPrivate::findBindingLoops()
{
    Q_Q(QmlProfilerRangeModel);
    if (rangeType != QmlDebug::Binding && rangeType != QmlDebug::HandlingSignal)
        return;

    typedef QPair<int, int> CallStackEntry;
    QStack<CallStackEntry> callStack;

    for (int i = 0; i < q->count(); ++i) {
        const Range *potentialParent = callStack.isEmpty()
                ? 0 : &ranges[callStack.top().second];

        while (potentialParent
               && !(potentialParent->start + potentialParent->duration > ranges[i].start)) {
            callStack.pop();
            potentialParent = callStack.isEmpty() ? 0 : &ranges[callStack.top().second];
        }

        // check whether event is already in stack
        for (int ii = 0; ii < callStack.size(); ++ii) {
            if (callStack.at(ii).first == ranges[i].typeId) {
                data[i].bindingLoopHead = callStack.at(ii).second;
                break;
            }
        }

        CallStackEntry newEntry(ranges[i].typeId, i);
        callStack.push(newEntry);
    }

}

/////////////////// QML interface

QString QmlProfilerRangeModel::categoryLabel(QmlDebug::RangeType rangeType)
{
    return QCoreApplication::translate("MainView",
            QmlProfilerModelManager::featureName(QmlDebug::featureFromRangeType(rangeType)));
}

int QmlProfilerRangeModel::row(int index) const
{
    Q_D(const QmlProfilerRangeModel);
    if (d->expanded)
        return d->data[index].displayRowExpanded;
    else
        return d->data[index].displayRowCollapsed;
}

int QmlProfilerRangeModel::bindingLoopDest(int index) const
{
    Q_D(const QmlProfilerRangeModel);
    return d->data[index].bindingLoopHead;
}

QColor QmlProfilerRangeModel::color(int index) const
{
    return colorBySelectionId(index);
}

QVariantList QmlProfilerRangeModel::labels() const
{
    Q_D(const QmlProfilerRangeModel);
    QVariantList result;

    if (d->expanded && !d->hidden) {
        const QVector<QmlProfilerDataModel::QmlEventTypeData> &types =
                d->modelManager->qmlModel()->getEventTypes();
        for (int i = 1; i < d->expandedRowCount; i++) { // Ignore the -1 for the first row
            QVariantMap element;
            int typeId = d->expandedRowTypes[i];
            element.insert(QLatin1String("displayName"), QVariant(types[typeId].displayName));
            element.insert(QLatin1String("description"), QVariant(types[typeId].data));
            element.insert(QLatin1String("id"), QVariant(typeId));
            result << element;
        }
    }

    return result;
}

QVariantMap QmlProfilerRangeModel::details(int index) const
{
    Q_D(const QmlProfilerRangeModel);
    QVariantMap result;
    int id = selectionId(index);
    const QVector<QmlProfilerDataModel::QmlEventTypeData> &types =
            d->modelManager->qmlModel()->getEventTypes();

    result.insert(QStringLiteral("displayName"), categoryLabel(d->rangeType));
    result.insert(tr("Duration"), QmlProfilerBaseModel::formatTime(duration(index)));

    result.insert(tr("Details"), types[id].data);
    result.insert(tr("Location"), types[id].displayName);
    return result;
}

QVariantMap QmlProfilerRangeModel::location(int index) const
{
    Q_D(const QmlProfilerRangeModel);
    QVariantMap result;
    int id = selectionId(index);

    const QmlDebug::QmlEventLocation &location
            = d->modelManager->qmlModel()->getEventTypes().at(id).location;

    result.insert(QStringLiteral("file"), location.filename);
    result.insert(QStringLiteral("line"), location.line);
    result.insert(QStringLiteral("column"), location.column);

    return result;
}

bool QmlProfilerRangeModel::isSelectionIdValid(int typeId) const
{
    Q_D(const QmlProfilerRangeModel);
    if (typeId < 0)
        return false;
    const QmlProfilerDataModel::QmlEventTypeData &type =
            d->modelManager->qmlModel()->getEventTypes().at(typeId);
    if (type.message != d->message || type.rangeType != d->rangeType)
        return false;
    return true;
}

int QmlProfilerRangeModel::selectionIdForLocation(const QString &filename, int line, int column) const
{
    Q_D(const QmlProfilerRangeModel);
    // if this is called from v8 view, we don't have the column number, it will be -1
    const QVector<QmlProfilerDataModel::QmlEventTypeData> &types =
            d->modelManager->qmlModel()->getEventTypes();
    for (int i = 1; i < d->expandedRowCount; ++i) {
        int typeId = d->expandedRowTypes[i];
        const QmlProfilerDataModel::QmlEventTypeData &eventData = types[typeId];
        if (eventData.location.filename == filename &&
                eventData.location.line == line &&
                (column == -1 || eventData.location.column == column))
            return typeId;
    }
    return -1;
}



}
}
