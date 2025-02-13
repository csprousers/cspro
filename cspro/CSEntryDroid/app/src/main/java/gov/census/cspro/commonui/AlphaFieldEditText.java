package gov.census.cspro.commonui;

import android.content.Context;
import android.text.Editable;
import android.text.InputFilter;
import android.text.InputType;
import android.text.TextWatcher;
import android.util.AttributeSet;
import android.view.Gravity;
import android.widget.LinearLayout;

import gov.census.cspro.csentry.CSEntry;

/**
 * EditText view specialized for use with CSPro alpha text fields
 */

public class AlphaFieldEditText extends FieldEditText {

    private static final int        SINGLE_LINE_HEIGHT = 50;
    private static final float		MULTILINE_HEIGHT_SCALAR_TABLET	= 2.8f;
    private static final float		MULTILINE_HEIGHT_SCALAR_PHONE	= 1.8f;

    private TextChangedListener m_textChangedListener = null;

    public AlphaFieldEditText(Context context) {
        super(context);
        init();
    }

    public AlphaFieldEditText(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public AlphaFieldEditText(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    public interface TextChangedListener {
        void onTextChanged(String newText);
    }

    public void setTextChangedListener(TextChangedListener listener) {
        m_textChangedListener = listener;
    }

    public void setMaxLength(int length)
    {
        setFilters(new InputFilter[]{new InputFilter.LengthFilter(length)});
    }

    public void setMultiline(boolean multiline)
    {
        LinearLayout.LayoutParams layoutParams = (LinearLayout.LayoutParams) getLayoutParams();
        float height;
        if (multiline) {
            setInputType(getInputType() | InputType.TYPE_TEXT_FLAG_MULTI_LINE);
            setGravity(Gravity.TOP | Gravity.START);

            layoutParams.height = (int) (SINGLE_LINE_HEIGHT * getContext().getResources().getDisplayMetrics().density + 0.5f);
            height = SINGLE_LINE_HEIGHT * (CSEntry.Companion.isTablet() ? MULTILINE_HEIGHT_SCALAR_TABLET : MULTILINE_HEIGHT_SCALAR_PHONE);
        } else {
            setInputType(getInputType() & ~InputType.TYPE_TEXT_FLAG_MULTI_LINE);
            setGravity(Gravity.CENTER);
            height = SINGLE_LINE_HEIGHT;
        }
        layoutParams.height = (int) (0.5f + height * getContext().getResources().getDisplayMetrics().density);
        setLayoutParams(layoutParams);
    }

    public void setUppercase(boolean uppercase)
    {
        if (uppercase)
            setInputType(getInputType() | InputType.TYPE_TEXT_FLAG_CAP_CHARACTERS);
        else
            setInputType(getInputType() & ~InputType.TYPE_TEXT_FLAG_CAP_CHARACTERS);
    }

    private void init()
    {
        setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);

        addTextChangedListener(new TextWatcher() {

            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {
            }

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {
            }

            @Override
            public void afterTextChanged(Editable editable) {
                if (m_textChangedListener != null) {
                    m_textChangedListener.onTextChanged(editable.toString());
                }
            }
        });
    }
}
