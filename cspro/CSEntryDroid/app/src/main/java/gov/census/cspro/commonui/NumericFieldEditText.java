package gov.census.cspro.commonui;

import android.content.Context;
import android.text.Editable;
import android.text.InputFilter;
import android.text.InputType;
import android.text.TextWatcher;
import android.text.method.DigitsKeyListener;
import android.util.AttributeSet;

import java.util.Locale;

import gov.census.cspro.engine.Util;

/**
 * EditText view specialized for use with CSPro numeric text fields
 */

public class NumericFieldEditText extends FieldEditText {

    private ValueChangedListener m_valueChangedListener = null;
    private int m_fractionalPartLength = -1;

    public NumericFieldEditText(Context context) {
        super(context);
        init();
    }

    public NumericFieldEditText(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public NumericFieldEditText(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    void init() {
        setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_SIGNED);
        setKeyListener(DigitsKeyListener.getInstance(true, true));
        addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {
            }

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {
            }

            @Override
            public void afterTextChanged(Editable editable) {
                if (m_valueChangedListener != null) {
                    String textValue = editable.toString().trim();
                    if (Util.stringIsNullOrEmpty(textValue)) {
                        m_valueChangedListener.onValueBlank();
                    } else {
                        try {
                            m_valueChangedListener.onValueChanged(Double.parseDouble(textValue));
                        } catch (NumberFormatException ignored) {
                        }
                    }
                }
            }
        });
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

        if (m_fractionalPartLength > 0)
            setInputType(getInputType() | InputType.TYPE_NUMBER_FLAG_DECIMAL);
        else
            setInputType(getInputType() & ~InputType.TYPE_NUMBER_FLAG_DECIMAL);

        setFilters(new InputFilter[]{new DecimalValueFilter(integerPartLength, fractionalPartLength), new InputFilter.LengthFilter(totalLength)});
    }

    public interface ValueChangedListener {

        void onValueChanged(double value);

        void onValueBlank();
    }

    public void setValueChangedListener(ValueChangedListener listener)
    {
        m_valueChangedListener = listener;
    }
}
