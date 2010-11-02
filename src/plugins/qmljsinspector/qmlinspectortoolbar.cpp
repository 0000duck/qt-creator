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

#include "qmlinspectortoolbar.h"
#include "qmljsinspectorconstants.h"
#include "qmljstoolbarcolorbox.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/uniqueidmanager.h>

#include <utils/styledbar.h>
#include <utils/filterlineedit.h>

#include <QHBoxLayout>
#include <QAction>
#include <QToolButton>
#include <QMenu>
#include <QActionGroup>

namespace QmlJSInspector {
namespace Internal {

static QToolButton *createToolButton(QAction *action)
{
    QToolButton *button = new QToolButton;
    button->setDefaultAction(action);
    return button;
}

QmlInspectorToolbar::QmlInspectorToolbar(QObject *parent) :
    QObject(parent),
    m_observerModeAction(0),
//    m_reloadAction(0),
    m_playAction(0),
    m_selectAction(0),
    m_zoomAction(0),
    m_colorPickerAction(0),
    m_toQmlAction(0),
    m_fromQmlAction(0),
    m_defaultAnimSpeedAction(0),
    m_halfAnimSpeedAction(0),
    m_fourthAnimSpeedAction(0),
    m_eighthAnimSpeedAction(0),
    m_tenthAnimSpeedAction(0),
    m_menuPauseAction(0),
    m_playIcon(QIcon(QLatin1String(":/qml/images/play-small.png"))),
    m_pauseIcon(QIcon(QLatin1String(":/qml/images/pause-small.png"))),
    m_colorBox(0),
    m_emitSignals(true),
    m_isRunning(false),
    m_animationSpeed(1.0f),
    m_previousAnimationSpeed(0.0f),
    m_activeTool(NoTool),
    m_barWidget(0)
{

}

void QmlInspectorToolbar::setEnabled(bool value)
{
    m_observerModeAction->setEnabled(value);
    //m_toQmlAction->setEnabled(value);
    m_fromQmlAction->setEnabled(value);

//    m_reloadAction->setEnabled(value);
    m_playAction->setEnabled(value);
    m_selectAction->setEnabled(value);
    m_zoomAction->setEnabled(value);
    m_colorPickerAction->setEnabled(value);
    //m_toQmlAction->setEnabled(value);
    m_fromQmlAction->setEnabled(value);
    m_colorBox->setEnabled(value);
}

void QmlInspectorToolbar::enable()
{
    setEnabled(true);
    m_emitSignals = false;
    m_observerModeAction->setChecked(false);
    setAnimationSpeed(1.0f);
    activateDesignModeOnClick();
    m_emitSignals = true;
}

void QmlInspectorToolbar::disable()
{
    setAnimationSpeed(1.0f);
    activateSelectTool();
    setEnabled(false);
}

void QmlInspectorToolbar::activateColorPicker()
{
    m_emitSignals = false;
    activateColorPickerOnClick();
    m_emitSignals = true;
}

void QmlInspectorToolbar::activateSelectTool()
{
    m_emitSignals = false;
    activateSelectToolOnClick();
    m_emitSignals = true;
}

void QmlInspectorToolbar::activateZoomTool()
{
    m_emitSignals = false;
    activateZoomOnClick();
    m_emitSignals = true;
}

void QmlInspectorToolbar::setAnimationSpeed(qreal slowdownFactor)
{
    m_emitSignals = false;
    if (slowdownFactor != 0) {
        m_animationSpeed = slowdownFactor;

        if (slowdownFactor == 1.0f) {
            m_defaultAnimSpeedAction->setChecked(true);
        } else if (slowdownFactor == 2.0f) {
            m_halfAnimSpeedAction->setChecked(true);
        } else if (slowdownFactor == 4.0f) {
            m_fourthAnimSpeedAction->setChecked(true);
        } else if (slowdownFactor == 8.0f) {
            m_eighthAnimSpeedAction->setChecked(true);
        } else if (slowdownFactor == 10.0f) {
            m_tenthAnimSpeedAction->setChecked(true);
        }
        updatePlayAction();
    } else {
        m_menuPauseAction->setChecked(true);
        updatePauseAction();
    }

    m_emitSignals = true;
}

void QmlInspectorToolbar::setDesignModeBehavior(bool inDesignMode)
{
    m_emitSignals = false;
    m_observerModeAction->setChecked(inDesignMode);
    activateDesignModeOnClick();
    m_emitSignals = true;
}

void QmlInspectorToolbar::createActions(const Core::Context &context)
{
    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *am = core->actionManager();

    m_fromQmlAction = new QAction(QIcon(QLatin1String(":/qml/images/from-qml-small.png")), tr("Apply Changes on Save"), this);
//    m_reloadAction = new QAction(QIcon(QLatin1String(":/qml/images/reload.png")), tr("Reload QML"), this);

    m_observerModeAction = new QAction(QIcon(QLatin1String(":/qml/images/observermode.png")), tr("Observer Mode"), this);
    m_playAction = new QAction(m_pauseIcon, tr("Play/Pause Animations"), this);
    m_selectAction = new QAction(QIcon(QLatin1String(":/qml/images/select-small.png")), tr("Select"), this);
    m_zoomAction = new QAction(QIcon(QLatin1String(":/qml/images/zoom-small.png")), tr("Zoom"), this);
    m_colorPickerAction = new QAction(QIcon(QLatin1String(":/qml/images/color-picker-small.png")), tr("Color Picker"), this);
    m_toQmlAction = new QAction(QIcon(QLatin1String(":/qml/images/to-qml-small.png")), tr("Live Preview Changes in QML Viewer"), this);

    m_observerModeAction->setCheckable(true);
    m_observerModeAction->setChecked(false);
    m_selectAction->setCheckable(true);
    m_zoomAction->setCheckable(true);
    m_colorPickerAction->setCheckable(true);

    m_fromQmlAction->setCheckable(true);
    m_fromQmlAction->setChecked(true);

    am->registerAction(m_observerModeAction, QmlJSInspector::Constants::DESIGNMODE_ACTION, context);
//    am->registerAction(m_reloadAction, QmlJSInspector::Constants::RELOAD_ACTION, context);
    am->registerAction(m_playAction, QmlJSInspector::Constants::PLAY_ACTION, context);
    am->registerAction(m_selectAction, QmlJSInspector::Constants::SELECT_ACTION, context);
    am->registerAction(m_zoomAction, QmlJSInspector::Constants::ZOOM_ACTION, context);
    am->registerAction(m_colorPickerAction, QmlJSInspector::Constants::COLOR_PICKER_ACTION, context);
    am->registerAction(m_toQmlAction, QmlJSInspector::Constants::TO_QML_ACTION, context);
    am->registerAction(m_fromQmlAction, QmlJSInspector::Constants::FROM_QML_ACTION, context);

    m_barWidget = new Utils::StyledBar;
    m_barWidget->setSingleRow(true);
    m_barWidget->setProperty("topBorder", true);

    QHBoxLayout *configBarLayout = new QHBoxLayout(m_barWidget);
    configBarLayout->setMargin(0);
    configBarLayout->setSpacing(5);

    QMenu *playSpeedMenu = new QMenu(m_barWidget);
    QActionGroup *playSpeedMenuActions = new QActionGroup(this);
    playSpeedMenuActions->setExclusive(true);
    playSpeedMenu->addAction(tr("Animation Speed"));
    playSpeedMenu->addSeparator();
    m_defaultAnimSpeedAction = playSpeedMenu->addAction(tr("1x"), this, SLOT(changeToDefaultAnimSpeed()));
    m_defaultAnimSpeedAction->setCheckable(true);
    m_defaultAnimSpeedAction->setChecked(true);
    playSpeedMenuActions->addAction(m_defaultAnimSpeedAction);

    m_halfAnimSpeedAction = playSpeedMenu->addAction(tr("0.5x"), this, SLOT(changeToHalfAnimSpeed()));
    m_halfAnimSpeedAction->setCheckable(true);
    playSpeedMenuActions->addAction(m_halfAnimSpeedAction);

    m_fourthAnimSpeedAction = playSpeedMenu->addAction(tr("0.25x"), this, SLOT(changeToFourthAnimSpeed()));
    m_fourthAnimSpeedAction->setCheckable(true);
    playSpeedMenuActions->addAction(m_fourthAnimSpeedAction);

    m_eighthAnimSpeedAction = playSpeedMenu->addAction(tr("0.125x"), this, SLOT(changeToEighthAnimSpeed()));
    m_eighthAnimSpeedAction->setCheckable(true);
    playSpeedMenuActions->addAction(m_eighthAnimSpeedAction);

    m_tenthAnimSpeedAction = playSpeedMenu->addAction(tr("0.1x"), this, SLOT(changeToTenthAnimSpeed()));
    m_tenthAnimSpeedAction->setCheckable(true);
    playSpeedMenuActions->addAction(m_tenthAnimSpeedAction);

    m_menuPauseAction = playSpeedMenu->addAction(tr("Pause"), this, SLOT(updatePauseAction()));
    m_menuPauseAction->setCheckable(true);
    m_menuPauseAction->setIcon(m_pauseIcon);
    playSpeedMenuActions->addAction(m_menuPauseAction);

//    configBarLayout->addWidget(createToolButton(am->command(ProjectExplorer::Constants::DEBUG)->action()));
//    configBarLayout->addWidget(createToolButton(am->command(ProjectExplorer::Constants::STOP)->action()));
    configBarLayout->addWidget(createToolButton(am->command(QmlJSInspector::Constants::FROM_QML_ACTION)->action()));
//    configBarLayout->addWidget(createToolButton(am->command(QmlJSInspector::Constants::RELOAD_ACTION)->action()));
    configBarLayout->addSpacing(10);

    configBarLayout->addWidget(createToolButton(am->command(QmlJSInspector::Constants::DESIGNMODE_ACTION)->action()));
    m_playButton = createToolButton(am->command(QmlJSInspector::Constants::PLAY_ACTION)->action());
    m_playButton->setMenu(playSpeedMenu);
    configBarLayout->addWidget(m_playButton);
    //configBarLayout->addWidget(createToolButton(am->command(QmlJSInspector::Constants::PAUSE_ACTION)->action()));

    configBarLayout->addWidget(createToolButton(am->command(QmlJSInspector::Constants::SELECT_ACTION)->action()));

    configBarLayout->addWidget(createToolButton(am->command(QmlJSInspector::Constants::ZOOM_ACTION)->action()));
    configBarLayout->addWidget(createToolButton(am->command(QmlJSInspector::Constants::COLOR_PICKER_ACTION)->action()));

    m_colorBox = new ToolBarColorBox(m_barWidget);
    m_colorBox->setMinimumSize(20, 20);
    m_colorBox->setMaximumSize(20, 20);
    m_colorBox->setInnerBorderColor(QColor(192,192,192));
    m_colorBox->setOuterBorderColor(QColor(58,58,58));
    configBarLayout->addWidget(m_colorBox);
    //configBarLayout->addWidget(createToolButton(am->command(QmlJSInspector::Constants::TO_QML_ACTION)->action()));

    //m_filterLineEdit = new Utils::FilterLineEdit(m_barWidget);

    configBarLayout->addStretch();
    //configBarLayout->addWidget(m_filterLineEdit);

    setEnabled(false);

    connect(m_observerModeAction, SIGNAL(triggered()), SLOT(activateDesignModeOnClick()));
//    connect(m_reloadAction, SIGNAL(triggered()), SIGNAL(reloadSelected()));

    connect(m_colorPickerAction, SIGNAL(triggered()), SLOT(activateColorPickerOnClick()));

    connect(m_playAction, SIGNAL(triggered()), SLOT(activatePlayOnClick()));

    connect(m_zoomAction, SIGNAL(triggered()), SLOT(activateZoomOnClick()));
    connect(m_colorPickerAction, SIGNAL(triggered()), SLOT(activateColorPickerOnClick()));
    connect(m_selectAction, SIGNAL(triggered()), SLOT(activateSelectToolOnClick()));

    //connect(m_toQmlAction, SIGNAL(triggered()), SLOT(activateToQml()));
    connect(m_fromQmlAction, SIGNAL(triggered()), SLOT(activateFromQml()));
}

QWidget *QmlInspectorToolbar::widget() const
{
    return m_barWidget;
}

void QmlInspectorToolbar::changeToDefaultAnimSpeed()
{
    m_animationSpeed = 1.0f;
    updatePlayAction();
}

void QmlInspectorToolbar::changeToHalfAnimSpeed()
{
    m_animationSpeed = 2.0f;
    updatePlayAction();
}

void QmlInspectorToolbar::changeToFourthAnimSpeed()
{
    m_animationSpeed = 4.0f;
    updatePlayAction();
}

void QmlInspectorToolbar::changeToEighthAnimSpeed()
{
    m_animationSpeed = 8.0f;
    updatePlayAction();
}

void QmlInspectorToolbar::changeToTenthAnimSpeed()
{
    m_animationSpeed = 10.0f;
    updatePlayAction();
}

void QmlInspectorToolbar::activateDesignModeOnClick()
{
    bool checked = m_observerModeAction->isChecked();

//    m_reloadAction->setEnabled(true);
    m_playAction->setEnabled(checked);
    m_selectAction->setEnabled(checked);
    m_zoomAction->setEnabled(checked);
    m_colorPickerAction->setEnabled(checked);

    if (m_emitSignals)
        emit designModeSelected(checked);
}

void QmlInspectorToolbar::activatePlayOnClick()
{
    if (m_isRunning) {
        updatePauseAction();
    } else {
        updatePlayAction();
    }
}

void QmlInspectorToolbar::updatePlayAction()
{
    m_isRunning = true;
    m_playAction->setIcon(m_pauseIcon);
    if (m_animationSpeed != m_previousAnimationSpeed)
        m_previousAnimationSpeed = m_animationSpeed;

    if (m_emitSignals)
        emit animationSpeedChanged(m_animationSpeed);

    m_playButton->setDefaultAction(m_playAction);
}

void QmlInspectorToolbar::updatePauseAction()
{
    m_isRunning = false;
    m_playAction->setIcon(m_playIcon);
    if (m_emitSignals)
        emit animationSpeedChanged(0.0f);

    m_playButton->setDefaultAction(m_playAction);
}

void QmlInspectorToolbar::activateColorPickerOnClick()
{
    m_zoomAction->setChecked(false);
    m_selectAction->setChecked(false);

    m_colorPickerAction->setChecked(true);
    if (m_activeTool != ColorPickerMode) {
        m_activeTool = ColorPickerMode;
        if (m_emitSignals)
            emit colorPickerSelected();
    }
}

void QmlInspectorToolbar::activateSelectToolOnClick()
{
    m_zoomAction->setChecked(false);
    m_colorPickerAction->setChecked(false);

    m_selectAction->setChecked(true);
    if (m_activeTool != SelectionToolMode) {
        m_activeTool = SelectionToolMode;
        if (m_emitSignals)
            emit selectToolSelected();
    }
}

void QmlInspectorToolbar::activateZoomOnClick()
{
    m_selectAction->setChecked(false);
    m_colorPickerAction->setChecked(false);

    m_zoomAction->setChecked(true);
    if (m_activeTool != ZoomMode) {
        m_activeTool = ZoomMode;
        if (m_emitSignals)
            emit zoomToolSelected();
    }
}

void QmlInspectorToolbar::setLivePreviewChecked(bool value)
{
    m_fromQmlAction->setChecked(value);
}

void QmlInspectorToolbar::setSelectedColor(const QColor &color)
{
    m_colorBox->setColor(color);
}

void QmlInspectorToolbar::activateFromQml()
{
    if (m_emitSignals)
        emit applyChangesFromQmlFileTriggered(m_fromQmlAction->isChecked());
}

void QmlInspectorToolbar::activateToQml()
{
    if (m_emitSignals)
        emit applyChangesToQmlFileSelected();
}

} // namespace Internal
} // namespace QmlJSInspector
