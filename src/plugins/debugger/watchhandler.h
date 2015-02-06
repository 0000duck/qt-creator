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

#ifndef DEBUGGER_WATCHHANDLER_H
#define DEBUGGER_WATCHHANDLER_H

#include "watchdata.h"

#include <utils/treemodel.h>

#include <QPointer>
#include <QVector>

namespace Debugger {
namespace Internal {

class SeparatedView;
class WatchModel;

class WatchItem : public Utils::TreeItem
{
public:
    WatchItem();
    WatchItem(const QByteArray &i, const QString &n);
    explicit WatchItem(const WatchData &data);
    explicit WatchItem(const GdbMi &data);

    WatchItem *parentItem() const;
    const WatchModel *watchModel() const;
    WatchModel *watchModel();

    QVariant data(int column, int role) const;
    Qt::ItemFlags flags(int column) const;

    bool canFetchMore() const;
    void fetchMore();

    QString displayName() const;
    QString displayType() const;
    QString displayValue() const;
    QString formattedValue() const;
    QString expression() const;

    int itemFormat() const;

    QVariant editValue() const;
    int editType() const;

    void formatRequests(QByteArray *out) const;
    void showInEditorHelper(QString *contents, int depth) const;
    WatchItem *findItem(const QByteArray &iname);
    void parseWatchData(const GdbMi &input);

public:
    WatchData d;
    bool fetchTriggered;
};

// Special formats. Keep in sync with dumper.py.
enum DisplayFormat
{
    AutomaticFormat = -1, // Based on type for individuals, dumper default for types.
    RawFormat = 0,

    // Values between 1 and 99 refer to dumper provided custom formats.

    // Values between 100 and 199 refer to well-known formats handled in dumpers.
    KnownDumperFormatBase = 100,
    Latin1StringFormat,
    Utf8StringFormat,
    Local8BitStringFormat,
    Utf16StringFormat,
    Ucs4StringFormat,

    Array10Format,
    Array100Format,
    Array1000Format,
    Array10000Format,

    SeparateLatin1StringFormat,
    SeparateUtf8StringFormat,


    // Values above 200 refer to format solely handled in the WatchHandler code
    ArtificialFormatBase = 200,

    BoolTextFormat,
    BoolIntegerFormat,

    DecimalIntegerFormat,
    HexadecimalIntegerFormat,
    BinaryIntegerFormat,
    OctalIntegerFormat,

    CompactFloatFormat,
    ScientificFloatFormat,
};


class TypeFormatItem
{
public:
    TypeFormatItem() : format(-1) {}
    TypeFormatItem(const QString &display, int format);

    QString display;
    int format;
};

class TypeFormatList : public QVector<TypeFormatItem>
{
public:
    using QVector::append;
    void append(int format);
    TypeFormatItem find(int format) const;
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::TypeFormatList)

namespace Debugger {
namespace Internal {

class DebuggerEngine;

class UpdateParameters
{
public:
    UpdateParameters() { tryPartial = false; }

    bool tryPartial;
    QByteArray varList;
};

typedef QHash<QString, QStringList> DumperTypeFormats; // Type name -> Dumper Formats

class WatchModelBase : public Utils::TreeModel
{
    Q_OBJECT

public:
    WatchModelBase() {}

signals:
    void currentIndexRequested(const QModelIndex &idx);
    void itemIsExpanded(const QModelIndex &idx);
    void columnAdjustmentRequested();
};

class WatchHandler : public QObject
{
    Q_OBJECT

public:
    explicit WatchHandler(DebuggerEngine *engine);
    ~WatchHandler();

    WatchModelBase *model() const;

    void cleanup();
    void watchExpression(const QString &exp, const QString &name = QString());
    void watchVariable(const QString &exp);
    Q_SLOT void clearWatches();

    void showEditValue(const WatchData &data);

    const WatchData *watchData(const QModelIndex &) const;
    void fetchMore(const QByteArray &iname) const;
    const WatchData *findData(const QByteArray &iname) const;
    WatchItem *findItem(const QByteArray &iname) const;
    const WatchData *findCppLocalVariable(const QString &name) const;
    bool hasItem(const QByteArray &iname) const;

    void loadSessionData();
    void saveSessionData();

    bool isExpandedIName(const QByteArray &iname) const;
    QSet<QByteArray> expandedINames() const;

    static QStringList watchedExpressions();
    static QHash<QByteArray, int> watcherNames();

    QByteArray expansionRequests() const;
    QByteArray typeFormatRequests() const;
    QByteArray individualFormatRequests() const;

    int format(const QByteArray &iname) const;

    void addDumpers(const GdbMi &dumpers);
    void addTypeFormats(const QByteArray &type, const QStringList &formats);
    void setTypeFormats(const DumperTypeFormats &typeFormats);
    DumperTypeFormats typeFormats() const;

    void setUnprintableBase(int base);
    static int unprintableBase();

    QByteArray watcherName(const QByteArray &exp);
    QString editorContents();
    void editTypeFormats(bool includeLocals, const QByteArray &iname);

    void scheduleResetLocation();
    void resetLocation();

    void setCurrentItem(const QByteArray &iname);
    void updateWatchersWindow();

    void insertData(const WatchData &data); // Convenience.
    void insertData(const QList<WatchData> &list);
    void insertIncompleteData(const WatchData &data);
    void insertItem(WatchItem *item);
    void removeData(const QByteArray &iname);
    void removeChildren(const QByteArray &iname);
    void removeAllData(bool includeInspectData = false);
    void resetValueCache();
    void purgeOutdatedItems(const QSet<QByteArray> &inames);

private:
    friend class WatchModel;

    void saveWatchers();
    static void loadFormats();
    static void saveFormats();

    void setFormat(const QByteArray &type, int format);

    WatchModel *m_model; // Owned.
    DebuggerEngine *m_engine; // Not owned.
    SeparatedView *m_separatedView; // Owned.

    int m_watcherCounter;

    bool m_contentsValid;
    bool m_resetLocationScheduled;
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::UpdateParameters)

#endif // DEBUGGER_WATCHHANDLER_H
