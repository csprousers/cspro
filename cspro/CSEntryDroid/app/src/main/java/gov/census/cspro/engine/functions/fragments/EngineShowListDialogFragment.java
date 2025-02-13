package gov.census.cspro.engine.functions.fragments;

import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Typeface;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.fragment.app.DialogFragment;
import androidx.appcompat.widget.AppCompatRadioButton;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.SearchView;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import gov.census.cspro.csentry.R;
import gov.census.cspro.engine.Messenger;

/**
 * Dialog fragment to be used with ShowListFunction. Displays a table with radio buttons or checkboxes.
 */
public class EngineShowListDialogFragment extends DialogFragment implements SearchView.OnQueryTextListener, CompoundButton.OnCheckedChangeListener
{

    private static final String HEADERS = "HEADERS";
    private static final String ROW_TEXT_COLORS = "ROW_TEXT_COLORS";
    private static final String LINES = "LINES";
    private static final String HEADING_TEXT = "HEADING_TEXT";
    private static final String MULTIPLE_SELECTION = "MULTIPLE_SELECTION";
    private static final String SELECT_FUNCTION = "SELECT_FUNCTION";

    private String m_headingText;
    private String[] m_headers;
    private int[] m_rowTextColors;
    private String[] m_lines;
    private boolean m_multipleSelection;
    private boolean m_selectFunction;

    private TableLayout m_tableLayout;
    private final ArrayList<Integer> m_checkedIndices = new ArrayList<>();

    // according to Android's documentation the
    // hashmap is faster than the sparsearray in this case
    @SuppressLint("UseSparseArrays")
    private final HashMap<Integer, Integer> m_indexMap = new HashMap<>();

    public static @NonNull
    EngineShowListDialogFragment newInstance(String[] headers, int[] rowTextColors, String[] lines,
        String headingText, boolean multipleSelection, boolean selectFunction)
    {
        EngineShowListDialogFragment frag = new EngineShowListDialogFragment();
        Bundle args = new Bundle();
        args.putStringArray(HEADERS, headers);
        args.putIntArray(ROW_TEXT_COLORS, rowTextColors);
        args.putStringArray(LINES, lines);
        args.putString(HEADING_TEXT, headingText);
        args.putBoolean(MULTIPLE_SELECTION, multipleSelection);
        args.putBoolean(SELECT_FUNCTION, selectFunction);
        frag.setArguments(args);
        return frag;
    }

    @SuppressWarnings("ConstantConditions")
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        m_headers = getArguments().getStringArray(HEADERS);
        m_rowTextColors = getArguments().getIntArray(ROW_TEXT_COLORS);
        m_lines = getArguments().getStringArray(LINES);
        m_headingText = getArguments().getString(HEADING_TEXT);
        m_multipleSelection = getArguments().getBoolean(MULTIPLE_SELECTION);
        m_selectFunction = getArguments().getBoolean(SELECT_FUNCTION);
    }

    @Override
    public @NonNull
    Dialog onCreateDialog(Bundle savedInstanceState)
    {
        //noinspection ConstantConditions
        LayoutInflater inflator = getActivity().getLayoutInflater();
        @SuppressLint("InflateParams") View view = inflator.inflate(R.layout.activity_engine_showlist,null);

        final TextView headingView = view.findViewById(R.id.textview_heading_showlist);
        if (m_headingText.isEmpty() )
            headingView.setVisibility(View.GONE);
        else
            headingView.setText(m_headingText);

        m_tableLayout = view.findViewById(R.id.tableLayout_showlist);

        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        builder	.setView(view)
                   .setPositiveButton(getString(R.string.engine_showlist_select),new DialogInterface.OnClickListener() {
                       @Override
                       public void onClick(DialogInterface dialog, int which) {
                           endFunction(false);
                       }
                   });

        AlertDialog dialog	= builder.create();
        dialog.setCanceledOnTouchOutside(false);

        // wire up the search view with this class
        SearchView searchView = view.findViewById(R.id.searchview_list_items);
        searchView.setOnQueryTextListener(this);

        populateTable();

        return dialog;
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
    {
        if(buttonView instanceof CheckBox)
        {
            if(m_multipleSelection)
            {
                int index = checkBoxes.indexOf(buttonView);

                if(index != -1)
                {
                    Integer idx = index;

                    CheckBox cb = (CheckBox)buttonView;

                    if(cb.isChecked() && !m_checkedIndices.contains(idx))
                    {
                        m_checkedIndices.add(idx);
                    }
                    else if(!cb.isChecked())
                    {
                        m_checkedIndices.remove(idx);
                    }
                }
            }
        }
        else if(buttonView instanceof ShowListRadioButton)
        {
            ShowListRadioButton radioButton = (ShowListRadioButton)buttonView;
            Integer idx = radioButton.getIndex();
            Integer throwAway = m_indexMap.get(idx);


            if(isChecked && throwAway == null)
            {
                if(selectedRadioButton != null)
                {
                    // Integer prevSelectedIndex = Integer.valueOf(selectedRadioButton.getIndex());
                    // m_indexMap.remove(prevSelectedIndex);
                    selectedRadioButton.setChecked(false);
                }

                selectedRadioButton = radioButton;
                m_indexMap.put(idx, idx);
            }
            else if(!isChecked && throwAway != null)
            {
                selectedRadioButton = null;
                m_indexMap.remove(idx);
            }
        }
    }

    private class ShowListRadioButton extends AppCompatRadioButton
    {
        private int m_index;

        public ShowListRadioButton(Context context, int index)
        {
            super(context);
            m_index = index;

            setOnCheckedChangeListener(EngineShowListDialogFragment.this);
        }

        public int getIndex()
        {
            return m_index;
        }
    }

    private ShowListRadioButton selectedRadioButton;
    private ArrayList<CheckBox> checkBoxes = new ArrayList<>();

    private void addHeaderRow(TableRow.LayoutParams cellParams, TableRow.LayoutParams rowParams)
    {
        // add the headers
        TableRow headerRow = new TableRow(getContext());
        headerRow.setLayoutParams(rowParams);

        headerRow.addView(new TextView(getContext())); // the radio button / checkbox column

        boolean headerExists = false;

        for (String m_header : m_headers)
        {
            TextView textView = new TextView(getContext());
            textView.setText(m_header);
            textView.setLayoutParams(cellParams);
            textView.setTextAppearance(getContext(), android.R.style.TextAppearance_Medium);
            textView.setTypeface(null, Typeface.BOLD);
            headerRow.addView(textView);
            headerExists |= !m_header.isEmpty();
        }

        if( headerExists )
            m_tableLayout.addView(headerRow);
    }

    private int filterTable(String searchString)
    {
        m_tableLayout.removeAllViews();
        int 					numRows 		= m_lines.length / m_headers.length;
        int 					result 			= 0;
        Pattern searchPattern	= Pattern.compile(searchString, Pattern.CASE_INSENSITIVE | Pattern.LITERAL);

        TableRow.LayoutParams 	rowParams 		= new TableRow.LayoutParams(TableRow.LayoutParams.WRAP_CONTENT,TableRow.LayoutParams.WRAP_CONTENT);
        TableRow.LayoutParams 	cellParams 		= new TableRow.LayoutParams(TableRow.LayoutParams.WRAP_CONTENT,TableRow.LayoutParams.WRAP_CONTENT);
        cellParams.setMargins(0, 0, 20, 0);

        addHeaderRow(cellParams, rowParams);

        for( int i = 0; i < numRows; i++ )
        {
            // set the flag used to add rows to false
            boolean 	addrow 	= false;
            // get an object index of the row for checking against row selection
            Integer		index	= i;
            // create a new row to add to this data table
            TableRow 	dataRow = new TableRow(getContext());
            dataRow.setLayoutParams(rowParams);

            if (m_multipleSelection)
            {
                CheckBox checkBox;
                // we're going to reuse the checkboxes so
                // that only one copy exists per display
                if(i < checkBoxes.size())
                {
                    checkBox = checkBoxes.get(i);
                    // we need to remove the checkbox from the parent first
                    TableRow parentRow = (TableRow)checkBox.getParent();
                    parentRow.removeView(checkBox);
                }
                else
                {
                    // create a new checkbox object
                    checkBox = new CheckBox(getContext());
                    checkBox.setOnCheckedChangeListener(this);
                    // add the checkbox
                    checkBoxes.add(checkBox);
                }

                if(m_checkedIndices.contains(index))
                {
                    // set the check state to 'checked'
                    checkBox.setChecked(true);
                }
                else
                {
                    checkBox.setChecked(false);
                }

                dataRow.addView(checkBox);
            }
            else
            {
                ShowListRadioButton radioButton = new ShowListRadioButton(getContext(), i + 1);

                Integer radioIndex = i + 1;

                if(m_indexMap.containsKey(radioIndex))
                {
                    radioButton.setChecked(true);
                    // set the selected radio button
                    selectedRadioButton = radioButton;
                }

                dataRow.addView(radioButton);
            }

            for( int j = 0; j < m_headers.length; j++ )
            {
                // pull out the string for this cell
                String 		cellValue	= m_lines[i * m_headers.length + j];

                if(!addrow)
                {
                    // if this row has not been flagged to be added
                    // check the cell value to see if a match occurs
                    // check the cell value for a match against the search string
                    Matcher match	= searchPattern.matcher(cellValue);
                    addrow				= match.find();
                }

                // create a new text view for this table cell
                TextView 	textView 	= new TextView(getContext());
                textView.setText(cellValue);
                textView.setTextAppearance(getContext(),android.R.style.TextAppearance_Medium);
                if( m_rowTextColors != null )
                    textView.setTextColor(m_rowTextColors[i]);
                textView.setLayoutParams(cellParams);
                dataRow.addView(textView);
            }

            if(addrow)
            {	// if the row should be added do so
                m_tableLayout.addView(dataRow);
            }
            // else we're letting dataRow fall on the floor

        }

        return result;
    }

    private void populateTable()
    {
        m_tableLayout.removeAllViews();

        int 					numRows		= m_lines.length / m_headers.length;
        TableRow.LayoutParams	rowParams 	= new TableRow.LayoutParams(TableRow.LayoutParams.WRAP_CONTENT,TableRow.LayoutParams.WRAP_CONTENT);
        TableRow.LayoutParams 	cellParams 	= new TableRow.LayoutParams(TableRow.LayoutParams.WRAP_CONTENT,TableRow.LayoutParams.WRAP_CONTENT);
        cellParams.setMargins(0, 0, 20, 0);

        // add the headers
        addHeaderRow(cellParams, rowParams);

        for( int i = 0; i < numRows; i++ )
        {
            // get an object index of the row for checking against row selection
            Integer					index		= i;
            TableRow 				dataRow 	= new TableRow(getContext());
            dataRow.setLayoutParams(rowParams);

            if( m_multipleSelection )
            {
                CheckBox checkBox;
                // we're going to reuse the checkboxes so
                // that only one copy exists per display
                if(i < checkBoxes.size())
                {
                    checkBox 					= checkBoxes.get(i);
                    // we need to remove the checkbox from the parent first
                    TableRow parentRow 			= (TableRow)checkBox.getParent();
                    parentRow.removeView(checkBox);
                }
                else
                {
                    // create a new checkbox object
                    checkBox = new CheckBox(getContext());
                    checkBox.setOnCheckedChangeListener(this);
                    // add the checkbox
                    checkBoxes.add(checkBox);
                }

                if(m_checkedIndices.contains(index))
                {
                    // set the check state to 'checked'
                    checkBox.setChecked(true);
                }
                else
                {
                    checkBox.setChecked(false);
                }

                dataRow.addView(checkBox);
            }

            else
            {
                ShowListRadioButton radioButton = new ShowListRadioButton(getContext(), i + 1);

                Integer radioIndex = i+1;

                if(m_indexMap.containsKey(radioIndex))
                {
                    radioButton.setChecked(true);
                    // set the selected radio button
                    selectedRadioButton = radioButton;
                }

                dataRow.addView(radioButton);
            }

            for( int j = 0; j < m_headers.length; j++ )
            {
                TextView textView = new TextView(getContext());
                textView.setText(m_lines[i * m_headers.length + j]);
                textView.setTextAppearance(getContext(),android.R.style.TextAppearance_Medium);
                if( m_rowTextColors != null )
                    textView.setTextColor(m_rowTextColors[i]);
                textView.setLayoutParams(cellParams);
                dataRow.addView(textView);
            }

            m_tableLayout.addView(dataRow);
        }
    }

    @Override
    public void onCancel(DialogInterface dlg)
    {
        super.onCancel(dlg);
        endFunction(true);
    }

    private void endFunction(boolean canceled)
    {
        if (m_selectFunction)
        {
            String retVal = null;

            if( !canceled )
            {
                if( m_multipleSelection )
                {
                    StringBuilder sb = new StringBuilder();

                    for( int i = 0; i < checkBoxes.size(); i++ )
                    {
                        if( checkBoxes.get(i).isChecked() )
                            sb.append((char) (i + 1));
                    }

                    retVal = sb.toString();
                }

                else if( selectedRadioButton != null )
                {
                    char selectedValues[] = new char[1];
                    selectedValues[0] = (char)selectedRadioButton.getIndex();
                    retVal = new String(selectedValues);
                }

                else
                    retVal = "";
            }

            Messenger.getInstance().engineFunctionComplete(retVal);
        }

        else // show
        {
            long retVal = 0;

            if( !canceled && selectedRadioButton != null )
                retVal = (long)selectedRadioButton.getIndex();

            Messenger.getInstance().engineFunctionComplete(retVal);
        }
    }

    @Override
    public boolean onQueryTextSubmit(String s)
    {
        return true;
    }

    @Override
    public boolean onQueryTextChange(String newText)
    {
        // get the adapter from the listview
        boolean handled = false;

        if (newText == null || newText.length() == 0)
        {
            populateTable();
            handled	= true;
        }
        else
        {
            if(filterTable(newText) > 0)
            {
                handled = true;
            }
        }

        return handled;
    }
}
