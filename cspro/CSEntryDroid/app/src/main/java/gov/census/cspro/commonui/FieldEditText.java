package gov.census.cspro.commonui;

import android.content.Context;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.inputmethod.EditorInfo;
import android.widget.TextView;

import gov.census.cspro.engine.Util;

/**
 * EditText view specialized for use with CSPro text fields
 */

public class FieldEditText extends androidx.appcompat.widget.AppCompatEditText {

    public FieldEditText(Context context) {
        super(context);
    }

    public FieldEditText(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public FieldEditText(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public void setReadOnly(boolean readOnly)
    {
        setEnabled(!readOnly);
        setFocusable(!readOnly);
        setFocusableInTouchMode(!readOnly);
    }

    public void focus() {

        Util.setFocus(this);
    }

    public interface ImeNextListener {
        boolean onImeNext();
    }

    public void setImeNextListener(final ImeNextListener listener)
    {
        setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView textView, int actionId, KeyEvent keyEvent) {
                if(actionId == EditorInfo.IME_ACTION_NEXT) {
                    if (listener!=null && listener.onImeNext()) {
                        return true;
                    }
                }
                return false;
            }
        });
    }
}
