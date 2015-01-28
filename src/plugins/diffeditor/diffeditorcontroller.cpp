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

#include "diffeditorconstants.h"
#include "diffeditorcontroller.h"
#include "diffeditorreloader.h"

#include <coreplugin/icore.h>

#include <QStringList>

static const char settingsGroupC[] = "DiffEditor";
static const char contextLineNumbersKeyC[] = "ContextLineNumbers";
static const char ignoreWhitespaceKeyC[] = "IgnoreWhitespace";

namespace DiffEditor {

DiffEditorController::DiffEditorController(QObject *parent)
    : QObject(parent),
      m_reloader(0),
      m_contextLinesNumber(3),
      m_diffFileIndex(-1),
      m_chunkIndex(-1),
      m_descriptionEnabled(false),
      m_contextLinesNumberEnabled(true),
      m_ignoreWhitespace(true)
{
    QSettings *s = Core::ICore::settings();
    s->beginGroup(QLatin1String(settingsGroupC));
    m_contextLinesNumber = s->value(QLatin1String(contextLineNumbersKeyC),
                                    m_contextLinesNumber).toInt();
    m_ignoreWhitespace = s->value(QLatin1String(ignoreWhitespaceKeyC),
                                  m_ignoreWhitespace).toBool();
    s->endGroup();

    clear();
}

DiffEditorController::~DiffEditorController()
{
    delete m_reloader;
}

QString DiffEditorController::clearMessage() const
{
    return m_clearMessage;
}

QList<FileData> DiffEditorController::diffFiles() const
{
    return m_diffFiles;
}

QString DiffEditorController::workingDirectory() const
{
    return m_workingDirectory;
}

QString DiffEditorController::description() const
{
    return m_description;
}

bool DiffEditorController::isDescriptionEnabled() const
{
    return m_descriptionEnabled;
}

int DiffEditorController::contextLinesNumber() const
{
    return m_contextLinesNumber;
}

bool DiffEditorController::isContextLinesNumberEnabled() const
{
    return m_contextLinesNumberEnabled;
}

bool DiffEditorController::isIgnoreWhitespace() const
{
    return m_ignoreWhitespace;
}

// ### fixme: git-specific handling should be done in the git plugin:
// Remove unexpanded branches and follows-tag, clear indentation
// and create E-mail
static void formatGitDescription(QString *description)
{
    QString result;
    result.reserve(description->size());
    foreach (QString line, description->split(QLatin1Char('\n'))) {
        if (line.startsWith(QLatin1String("commit "))
            || line.startsWith(QLatin1String("Branches: <Expand>"))) {
            continue;
        }
        if (line.startsWith(QLatin1String("Author: ")))
            line.replace(0, 8, QStringLiteral("From: "));
        else if (line.startsWith(QLatin1String("    ")))
            line.remove(0, 4);
        result.append(line);
        result.append(QLatin1Char('\n'));
    }
    *description = result;
}

QString DiffEditorController::contents() const
{
    QString result = m_description;
    const int formattingOptions = DiffUtils::GitFormat;
    if (formattingOptions & DiffUtils::GitFormat)
        formatGitDescription(&result);

    const QString diff = DiffUtils::makePatch(diffFiles(), formattingOptions);
    if (!diff.isEmpty()) {
        if (!result.isEmpty())
            result += QLatin1Char('\n');
        result += diff;
    }
    return result;
}

QString DiffEditorController::makePatch(bool revert, bool addPrefix) const
{
    if (m_diffFileIndex < 0 || m_chunkIndex < 0)
        return QString();

    if (m_diffFileIndex >= m_diffFiles.count())
        return QString();

    const FileData fileData = m_diffFiles.at(m_diffFileIndex);
    if (m_chunkIndex >= fileData.chunks.count())
        return QString();

    const ChunkData chunkData = fileData.chunks.at(m_chunkIndex);
    const bool lastChunk = (m_chunkIndex == fileData.chunks.count() - 1);

    const QString fileName = revert
            ? fileData.rightFileInfo.fileName
            : fileData.leftFileInfo.fileName;

    QString leftPrefix, rightPrefix;
    if (addPrefix) {
        leftPrefix = QLatin1String("a/");
        rightPrefix = QLatin1String("b/");
    }
    return DiffUtils::makePatch(chunkData,
                                leftPrefix + fileName,
                                rightPrefix + fileName,
                                lastChunk && fileData.lastChunkAtTheEndOfFile);
}

DiffEditorReloader *DiffEditorController::reloader() const
{
    return m_reloader;
}

// The ownership of reloader is passed to the controller
void DiffEditorController::setReloader(DiffEditorReloader *reloader)
{
    if (m_reloader == reloader)
        return; // nothing changes

    delete m_reloader;

    m_reloader = reloader;

    if (m_reloader)
        m_reloader->setController(this);

    emit reloaderChanged();
}

void DiffEditorController::clear()
{
    clear(tr("No difference"));
}

void DiffEditorController::clear(const QString &message)
{
    setDescription(QString());
    setDiffFiles(QList<FileData>());
    m_clearMessage = message;
    emit cleared(message);
}

void DiffEditorController::setDiffFiles(const QList<FileData> &diffFileList,
                  const QString &workingDirectory)
{
    m_diffFiles = diffFileList;
    m_workingDirectory = workingDirectory;
    emit diffFilesChanged(diffFileList, workingDirectory);
}

void DiffEditorController::setDescription(const QString &description)
{
    if (m_description == description)
        return;

    m_description = description;
    emit descriptionChanged(m_description);
}

void DiffEditorController::setDescriptionEnabled(bool on)
{
    if (m_descriptionEnabled == on)
        return;

    m_descriptionEnabled = on;
    emit descriptionEnablementChanged(on);
}

void DiffEditorController::branchesForCommitReceived(const QString &output)
{
    const QString branches = prepareBranchesForCommit(output);

    m_description.replace(QLatin1String(Constants::EXPAND_BRANCHES), branches);
    emit descriptionChanged(m_description);
}

void DiffEditorController::expandBranchesRequested()
{
    emit requestBranchList(m_description.mid(7, 8));
}

QString DiffEditorController::prepareBranchesForCommit(const QString &output)
{
    QString moreBranches;
    QString branches;
    QStringList res;
    foreach (const QString &branch, output.split(QLatin1Char('\n'))) {
        const QString b = branch.mid(2).trimmed();
        if (!b.isEmpty())
            res << b;
    }
    const int branchCount = res.count();
    // If there are more than 20 branches, list first 10 followed by a hint
    if (branchCount > 20) {
        const int leave = 10;
        //: Displayed after the untranslated message "Branches: branch1, branch2 'and %n more'"
        //  in git show.
        moreBranches = QLatin1Char(' ') + tr("and %n more", 0, branchCount - leave);
        res.erase(res.begin() + leave, res.end());
    }
    if (!res.isEmpty())
        branches = (QLatin1String("Branches: ") + res.join(QLatin1String(", ")) + moreBranches);

    return branches;
}

void DiffEditorController::setContextLinesNumber(int lines)
{
    const int l = qMax(lines, 1);
    if (m_contextLinesNumber == l)
        return;

    m_contextLinesNumber = l;

    QSettings *s = Core::ICore::settings();
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(contextLineNumbersKeyC), m_contextLinesNumber);
    s->endGroup();

    emit contextLinesNumberChanged(l);
}

void DiffEditorController::setContextLinesNumberEnabled(bool on)
{
    if (m_contextLinesNumberEnabled == on)
        return;

    m_contextLinesNumberEnabled = on;
    emit contextLinesNumberEnablementChanged(on);
}

void DiffEditorController::setIgnoreWhitespace(bool ignore)
{
    if (m_ignoreWhitespace == ignore)
        return;

    m_ignoreWhitespace = ignore;

    QSettings *s = Core::ICore::settings();
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(ignoreWhitespaceKeyC), m_ignoreWhitespace);
    s->endGroup();

    emit ignoreWhitespaceChanged(ignore);
}

void DiffEditorController::requestReload()
{
    if (m_reloader)
        m_reloader->requestReload();
}

void DiffEditorController::requestChunkActions(QMenu *menu,
                         int diffFileIndex,
                         int chunkIndex)
{
    m_diffFileIndex = diffFileIndex;
    m_chunkIndex = chunkIndex;
    emit chunkActionsRequested(menu, diffFileIndex >= 0 && chunkIndex >= 0);
}

void DiffEditorController::requestSaveState()
{
    emit saveStateRequested();
}

void DiffEditorController::requestRestoreState()
{
    emit restoreStateRequested();
}

} // namespace DiffEditor
