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

#include "timelinemodelaggregator.h"

#include "qmlprofilertimelinemodelproxy.h"
#include "qmlprofilerpainteventsmodelproxy.h"
#include "qmlprofilerplugin.h"
#include "notesmodel.h"

#include <QStringList>
#include <QVariant>

namespace QmlProfiler {
namespace Internal {


class TimelineModelAggregator::TimelineModelAggregatorPrivate {
public:
    TimelineModelAggregatorPrivate(TimelineModelAggregator *qq):q(qq) {}
    ~TimelineModelAggregatorPrivate() {}

    TimelineModelAggregator *q;

    // mapping of modelId assigned by manager to index in our list
    QList <int> modelManagerIndexMapping;

    QList <AbstractTimelineModel *> modelList;
    QmlProfilerModelManager *modelManager;
};

TimelineModelAggregator::TimelineModelAggregator(QObject *parent)
    : QObject(parent), d(new TimelineModelAggregatorPrivate(this))
{
    connect(this,SIGNAL(modelsChanged(int,int)),this,SIGNAL(heightChanged()));
    connect(this,SIGNAL(stateChanged()),this,SIGNAL(heightChanged()));
}

TimelineModelAggregator::~TimelineModelAggregator()
{
    delete d;
}

int TimelineModelAggregator::height() const
{
    return modelOffset(d->modelList.length());
}

void TimelineModelAggregator::setModelManager(QmlProfilerModelManager *modelManager)
{
    d->modelManager = modelManager;
    connect(modelManager,SIGNAL(stateChanged()),this,SLOT(dataChanged()));
    connect(modelManager,SIGNAL(dataAvailable()),this,SIGNAL(dataAvailable()));

    // external models pushed on top
    foreach (AbstractTimelineModel *timelineModel, QmlProfilerPlugin::instance->getModels()) {
        timelineModel->setModelManager(modelManager);
        addModel(timelineModel);
    }

    PaintEventsModelProxy *paintEventsModelProxy = new PaintEventsModelProxy(this);
    paintEventsModelProxy->setModelManager(modelManager);
    addModel(paintEventsModelProxy);

    for (int i = 0; i < QmlDebug::MaximumRangeType; ++i) {
        RangeTimelineModel *rangeModel = new RangeTimelineModel((QmlDebug::RangeType)i, this);
        rangeModel->setModelManager(modelManager);
        addModel(rangeModel);
    }

    // Connect this last so that it's executed after the models have updated their data.
    connect(modelManager->qmlModel(),SIGNAL(changed()),this,SIGNAL(stateChanged()));
    connect(modelManager->notesModel(), SIGNAL(changed(int,int,int)),
            this, SIGNAL(notesChanged(int,int,int)));
}

void TimelineModelAggregator::addModel(AbstractTimelineModel *m)
{
    while (d->modelManagerIndexMapping.size() <= m->modelId())
        d->modelManagerIndexMapping.append(-1);
    d->modelManagerIndexMapping[m->modelId()] = d->modelList.size();
    d->modelList << m;
    connect(m,SIGNAL(expandedChanged()),this,SIGNAL(expandedChanged()));
    connect(m,SIGNAL(hiddenChanged()),this,SIGNAL(hiddenChanged()));
    connect(m,SIGNAL(rowHeightChanged()),this,SIGNAL(rowHeightChanged()));
    connect(m,SIGNAL(heightChanged()),this,SIGNAL(heightChanged()));
    d->modelManager->notesModel()->addTimelineModel(m);
    emit modelsChanged(d->modelList.length(), d->modelList.length());
}

const AbstractTimelineModel *TimelineModelAggregator::model(int modelIndex) const
{
    return d->modelList[modelIndex];
}

QVariantList TimelineModelAggregator::models() const
{
    QVariantList ret;
    foreach (AbstractTimelineModel *model, d->modelList)
        ret << QVariant::fromValue(model);
    return ret;
}

int TimelineModelAggregator::modelIndexFromManagerIndex(int modelManagerIndex) const
{
    return d->modelManagerIndexMapping[modelManagerIndex];
}

NotesModel *TimelineModelAggregator::notes() const
{
    return d->modelManager->notesModel();
}

int TimelineModelAggregator::count(int modelIndex) const
{
    return d->modelList[modelIndex]->count();
}

bool TimelineModelAggregator::isEmpty() const
{
    foreach (const AbstractTimelineModel *modelProxy, d->modelList)
        if (!modelProxy->isEmpty())
            return false;
    return true;
}

int TimelineModelAggregator::modelOffset(int modelIndex) const
{
    int ret = 0;
    for (int i = 0; i < modelIndex; ++i)
        ret += d->modelList[i]->height();
    return ret;
}

int TimelineModelAggregator::rowHeight(int modelIndex, int row) const
{
    return d->modelList[modelIndex]->rowHeight(row);
}

int TimelineModelAggregator::rowOffset(int modelIndex, int row) const
{
    return d->modelList[modelIndex]->rowOffset(row);
}

void TimelineModelAggregator::setRowHeight(int modelIndex, int row, int height)
{
    d->modelList[modelIndex]->setRowHeight(row, height);
}

bool TimelineModelAggregator::expanded(int modelIndex) const
{
    return d->modelList[modelIndex]->expanded();
}

void TimelineModelAggregator::setExpanded(int modelIndex, bool expanded)
{
    d->modelList[modelIndex]->setExpanded(expanded);
}

bool TimelineModelAggregator::hidden(int modelIndex) const
{
    return d->modelList[modelIndex]->hidden();
}

void TimelineModelAggregator::setHidden(int modelIndex, bool hidden)
{
    d->modelList[modelIndex]->setHidden(hidden);
}

int TimelineModelAggregator::rowCount(int modelIndex) const
{
    return d->modelList[modelIndex]->rowCount();
}

QString TimelineModelAggregator::displayName(int modelIndex) const
{
    return d->modelList[modelIndex]->displayName();
}

int TimelineModelAggregator::rowMinValue(int modelIndex, int row) const
{
    return d->modelList[modelIndex]->rowMinValue(row);
}

int TimelineModelAggregator::rowMaxValue(int modelIndex, int row) const
{
    return d->modelList[modelIndex]->rowMaxValue(row);
}

int TimelineModelAggregator::firstIndex(int modelIndex, qint64 startTime) const
{
    return d->modelList[modelIndex]->firstIndex(startTime);
}

int TimelineModelAggregator::firstIndexNoParents(int modelIndex, qint64 startTime) const
{
    return d->modelList[modelIndex]->firstIndexNoParents(startTime);
}

int TimelineModelAggregator::lastIndex(int modelIndex, qint64 endTime) const
{
    return d->modelList[modelIndex]->lastIndex(endTime);
}

int TimelineModelAggregator::row(int modelIndex, int index) const
{
    return d->modelList[modelIndex]->row(index);
}

qint64 TimelineModelAggregator::duration(int modelIndex, int index) const
{
    return d->modelList[modelIndex]->duration(index);
}

qint64 TimelineModelAggregator::startTime(int modelIndex, int index) const
{
    return d->modelList[modelIndex]->startTime(index);
}

qint64 TimelineModelAggregator::endTime(int modelIndex, int index) const
{
    return d->modelList[modelIndex]->endTime(index);
}

int TimelineModelAggregator::selectionId(int modelIndex, int index) const
{
    return d->modelList[modelIndex]->selectionId(index);
}

int TimelineModelAggregator::typeId(int modelIndex, int index) const
{
    return d->modelList[modelIndex]->typeId(index);
}

int TimelineModelAggregator::bindingLoopDest(int modelIndex,int index) const
{
    return d->modelList[modelIndex]->bindingLoopDest(index);
}

QColor TimelineModelAggregator::color(int modelIndex, int index) const
{
    return d->modelList[modelIndex]->color(index);
}

float TimelineModelAggregator::relativeHeight(int modelIndex, int index) const
{
    return d->modelList[modelIndex]->relativeHeight(index);
}

QVariantList TimelineModelAggregator::labels(int modelIndex) const
{
    return d->modelList[modelIndex]->labels();
}

QVariantMap TimelineModelAggregator::details(int modelIndex, int index) const
{
    return d->modelList[modelIndex]->details(index);
}

QVariantMap TimelineModelAggregator::location(int modelIndex, int index) const
{
    return d->modelList[modelIndex]->location(index);
}

bool TimelineModelAggregator::isSelectionIdValid(int modelIndex, int typeIndex) const
{
    return d->modelList[modelIndex]->isSelectionIdValid(typeIndex);
}

int TimelineModelAggregator::selectionIdForLocation(int modelIndex, const QString &filename,
                                                   int line, int column) const
{
    return d->modelList[modelIndex]->selectionIdForLocation(filename, line, column);
}

void TimelineModelAggregator::swapModels(int modelIndex1, int modelIndex2)
{
    AbstractTimelineModel *&model1 = d->modelList[modelIndex1];
    AbstractTimelineModel *&model2 = d->modelList[modelIndex2];
    std::swap(d->modelManagerIndexMapping[model1->modelId()],
              d->modelManagerIndexMapping[model2->modelId()]);
    std::swap(model1, model2);
    emit modelsChanged(modelIndex1, modelIndex2);
}

QString TimelineModelAggregator::noteText(int noteId) const
{
    return d->modelManager->notesModel()->text(noteId);
}

QString TimelineModelAggregator::noteText(int modelIndex, int index) const
{
    int managerId = d->modelList[modelIndex]->modelId();
    int noteId = d->modelManager->notesModel()->get(managerId, index);
    return noteId != -1 ? noteText(noteId) : QString();
}

void TimelineModelAggregator::setNoteText(int noteId, const QString &text)
{
    if (text.length() > 0) {
        notes()->update(noteId, text);
    } else {
        notes()->remove(noteId);
    }
}

void TimelineModelAggregator::setNoteText(int modelIndex, int index, const QString &text)
{
    int managerId = d->modelList[modelIndex]->modelId();
    NotesModel *notesModel = notes();
    int noteId = notesModel->get(managerId, index);
    if (noteId == -1) {
        if (text.length() > 0)
            notesModel->add(managerId, index, text);
    } else {
        setNoteText(noteId, text);
    }
}

int TimelineModelAggregator::noteTimelineModel(int noteIndex) const
{
    return d->modelManagerIndexMapping[notes()->timelineModel(noteIndex)];
}

int TimelineModelAggregator::noteTimelineIndex(int noteIndex) const
{
    return notes()->timelineIndex(noteIndex);
}

QVariantList TimelineModelAggregator::notesByTimelineModel(int modelIndex) const
{
    int managerId = d->modelList[modelIndex]->modelId();
    return notes()->byTimelineModel(managerId);
}

QVariantList TimelineModelAggregator::notesByTypeId(int typeId) const
{
    return notes()->byTypeId(typeId);
}

int TimelineModelAggregator::noteCount() const
{
    return notes()->count();
}

void TimelineModelAggregator::dataChanged()
{
    // this is a slot connected for every modelproxy
    // nothing to do here, each model will take care of itself
}

int TimelineModelAggregator::modelCount() const
{
    return d->modelList.count();
}

qint64 TimelineModelAggregator::traceStartTime() const
{
    return d->modelManager->traceTime()->startTime();
}

qint64 TimelineModelAggregator::traceEndTime() const
{
    return d->modelManager->traceTime()->endTime();
}

qint64 TimelineModelAggregator::traceDuration() const
{
    return d->modelManager->traceTime()->duration();
}

} // namespace Internal
} // namespace QmlProfiler
