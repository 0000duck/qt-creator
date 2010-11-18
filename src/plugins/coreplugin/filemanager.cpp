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

#include "filemanager.h"

#include "editormanager.h"
#include "ieditor.h"
#include "icore.h"
#include "ifile.h"
#include "iversioncontrol.h"
#include "mimedatabase.h"
#include "saveitemsdialog.h"
#include "vcsmanager.h"
#include "coreconstants.h"

#include <utils/qtcassert.h>
#include <utils/pathchooser.h>
#include <utils/reloadpromptutils.h>

#include <QtCore/QSettings>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QTimer>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QDateTime>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QMainWindow>

/*!
  \class Core::FileManager
  \mainclass
  \inheaderfile filemanager.h
  \brief Manages a set of IFile objects.

  The FileManager service monitors a set of IFile's. Plugins should register
  files they work with at the service. The files the IFile's point to will be
  monitored at filesystem level. If a file changes, the status of the IFile's
  will be adjusted accordingly. Furthermore, on application exit the user will
  be asked to save all modified files.

  Different IFile objects in the set can point to the same file in the
  filesystem. The monitoring for a IFile can be blocked by blockFileChange(), and
  enabled again by unblockFileChange().

  The functions expectFileChange() and unexpectFileChange() mark a file change
  as expected. On expected file changes all IFile objects are notified to reload
  themselves.

  The FileManager service also provides two convenience methods for saving
  files: saveModifiedFiles() and saveModifiedFilesSilently(). Both take a list
  of FileInterfaces as an argument, and return the list of files which were
  _not_ saved.

  The service also manages the list of recent files to be shown to the user
  (see addToRecentFiles() and recentFiles()).
 */

static const char settingsGroupC[] = "RecentFiles";
static const char filesKeyC[] = "Files";

static const char directoryGroupC[] = "Directories";
static const char projectDirectoryKeyC[] = "Projects";
static const char useProjectDirectoryKeyC[] = "UseProjectsDirectory";

namespace Core {
namespace Internal {

struct FileStateItem
{
    QDateTime modified;
    QFile::Permissions permissions;
};

struct FileState
{
    QMap<IFile *, FileStateItem> lastUpdatedState;
    FileStateItem expected;
};


struct FileManagerPrivate {
    explicit FileManagerPrivate(QObject *q, QMainWindow *mw);

    QMap<QString, FileState> m_states;
    QStringList m_changedFiles;
    QList<IFile *> m_filesWithoutWatch;

    QStringList m_recentFiles;
    static const int m_maxRecentFiles = 7;

    QString m_currentFile;

    QMainWindow *m_mainWindow;
    QFileSystemWatcher *m_fileWatcher;
    bool m_blockActivated;
    QString m_lastVisitedDirectory;
    QString m_projectsDirectory;
    bool m_useProjectsDirectory;
    // When we are callling into a IFile
    // we don't want to receive a changed()
    // signal
    // That makes the code easier
    IFile *m_blockedIFile;
};

FileManagerPrivate::FileManagerPrivate(QObject *q, QMainWindow *mw) :
    m_mainWindow(mw),
    m_fileWatcher(new QFileSystemWatcher(q)),
    m_blockActivated(false),
    m_lastVisitedDirectory(QDir::currentPath()),
#ifdef Q_OS_MAC  // Creator is in bizarre places when launched via finder.
    m_useProjectsDirectory(true),
#else
    m_useProjectsDirectory(false),
#endif
    m_blockedIFile(0)
{
}

} // namespace Internal

FileManager::FileManager(QMainWindow *mw)
  : QObject(mw),
    d(new Internal::FileManagerPrivate(this, mw))
{
    Core::ICore *core = Core::ICore::instance();
    connect(d->m_fileWatcher, SIGNAL(fileChanged(QString)),
        this, SLOT(changedFile(QString)));
    connect(d->m_mainWindow, SIGNAL(windowActivated()),
        this, SLOT(mainWindowActivated()));
    connect(core, SIGNAL(contextChanged(Core::IContext*,Core::Context)),
        this, SLOT(syncWithEditor(Core::IContext*)));

    readSettings();
}

FileManager::~FileManager()
{
    delete d;
}

/*!
    \fn bool FileManager::addFiles(const QList<IFile *> &files, bool addWatcher)

    Adds a list of IFile's to the collection. If \a addWatcher is true (the default),
    the files are added to a file system watcher that notifies the file manager
    about file changes.

    Returns true if the file specified by \a files have not been yet part of the file list.
*/
bool FileManager::addFiles(const QList<IFile *> &files, bool addWatcher)
{
    if (!addWatcher) {
        // We keep those in a separate list

        foreach(IFile *file, files)
            connect(file, SIGNAL(destroyed(QObject *)), this, SLOT(fileDestroyed(QObject *)));

        d->m_filesWithoutWatch.append(files);
        return true;
    }

    bool filesAdded = false;
    foreach (IFile *file, files) {
        if (!file)
            continue;
        const QString &fixedFileName = fixFileName(file->fileName());
        if (d->m_states.value(fixedFileName).lastUpdatedState.contains(file))
            continue;
        connect(file, SIGNAL(changed()), this, SLOT(checkForNewFileName()));
        connect(file, SIGNAL(destroyed(QObject *)), this, SLOT(fileDestroyed(QObject *)));
        filesAdded = true;

        addFileInfo(file);
    }
    return filesAdded;
}

void FileManager::addFileInfo(IFile *file)
{
    // We do want to insert the IFile into d->m_states even if the filename is empty
    // Such that m_states always contains all IFiles

    const QString fixedname = fixFileName(file->fileName());
    Internal::FileStateItem item;
    if (!fixedname.isEmpty()) {
        const QFileInfo fi(file->fileName());
        item.modified = fi.lastModified();
        item.permissions = fi.permissions();
    }

    if (!d->m_states.contains(fixedname)) {
        d->m_states.insert(fixedname, Internal::FileState());
        if (!fixedname.isEmpty()) {
            d->m_fileWatcher->addPath(fixedname);
        }
    }

    d->m_states[fixedname].lastUpdatedState.insert(file, item);
}

void FileManager::updateFileInfo(IFile *file)
{
    const QString fixedname = fixFileName(file->fileName());
    // If the filename is empty there's nothing to do
    if (fixedname.isEmpty())
        return;
    const QFileInfo fi(file->fileName());

    Internal::FileStateItem item;
    item.modified = fi.lastModified();
    item.permissions = fi.permissions();

    if (d->m_states.contains(fixedname) && d->m_states.value(fixedname).lastUpdatedState.contains(file))
        d->m_states[fixedname].lastUpdatedState.insert(file, item);
}

/// Dumps the state of the file manager's map
/// For debugging purposes
void FileManager::dump()
{
    QMap<QString, Internal::FileState>::const_iterator it, end;
    it = d->m_states.constBegin();
    end = d->m_states.constEnd();
    for (; it != end; ++it) {
        qDebug()<<" ";
        qDebug() << it.key();
        qDebug() << it.value().expected.modified;

        QMap<IFile *, Internal::FileStateItem>::const_iterator jt, jend;
        jt = it.value().lastUpdatedState.constBegin();
        jend = it.value().lastUpdatedState.constEnd();
        for (; jt != jend; ++jt) {
            qDebug() << jt.key() << jt.value().modified;
        }
    }
}

void FileManager::renamedFile(const QString &from, QString &to)
{
    QString fixedFrom = fixFileName(from);
    QString fixedTo = fixFileName(to);
    if (d->m_states.contains(fixedFrom)) {
        QTC_ASSERT(!d->m_states.contains(to), return);
        d->m_states.insert(fixedTo, d->m_states.value(fixedFrom));
        d->m_states.remove(fixedFrom);
        QFileInfo fi(to);
        d->m_states[fixedTo].expected.modified = fi.lastModified();
        d->m_states[fixedTo].expected.permissions = fi.permissions();

        d->m_fileWatcher->removePath(fixedFrom);
        d->m_fileWatcher->addPath(fixedTo);

        QMap<IFile *, Internal::FileStateItem>::iterator it, end;
        it = d->m_states[fixedTo].lastUpdatedState.begin();
        end = d->m_states[fixedTo].lastUpdatedState.end();

        for ( ; it != end; ++it) {
            d->m_blockedIFile = it.key();
            it.key()->rename(to);
            d->m_blockedIFile = it.key();
            it.value().modified = fi.lastModified();
        }
    }
}

///
/// Does not use file->fileName, as such is save to use
/// with renamed files and deleted files
void FileManager::removeFileInfo(IFile *file)
{
    QMap<QString, Internal::FileState>::const_iterator it, end;
    end = d->m_states.constEnd();
    for (it = d->m_states.constBegin(); it != end; ++it) {
        if (it.value().lastUpdatedState.contains(file)) {
            removeFileInfo(it.key(), file);
            break;
        }
    }

}

/* only called from removeFileInfo(IFile*) */
void FileManager::removeFileInfo(const QString &fileName, IFile *file)
{
    QTC_ASSERT(d->m_states.value(fileName).lastUpdatedState.contains(file), return);
    d->m_states[fileName].lastUpdatedState.remove(file);
    if (d->m_states.value(fileName).lastUpdatedState.isEmpty()) {
        if (!fileName.isEmpty())
            d->m_fileWatcher->removePath(fileName);
        d->m_states.remove(fileName); // this deletes fileName
    }
}

/*!
    \fn bool FileManager::addFile(IFile *files, bool addWatcher)

    Adds a IFile object to the collection. If \a addWatcher is true (the default),
    the file is added to a file system watcher that notifies the file manager
    about file changes.

    Returns true if the file specified by \a file has not been yet part of the file list.
*/
bool FileManager::addFile(IFile *file, bool addWatcher)
{
    return addFiles(QList<IFile *>() << file, addWatcher);
}

void FileManager::fileDestroyed(QObject *obj)
{
    // removeFileInfo works even if the file does not really exist anymore
    IFile *file = static_cast<IFile*>(obj);
    // Check the special unwatched first:
    if (d->m_filesWithoutWatch.contains(file)) {
        d->m_filesWithoutWatch.removeOne(file);
        return;
    }
    removeFileInfo(file);
}

/*!
    \fn bool FileManager::removeFile(IFile *file)

    Removes a IFile object from the collection.

    Returns true if the file specified by \a file has been part of the file list.
*/
bool FileManager::removeFile(IFile *file)
{
    if (!file)
        return false;

    // Special casing unwatched files
    if (d->m_filesWithoutWatch.contains(file)) {
        disconnect(file, SIGNAL(destroyed(QObject *)), this, SLOT(fileDestroyed(QObject *)));
        d->m_filesWithoutWatch.removeOne(file);
        return true;
    }

    disconnect(file, SIGNAL(changed()), this, SLOT(checkForNewFileName()));
    disconnect(file, SIGNAL(destroyed(QObject *)), this, SLOT(fileDestroyed(QObject *)));

    removeFileInfo(file);
    return true;
}

void FileManager::checkForNewFileName()
{
    IFile *file = qobject_cast<IFile *>(sender());
    // We modified the IFile
    // Trust the other code to also update the m_states map
    if (file == d->m_blockedIFile)
        return;
    QTC_ASSERT(file, return);
    const QString &fileName = fixFileName(file->fileName());

    // check if the IFile is in the map
    if (d->m_states.value(fileName).lastUpdatedState.contains(file)) {
        // the file might have been deleted and written again, so guard against that
        d->m_fileWatcher->removePath(fileName);
        d->m_fileWatcher->addPath(fileName);
        updateFileInfo(file);
        return;
    }

    // Probably the name has changed...
    // This also updates the state to the on disk state
    removeFileInfo(file);
    addFileInfo(file);
}

// TODO Rename to nativeFileName
QString FileManager::fixFileName(const QString &fileName)
{
    QString s = fileName;
    QFileInfo fi(s);
    if (!fi.exists())
        s = QDir::toNativeSeparators(s);
    else
        s = QDir::toNativeSeparators(fi.canonicalFilePath());
#ifdef Q_OS_WIN
    s = s.toLower();
#endif
    return s;
}

/*!
    \fn bool FileManager::isFileManaged(const QString  &fileName) const

    Returns true if at least one IFile in the set points to \a fileName.
*/
bool FileManager::isFileManaged(const QString &fileName) const
{
    if (fileName.isEmpty())
        return false;

    // TOOD check d->m_filesWithoutWatch

    return !d->m_states.contains(fixFileName(fileName));
}

/*!
    \fn QList<IFile*> FileManager::modifiedFiles() const

    Returns the list of IFile's that have been modified.
*/
QList<IFile *> FileManager::modifiedFiles() const
{
    QList<IFile *> modifiedFiles;

    QMap<QString, Internal::FileState>::const_iterator it, end;
    end = d->m_states.constEnd();
    for(it = d->m_states.constBegin(); it != end; ++it) {
        QMap<IFile *, Internal::FileStateItem>::const_iterator jt, jend;
        jt = (*it).lastUpdatedState.constBegin();
        jend = (*it).lastUpdatedState.constEnd();
        for( ; jt != jend; ++jt)
            if (jt.key()->isModified())
                modifiedFiles << jt.key();
    }
    foreach(IFile *file, d->m_filesWithoutWatch) {
        if (file->isModified())
            modifiedFiles << file;
    }

    return modifiedFiles;
}

/*!
    \fn void FileManager::blockFileChange(IFile *file)

    Blocks the monitoring of the file the \a file argument points to.
*/
void FileManager::blockFileChange(IFile *file)
{
    // Nothing to do
    Q_UNUSED(file);
}

/*!
    \fn void FileManager::unblockFileChange(IFile *file)

    Enables the monitoring of the file the \a file argument points to, and update the status of the corresponding IFile's.
*/
void FileManager::unblockFileChange(IFile *file)
{
    // We are updating the lastUpdated time to the current modification time
    // in changedFile we'll compare the modification time with the last updated
    // time, and if they are the same, then we don't deliver that notification
    // to corresponding IFile
    //
    // Also we are updating the expected time of the file
    // in changedFile we'll check if the modification time
    // is the same as the saved one here
    // If so then it's a expected change

    updateFileInfo(file);
    updateExpectedState(fixFileName(file->fileName()));
}

/*!
    \fn void FileManager::expectFileChange(const QString &fileName)

    Any subsequent change to \a fileName is treated as a expected file change.

    \see FileManager::unexpectFileChange(const QString &fileName)
*/
void FileManager::expectFileChange(const QString &fileName)
{
    // Nothing to do
    Q_UNUSED(fileName);
}

/*!
    \fn void FileManager::unexpectFileChange(const QString &fileName)

    Any change to \a fileName are unexpected again.

    \see FileManager::expectFileChange(const QString &fileName)
*/
void FileManager::unexpectFileChange(const QString &fileName)
{
    // We are updating the expected time of the file
    // And in changedFile we'll check if the modification time
    // is the same as the saved one here
    // If so then it's a expected change

    updateExpectedState(fileName);
}

void FileManager::updateExpectedState(const QString &fileName)
{
    const QString &fixedName = fixFileName(fileName);
    if (fixedName.isEmpty())
        return;
    QFileInfo fi(fixedName);
    if (d->m_states.contains(fixedName)) {
        d->m_states[fixedName].expected.modified = fi.lastModified();
        d->m_states[fixedName].expected.permissions = fi.permissions();
    }
}

/*!
    \fn QList<IFile*> FileManager::saveModifiedFilesSilently(const QList<IFile*> &files)

    Tries to save the files listed in \a files . Returns the files that could not be saved.
*/
QList<IFile *> FileManager::saveModifiedFilesSilently(const QList<IFile *> &files)
{
    return saveModifiedFiles(files, 0, true, QString());
}

/*!
    \fn QList<IFile*> FileManager::saveModifiedFiles(const QList<IFile *> &files, bool *cancelled, const QString &message, const QString &alwaysSaveMessage, bool *alwaysSave)

    Asks the user whether to save the files listed in \a files .
    Opens a dialog with the given \a message, and a additional
    text that should be used to ask if the user wants to enabled automatic save
    of modified files (in this context).
    The \a cancelled argument is set to true if the user cancelled the dialog,
    \a alwaysSave is set to match the selection of the user, if files should
    always automatically be saved.
    Returns the files that have not been saved.
*/
QList<IFile *> FileManager::saveModifiedFiles(const QList<IFile *> &files,
                                              bool *cancelled, const QString &message,
                                              const QString &alwaysSaveMessage,
                                              bool *alwaysSave)
{
    return saveModifiedFiles(files, cancelled, false, message, alwaysSaveMessage, alwaysSave);
}

static QMessageBox::StandardButton skipFailedPrompt(QWidget *parent, const QString &fileName)
{
    return QMessageBox::question(parent,
                                 FileManager::tr("Cannot save file"),
                                 FileManager::tr("Cannot save changes to '%1'. Do you want to continue and lose your changes?").arg(fileName),
                                 QMessageBox::YesToAll| QMessageBox::Yes|QMessageBox::No,
                                 QMessageBox::No);
}

QList<IFile *> FileManager::saveModifiedFiles(const QList<IFile *> &files,
                                              bool *cancelled,
                                              bool silently,
                                              const QString &message,
                                              const QString &alwaysSaveMessage,
                                              bool *alwaysSave)
{
    if (cancelled)
        (*cancelled) = false;

    QList<IFile *> notSaved;
    QMap<IFile *, QString> modifiedFilesMap;
    QList<IFile *> modifiedFiles;

    foreach (IFile *file, files) {
        if (file->isModified()) {
            QString name = file->fileName();
            if (name.isEmpty())
                name = file->suggestedFileName();

            // There can be several FileInterfaces pointing to the same file
            // Select one that is not readonly.
            if (!(modifiedFilesMap.key(name, 0)
                    && file->isReadOnly()))
                modifiedFilesMap.insert(file, name);
        }
    }
    modifiedFiles = modifiedFilesMap.keys();
    if (!modifiedFiles.isEmpty()) {
        QList<IFile *> filesToSave;
        if (silently) {
            filesToSave = modifiedFiles;
        } else {
            Internal::SaveItemsDialog dia(d->m_mainWindow, modifiedFiles);
            if (!message.isEmpty())
                dia.setMessage(message);
            if (!alwaysSaveMessage.isNull())
                dia.setAlwaysSaveMessage(alwaysSaveMessage);
            if (dia.exec() != QDialog::Accepted) {
                if (cancelled)
                    (*cancelled) = true;
                if (alwaysSave)
                    *alwaysSave = dia.alwaysSaveChecked();
                notSaved = modifiedFiles;
                return notSaved;
            }
            if (alwaysSave)
                *alwaysSave = dia.alwaysSaveChecked();
            filesToSave = dia.itemsToSave();
        }

        bool yestoall = false;
        Core::VCSManager *vcsManager = Core::ICore::instance()->vcsManager();
        foreach (IFile *file, filesToSave) {
            if (file->isReadOnly()) {
                const QString directory = QFileInfo(file->fileName()).absolutePath();
                if (IVersionControl *versionControl = vcsManager->findVersionControlForDirectory(directory))
                    versionControl->vcsOpen(file->fileName());
            }
            if (!file->isReadOnly() && !file->fileName().isEmpty()) {
                blockFileChange(file);
                const bool ok = file->save();
                unblockFileChange(file);
                if (!ok)
                    notSaved.append(file);
            } else if (QFile::exists(file->fileName()) && !file->isSaveAsAllowed()) {
                if (yestoall)
                    continue;
                const QFileInfo fi(file->fileName());
                switch (skipFailedPrompt(d->m_mainWindow, fi.fileName())) {
                case QMessageBox::YesToAll:
                    yestoall = true;
                    break;
                case QMessageBox::No:
                    if (cancelled)
                        *cancelled = true;
                    return filesToSave;
                default:
                    break;
                }
            } else {
                QString fileName = getSaveAsFileName(file);
                bool ok = false;
                if (!fileName.isEmpty()) {
                    blockFileChange(file);
                    ok = file->save(fileName);
                    file->checkPermissions();
                    unblockFileChange(file);
                }
                if (!ok)
                    notSaved.append(file);
            }
        }
    }
    return notSaved;
}

QString FileManager::getSaveFileName(const QString &title, const QString &pathIn,
                                     const QString &filter, QString *selectedFilter)
{
    const QString &path = pathIn.isEmpty() ? fileDialogInitialDirectory() : pathIn;
    QString fileName;
    bool repeat;
    do {
        repeat = false;
        fileName = QFileDialog::getSaveFileName(
            d->m_mainWindow, title, path, filter, selectedFilter, QFileDialog::DontConfirmOverwrite);
        if (!fileName.isEmpty()) {
            // If the selected filter is All Files (*) we leave the name exactly as the user
            // specified. Otherwise the suffix must be one available in the selected filter. If
            // the name already ends with such suffix nothing needs to be done. But if not, the
            // first one from the filter is appended.
            if (selectedFilter && *selectedFilter != QCoreApplication::translate(
                    "Core", Constants::ALL_FILES_FILTER)) {
                // Mime database creates filter strings like this: Anything here (*.foo *.bar)
                QRegExp regExp(".*\\s+\\((.*)\\)$");
                const int index = regExp.lastIndexIn(*selectedFilter);
                bool suffixOk = false;
                if (index != -1) {
                    const QStringList &suffixes = regExp.cap(1).remove('*').split(' ');
                    foreach (const QString &suffix, suffixes)
                        if (fileName.endsWith(suffix)) {
                            suffixOk = true;
                            break;
                        }
                    if (!suffixOk && !suffixes.isEmpty())
                        fileName.append(suffixes.at(0));
                }
            }
            if (QFile::exists(fileName)) {
                if (QMessageBox::warning(d->m_mainWindow, tr("Overwrite?"),
                    tr("An item named '%1' already exists at this location. "
                       "Do you want to overwrite it?").arg(fileName),
                    QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
                    repeat = true;
                }
            }
        }
    } while (repeat);
    if (!fileName.isEmpty())
        setFileDialogLastVisitedDirectory(QFileInfo(fileName).absolutePath());
    return fileName;
}

QString FileManager::getSaveFileNameWithExtension(const QString &title, const QString &pathIn,
                                                  const QString &filter)
{
    QString selected = filter;
    return getSaveFileName(title, pathIn, filter, &selected);
}

/*!
    \fn QString FileManager::getSaveAsFileName(IFile *file)

    Asks the user for a new file name (Save File As) for /arg file.
*/
QString FileManager::getSaveAsFileName(IFile *file, const QString &filter, QString *selectedFilter)
{
    if (!file)
        return QLatin1String("");
    QString absoluteFilePath = file->fileName();
    const QFileInfo fi(absoluteFilePath);
    QString fileName = fi.fileName();
    QString path = fi.absolutePath();
    if (absoluteFilePath.isEmpty()) {
        fileName = file->suggestedFileName();
        const QString defaultPath = file->defaultPath();
        if (!defaultPath.isEmpty())
            path = defaultPath;
    }

    QString filterString;
    if (filter.isEmpty()) {
        if (const MimeType &mt = Core::ICore::instance()->mimeDatabase()->findByFile(fi))
            filterString = mt.filterString();
        selectedFilter = &filterString;
    } else {
        filterString = filter;
    }

    absoluteFilePath = getSaveFileName(tr("Save File As"),
        path + QDir::separator() + fileName,
        filterString,
        selectedFilter);
    return absoluteFilePath;
}

/*!
    \fn QString FileManager::getOpenFileNames(const QString &filters, const QString &pathIn, QString *selectedFilter) const

    Asks the user for a set of file names to be opened. The \a filters
    and \a selectedFilter parameters is interpreted like in
    QFileDialog::getOpenFileNames(), \a pathIn specifies a path to open the dialog
    in, if that is not overridden by the users policy.
*/

QStringList FileManager::getOpenFileNames(const QString &filters,
                                          const QString pathIn,
                                          QString *selectedFilter)
{
    QString path = pathIn;
    if (path.isEmpty()) {
        if (!d->m_currentFile.isEmpty())
            path = QFileInfo(d->m_currentFile).absoluteFilePath();
        if (path.isEmpty() && useProjectsDirectory())
            path = projectsDirectory();
    }
    const QStringList files = QFileDialog::getOpenFileNames(d->m_mainWindow,
                                                      tr("Open File"),
                                                      path, filters,
                                                      selectedFilter);
    if (!files.isEmpty())
        setFileDialogLastVisitedDirectory(QFileInfo(files.front()).absolutePath());
    return files;
}


void FileManager::changedFile(const QString &fileName)
{
    const bool wasempty = d->m_changedFiles.isEmpty();

    const QString &fixedName = fixFileName(fileName);
    if (!d->m_changedFiles.contains(fixedName))
        d->m_changedFiles.append(fixedName);

    if (wasempty && !d->m_changedFiles.isEmpty()) {
        QTimer::singleShot(200, this, SLOT(checkForReload()));
    }
}

void FileManager::mainWindowActivated()
{
    //we need to do this asynchronously because
    //opening a dialog ("Reload?") in a windowactivated event
    //freezes on Mac
    QTimer::singleShot(0, this, SLOT(checkForReload()));
}

void FileManager::checkForReload()
{
    if (QApplication::activeWindow() != d->m_mainWindow)
        return;

    if (d->m_blockActivated)
        return;

    d->m_blockActivated = true;

    IFile::ReloadSetting defaultBehavior = EditorManager::instance()->reloadSetting();
    Utils::ReloadPromptAnswer previousAnswer = Utils::ReloadCurrent;

    QList<IEditor*> editorsToClose;
    QMap<IFile*, QString> filesToSave;
    QStringList modifiedFileNames;
    foreach (const QString &fileName, d->m_changedFiles) {
        // Get the information from the filesystem
        IFile::ChangeTrigger behavior = IFile::TriggerExternal;
        IFile::ChangeType type = IFile::TypeContents;
        QFileInfo fi(fileName);
        if (!fi.exists()) {
            type = IFile::TypeRemoved;
        } else {
            modifiedFileNames << fileName;
            if (fi.lastModified() == d->m_states.value(fileName).expected.modified
                && fi.permissions() == d->m_states.value(fileName).expected.permissions) {
                behavior = IFile::TriggerInternal;
            }
        }

        const QMap<IFile *, Internal::FileStateItem> &lastUpdated =
                d->m_states.value(fileName).lastUpdatedState;
        QMap<IFile *, Internal::FileStateItem>::const_iterator it, end;
        it = lastUpdated.constBegin();
        end = lastUpdated.constEnd();
        for ( ; it != end; ++it) {
            IFile *file = it.key();
            d->m_blockedIFile = file;
            // Compare
            if (it.value().modified == fi.lastModified()
                && it.value().permissions == fi.permissions()) {
                // Already up to date
                continue;
            }
            // we've got some modification
            // check if it's contents or permissions:
            if (it.value().modified == fi.lastModified()) {
                // Only permission change
                file->reload(IFile::FlagReload, IFile::TypePermissions);
            // now we know it's a content change or file was removed
            } else if (defaultBehavior == IFile::ReloadUnmodified
                       && type == IFile::TypeContents && !file->isModified()) {
                // content change, but unmodified (and settings say to reload in this case)
                file->reload(IFile::FlagReload, type);
            // file was removed or it's a content change and the default behavior for
            // unmodified files didn't kick in
            } else if (defaultBehavior == IFile::IgnoreAll) {
                // content change or removed, but settings say ignore
                file->reload(IFile::FlagIgnore, type);
            // either the default behavior is to always ask,
            // or the ReloadUnmodified default behavior didn't kick in,
            // so do whatever the IFile wants us to do
            } else {
                // check if IFile wants us to ask
                if (file->reloadBehavior(behavior, type) == IFile::BehaviorSilent) {
                    // content change or removed, IFile wants silent handling
                    file->reload(IFile::FlagReload, type);
                // IFile wants us to ask
                } else if (type == IFile::TypeContents) {
                    // content change, IFile wants to ask user
                    if (previousAnswer == Utils::ReloadNone) {
                        // answer already given, ignore
                        file->reload(IFile::FlagIgnore, IFile::TypeContents);
                    } else if (previousAnswer == Utils::ReloadAll) {
                        // answer already given, reload
                        file->reload(IFile::FlagReload, IFile::TypeContents);
                    } else {
                        // Ask about content change
                        previousAnswer = Utils::reloadPrompt(fileName, file->isModified(), QApplication::activeWindow());
                        switch (previousAnswer) {
                        case Utils::ReloadAll:
                        case Utils::ReloadCurrent:
                            file->reload(IFile::FlagReload, IFile::TypeContents);
                            break;
                        case Utils::ReloadSkipCurrent:
                        case Utils::ReloadNone:
                            file->reload(IFile::FlagIgnore, IFile::TypeContents);
                            break;
                        }
                    }
                // IFile wants us to ask, and it's the TypeRemoved case
                } else {
                    // Ask about removed file
                    bool unhandled = true;
                    while (unhandled) {
                        switch (Utils::fileDeletedPrompt(fileName, QApplication::activeWindow())) {
                        case Utils::FileDeletedSave:
                            filesToSave.insert(file, fileName);
                            unhandled = false;
                            break;
                        case Utils::FileDeletedSaveAs:
                            {
                                const QString &saveFileName = getSaveAsFileName(file);
                                if (!saveFileName.isEmpty()) {
                                    filesToSave.insert(file, saveFileName);
                                    unhandled = false;
                                }
                                break;
                            }
                        case Utils::FileDeletedClose:
                            editorsToClose << EditorManager::instance()->editorsForFile(file);
                            unhandled = false;
                            break;
                        }
                    }
                }
            }

            updateFileInfo(file);
            d->m_blockedIFile = 0;
        }
    }

    // cleanup
    if (!modifiedFileNames.isEmpty()) {
        d->m_fileWatcher->removePaths(modifiedFileNames);
        d->m_fileWatcher->addPaths(modifiedFileNames);
    }
    d->m_changedFiles.clear();

    // handle deleted files
    EditorManager::instance()->closeEditors(editorsToClose, false);
    QMapIterator<IFile *, QString> it(filesToSave);
    while (it.hasNext()) {
        it.next();
        blockFileChange(it.key());
        it.key()->save(it.value());
        unblockFileChange(it.key());
        it.key()->checkPermissions();
    }

    d->m_blockActivated = false;
}

void FileManager::syncWithEditor(Core::IContext *context)
{
    if (!context)
        return;

    Core::IEditor *editor = Core::EditorManager::instance()->currentEditor();
    if (editor && (editor->widget() == context->widget()) &&
        !editor->isTemporary())
        setCurrentFile(editor->file()->fileName());
}

/*!
    \fn void FileManager::addToRecentFiles(const QString &fileName)

    Adds the \a fileName to the list of recent files.
*/
void FileManager::addToRecentFiles(const QString &fileName)
{
    if (fileName.isEmpty())
        return;
    QString unifiedForm(fixFileName(fileName));
    QMutableStringListIterator it(d->m_recentFiles);
    while (it.hasNext()) {
        QString recentUnifiedForm(fixFileName(it.next()));
        if (unifiedForm == recentUnifiedForm)
            it.remove();
    }
    if (d->m_recentFiles.count() > d->m_maxRecentFiles)
        d->m_recentFiles.removeLast();
    d->m_recentFiles.prepend(fileName);
}

/*!
    \fn QStringList FileManager::recentFiles() const

    Returns the list of recent files.
*/
QStringList FileManager::recentFiles() const
{
    return d->m_recentFiles;
}

void FileManager::saveSettings()
{
    QSettings *s = Core::ICore::instance()->settings();
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(filesKeyC), d->m_recentFiles);
    s->endGroup();
    s->beginGroup(QLatin1String(directoryGroupC));
    s->setValue(QLatin1String(projectDirectoryKeyC), d->m_projectsDirectory);
    s->setValue(QLatin1String(useProjectDirectoryKeyC), d->m_useProjectsDirectory);
    s->endGroup();
}

void FileManager::readSettings()
{
    const QSettings *s = Core::ICore::instance()->settings();
    d->m_recentFiles.clear();
    QStringList recentFiles = s->value(QLatin1String(settingsGroupC) + QLatin1Char('/') + QLatin1String(filesKeyC), QStringList()).toStringList();
    // clean non-existing files
    foreach (const QString &file, recentFiles) {
        if (QFileInfo(file).isFile())
            d->m_recentFiles.append(QDir::fromNativeSeparators(file)); // from native to guard against old settings
    }

    const QString directoryGroup = QLatin1String(directoryGroupC) + QLatin1Char('/');
    const QString settingsProjectDir = s->value(directoryGroup + QLatin1String(projectDirectoryKeyC),
                                       QString()).toString();
    if (!settingsProjectDir.isEmpty() && QFileInfo(settingsProjectDir).isDir()) {
        d->m_projectsDirectory = settingsProjectDir;
    } else {
        d->m_projectsDirectory = Utils::PathChooser::homePath();
    }
    d->m_useProjectsDirectory = s->value(directoryGroup + QLatin1String(useProjectDirectoryKeyC),
                                         d->m_useProjectsDirectory).toBool();
}

/*!

  The current file is e.g. the file currently opened when an editor is active,
  or the selected file in case a Project Explorer is active ...

  \sa currentFile
  */
void FileManager::setCurrentFile(const QString &filePath)
{
    if (d->m_currentFile == filePath)
        return;
    d->m_currentFile = filePath;
    emit currentFileChanged(d->m_currentFile);
}

/*!
  Returns the absolute path of the current file

  The current file is e.g. the file currently opened when an editor is active,
  or the selected file in case a Project Explorer is active ...

  \sa setCurrentFile
  */
QString FileManager::currentFile() const
{
    return d->m_currentFile;
}

/*!

  Returns the initial directory for a new file dialog. If there is
  a current file, use that, else use last visited directory.

  \sa setFileDialogLastVisitedDirectory
*/

QString FileManager::fileDialogInitialDirectory() const
{
    if (!d->m_currentFile.isEmpty())
        return QFileInfo(d->m_currentFile).absolutePath();
    return d->m_lastVisitedDirectory;
}

/*!

  Returns the directory for projects. Defaults to HOME.

  \sa setProjectsDirectory, setUseProjectsDirectory
*/

QString FileManager::projectsDirectory() const
{
    return d->m_projectsDirectory;
}

/*!

  Set the directory for projects.

  \sa projectsDirectory, useProjectsDirectory
*/

void FileManager::setProjectsDirectory(const QString &dir)
{
    d->m_projectsDirectory = dir;
}

/*!

  Returns whether the directory for projects is to be
  used or the user wants the current directory.

  \sa setProjectsDirectory, setUseProjectsDirectory
*/

bool FileManager::useProjectsDirectory() const
{
    return d->m_useProjectsDirectory;
}

/*!

  Sets whether the directory for projects is to be used.

  \sa projectsDirectory, useProjectsDirectory
*/

void FileManager::setUseProjectsDirectory(bool useProjectsDirectory)
{
    d->m_useProjectsDirectory = useProjectsDirectory;
}

/*!

  Returns last visited directory of a file dialog.

  \sa setFileDialogLastVisitedDirectory, fileDialogInitialDirectory

*/

QString FileManager::fileDialogLastVisitedDirectory() const
{
    return d->m_lastVisitedDirectory;
}

/*!

  Set the last visited directory of a file dialog that will be remembered
  for the next one.

  \sa fileDialogLastVisitedDirectory, fileDialogInitialDirectory

  */

void FileManager::setFileDialogLastVisitedDirectory(const QString &directory)
{
    d->m_lastVisitedDirectory = directory;
}

void FileManager::notifyFilesChangedInternally(const QStringList &files)
{
    emit filesChangedInternally(files);
}

// -------------- FileChangeBlocker

FileChangeBlocker::FileChangeBlocker(const QString &fileName)
    : m_fileName(fileName)
{
    Core::FileManager *fm = Core::ICore::instance()->fileManager();
    fm->expectFileChange(fileName);
}

FileChangeBlocker::~FileChangeBlocker()
{
    Core::FileManager *fm = Core::ICore::instance()->fileManager();
    fm->unexpectFileChange(m_fileName);
}

} // namespace Core
