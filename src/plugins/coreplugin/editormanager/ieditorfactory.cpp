/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "ieditorfactory.h"
#include "editormanager.h"
#include "editormanager_p.h"

#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>

#include <QFileInfo>

namespace Core {

static QList<IEditorFactory *> g_editorFactories;

IEditorFactory::IEditorFactory(QObject *parent)
    : QObject(parent)
{
    g_editorFactories.append(this);
}

IEditorFactory::~IEditorFactory()
{
    g_editorFactories.removeOne(this);
}

const EditorFactoryList IEditorFactory::allEditorFactories()
{
    return g_editorFactories;
}

const EditorFactoryList IEditorFactory::editorFactories(const Utils::MimeType &mimeType, bool bestMatchOnly)
{
    EditorFactoryList rc;
    const EditorFactoryList allFactories = IEditorFactory::allEditorFactories();
    Internal::mimeTypeFactoryLookup(mimeType, allFactories, bestMatchOnly, &rc);
    return rc;
}

const EditorFactoryList IEditorFactory::editorFactories(const QString &fileName, bool bestMatchOnly)
{
    const QFileInfo fileInfo(fileName);
    // Find by mime type
    Utils::MimeType mimeType = Utils::mimeTypeForFile(fileInfo);
    if (!mimeType.isValid()) {
        qWarning("%s unable to determine mime type of %s. Falling back to text/plain",
                 Q_FUNC_INFO, fileName.toUtf8().constData());
        mimeType = Utils::mimeTypeForName("text/plain");
    }
    // open text files > 48 MB in binary editor
    if (fileInfo.size() > EditorManager::maxTextFileSize()
            && mimeType.name().startsWith("text")) {
        mimeType = Utils::mimeTypeForName("application/octet-stream");
    }

    return IEditorFactory::editorFactories(mimeType, bestMatchOnly);
}


} // Core
