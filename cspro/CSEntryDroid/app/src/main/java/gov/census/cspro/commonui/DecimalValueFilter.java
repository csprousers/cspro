package gov.census.cspro.commonui;

import android.text.InputFilter;
import android.text.Spanned;

public class DecimalValueFilter implements InputFilter {
    private int m_beforeDecimal = 5;
    private int m_afterDecimal = 2;

    public DecimalValueFilter(int digitsBefore, int digitsAfter) {
        m_beforeDecimal = digitsBefore;
        m_afterDecimal = digitsAfter;
    }

    @Override
    public CharSequence filter(CharSequence source, int start, int end, Spanned dest, int dstart, int dend) {
        CharSequence ret = null;
        StringBuilder newstring = new StringBuilder(dest.toString());


        if (dend - dstart > 0) {
            // replacing a range of characters
            newstring.delete(dstart, dend);
        }

        {
            // insertion
            // replace starts at index
            int index = dstart;
            for (int i = start; i < end; i++) {
                char c = source.charAt(i);
                // insert this into the string
                newstring.insert(index, c);
                index++;
            }
        }

        String test = newstring.toString();

        if (test.indexOf(".") == -1) {

            // no decimal point placed yet
            if (test.length() > m_beforeDecimal) {
                ret = "";

            }
        } else {
            // let's perform a numeric check
            // split the string on the decimal
            // into the integer and fractional parts
            int index = test.indexOf('.');
            String intpart = test.substring(0, index);
            String fractional = test.substring(index + 1, test.length());

            if (intpart.length() <= m_beforeDecimal && fractional.length() <= m_afterDecimal) {
                // the integer and fractional digit counts, fit in the
                // specified ranges
                if (source.equals(".")) {
                    // only a period was entered, accept the input
                    ; // do nothing ret = null;
                }
                // don't alter the string
                ret = null;
            } else {
                // the integer and/or fractional parts are outside the digit spec
                // don't allow this string to be entered
                ret = "";
            }
        }
        return ret;
    }

    public void setDigitsAfter(int digitsAfter) {
        m_afterDecimal = digitsAfter;
    }

    public void setDigitsBefore(int digitsBefore) {
        m_beforeDecimal = digitsBefore;
    }
}
