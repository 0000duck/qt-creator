/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef STATESEDITORVIEW_H
#define STATESEDITORVIEW_H

#include <qmlmodelview.h>

namespace QmlDesigner {

namespace Internal {

class StatesEditorModel;

class StatesEditorView : public QmlModelView {
    Q_OBJECT

public:
    explicit StatesEditorView(StatesEditorModel *model, QObject *parent = 0);

    void setCurrentState(int index);
    void setCurrentStateSilent(int index);
    void createState(const QString &name);
    void removeState(int index);
    void renameState(int index,const QString &newName);
    void duplicateCurrentState(int index);

    QPixmap renderState(int i);
    QmlItemNode stateRootNode() { return m_stateRootNode; }
    bool isAttachedToModel() const { return m_attachedToModel; }

    void nodeInstancePropertyChanged(const ModelNode &node, const QString &propertyName);

    // AbstractView
    void modelAttached(Model *model);
    void modelAboutToBeDetached(Model *model);
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList);
    void propertiesRemoved(const QList<AbstractProperty>& propertyList);
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange);

    void nodeAboutToBeRemoved(const ModelNode &removedNode);
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex);

    // QmlModelView
    void stateChanged(const QmlModelState &newQmlModelState, const QmlModelState &oldQmlModelState);
    void transformChanged(const QmlObjectNode &qmlObjectNode, const QString &propertyName);
    void parentChanged(const QmlObjectNode &qmlObjectNode);
    void otherPropertyChanged(const QmlObjectNode &qmlObjectNode, const QString &propertyName);

    void customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data);
    void scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList);
    void nodeIdChanged(const ModelNode &node, const QString &newId, const QString &oldId);
    void bindingPropertiesChanged(const QList<BindingProperty> &propertyList, PropertyChangeFlags propertyChange);
    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList, const QList<ModelNode> &lastSelectedNodeList);


protected:
    void timerEvent(QTimerEvent*);

private slots:
    void sceneChanged();

private:
    void insertModelState(int i, const QmlModelState &state);
    void removeModelState(const QmlModelState &state);
    void clearModelStates();
    int modelStateIndex(const QmlModelState &state);

    void startUpdateTimer(int i, int offset);

    QList<QmlModelState> m_modelStates;
    StatesEditorModel *m_editorModel;
    QmlItemNode m_stateRootNode;

    QList<int> m_updateTimerIdList;
    QmlModelState m_oldRewriterAmendState;
    bool m_attachedToModel;
    bool m_settingSilentState;

    QList<bool> m_thumbnailsToUpdate;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // STATESEDITORVIEW_H
