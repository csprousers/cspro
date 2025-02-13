package gov.census.cspro.form

import gov.census.cspro.dict.ValuePair

class CDEField(private val nativeInstance: Long,
               val name: String,
               val label: String,
               val captureType: Int,
               val questionTextUrl: String?,
               val helpTextUrl: String?,
               note: String,
               val isNumeric: Boolean,
               val isReadOnly: Boolean,
               val integerPartLength: Int,
               val fractionalPartLength: Int,
               val alphaLength: Int,
               captureDateFormat: String?,
               val isUpperCase: Boolean,
               val isMultiline: Boolean,
               val isMirror: Boolean,
               val maxCheckboxSelections: Int,
               val sliderMinValue: Double,
               val sliderMaxValue: Double,
               val sliderStep: Double,
               responses: Array<ValuePair>?,
               numericValue: Double?,
               alphaValue: String?,
               selectedIndex: Int?,
               checkedIndices: IntArray?) {

    val isAlpha = !isNumeric

    val captureDateFormat = captureDateFormat ?: ""

    val responses = responses ?: emptyArray()

    var numericValue: Double? = numericValue
        set(value) {
            field = value
            if (value == null)
                setBlankValue(nativeInstance)
            else
                setNumericValue(nativeInstance, value)
        }

    var alphaValue: String? = alphaValue
        set(value) {
            field = value
            setAlphaValue(nativeInstance, value ?: "")
        }

    var selectedIndex: Int = selectedIndex ?: -1
        set(value) {
            field = value
            if (value == -1)
                setBlankValue(nativeInstance)
            else
                setSelectedIndex(nativeInstance, value)
        }

    var checkedIndices: IntArray = checkedIndices ?: IntArray(0)
        set(value) {
            field = value
            setCheckedIndices(nativeInstance, value)
        }

    var note: String = note
        set(value) {
            field = value
            setNote(nativeInstance, value)
        }

    companion object {
        // field capture types
        const val DATA_CAPTURE_TEXT_BOX = 0
        const val DATA_CAPTURE_RADIO_BUTTON = 1
        const val DATA_CAPTURE_CHECK_BOX = 2
        const val DATA_CAPTURE_DROP_DOWN = 3
        const val DATA_CAPTURE_COMBO_BOX = 4
        const val DATA_CAPTURE_DATE = 5
        const val DATA_CAPTURE_BARCODE = 7
        const val DATA_CAPTURE_SLIDER = 8
        const val DATA_CAPTURE_TOGGLE_BUTTON = 9
        const val DATA_CAPTURE_PHOTO = 10
        const val DATA_CAPTURE_SIGNATURE = 11
        const val DATA_CAPTURE_AUDIO = 12
    }

    private external fun setNote(nativeInstance: Long, value: String)
    private external fun setBlankValue(nativeInstance: Long)
    private external fun setNumericValue(nativeInstance: Long, value: Double)
    private external fun setAlphaValue(nativeInstance: Long, value: String)
    private external fun setSelectedIndex(nativeInstance: Long, index: Int)
    private external fun setCheckedIndices(nativeInstance: Long, indices: IntArray)
}