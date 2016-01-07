/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise ClangStaticAnalyzer Add-on.
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

#ifndef CLANGSTATICANALYZERRUNCONTROL_H
#define CLANGSTATICANALYZERRUNCONTROL_H

#include <analyzerbase/analyzerruncontrol.h>
#include <cpptools/cppprojects.h>
#include <utils/environment.h>

#include <QFutureInterface>
#include <QStringList>

namespace ClangStaticAnalyzer {
namespace Internal {

class ClangStaticAnalyzerRunner;
class Diagnostic;

struct AnalyzeUnit {
    AnalyzeUnit(const QString &file, const QStringList &options)
        : file(file), arguments(options) {}

    QString file;
    QStringList arguments; // without file itself and "-o somePath"
};
typedef QList<AnalyzeUnit> AnalyzeUnits;

class ClangStaticAnalyzerRunControl : public Analyzer::AnalyzerRunControl
{
    Q_OBJECT

public:
    ClangStaticAnalyzerRunControl(const Analyzer::AnalyzerStartParameters &startParams,
                                  ProjectExplorer::RunConfiguration *runConfiguration,
                                  Core::Id runMode,
                                  const CppTools::ProjectInfo &projectInfo);

    bool startEngine();
    void stopEngine();

    bool success() const { return m_success; } // For testing.

signals:
    void newDiagnosticsAvailable(const QList<Diagnostic> &diagnostics);

private:
    AnalyzeUnits sortedUnitsToAnalyze();
    void analyzeNextFile();
    ClangStaticAnalyzerRunner *createRunner();

    void onRunnerFinishedWithSuccess(const QString &logFilePath);
    void onRunnerFinishedWithFailure(const QString &errorMessage, const QString &errorDetails);
    void handleFinished();

    void onProgressCanceled();
    void updateProgressValue();

    void finalize();

private:
    const CppTools::ProjectInfo m_projectInfo;
    const unsigned char m_wordWidth;

    Utils::Environment m_environment;
    QString m_clangExecutable;
    QString m_clangLogFileDir;
    QFutureInterface<void> m_progress;
    AnalyzeUnits m_unitsToProcess;
    QSet<ClangStaticAnalyzerRunner *> m_runners;
    int m_initialFilesToProcessSize;
    int m_filesAnalyzed;
    int m_filesNotAnalyzed;
    bool m_success;
};

} // namespace Internal
} // namespace ClangStaticAnalyzer

#endif // CLANGSTATICANALYZERRUNCONTROL_H
