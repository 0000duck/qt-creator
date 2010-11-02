TEMPLATE = lib
TARGET = QmlJSEditor
include(../../qtcreatorplugin.pri)
include(qmljseditor_dependencies.pri)

DEFINES += \
    QMLJSEDITOR_LIBRARY \
    QT_CREATOR

HEADERS += \
    qmljscodecompletion.h \
    qmljseditor.h \
    qmljseditor_global.h \
    qmljseditoractionhandler.h \
    qmljseditorconstants.h \
    qmljseditorfactory.h \
    qmljseditorplugin.h \
    qmlexpressionundercursor.h \
    qmlfilewizard.h \
    qmljshighlighter.h \
    qmljshoverhandler.h \
    qmljsmodelmanager.h \
    qmljspreviewrunner.h \
    qmljsquickfix.h \
    qmljsrefactoringchanges.h \
    qmljscomponentfromobjectdef.h \
    qmljsoutline.h \
    qmloutlinemodel.h \
    qmltaskmanager.h \
    qmljseditorcodeformatter.h \
    qmljsoutlinetreeview.h \
    quicktoolbarsettingspage.h \
    quicktoolbar.h \
    qmljscomponentnamedialog.h \
    qmljsfindreferences.h \
    qmljseditoreditable.h \
    qmljssemantichighlighter.h

SOURCES += \
    qmljscodecompletion.cpp \
    qmljseditor.cpp \
    qmljseditoractionhandler.cpp \
    qmljseditorfactory.cpp \
    qmljseditorplugin.cpp \
    qmlexpressionundercursor.cpp \
    qmlfilewizard.cpp \
    qmljshighlighter.cpp \
    qmljshoverhandler.cpp \
    qmljsmodelmanager.cpp \
    qmljspreviewrunner.cpp \
    qmljsquickfix.cpp \
    qmljsrefactoringchanges.cpp \
    qmljscomponentfromobjectdef.cpp \
    qmljsoutline.cpp \
    qmloutlinemodel.cpp \
    qmltaskmanager.cpp \
    qmljsquickfixes.cpp \
    qmljseditorcodeformatter.cpp \
    qmljsoutlinetreeview.cpp \
    quicktoolbarsettingspage.cpp \
    quicktoolbar.cpp \
    qmljscomponentnamedialog.cpp \
    qmljsfindreferences.cpp \
    qmljseditoreditable.cpp \
    qmljssemantichighlighter.cpp

RESOURCES += qmljseditor.qrc
OTHER_FILES += QmlJSEditor.pluginspec QmlJSEditor.mimetypes.xml

FORMS += \
    quicktoolbarsettingspage.ui \
    qmljscomponentnamedialog.ui
