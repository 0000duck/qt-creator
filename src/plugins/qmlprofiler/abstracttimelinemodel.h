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

#ifndef ABSTRACTTIMELINEMODEL_H
#define ABSTRACTTIMELINEMODEL_H


#include "qmlprofiler_global.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerdatamodel.h"
#include "sortedtimelinemodel.h"
#include <QVariant>
#include <QColor>

namespace QmlProfiler {

class QMLPROFILER_EXPORT AbstractTimelineModel : public SortedTimelineModel
{
    Q_OBJECT
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(bool empty READ isEmpty NOTIFY emptyChanged)
    Q_PROPERTY(bool hidden READ hidden WRITE setHidden NOTIFY hiddenChanged)
    Q_PROPERTY(int height READ height NOTIFY heightChanged)

public:
    class AbstractTimelineModelPrivate;
    ~AbstractTimelineModel();

    // Trivial methods implemented by the abstract model itself
    void setModelManager(QmlProfilerModelManager *modelManager);
    bool isEmpty() const;
    int modelId() const;

    // Methods are directly passed on to the private model and relying on its virtual methods.
    int rowHeight(int rowNumber) const;
    int rowOffset(int rowNumber) const;
    void setRowHeight(int rowNumber, int height);
    int height() const;

    bool accepted(const QmlProfilerDataModel::QmlEventTypeData &event) const;
    bool expanded() const;
    bool hidden() const;
    void setExpanded(bool expanded);
    void setHidden(bool hidden);
    QString displayName() const;
    int rowCount() const;

    // Methods that have to be implemented by child models
    virtual QColor color(int index) const = 0;
    virtual QVariantList labels() const = 0;
    virtual QVariantMap details(int index) const = 0;
    virtual int row(int index) const = 0;
    virtual quint64 features() const = 0;

    // Methods which can optionally be implemented by child models.
    // returned map should contain "file", "line", "column" properties, or be empty
    virtual QVariantMap location(int index) const;
    virtual int selectionId(int index) const;
    virtual bool isSelectionIdValid(int selectionId) const;
    virtual int selectionIdForLocation(const QString &filename, int line, int column) const;
    virtual int bindingLoopDest(int index) const;
    virtual float relativeHeight(int index) const;
    virtual int rowMinValue(int rowNumber) const;
    virtual int rowMaxValue(int rowNumber) const;

signals:
    void expandedChanged();
    void hiddenChanged();
    void rowHeightChanged();
    void emptyChanged();
    void heightChanged();

protected:
    static const int DefaultRowHeight = 30;

    enum BoxColorProperties {
        SelectionIdHueMultiplier = 25,
        FractionHueMultiplier = 96,
        FractionHueMininimum = 10,
        Saturation = 150,
        Lightness = 166
    };

    QColor colorBySelectionId(int index) const
    {
        return colorByHue(selectionId(index) * SelectionIdHueMultiplier);
    }

    QColor colorByFraction(double fraction) const
    {
        return colorByHue(fraction * FractionHueMultiplier + FractionHueMininimum);
    }

    QColor colorByHue(int hue) const
    {
        return QColor::fromHsl(hue % 360, Saturation, Lightness);
    }

    explicit AbstractTimelineModel(AbstractTimelineModelPrivate *dd, const QString &displayName,
                                   QmlDebug::Message message, QmlDebug::RangeType rangeType,
                                   QObject *parent);
    AbstractTimelineModelPrivate *d_ptr;

    virtual void loadData() = 0;
    virtual void clear();

protected slots:
    void dataChanged();

private:
    Q_DECLARE_PRIVATE(AbstractTimelineModel)
};

}

#endif // ABSTRACTTIMELINEMODEL_H
