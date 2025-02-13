/**
 * 
 */
package gov.census.cspro.commonui;

import java.util.ArrayList;

import android.content.Context;
import android.text.InputFilter;
import android.text.InputType;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.widget.EditText;

// Project Imports
import gov.census.cspro.commonui.IInputControlValueChangedListener;
import gov.census.cspro.form.CDEField;

/**
 * @author wmapp
 *
 */
public class EditTextExt extends EditText implements IGenericInputControl
{
	protected int m_maxValue = Integer.MAX_VALUE;
    protected int m_minValue = Integer.MIN_VALUE;

    private ArrayList<IInputControlValueChangedListener> m_valueChangedListeners = null;
        
	public EditTextExt(Context context) 
	{
		super(context);	
		
		init();
	}
	
	 public EditTextExt(Context context, AttributeSet attrs)
	 {
		 super(context, attrs);		 		 

		 init();
	 }

	 public EditTextExt(Context context, AttributeSet attrs, int defStyle)
	 {
	     super(context, attrs, defStyle);	     
	     
		init();
	 }
	 
	 public void init()
	 {
		m_valueChangedListeners = new ArrayList<IInputControlValueChangedListener>();
	 }
	 
	 @Override
	 public boolean onKeyPreIme(int keyCode, KeyEvent event)
	 {
		 if(keyCode == KeyEvent.KEYCODE_BACK)
	     {
			 clearFocus();
	     }
		 return super.onKeyPreIme(keyCode, event);	
	 }

	// checks whether the limits are set and corrects them if not within limits
    @Override
    protected void onTextChanged(CharSequence text, int start, int before, int after) 
    {
    	if(this.getInputType() == InputType.TYPE_CLASS_NUMBER)
    	{
    		if (m_maxValue != Integer.MAX_VALUE) 
    		{
    			try 
    			{
    				if (Integer.parseInt(this.getText().toString()) > m_maxValue) 
    				{
    					// change value and keep cursor position
    					int selection = this.getSelectionStart();
    					this.setText(String.valueOf(m_maxValue));
    					
    					if (selection >= this.getText().toString().length()) 
    					{
    						selection = this.getText().toString().length();
    					}
    					this.setSelection(selection);
    				}
    			} 
    			catch (NumberFormatException exception) 
    			{
    				super.onTextChanged(text, start, before, after);
    			}
    		}
        
    		if (m_minValue != Integer.MIN_VALUE) 
    		{
    			try 
    			{
    				if (Integer.parseInt(this.getText().toString()) < m_minValue) 
    				{
    					// change value and keep cursor position
    					int selection = this.getSelectionStart();
    					this.setText(String.valueOf(m_minValue));
    					if (selection >= this.getText().toString().length()) 
    					{
    						selection = this.getText().toString().length();
    					}
    					this.setSelection(selection);
    				}
    			} 
    			catch (NumberFormatException exception) 
    			{
    				super.onTextChanged(text, start, before, after);
    			}
    		}
    	}
    	
        super.onTextChanged(text, start, before, after);
        
        // alert listeners
        // we need to check for null here because events will fire
        // during this view construction, because an empty
        // string = a text change
        notifyChangeListeners(text.toString());
    }

    // set the max number of digits the user can enter
    public void setMaxLength(int length) 
    {
        InputFilter[] 	FilterArray = new InputFilter[1];
        FilterArray[0] 				= new InputFilter.LengthFilter(length);
        this.setFilters(FilterArray);
    }

    // set the maximum integer value the user can enter.
    // if exeeded, input value will become equal to the set limit
    public void setMaxValue(int value) {
        m_maxValue = value;
    }
    // set the minimum integer value the user can enter.
    // if entered value is inferior, input value will become equal to the set limit
    public void setMinValue(int value)
    {
        m_minValue = value;
    }

    // returns integer value or 0 if errorous value
    public int getValue() throws Exception
    {
    	int result = -1;
    	
    	if(getInputType() == InputType.TYPE_CLASS_NUMBER)
    	{
    		try 
    		{
    			result = Integer.parseInt(this.getText().toString());
    		} 
    		catch (NumberFormatException nfe) 
    		{
    			throw new Exception("An Error Occurred While Formatting Value: " + getText(), nfe);
    		}
    	}
        
        return result;
    }

    public void addValueChangedListener(IInputControlValueChangedListener listener)
    {
    	if(!m_valueChangedListeners.contains(listener))
    	{
        	m_valueChangedListeners.add(listener);    		
    	}
    }
    
    public void notifyChangeListeners(Object value)
    {
        if(!isInEditMode() && m_valueChangedListeners != null)
        {
        	for(IInputControlValueChangedListener listener : m_valueChangedListeners)
        	{
        		listener.onValueChanged(this, value);
        	}
        }
    }
    
    public void removeValueChangedListener(IInputControlValueChangedListener listener)
    {
    	m_valueChangedListeners.remove(listener);
    }

	@Override
	public CDEField getEditField() 
	{
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public void setEditField(CDEField editField)
	{
	}
}
