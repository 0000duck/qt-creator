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

#ifndef GETTINGSTARTEDWELCOMEPAGEWIDGET_H
#define GETTINGSTARTEDWELCOMEPAGEWIDGET_H

#include <QtGui/QWidget>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

QT_BEGIN_NAMESPACE
class QUrl;
class QLabel;
class QFile;
class QMenu;
QT_END_NAMESPACE

namespace Core {
    namespace Internal {
    class RssFetcher;
    class RssItem;
    }
}

using namespace Core::Internal;

namespace Qt4ProjectManager {
namespace Internal {

namespace Ui {
    class GettingStartedWelcomePageWidget;
}

typedef QHash<QString, QMenu*> QMenuHash;

class PixmapDownloader : public QNetworkAccessManager {
    Q_OBJECT
public:
    PixmapDownloader(const QUrl& url, QLabel* label, QObject *parent = 0)
        : QNetworkAccessManager(parent), m_url(url), m_label(label)
    {
        connect(this, SIGNAL(finished(QNetworkReply*)), SLOT(populatePixmap(QNetworkReply*)));
        get(QNetworkRequest(url));
    }
public slots:
    void populatePixmap(QNetworkReply* reply);

private:
    QUrl m_url;
    QLabel *m_label;

};

class GettingStartedWelcomePageWidget : public QWidget
{
    Q_OBJECT
public:
    GettingStartedWelcomePageWidget(QWidget *parent = 0);
    ~GettingStartedWelcomePageWidget();

public slots:
    void updateExamples(const QString &examplePath,
                        const QString &demosPath,
                        const QString &sourcePath);

private slots:
    void slotOpenHelpPage(const QString &url);
    void slotOpenContextHelpPage(const QString &url);
    void slotOpenExample();
    void slotNextTip();
    void slotPrevTip();
    void slotNextFeature();
    void slotPrevFeature();
    void slotCreateNewProject();
    void addToFeatures(const RssItem&);
    void showFeature(int feature = -1);

signals:
    void startRssFetching(const QUrl&);

private:
    void parseXmlFile(QFile *file, QMenuHash &cppSubMenuHash, QMenuHash &qmlSubMenuHash,
                      const QString &examplePath, const QString &sourcePath);
    QStringList tipsOfTheDay();
    Ui::GettingStartedWelcomePageWidget *ui;
    int m_currentTip;
    int m_currentFeature;
    QList<Core::Internal::RssItem> m_featuredItems;
    Core::Internal::RssFetcher *m_rssFetcher;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // GETTINGSTARTEDWELCOMEPAGEWIDGET_H
