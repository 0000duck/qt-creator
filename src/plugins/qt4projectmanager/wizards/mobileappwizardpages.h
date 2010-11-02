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

#ifndef MOBILEAPPWIZARDPAGES_H
#define MOBILEAPPWIZARDPAGES_H

#include "abstractmobileapp.h"

#include <QtGui/QWizardPage>

namespace Qt4ProjectManager {
namespace Internal {

class MobileAppWizardOptionsPage : public QWizardPage
{
    Q_OBJECT
    Q_DISABLE_COPY(MobileAppWizardOptionsPage)

public:
    explicit MobileAppWizardOptionsPage(QWidget *parent = 0);
    virtual ~MobileAppWizardOptionsPage();

    void setOrientation(AbstractMobileApp::ScreenOrientation orientation);
    AbstractMobileApp::ScreenOrientation orientation() const;
    QString symbianSvgIcon() const;
    void setSymbianSvgIcon(const QString &icon);
    QString maemoPngIcon() const;
    void setMaemoPngIcon(const QString &icon);
    QString symbianUid() const;
    void setNetworkEnabled(bool enableIt);
    bool networkEnabled() const;
    void setSymbianUid(const QString &uid);

private slots:
    void openSymbianSvgIcon(); // Via file open dialog
    void openMaemoPngIcon();

private:
    class MobileAppWizardOptionsPagePrivate *m_d;
};

} // end of namespace Internal
} // end of namespace Qt4ProjectManager

#endif // MOBILEAPPWIZARDPAGES_H
