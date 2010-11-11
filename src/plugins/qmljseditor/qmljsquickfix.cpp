/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmljsquickfix.h"
#include "qmljscomponentfromobjectdef.h"
#include "qmljseditor.h"
#include "qmljs/parser/qmljsast_p.h"

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <QtCore/QDebug>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;
using namespace QmlJSTools;
using namespace TextEditor;
using TextEditor::RefactoringChanges;

QmlJSQuickFixState::QmlJSQuickFixState(TextEditor::BaseTextEditor *editor)
    : QuickFixState(editor)
{
}

SemanticInfo QmlJSQuickFixState::semanticInfo() const
{
    return _semanticInfo;
}

Snapshot QmlJSQuickFixState::snapshot() const
{
    return _semanticInfo.snapshot;
}

Document::Ptr QmlJSQuickFixState::document() const
{
    return _semanticInfo.document;
}

const QmlJSRefactoringFile QmlJSQuickFixState::currentFile() const
{
    return QmlJSRefactoringFile(editor(), document());
}

QmlJSQuickFixOperation::QmlJSQuickFixOperation(const QmlJSQuickFixState &state, int priority)
    : QuickFixOperation(priority)
    , _state(state)
{
}

QmlJSQuickFixOperation::~QmlJSQuickFixOperation()
{
}

void QmlJSQuickFixOperation::perform()
{
    QmlJSRefactoringChanges refactoring(ExtensionSystem::PluginManager::instance()->getObject<QmlJS::ModelManagerInterface>(),
                                    _state.snapshot());
    QmlJSRefactoringFile current = refactoring.file(fileName());

    performChanges(&current, &refactoring);
}

const QmlJSQuickFixState &QmlJSQuickFixOperation::state() const
{
    return _state;
}

QString QmlJSQuickFixOperation::fileName() const
{
    return state().document()->fileName();
}

QmlJSQuickFixFactory::QmlJSQuickFixFactory()
{
}

QmlJSQuickFixFactory::~QmlJSQuickFixFactory()
{
}

QList<QuickFixOperation::Ptr> QmlJSQuickFixFactory::matchingOperations(QuickFixState *state)
{
    if (QmlJSQuickFixState *qmljsState = static_cast<QmlJSQuickFixState *>(state))
        return match(*qmljsState);
    else
        return QList<TextEditor::QuickFixOperation::Ptr>();
}

QList<QmlJSQuickFixOperation::Ptr> QmlJSQuickFixFactory::noResult()
{
    return QList<QmlJSQuickFixOperation::Ptr>();
}

QList<QmlJSQuickFixOperation::Ptr> QmlJSQuickFixFactory::singleResult(QmlJSQuickFixOperation *operation)
{
    QList<QmlJSQuickFixOperation::Ptr> result;
    result.append(QmlJSQuickFixOperation::Ptr(operation));
    return result;
}

QmlJSQuickFixCollector::QmlJSQuickFixCollector()
{
}

QmlJSQuickFixCollector::~QmlJSQuickFixCollector()
{
}

bool QmlJSQuickFixCollector::supportsEditor(TextEditor::ITextEditable *editable)
{
    return qobject_cast<QmlJSTextEditor *>(editable->widget()) != 0;
}

TextEditor::QuickFixState *QmlJSQuickFixCollector::initializeCompletion(TextEditor::BaseTextEditor *editor)
{
    if (QmlJSTextEditor *qmljsEditor = qobject_cast<QmlJSTextEditor *>(editor)) {
        const SemanticInfo info = qmljsEditor->semanticInfo();

        if (! info.isValid() || qmljsEditor->isOutdated()) {
            // outdated
            qWarning() << "TODO: outdated semantic info, force a reparse.";
            return 0;
        }

        QmlJSQuickFixState *state = new QmlJSQuickFixState(editor);
        state->_semanticInfo = info;
        return state;
    }

    return 0;
}

QList<TextEditor::QuickFixFactory *> QmlJSQuickFixCollector::quickFixFactories() const
{
    QList<TextEditor::QuickFixFactory *> results;
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    foreach (QmlJSQuickFixFactory *f, pm->getObjects<QmlJSEditor::QmlJSQuickFixFactory>())
        results.append(f);
    return results;
}
