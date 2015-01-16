/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "abstracttimelinemodel.h"
#include "abstracttimelinemodel_p.h"

namespace QmlProfiler {

AbstractTimelineModel::AbstractTimelineModel(AbstractTimelineModelPrivate *dd,
        const QString &displayName, QmlDebug::Message message, QmlDebug::RangeType rangeType,
        QObject *parent) :
    SortedTimelineModel(parent), d_ptr(dd)
{
    Q_D(AbstractTimelineModel);
    connect(this,SIGNAL(rowHeightChanged()),this,SIGNAL(heightChanged()));
    connect(this,SIGNAL(expandedChanged()),this,SIGNAL(heightChanged()));
    connect(this,SIGNAL(hiddenChanged()),this,SIGNAL(heightChanged()));

    d->q_ptr = this;
    d->modelId = 0;
    d->modelManager = 0;
    d->expanded = false;
    d->hidden = false;
    d->displayName = displayName;
    d->message = message;
    d->rangeType = rangeType;
    d->expandedRowCount = 1;
    d->collapsedRowCount = 1;
}

AbstractTimelineModel::~AbstractTimelineModel()
{
    Q_D(AbstractTimelineModel);
    delete d;
}

void AbstractTimelineModel::setModelManager(QmlProfilerModelManager *modelManager)
{
    Q_D(AbstractTimelineModel);
    d->modelManager = modelManager;
    connect(d->modelManager->qmlModel(),SIGNAL(changed()),this,SLOT(dataChanged()));
    d->modelId = d->modelManager->registerModelProxy();
    d->modelManager->announceFeatures(d->modelId, features());
}

bool AbstractTimelineModel::isEmpty() const
{
    return count() == 0;
}

int AbstractTimelineModel::modelId() const
{
    Q_D(const AbstractTimelineModel);
    return d->modelId;
}

int AbstractTimelineModel::rowHeight(int rowNumber) const
{
    Q_D(const AbstractTimelineModel);
    if (!expanded())
        return DefaultRowHeight;

    if (d->rowOffsets.size() > rowNumber)
        return d->rowOffsets[rowNumber] - (rowNumber > 0 ? d->rowOffsets[rowNumber - 1] : 0);
    return DefaultRowHeight;
}

int AbstractTimelineModel::rowOffset(int rowNumber) const
{
    Q_D(const AbstractTimelineModel);
    if (rowNumber == 0)
        return 0;
    if (!expanded())
        return DefaultRowHeight * rowNumber;

    if (d->rowOffsets.size() >= rowNumber)
        return d->rowOffsets[rowNumber - 1];
    if (!d->rowOffsets.empty())
        return d->rowOffsets.last() + (rowNumber - d->rowOffsets.size()) * DefaultRowHeight;
    return rowNumber * DefaultRowHeight;
}

void AbstractTimelineModel::setRowHeight(int rowNumber, int height)
{
    Q_D(AbstractTimelineModel);
    if (d->hidden || !d->expanded)
        return;
    if (height < DefaultRowHeight)
        height = DefaultRowHeight;

    int nextOffset = d->rowOffsets.empty() ? 0 : d->rowOffsets.last();
    while (d->rowOffsets.size() <= rowNumber)
        d->rowOffsets << (nextOffset += DefaultRowHeight);
    int difference = height - d->rowOffsets[rowNumber] +
            (rowNumber > 0 ? d->rowOffsets[rowNumber - 1] : 0);
    if (difference != 0) {
        for (; rowNumber < d->rowOffsets.size(); ++rowNumber) {
            d->rowOffsets[rowNumber] += difference;
        }
        emit rowHeightChanged();
    }
}

int AbstractTimelineModel::height() const
{
    Q_D(const AbstractTimelineModel);
    int depth = rowCount();
    if (d->hidden || !d->expanded || d->rowOffsets.empty())
        return depth * DefaultRowHeight;

    return d->rowOffsets.last() + (depth - d->rowOffsets.size()) * DefaultRowHeight;
}

qint64 AbstractTimelineModel::traceStartTime() const
{
    Q_D(const AbstractTimelineModel);
    return d->modelManager->traceTime()->startTime();
}

qint64 AbstractTimelineModel::traceEndTime() const
{
    Q_D(const AbstractTimelineModel);
    return d->modelManager->traceTime()->endTime();
}

qint64 AbstractTimelineModel::traceDuration() const
{
    Q_D(const AbstractTimelineModel);
    return d->modelManager->traceTime()->duration();
}

QVariantMap AbstractTimelineModel::location(int index) const
{
    Q_UNUSED(index);
    QVariantMap map;
    return map;
}

bool AbstractTimelineModel::isSelectionIdValid(int typeIndex) const
{
    Q_UNUSED(typeIndex);
    return false;
}

int AbstractTimelineModel::selectionIdForLocation(const QString &filename, int line, int column) const
{
    Q_UNUSED(filename);
    Q_UNUSED(line);
    Q_UNUSED(column);
    return -1;
}

int AbstractTimelineModel::bindingLoopDest(int index) const
{
    Q_UNUSED(index);
    return -1;
}

float AbstractTimelineModel::relativeHeight(int index) const
{
    Q_UNUSED(index);
    return 1.0f;
}

int AbstractTimelineModel::rowMinValue(int rowNumber) const
{
    Q_UNUSED(rowNumber);
    return 0;
}

int AbstractTimelineModel::rowMaxValue(int rowNumber) const
{
    Q_UNUSED(rowNumber);
    return 0;
}

void AbstractTimelineModel::dataChanged()
{
    Q_D(AbstractTimelineModel);
    bool wasEmpty = isEmpty();
    switch (d->modelManager->state()) {
    case QmlProfilerDataState::ProcessingData:
        loadData();
        break;
    case QmlProfilerDataState::ClearingData:
        clear();
        break;
    default:
        break;
    }
    if (wasEmpty != isEmpty())
        emit emptyChanged();
}

bool AbstractTimelineModel::accepted(const QmlProfilerDataModel::QmlEventTypeData &event) const
{
    Q_D(const AbstractTimelineModel);
    return (event.rangeType == d->rangeType && event.message == d->message);
}

bool AbstractTimelineModel::expanded() const
{
    Q_D(const AbstractTimelineModel);
    return d->expanded;
}

void AbstractTimelineModel::setExpanded(bool expanded)
{
    Q_D(AbstractTimelineModel);
    if (expanded != d->expanded) {
        d->expanded = expanded;
        emit expandedChanged();
    }
}

bool AbstractTimelineModel::hidden() const
{
    Q_D(const AbstractTimelineModel);
    return d->hidden;
}

void AbstractTimelineModel::setHidden(bool hidden)
{
    Q_D(AbstractTimelineModel);
    if (hidden != d->hidden) {
        d->hidden = hidden;
        emit hiddenChanged();
    }
}

QString AbstractTimelineModel::displayName() const
{
    Q_D(const AbstractTimelineModel);
    return d->displayName;
}

int AbstractTimelineModel::rowCount() const
{
    Q_D(const AbstractTimelineModel);
    if (d->hidden)
        return 0;
    if (isEmpty())
        return d->modelManager->isEmpty() ? 1 : 0;
    return d->expanded ? d->expandedRowCount : d->collapsedRowCount;
}

int AbstractTimelineModel::selectionId(int index) const
{
    return range(index).typeId;
}

void AbstractTimelineModel::clear()
{
    Q_D(AbstractTimelineModel);
    d->collapsedRowCount = d->expandedRowCount = 1;
    bool wasExpanded = d->expanded;
    bool wasHidden = d->hidden;
    bool hadRowHeights = !d->rowOffsets.empty();
    d->rowOffsets.clear();
    d->expanded = false;
    d->hidden = false;
    SortedTimelineModel::clear();
    if (hadRowHeights)
        emit rowHeightChanged();
    if (wasExpanded)
        emit expandedChanged();
    if (wasHidden)
        emit hiddenChanged();
    d->modelManager->modelProxyCountUpdated(d->modelId, 0, 1);
}

}
