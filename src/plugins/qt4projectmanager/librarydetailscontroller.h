#ifndef LIBRARYDETAILSCONTROLLER_H
#define LIBRARYDETAILSCONTROLLER_H

#include <QtGui/QWidget>
#include "addlibrarywizard.h"

namespace Qt4ProjectManager {
namespace Internal {

namespace Ui {
    class LibraryDetailsWidget;
}

class Qt4ProFileNode;

class LibraryDetailsController : public QObject
{
    Q_OBJECT
public:
    explicit LibraryDetailsController(Ui::LibraryDetailsWidget *libraryDetails,
                                      const QString &proFile,
                                      QObject *parent = 0);
    virtual bool isComplete() const = 0;
    virtual QString snippet() const = 0;
signals:
    void completeChanged();
protected:

    enum CreatorPlatform {
        CreatorLinux,
        CreatorMac,
        CreatorWindows
    };

    CreatorPlatform creatorPlatform() const;

    Ui::LibraryDetailsWidget *libraryDetailsWidget() const;

    AddLibraryWizard::Platforms platforms() const;
    AddLibraryWizard::LinkageType linkageType() const;
    AddLibraryWizard::MacLibraryType macLibraryType() const;
    QString proFile() const;
    bool isIncludePathChanged() const;
    bool guiSignalsIgnored() const;

    void updateGui();
    virtual AddLibraryWizard::LinkageType suggestedLinkageType() const = 0;
    virtual AddLibraryWizard::MacLibraryType suggestedMacLibraryType() const = 0;
    virtual QString suggestedIncludePath() const = 0;
    virtual void updateWindowsOptionsEnablement() = 0;

    void setIgnoreGuiSignals(bool ignore);

    void setPlatformsVisible(bool ena);
    void setLinkageRadiosVisible(bool ena);
    void setLinkageGroupVisible(bool ena);
    void setMacLibraryRadiosVisible(bool ena);
    void setMacLibraryGroupVisible(bool ena);
    void setLibraryPathChooserVisible(bool ena);
    void setLibraryComboBoxVisible(bool ena);
    void setPackageLineEditVisible(bool ena);
    void setIncludePathVisible(bool ena);
    void setWindowsGroupVisible(bool ena);
    void setRemoveSuffixVisible(bool ena);

    bool isMacLibraryRadiosVisible() const;
    bool isIncludePathVisible() const;
    bool isWindowsGroupVisible() const;

private slots:
    void slotIncludePathChanged();
    void slotPlatformChanged();
    void slotMacLibraryTypeChanged();
    void slotUseSubfoldersChanged(bool ena);
    void slotAddSuffixChanged(bool ena);
private:

    void showLinkageType(AddLibraryWizard::LinkageType linkageType);
    void showMacLibraryType(AddLibraryWizard::MacLibraryType libType);

    AddLibraryWizard::Platforms m_platforms;
    AddLibraryWizard::LinkageType m_linkageType;
    AddLibraryWizard::MacLibraryType m_macLibraryType;

    QString m_proFile;

    CreatorPlatform m_creatorPlatform;

    bool m_ignoreGuiSignals;
    bool m_includePathChanged;

    bool m_linkageRadiosVisible;
    bool m_macLibraryRadiosVisible;
    bool m_includePathVisible;
    bool m_windowsGroupVisible;

    Ui::LibraryDetailsWidget *m_libraryDetailsWidget;
};

class NonInternalLibraryDetailsController : public LibraryDetailsController
{
    Q_OBJECT
public:
    explicit NonInternalLibraryDetailsController(Ui::LibraryDetailsWidget *libraryDetails,
                                                 const QString &proFile,
                                                 QObject *parent = 0);
    virtual bool isComplete() const;
    virtual QString snippet() const;
protected:
    virtual AddLibraryWizard::LinkageType suggestedLinkageType() const;
    virtual AddLibraryWizard::MacLibraryType suggestedMacLibraryType() const;
    virtual QString suggestedIncludePath() const;
    virtual void updateWindowsOptionsEnablement();
private slots:
    void slotLinkageTypeChanged();
    void slotRemoveSuffixChanged(bool ena);
    void slotLibraryPathChanged();
};

class PackageLibraryDetailsController : public NonInternalLibraryDetailsController
{
    Q_OBJECT
public:
    explicit PackageLibraryDetailsController(Ui::LibraryDetailsWidget *libraryDetails,
                                            const QString &proFile,
                                            QObject *parent = 0);
    virtual bool isComplete() const;
    virtual QString snippet() const;
private:
    bool isLinkPackageGenerated() const;
};

class SystemLibraryDetailsController : public NonInternalLibraryDetailsController
{
    Q_OBJECT
public:
    explicit SystemLibraryDetailsController(Ui::LibraryDetailsWidget *libraryDetails,
                                            const QString &proFile,
                                            QObject *parent = 0);
};

class ExternalLibraryDetailsController : public NonInternalLibraryDetailsController
{
    Q_OBJECT
public:
    explicit ExternalLibraryDetailsController(Ui::LibraryDetailsWidget *libraryDetails,
                                              const QString &proFile,
                                              QObject *parent = 0);
protected:
    virtual void updateWindowsOptionsEnablement();
};

class InternalLibraryDetailsController : public LibraryDetailsController
{
    Q_OBJECT
public:
    explicit InternalLibraryDetailsController(Ui::LibraryDetailsWidget *libraryDetails,
                                              const QString &proFile,
                                              QObject *parent = 0);
    virtual bool isComplete() const;
    virtual QString snippet() const;
protected:
    virtual AddLibraryWizard::LinkageType suggestedLinkageType() const;
    virtual AddLibraryWizard::MacLibraryType suggestedMacLibraryType() const;
    virtual QString suggestedIncludePath() const;
    virtual void updateWindowsOptionsEnablement();
private slots:
    void slotCurrentLibraryChanged();
    void updateProFile();
private:
    QString m_rootProjectPath;
    QVector<Qt4ProFileNode *> m_proFileNodes;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // LIBRARYDETAILSCONTROLLER_H
