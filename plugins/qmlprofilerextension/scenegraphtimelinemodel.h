/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise Qt Quick Profiler Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#ifndef SCENEGRAPHTIMELINEMODEL_H
#define SCENEGRAPHTIMELINEMODEL_H

#include "qmlprofiler/abstracttimelinemodel.h"
#include "qmlprofiler/qmlprofilermodelmanager.h"
#include "qmlprofiler/qmlprofilerdatamodel.h"

#include <QStringList>
#include <QColor>

namespace QmlProfilerExtension {
namespace Internal {

class SceneGraphTimelineModel : public QmlProfiler::AbstractTimelineModel
{
    Q_OBJECT
public:

    struct SceneGraphEvent {
        SceneGraphEvent(int stage = -1, int glyphCount = -1);
        int stage;
        int rowNumberCollapsed;
        int glyphCount; // only used for one event type
    };

    SceneGraphTimelineModel(QObject *parent = 0);
    quint64 features() const;

    int row(int index) const;
    int selectionId(int index) const;
    QColor color(int index) const;

    QVariantList labels() const;

    QVariantMap details(int index) const;

protected:
    void loadData();
    void clear();

private:
    class SceneGraphTimelineModelPrivate;
    Q_DECLARE_PRIVATE(SceneGraphTimelineModel)
};

} // namespace Internal
} // namespace QmlProfilerExtension

#endif // SCENEGRAPHTIMELINEMODEL_H
