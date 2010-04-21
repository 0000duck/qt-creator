import Qt 4.7
import Bauhaus 1.0

QGroupBox {
    id: aligmentHorizontalButtons
    layout: HorizontalLayout {
        topMargin: 6

        QWidget {
            fixedHeight: 32

            QPushButton {
                id: leftButton
                checkable: true
                fixedWidth: 32
                width: fixedWidth
                fixedHeight: 32
                height: fixedHeight
                styleSheetFile: "alignmentleftbutton.css";
                checked: backendValues.horizontalAlignment.value == "AlignLeft"

                onClicked: {
                    backendValues.horizontalAlignment.value = "AlignLeft";
                    checked = true;
                    centerButton.checked =  false;
                    rightButton.checked = false;
                }

            }
            QPushButton {
                id: centerButton
                x: 32
                checkable: true
                fixedWidth: 32
                width: fixedWidth
                fixedHeight: 32
                height: fixedHeight

                styleSheetFile: "alignmentcenterhbutton.css";
                checked: backendValues.horizontalAlignment.value == "AlignHCenter"

                onClicked: {
                    backendValues.horizontalAlignment.value = "AlignHCenter";
                    checked = true;
                    leftButton.checked =  false;
                    rightButton.checked = false;
                }

            }
            QPushButton {
                id: rightButton
                x: 64
                checkable: true
                fixedWidth: 32
                width: fixedWidth
                fixedHeight: 32
                height: fixedHeight

                styleSheetFile: "alignmentrightbutton.css";
                checked: backendValues.horizontalAlignment.value == "AlignRight"

                onClicked: {
                    backendValues.horizontalAlignment.value = "AlignRight";
                    checked = true;
                    centerButton.checked =  false;
                    leftButton.checked = false;
                }
            }
        }
    }
}
