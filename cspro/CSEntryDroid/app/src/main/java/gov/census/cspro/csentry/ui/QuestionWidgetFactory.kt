package gov.census.cspro.csentry.ui

import androidx.recyclerview.widget.RecyclerView
import com.bumptech.glide.RequestManager
import gov.census.cspro.form.CDEField

/**
 * Manages mapping from fields to question widget types
 */
internal class QuestionWidgetFactory {
    /**
     * / **
     * Return QuestionWidget implementation to use to create UI for a particular
     * type of field based on capture type and data type.
     *
     * @param field Field to get QuestionWidget for
     * @param adapter RecyclerView adapter that the question will be displayed in
     * @param setFocus Whether or not this widget should get focus when first displayed
     * @param emitNextPage Whether or not this widget should call next page listener when
     * the user finishes entering data in it.
     * @param showCodes Whether or not to display codes along with labels
     * @return QuestionWidget corresponding to this type of field
     */
    fun createWidgetForField(field: CDEField,
                             adapter: RecyclerView.Adapter<QuestionWidgetViewHolder>,
                             imageLoader: RequestManager,
                             setFocus: Boolean,
                             emitNextPage: Boolean,
                             showCodes: Boolean): QuestionWidget {

        return when (field.captureType) {

            CDEField.DATA_CAPTURE_TEXT_BOX -> if (field.isNumeric) QuestionWidgetTextBoxNumeric(field, adapter, setFocus, emitNextPage, showCodes)
                else QuestionWidgetTextBoxAlpha(field, adapter, setFocus, emitNextPage, showCodes)

            CDEField.DATA_CAPTURE_RADIO_BUTTON ->  QuestionWidgetRadioButtons(field, adapter, imageLoader, setFocus, emitNextPage, showCodes)

            CDEField.DATA_CAPTURE_CHECK_BOX ->  QuestionWidgetCheckBoxes(field, adapter, imageLoader, setFocus, emitNextPage, showCodes)

            CDEField.DATA_CAPTURE_DROP_DOWN ->  QuestionWidgetDropDown(field, adapter, imageLoader, setFocus, emitNextPage, showCodes)

            CDEField.DATA_CAPTURE_COMBO_BOX -> if (field.isNumeric) QuestionWidgetComboBoxNumeric(field, adapter, imageLoader, setFocus, emitNextPage, showCodes)
             else QuestionWidgetComboBoxAlpha(field, adapter, imageLoader, setFocus, emitNextPage, showCodes)

            CDEField.DATA_CAPTURE_DATE ->  QuestionWidgetDate(field, adapter, setFocus, emitNextPage, showCodes)

            CDEField.DATA_CAPTURE_BARCODE -> if (field.isReadOnly) {
                // Protected barcodes are just text boxes
                if (field.isNumeric) QuestionWidgetTextBoxNumeric(field, adapter, setFocus, emitNextPage, showCodes)
                else QuestionWidgetTextBoxAlpha(field, adapter, setFocus, emitNextPage, showCodes)
            } else {
                if (field.isNumeric) QuestionWidgetBarcodeNumeric(field, adapter, setFocus, emitNextPage, showCodes)
                else QuestionWidgetBarcodeAlpha(field, adapter, setFocus, emitNextPage, showCodes)
            }

            CDEField.DATA_CAPTURE_SLIDER ->  QuestionWidgetSliderNumeric(field, adapter, setFocus, emitNextPage, showCodes)

            CDEField.DATA_CAPTURE_TOGGLE_BUTTON ->  QuestionWidgetToggleButton(field, adapter, imageLoader, setFocus, emitNextPage, showCodes)

            CDEField.DATA_CAPTURE_PHOTO -> QuestionWidgetPhoto(field, adapter, imageLoader, setFocus, emitNextPage, showCodes)
            
			CDEField.DATA_CAPTURE_SIGNATURE -> QuestionWidgetSignature(field, adapter, imageLoader, setFocus, emitNextPage, showCodes)
			
            CDEField.DATA_CAPTURE_AUDIO -> QuestionWidgetAudio(field, adapter, setFocus, emitNextPage, showCodes)

            else ->  throw AssertionError("No widget found that matches capture type: "
            + field.captureType)
        }
    }
}