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

#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

// Replaces the "real" syntaxhighlighter.h file. The scope of this test is restricted to the
// highlight definition's context engine. Using a mock derived from QSyntaxHighlighter as a
// base instead of the real TextEditor::SyntaxHighlighter should not affect it.

#include <QtGui/QSyntaxHighlighter>

namespace TextEditor {

class SyntaxHighlighter : public QSyntaxHighlighter
{
public:
    SyntaxHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent) {}
    virtual ~SyntaxHighlighter() {}

protected:
    void SyntaxHighlighter::applyFormatToSpaces(const QString &, const QTextCharFormat &)
    {}
};

}

#endif //SYNTAXHIGHLIGHTER_H
