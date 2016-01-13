/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#ifndef TESTINFO_H
#define TESTINFO_H

#include <QStringList>

namespace Autotest {
namespace Internal {

class TestInfo
{
public:
    explicit TestInfo(const QString &className = QString(),
                      const QStringList &functions = QStringList(),
                      unsigned revision = 0, unsigned editorRevision = 0);

    ~TestInfo();
    const QString testClass() const { return m_className; }
    void setTestClass(const QString &className) { m_className = className; }
    const QStringList testFunctions() const { return m_functions; }
    void setTestFunctions(const QStringList &functions) { m_functions = functions; }
    unsigned revision() const { return m_revision; }
    void setRevision(unsigned revision) { m_revision = revision; }
    unsigned editorRevision() const { return m_editorRevision; }
    void setEditorRevision(unsigned editorRevision) { m_editorRevision = editorRevision; }
    const QString referencingFile() const { return m_referencingFile; }
    void setReferencingFile(const QString &refFile) { m_referencingFile = refFile; }
    const QString proFile() const { return m_proFile; }
    void setProFile(const QString &proFile) { m_proFile = proFile; }

private:
    QString m_className;
    QStringList m_functions;
    unsigned m_revision;
    unsigned m_editorRevision;
    QString m_referencingFile;
    QString m_proFile;
};

class UnnamedQuickTestInfo {
public:
    explicit UnnamedQuickTestInfo(const QString &function = QString(),
                                  const QString& fileName = QString());
    ~UnnamedQuickTestInfo() {}

    const QString function() const { return m_function; }
    void setFunction(const QString &function) { m_function = function; }
    const QString fileName() const { return m_fileName; }
    void setFileName(const QString &fileName) { m_fileName = fileName; }

private:
    QString m_function;
    QString m_fileName;
};

class GTestInfo {
public:
    explicit GTestInfo(const QString &caseName, const QString &setName, const QString &file);

    const QString caseName() const { return m_caseName; }
    void setCaseName(const QString &caseName) { m_caseName = caseName; }
    const QString setName() const { return m_setName; }
    void setSetName(const QString &setName) { m_setName = setName; }
    const QString fileName() const { return m_fileName; }
    void setFileName(const QString &fileName) { m_fileName = fileName; }
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }

private:
    QString m_caseName;
    QString m_setName;
    QString m_fileName;
    bool m_enabled;
};

} // namespace Internal
} // namespace Autotest

#endif // TESTINFO_H
