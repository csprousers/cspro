package gov.census.cspro.commonui;

import android.content.Context;
import android.text.InputFilter;
import android.text.InputType;
import android.util.AttributeSet;

import java.util.Locale;

import gov.census.cspro.engine.Util;

/**
 * Text view specialized for use with CSPro numeric text fields
 */

public class NumericFieldTextView extends androidx.appcompat.widget.AppCompatTextView {

    private int m_fractionalPartLength = -1;

    public NumericFieldTextView(Context context) {
        super(context);
    }

    public NumericFieldTextView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public NumericFieldTextView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public void focus() {

        Util.setFocus(this);
    }

    public void setNumericValue(double value)
    {
        if (m_fractionalPartLength >= 0) {
            String formatSpec = "%." + m_fractionalPartLength + "f";
            setText(String.format(Locale.US, formatSpec, value));
        } else {
            setText(String.format(Locale.US,"%f", value));
        }
    }

    public void setFormat(int integerPartLength, int fractionalPartLength)
    {
        m_fractionalPartLength = fractionalPartLength;
        final int totalLength = integerPartLength + (m_fractionalPartLength > 0 ? m_fractionalPartLength + 1 : 0);
        setFilters(new InputFilter[]{new DecimalValueFilter(integerPartLength, fractionalPartLength), new InputFilter.LengthFilter(totalLength)});
    }

}
