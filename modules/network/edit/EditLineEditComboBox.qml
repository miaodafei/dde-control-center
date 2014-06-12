import QtQuick 2.1
import Deepin.Widgets 1.0

BaseEditLine {
    id: root
    
    rightLoader.sourceComponent: DEditComboBox {
        id: comboBox
        activeFocusOnTab: true
        width: valueWidth
        state: root.showError ? "warning" : "normal"
        anchors.left: parent.left
        selectIndex: -1
        parentWindow: rootWindow

        Connections {
            target: root
            onWidgetShown: {
                text = root.cacheValue
                labels = root.getAvailableValuesText()
                values = root.getAvailableValuesValue()
                selectIndex = root.getAvailableValuesIndex()
            }
            onCacheValueChanged: {
                text = root.cacheValue
                labels = root.getAvailableValuesText()
                values = root.getAvailableValuesValue()
                selectIndex = root.getAvailableValuesIndex()
            }
        }

        onTextChanged: {
            setKey(text)
        }
    }
}
