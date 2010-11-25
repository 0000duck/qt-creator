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

#include "debuggeragents.h"

#include "breakhandler.h"
#include "debuggerengine.h"
#include "debuggercore.h"
#include "debuggerstringutils.h"
#include "stackframe.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/icore.h>

#include <texteditor/basetextdocument.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/basetextmark.h>
#include <texteditor/plaintexteditor.h>
#include <texteditor/texteditorconstants.h>

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QMetaObject>
#include <QtCore/QTimer>

#include <QtGui/QMessageBox>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>
#include <QtGui/QIcon>

#include <limits.h>

using namespace Core;

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// MemoryViewAgent
//
///////////////////////////////////////////////////////////////////////

/*!
    \class MemoryViewAgent

    Objects form this class are created in response to user actions in
    the Gui for showing raw memory from the inferior. After creation
    it handles communication between the engine and the bineditor.
*/

namespace { const int DataRange = 1024 * 1024; }

MemoryViewAgent::MemoryViewAgent(Debugger::DebuggerEngine *engine, quint64 addr)
    : QObject(engine), m_engine(engine)
{
    QTC_ASSERT(engine, /**/);
    createBinEditor(addr);
}

MemoryViewAgent::MemoryViewAgent(Debugger::DebuggerEngine *engine, const QString &addr)
    : QObject(engine), m_engine(engine)
{
    QTC_ASSERT(engine, /**/);
    bool ok = true;
    createBinEditor(addr.toULongLong(&ok, 0));
    //qDebug() <<  " ADDRESS: " << addr <<  addr.toUInt(&ok, 0);
}

MemoryViewAgent::~MemoryViewAgent()
{
    EditorManager *editorManager = EditorManager::instance();
    QList<IEditor *> editors;
    foreach (QPointer<IEditor> editor, m_editors)
        if (editor)
            editors.append(editor.data());
    editorManager->closeEditors(editors);
}

void MemoryViewAgent::createBinEditor(quint64 addr)
{
    EditorManager *editorManager = EditorManager::instance();
    QString titlePattern = tr("Memory $");
    IEditor *editor = editorManager->openEditorWithContents(
        Core::Constants::K_DEFAULT_BINARY_EDITOR_ID,
        &titlePattern);
    if (editor) {
        connect(editor->widget(),
            SIGNAL(lazyDataRequested(Core::IEditor *, quint64,bool)),
            SLOT(fetchLazyData(Core::IEditor *, quint64,bool)));
        connect(editor->widget(),
            SIGNAL(newWindowRequested(quint64)),
            SLOT(createBinEditor(quint64)));
        connect(editor->widget(),
            SIGNAL(newRangeRequested(Core::IEditor *, quint64)),
            SLOT(provideNewRange(Core::IEditor*,quint64)));
        connect(editor->widget(),
            SIGNAL(startOfFileRequested(Core::IEditor *)),
            SLOT(handleStartOfFileRequested(Core::IEditor*)));
        connect(editor->widget(),
            SIGNAL(endOfFileRequested(Core::IEditor *)),
            SLOT(handleEndOfFileRequested(Core::IEditor*)));
        m_editors << editor;
        editorManager->activateEditor(editor);
        QMetaObject::invokeMethod(editor->widget(), "setNewWindowRequestAllowed");
        QMetaObject::invokeMethod(editor->widget(), "setLazyData",
            Q_ARG(quint64, addr), Q_ARG(int, DataRange), Q_ARG(int, BinBlockSize));
    } else {
        showMessageBox(QMessageBox::Warning,
            tr("No memory viewer available"),
            tr("The memory contents cannot be shown as no viewer plugin "
               "for binary data has been loaded."));
        deleteLater();
    }
}

void MemoryViewAgent::fetchLazyData(IEditor *editor, quint64 block, bool sync)
{
    Q_UNUSED(sync); // FIXME: needed support for incremental searching
    m_engine->fetchMemory(this, editor, BinBlockSize * block, BinBlockSize);
}

void MemoryViewAgent::addLazyData(QObject *editorToken, quint64 addr,
                                  const QByteArray &ba)
{
    IEditor *editor = qobject_cast<IEditor *>(editorToken);
    if (editor && editor->widget()) {
        Core::EditorManager::instance()->activateEditor(editor);
        QMetaObject::invokeMethod(editor->widget(), "addLazyData",
            Q_ARG(quint64, addr / BinBlockSize), Q_ARG(QByteArray, ba));
    }
}

void MemoryViewAgent::provideNewRange(IEditor *editor, quint64 address)
{
    QMetaObject::invokeMethod(editor->widget(), "setLazyData",
        Q_ARG(quint64, address), Q_ARG(int, DataRange),
        Q_ARG(int, BinBlockSize));
}

// Since we are not dealing with files, we take these signals to mean
// "move to start/end of range". This seems to make more sense than
// jumping to the start or end of the address space, respectively.
void MemoryViewAgent::handleStartOfFileRequested(IEditor *editor)
{
    QMetaObject::invokeMethod(editor->widget(),
        "setCursorPosition", Q_ARG(int, 0));
}

void MemoryViewAgent::handleEndOfFileRequested(IEditor *editor)
{
    QMetaObject::invokeMethod(editor->widget(),
        "setCursorPosition", Q_ARG(int, DataRange - 1));
}



///////////////////////////////////////////////////////////////////////
//
// DisassemblerViewAgent
//
///////////////////////////////////////////////////////////////////////

class LocationMark2 : public TextEditor::ITextMark
{
public:
    LocationMark2() {}

    QIcon icon() const { return debuggerCore()->locationMarkIcon(); }
    void updateLineNumber(int /*lineNumber*/) {}
    void updateBlock(const QTextBlock & /*block*/) {}
    void removedFromEditor() {}
    void documentClosing() {}
};

class BreakpointMarker2 : public TextEditor::ITextMark
{
public:
    BreakpointMarker2(const QIcon &icon) : m_icon(icon) {}

    QIcon icon() const { return m_icon; }
    void updateLineNumber(int) {}
    void updateBlock(const QTextBlock &) {}
    void removedFromEditor() {}
    void documentClosing() {}

private:
    QIcon m_icon;
};


class DisassemblerViewAgentPrivate
{
public:
    DisassemblerViewAgentPrivate();
    ~DisassemblerViewAgentPrivate();
    void configureMimeType();

public:
    QPointer<TextEditor::ITextEditor> editor;
    StackFrame frame;
    bool tryMixed;
    bool setMarker;
    QPointer<DebuggerEngine> engine;
    TextEditor::ITextMark *locationMark;
    QList<TextEditor::ITextMark *> breakpointMarks;
    
    QHash<QString, DisassemblerLines> cache;
    QString mimeType;
};

DisassemblerViewAgentPrivate::DisassemblerViewAgentPrivate()
  : editor(0),
    tryMixed(true),
    setMarker(true),
    locationMark(new LocationMark2),
    mimeType(_("text/x-qtcreator-generic-asm"))
{
}

DisassemblerViewAgentPrivate::~DisassemblerViewAgentPrivate()
{
    if (editor) {
        EditorManager *editorManager = EditorManager::instance();
        editorManager->closeEditors(QList<IEditor *>() << editor);
    }
    editor = 0;
    delete locationMark;
}

/*!
    \class DisassemblerViewAgent

     Objects from this class are created in response to user actions in
     the Gui for showing disassembled memory from the inferior. After creation
     it handles communication between the engine and the editor.
*/

DisassemblerViewAgent::DisassemblerViewAgent(DebuggerEngine *engine)
    : QObject(0), d(new DisassemblerViewAgentPrivate)
{
    d->engine = engine;
}

DisassemblerViewAgent::~DisassemblerViewAgent()
{
    delete d;
    d = 0;
}

void DisassemblerViewAgent::cleanup()
{
    d->cache.clear();
}

void DisassemblerViewAgent::resetLocation()
{
    if (!d->editor)
        return;
    d->editor->markableInterface()->removeMark(d->locationMark);
}

QString frameKey(const StackFrame &frame)
{
    return _("%1:%2:%3").arg(frame.function).arg(frame.file).arg(frame.from);
}

const StackFrame &DisassemblerViewAgent::frame() const
{
    return d->frame;
}

bool DisassemblerViewAgent::isMixed() const
{
    return d->tryMixed
        && d->frame.line > 0
        && !d->frame.function.isEmpty()
        && d->frame.function != _("??");
}

void DisassemblerViewAgent::setFrame(const StackFrame &frame,
    bool tryMixed, bool setMarker)
{
    d->frame = frame;
    d->tryMixed = tryMixed;
    if (isMixed()) {
        QHash<QString, DisassemblerLines>::ConstIterator it =
            d->cache.find(frameKey(frame));
        if (it != d->cache.end()) {
            QString msg = _("Use cache disassembler for '%1' in '%2'")
                .arg(frame.function).arg(frame.file);
            d->engine->showMessage(msg);
            setContents(*it);
            updateBreakpointMarkers();
            updateLocationMarker();
            return;
        }
    }
    d->engine->fetchDisassembler(this);
}

void DisassemblerViewAgentPrivate::configureMimeType()
{
    QTC_ASSERT(editor, return);

    TextEditor::BaseTextDocument *doc =
        qobject_cast<TextEditor::BaseTextDocument *>(editor->file());
    QTC_ASSERT(doc, return);
    doc->setMimeType(mimeType);

    TextEditor::PlainTextEditor *pe =
        qobject_cast<TextEditor::PlainTextEditor *>(editor->widget());
    QTC_ASSERT(pe, return);

    MimeType mtype = ICore::instance()->mimeDatabase()->findByType(mimeType);
    if (mtype)
        pe->configure(mtype);
    else
        qWarning("Assembler mimetype '%s' not found.", qPrintable(mimeType));
}

QString DisassemblerViewAgent::mimeType() const
{
    return d->mimeType;
}

void DisassemblerViewAgent::setMimeType(const QString &mt)
{
    if (mt == d->mimeType)
        return;
    d->mimeType = mt;
    if (d->editor)
       d->configureMimeType();
}

void DisassemblerViewAgent::setContents(const DisassemblerLines &contents)
{
    QTC_ASSERT(d, return);
    using namespace Core;
    using namespace TextEditor;

    EditorManager *editorManager = EditorManager::instance();
    if (!d->editor) {
        QString titlePattern = "Disassembler";
        d->editor = qobject_cast<ITextEditor *>(
            editorManager->openEditorWithContents(
                Core::Constants::K_DEFAULT_TEXT_EDITOR_ID,
                &titlePattern));
        QTC_ASSERT(d->editor, return);
        d->editor->setProperty(Debugger::Constants::OPENED_BY_DEBUGGER, true);
        d->editor->setProperty(Debugger::Constants::OPENED_WITH_DISASSEMBLY, true);
        d->configureMimeType();

        BaseTextEditor *baseTextEdit =
                qobject_cast<BaseTextEditor *>(d->editor->widget());
        if (baseTextEdit)
            baseTextEdit->setRequestMarkEnabled(true);
    }

    editorManager->activateEditor(d->editor);

    QPlainTextEdit *plainTextEdit =
        qobject_cast<QPlainTextEdit *>(d->editor->widget());
    QTC_ASSERT(plainTextEdit, return);

    QString str;
    for (int i = 0, n = contents.size(); i != n; ++i) {
        const DisassemblerLine &dl = contents.at(i);
        if (dl.address) {
            str += QString("0x");
            str += QString::number(dl.address, 16);
            str += "  ";
        }
        str += dl.data;
        str += "\n";
    }
    plainTextEdit->setPlainText(str);
    plainTextEdit->setReadOnly(true);

    d->cache.insert(frameKey(d->frame), contents);
    d->editor->setDisplayName(_("Disassembler (%1)").arg(d->frame.function));

    updateBreakpointMarkers();
    updateLocationMarker();
}

void DisassemblerViewAgent::updateLocationMarker()
{
    QTC_ASSERT(d->editor, return);

    const DisassemblerLines &contents = d->cache.value(frameKey(d->frame));
    int lineNumber = contents.lineForAddress(d->frame.address);

    if (d->setMarker) {
        d->editor->markableInterface()->removeMark(d->locationMark);
        if (lineNumber)
            d->editor->markableInterface()->addMark(d->locationMark, lineNumber);
    }

    QPlainTextEdit *plainTextEdit =
        qobject_cast<QPlainTextEdit *>(d->editor->widget());
    QTC_ASSERT(plainTextEdit, return); 
    QTextCursor tc = plainTextEdit->textCursor();
    QTextBlock block = tc.document()->findBlockByNumber(lineNumber - 1);
    tc.setPosition(block.position());
    plainTextEdit->setTextCursor(tc);
}

void DisassemblerViewAgent::updateBreakpointMarkers()
{
    if (!d->editor)
        return;

    BreakHandler *handler = breakHandler();
    BreakpointIds ids = handler->engineBreakpointIds(d->engine);
    if (ids.isEmpty())
        return;

    const DisassemblerLines &contents = d->cache.value(frameKey(d->frame));

    foreach (TextEditor::ITextMark *marker, d->breakpointMarks)
        d->editor->markableInterface()->removeMark(marker);
    d->breakpointMarks.clear();
    foreach (BreakpointId id, ids) {
        const quint64 address = handler->address(id);
        if (!address)
            continue;
        const int lineNumber = contents.lineForAddress(address);
        if (!lineNumber)
            continue;
        BreakpointMarker2 *marker = new BreakpointMarker2(handler->icon(id));
        d->breakpointMarks.append(marker);
        d->editor->markableInterface()->addMark(marker, lineNumber);
    }
}

quint64 DisassemblerViewAgent::address() const
{
    return d->frame.address;
}

// Return address of an assembly line "0x0dfd  bla"
quint64 DisassemblerViewAgent::addressFromDisassemblyLine(const QString &line)
{
    return DisassemblerLine(line).address;
}

} // namespace Internal
} // namespace Debugger
