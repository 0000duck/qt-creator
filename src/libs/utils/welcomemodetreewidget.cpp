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

#include "welcomemodetreewidget.h"

#include <QtGui/QLabel>
#include <QtGui/QAction>
#include <QtGui/QBoxLayout>
#include <QtGui/QHeaderView>

namespace Utils {

void WelcomeModeLabel::setStyledText(const QString &text)
{
    QString  rc = QLatin1String(
    "<html><head><style type=\"text/css\">p, li { white-space: pre-wrap; }</style></head>"
    "<body style=\" font-weight:500; font-style:normal;\">"
    "<p style=\" margin-top:16px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">"
    "<span style=\" font-size:large; color:#555555;\">");
    rc += text;
    rc += QLatin1String("</span></p></body></html>");
    setText(rc);
}

struct WelcomeModeTreeWidgetPrivate
{
    WelcomeModeTreeWidgetPrivate();

    const QIcon bullet;
};

WelcomeModeTreeWidgetPrivate::WelcomeModeTreeWidgetPrivate() :
    bullet(QLatin1String(":/welcome/images/list_bullet_arrow.png"))
{
}

WelcomeModeTreeWidget::WelcomeModeTreeWidget(QWidget *parent) :
        QTreeWidget(parent), m_d(new WelcomeModeTreeWidgetPrivate)
{
    connect(this, SIGNAL(itemClicked(QTreeWidgetItem *, int)),
            SLOT(slotItemClicked(QTreeWidgetItem *)));

    setUniformRowHeights(true);
    viewport()->setAutoFillBackground(false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

WelcomeModeTreeWidget::~WelcomeModeTreeWidget()
{
    delete m_d;
}

QSize WelcomeModeTreeWidget::minimumSizeHint() const
{
    return QSize();
}

QSize WelcomeModeTreeWidget::sizeHint() const
{
    return QSize(QTreeWidget::sizeHint().width(), 30 * topLevelItemCount());
}

QTreeWidgetItem *WelcomeModeTreeWidget::addItem(const QString &label, const QString &data, const QString &toolTip)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(this);
    item->setIcon(0, m_d->bullet);
    item->setSizeHint(0, QSize(24, 30));
    QLabel *lbl = new QLabel(label);
    lbl->setTextInteractionFlags(Qt::NoTextInteraction);
    lbl->setCursor(QCursor(Qt::PointingHandCursor));
    lbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QBoxLayout *lay = new QVBoxLayout;
    lay->setContentsMargins(3, 2, 0, 0);
    lay->addWidget(lbl);
    QWidget *wdg = new QWidget;
    wdg->setLayout(lay);
    setItemWidget(item, 1, wdg);
    item->setData(0, Qt::UserRole, data);
    if (!toolTip.isEmpty())
        wdg->setToolTip(toolTip);
    return item;

}

void WelcomeModeTreeWidget::slotAddNewsItem(const QString &title, const QString &description, const QString &link)
{
    int itemWidth = width()-header()->sectionSize(0);
    QFont f = font();
    QString elidedText = QFontMetrics(f).elidedText(description, Qt::ElideRight, itemWidth);
    f.setBold(true);
    QString elidedTitle = QFontMetrics(f).elidedText(title, Qt::ElideRight, itemWidth);
    QString data = QString::fromLatin1("<b>%1</b><br /><font color='gray'>%2</font>").arg(elidedTitle).arg(elidedText);
    addTopLevelItem(addItem(data, link, link));
}

void WelcomeModeTreeWidget::slotItemClicked(QTreeWidgetItem *item)
{
    emit activated(item->data(0, Qt::UserRole).toString());
}

}
