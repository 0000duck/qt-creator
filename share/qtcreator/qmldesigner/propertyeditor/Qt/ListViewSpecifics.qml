import Qt 4.6
import Bauhaus 1.0

QWidget {
    layout: QVBoxLayout {
        topMargin: 0
        bottomMargin: 0
        leftMargin: 0
        rightMargin: 0
        spacing: 0
        GroupBox {


            GroupBox {
                finished: finishedNotify;
                caption: qsTr("List View")

                layout: QVBoxLayout {
                    topMargin: 15;
                    bottomMargin: 6;
                    leftMargin: 0;
                    rightMargin: 0;

                    QWidget {
                        id: contentWidget;
                        maximumHeight: 260;

                        layout: QHBoxLayout {
                            topMargin: 0;
                            bottomMargin: 0;
                            leftMargin: 10;
                            rightMargin: 10;

                            QWidget {
                                layout: QVBoxLayout {
                                    topMargin: 0;
                                    bottomMargin: 0;
                                    leftMargin: 0;
                                    rightMargin: 0;
                                    QLabel {
                                        minimumHeight: 22;
                                        text: "Highlight:"
                                        font.bold: true;
                                    }

                                    QLabel {
                                        minimumHeight: 22;
                                        text: "Spacing:"
                                        font.bold: true;
                                    }
                                }
                            }

                            QWidget {
                                layout: QVBoxLayout {
                                    topMargin: 0;
                                    bottomMargin: 0;
                                    leftMargin: 0;
                                    rightMargin: 0;

                                    CheckBox {
                                        id: highlightFollowsCurrentItemCheckBox;
                                        text: "Follows";
                                        backendValue: backendValues.highlightFollowsCurrentItem;
                                        baseStateFlag: isBaseState;
                                        checkable: true;
                                    }

                                    SpinBox {
                                        id: spacingSpinBox;
                                        objectName: "spacingSpinBox";
                                        backendValue: backendValues.spacing;
                                        minimumWidth: 30;
                                        minimum: 0;
                                        maximum: 1000;
                                        singleStep: 1;
                                        baseStateFlag: isBaseState;
                                    }

                                }
                            }
                        }
                    }
                }
            }

        }
    }
