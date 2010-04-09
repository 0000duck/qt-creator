import Qt 4.6
import Bauhaus 1.0

QWidget {
    id: lineEdit

    property var backendValue
    property alias enabled: lineEdit.enabled
    property var baseStateFlag
    property alias text: lineEditWidget.text
    property alias readOnly: lineEditWidget.readOnly

    minimumHeight: 24;

    onBaseStateFlagChanged: {
        evaluate();
    }

    property var isEnabled: lineEdit.enabled
    onIsEnabledChanged: {
        evaluate();
    }

    function evaluate() {
        if (!enabled) {
            lineEditWidget.setStyleSheet("color: "+scheme.disabledColor);
        } else {
            if (baseStateFlag) {
                if (backendValue != null && backendValue.isInModel)
                    lineEditWidget.setStyleSheet("color: "+scheme.changedBaseColor);
                else
                    lineEditWidget.setStyleSheet("color: "+scheme.defaultColor);
            } else {
                if (backendValue != null && backendValue.isInSubState)
                    lineEditWidget.setStyleSheet("color: "+scheme.changedStateColor);
                else
                    lineEditWidget.setStyleSheet("color: "+scheme.defaultColor);
            }
        }
    }

    ColorScheme { id:scheme; }

    QLineEdit {
        id: lineEditWidget
        styleSheet: "QLineEdit { padding-left: 32; }"
        width: lineEdit.width
        height: lineEdit.height

        text: (backendValue === undefined || backendValue.value === undefined) ? "" : backendValue.value

        onEditingFinished: {
            backendValue.value = text
            evaluate();
        }

        onFocusChanged: {
            if (focus)
            backendValue.lock();
            else
            backendValue.unlock();
        }


    }
    ExtendedFunctionButton {
        backendValue: lineEdit.backendValue
        y: 4
        x: 0
        visible: lineEdit.enabled
    }
}
