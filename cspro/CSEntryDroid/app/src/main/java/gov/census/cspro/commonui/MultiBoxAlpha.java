package gov.census.cspro.commonui;

import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.RadioButton;
import android.widget.RelativeLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.widget.ListPopupWindow;

import com.bumptech.glide.RequestManager;

import gov.census.cspro.csentry.R;
import gov.census.cspro.dict.ValuePair;
import gov.census.cspro.engine.Util;

import static gov.census.cspro.csentry.ui.QuestionWidget.getFileSignature;

public class MultiBoxAlpha extends RelativeLayout implements AlphaFieldEditText.TextChangedListener  {

    private MultiBoxAlpha.ValueChangedListener m_valueChangedListener  = null;
    private ValuePair[]	m_answerValues = null;
    private int m_selectedResponse = -1;
    private AlphaFieldEditText m_editText = null;
    private ListPopupWindow m_answerListView = null;
    private boolean m_isReadOnly = false;
    private RequestManager m_imageLoader = null;

    public MultiBoxAlpha(Context context)
    {
        super(context);
        init();
    }

    public MultiBoxAlpha(Context context, AttributeSet attrs)
    {
        super(context, attrs);
        init();
    }

    public MultiBoxAlpha(Context context, AttributeSet attrs, int defStyle)
    {
        super(context, attrs, defStyle);
        init();
    }

    public void setReadOnly(boolean readOnly) {
        m_editText.setReadOnly(readOnly);
        m_isReadOnly = readOnly;
    }

    public void setImageLoader(RequestManager imageLoader)
    {
        m_imageLoader = imageLoader;
    }

    public interface ValueChangedListener {

        void onEditTextChanged(String value);

        void onListItemSelected(int itemIndex);
    }

    public void setValueChangedListener(MultiBoxAlpha.ValueChangedListener listener)
    {
        m_valueChangedListener = listener;
    }

    @Override
    public void onTextChanged(String newText)
    {
        if (m_valueChangedListener != null) {
            m_valueChangedListener.onEditTextChanged(newText);
        }
        updateListViewSelectionToMatchEditText();
    }

    public void setResponses(ValuePair[] responses, boolean showCodes)
    {
        m_answerValues = responses;
        m_answerListView.setAdapter(new MultiBoxAlpha.ListViewAdapter(getContext(), responses, showCodes));
    }

    public void setAlphaValue(String value)
    {
        m_editText.setText(value);
    }

    public AlphaFieldEditText getEditText()
    {
        return m_editText;
    }

    public void init()
    {
        LayoutInflater inflater = LayoutInflater.from(getContext());
        inflater.inflate(R.layout.multibox_layout_alpha, this);

        // wire up the value change listening
        m_editText = findViewById(R.id.editText);

        m_editText.setTextChangedListener(this);

        m_answerListView = new ListPopupWindow(getContext());
        m_answerListView.setAnchorView(this);
        m_answerListView.setModal(true);
        m_answerListView.setBackgroundDrawable(getResources().getDrawable(R.drawable.multibox_listview_border));
        m_answerListView.setVerticalOffset(0);
        m_answerListView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> adapterView, View view, int position, long id) {
                m_editText.setText(m_answerValues[position].getCode());
                m_editText.setSelection(m_editText.getText().length());
                if (m_valueChangedListener != null) {
                    m_valueChangedListener.onListItemSelected(position);
                }
                m_answerListView.dismiss();
            }
        });


        // wire up button event handler
        final ImageButton displayButton	= findViewById(R.id.multibox_more_button);
        displayButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {

                if (!m_isReadOnly) {
                    if (m_answerListView.isShowing())
                        m_answerListView.dismiss();
                    else {
                        m_answerListView.show();
                    }
                }
            }
        });
    }

    private void updateListViewSelectionToMatchEditText()
    {
        String editText = m_editText.getText().toString();
        for (int i = 0; i < m_answerValues.length; ++i) {
            if (editText.equals(m_answerValues[i].getCode())) {
                m_selectedResponse = i;
                return;
            }
        }

        // Nothing found if we got here
        m_selectedResponse = -1;
    }

    private class ListViewAdapter extends ArrayAdapter<ValuePair> {

        private final boolean m_showCodes;

        ListViewAdapter(@NonNull Context context, @NonNull ValuePair[] m_responses, boolean showCodes) {
            super(context, R.layout.radio_button_answer_list_item, m_responses);
            m_showCodes = showCodes;
        }

        @NonNull
        @Override
        public View getView(int position, View convertView, @NonNull ViewGroup parent) {

            MultiBoxAlpha.ListViewAdapter.ViewHolder viewHolder;

            if (convertView == null) {
                viewHolder = new MultiBoxAlpha.ListViewAdapter.ViewHolder();
                LayoutInflater inflater = LayoutInflater.from(getContext());
                convertView = inflater.inflate(R.layout.radio_button_answer_list_item, parent, false);
                viewHolder.labelTextView = convertView.findViewById(R.id.valueLabel);
                viewHolder.codeTextView = convertView.findViewById(R.id.valueCode);
                viewHolder.radioButton = convertView.findViewById(R.id.radioButton);
                viewHolder.imageView = convertView.findViewById(R.id.valueSetImage);

                // Make the image half size of the usual list view image
                ViewGroup.LayoutParams layoutParams = viewHolder.imageView.getLayoutParams();
                layoutParams.height = layoutParams.height/2;
                viewHolder.imageView.setLayoutParams(layoutParams);

                convertView.setTag(viewHolder);

            } else {
                viewHolder = (MultiBoxAlpha.ListViewAdapter.ViewHolder) convertView.getTag();
            }

            ValuePair response = getItem(position);
            if (response != null) {

                viewHolder.labelTextView.setText(response.getLabel());
				viewHolder.labelTextView.setTextColor(response.getTextColor());

                String code = response.getCode();
                if (!m_showCodes || Util.stringIsNullOrEmpty(code)) {
                    viewHolder.codeTextView.setVisibility(View.GONE);
                } else {
                    viewHolder.codeTextView.setVisibility(View.VISIBLE);
                    viewHolder.codeTextView.setText(code);
                }

                if (response.isSelectable()) {
                    viewHolder.radioButton.setVisibility(View.VISIBLE);
                    viewHolder.radioButton.setChecked(m_selectedResponse == position);
                } else {
                    viewHolder.radioButton.setVisibility(View.GONE);
                }

                String imagePath = response.getImagePath();
                if (!Util.stringIsNullOrEmpty(imagePath)) {
                    viewHolder.imageView.setVisibility(View.VISIBLE);
                    m_imageLoader.load(imagePath)
                        .signature(getFileSignature(imagePath))
                        .fitCenter()
                        .into(viewHolder.imageView);
                } else {
                    m_imageLoader.clear(viewHolder.imageView);
                    viewHolder.imageView.setVisibility(View.GONE);
                }
            }
            return convertView;
        }

        @Override
        public boolean isEnabled(int position)
        {
            ValuePair response = getItem(position);
            return response != null && response.isSelectable();
        }

        private class ViewHolder {
            TextView labelTextView;
            TextView codeTextView;
            RadioButton radioButton;
            ImageView imageView;
        }
    }
}
