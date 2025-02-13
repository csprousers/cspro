package gov.census.cspro.commonui;

import android.content.Context;
import android.util.AttributeSet;

import com.google.android.material.slider.Slider;

/**
 * Slider view specialized for use with CSPro Slider fields
 */

public class NumericSliderField extends Slider {

    public NumericSliderField(Context context) {
        super(context);
    }

    public NumericSliderField(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public NumericSliderField(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

/*
     commented focus as this caused halo always visible https://github.com/material-components/material-components-android/issues/1559
//    public void focus()
//    {
//        requestFocus();
//    }
*/

    public void setReadOnly(boolean readOnly)
    {
        setEnabled(!readOnly);
        setFocusable(!readOnly);
        setFocusableInTouchMode(!readOnly);
    }
}
