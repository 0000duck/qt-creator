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

#include "tipcontents.h"
#include "tooltip.h"
#include "tips.h"

#include <utils/qtcassert.h>

#include <QtGlobal>

namespace Utils {

TipContent::TipContent()
{}

TipContent::~TipContent()
{}

ColorContent::ColorContent(const QColor &color) : m_color(color)
{}

ColorContent::~ColorContent()
{}

TipContent *ColorContent::clone() const
{
    return new ColorContent(*this);
}

int ColorContent::typeId() const
{
    return COLOR_CONTENT_ID;
}

bool ColorContent::isValid() const
{
    return m_color.isValid();
}

bool ColorContent::isInteractive() const
{
    return false;
}

int ColorContent::showTime() const
{
    return 4000;
}

bool ColorContent::equals(const TipContent &tipContent) const
{
    if (typeId() == tipContent.typeId()) {
        if (m_color == static_cast<const ColorContent &>(tipContent).m_color)
            return true;
    }
    return false;
}

const QColor &ColorContent::color() const
{
    return m_color;
}

TextContent::TextContent(const QString &text) : m_text(text)
{}

TextContent::~TextContent()
{}

TipContent *TextContent::clone() const
{
    return new TextContent(*this);
}

int TextContent::typeId() const
{
    return TEXT_CONTENT_ID;
}

bool TextContent::isValid() const
{
    return !m_text.isEmpty();
}

bool TextContent::isInteractive() const
{
    return false;
}

int TextContent::showTime() const
{
    return 10000 + 40 * qMax(0, m_text.length() - 100);
}

bool TextContent::equals(const TipContent &tipContent) const
{
    if (typeId() == tipContent.typeId()) {
        if (m_text == static_cast<const TextContent &>(tipContent).m_text)
            return true;
    }
    return false;
}

const QString &TextContent::text() const
{
    return m_text;
}

WidgetContent::WidgetContent(QWidget *w, bool interactive) :
    m_widget(w), m_interactive(interactive)
{
}

TipContent *WidgetContent::clone() const
{
    return new WidgetContent(m_widget, m_interactive);
}

int WidgetContent::typeId() const
{
    return WIDGET_CONTENT_ID;
}

bool WidgetContent::isValid() const
{
    return m_widget;
}

bool WidgetContent::isInteractive() const
{
    return m_interactive;
}

void WidgetContent::setInteractive(bool i)
{
    m_interactive = i;
}

int WidgetContent::showTime() const
{
    return 30000;
}

bool WidgetContent::equals(const TipContent &tipContent) const
{
    if (typeId() == tipContent.typeId()) {
        if (m_widget == static_cast<const WidgetContent &>(tipContent).m_widget)
            return true;
    }
    return false;
}

bool WidgetContent::pinToolTip(QWidget *w, QWidget *parent)
{
    QTC_ASSERT(w, return false);
    // Find the parent WidgetTip, tell it to pin/release the
    // widget and close.
    for (QWidget *p = w->parentWidget(); p ; p = p->parentWidget()) {
        if (Internal::WidgetTip *wt = qobject_cast<Internal::WidgetTip *>(p)) {
            wt->pinToolTipWidget(parent);
            ToolTip::hide();
            return true;
        }
    }
    return false;
}

} // namespace Utils
