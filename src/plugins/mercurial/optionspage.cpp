#include "optionspage.h"
#include "mercurialsettings.h"
#include "mercurialplugin.h"

#include <utils/pathchooser.h>
#include <vcsbase/vcsbaseconstants.h>

using namespace Mercurial::Internal;
using namespace Mercurial;

OptionsPageWidget::OptionsPageWidget(QWidget *parent) :
        QWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.commandChooser->setExpectedKind(Core::Utils::PathChooser::Command);
    m_ui.commandChooser->setPromptDialogTitle(tr("Mercurial Command"));
}

void OptionsPageWidget::updateOptions()
{
    MercurialSettings *settings = MercurialPlugin::instance()->settings();
    m_ui.commandChooser->setPath(settings->application());
    m_ui.defaultUsernameLineEdit->setText(settings->userName());
    m_ui.defaultEmailLineEdit->setText(settings->email());
    m_ui.logEntriesCount->setValue(settings->logCount());
    m_ui.timeout->setValue(settings->timeoutSeconds());
    m_ui.promptOnSubmitCheckBox->setChecked(settings->prompt());
}

void OptionsPageWidget::saveOptions()
{
    MercurialSettings *settings = MercurialPlugin::instance()->settings();

    settings->writeSettings(m_ui.commandChooser->path(), m_ui.defaultUsernameLineEdit->text(),
                            m_ui.defaultEmailLineEdit->text(), m_ui.logEntriesCount->value(),
                            m_ui.timeout->value(), m_ui.promptOnSubmitCheckBox->isChecked());
}

OptionsPage::OptionsPage()
{
}

QString OptionsPage::id() const
{
    return QLatin1String("Mercurial");
}

QString OptionsPage::trName() const
{
    return tr("Mercurial");
}

QString OptionsPage::category() const
{
    return QLatin1String(VCSBase::Constants::VCS_SETTINGS_CATEGORY);
}

QString OptionsPage::trCategory() const
{
    return QCoreApplication::translate("VCSBase", VCSBase::Constants::VCS_SETTINGS_CATEGORY);
}

QWidget *OptionsPage::createPage(QWidget *parent)
{
    if (!optionsPageWidget)
        optionsPageWidget = new OptionsPageWidget(parent);
    optionsPageWidget.data()->updateOptions();
    return optionsPageWidget;
}

void OptionsPage::apply()
{
    if (!optionsPageWidget)
        return;
    optionsPageWidget.data()->saveOptions();
    //assume success and emit signal that settings are changed;
    emit settingsChanged();
}

