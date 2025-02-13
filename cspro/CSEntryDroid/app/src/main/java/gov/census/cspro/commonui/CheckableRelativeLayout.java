package gov.census.cspro.commonui;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Checkable;
import android.widget.RadioButton;
import android.widget.RelativeLayout;
import android.widget.AdapterView.OnItemClickListener;

public class CheckableRelativeLayout extends RelativeLayout implements Checkable, OnClickListener
{
	private int					m_position			= -1;
	private OnItemClickListener	m_itemClickListener = null;
	private RadioButton 		m_radioButton 		= null;
	
	
	public CheckableRelativeLayout(Context context) 
	{
		super(context);
	}

	public CheckableRelativeLayout(Context context, AttributeSet attrs) 
	{
		super(context, attrs);
	}

	public CheckableRelativeLayout(Context context, AttributeSet attrs, int defStyle) 
	{
		super(context, attrs, defStyle);
	}
	
	@Override
    protected void onFinishInflate() 
	{
    	super.onFinishInflate();
    	// find checked text view
		int childCount = getChildCount();
		for (int i = 0; i < childCount; ++i) 
		{
			View v = getChildAt(i);
			
			if (v instanceof RadioButton) 
			{
				m_radioButton = (RadioButton)v;
				m_radioButton.setOnClickListener(this);
			}
		}    	
    }
	
	public int getPosition()
	{
		return m_position;
	}
	
	public void setPosition(int position)
	{
		m_position = position;
	}
    
	@Override
	public boolean isChecked() 
	{
		boolean result = false;
		
		if(m_radioButton != null)
		{
			result = m_radioButton.isChecked();
		}
		
		return result;
	}
	
	@Override
	public void onClick(View v) 
	{
		if(m_itemClickListener != null)
		{
			m_itemClickListener.onItemClick(null, this, m_position, -1);
		}
	}
	
	public void setOnItemClickListener(OnItemClickListener listener)
	{
		m_itemClickListener = listener;
	}
	
	@Override
	public void setChecked(boolean checked) 
	{
		if(m_radioButton != null)
		{
			m_radioButton.setChecked(checked);
		}
	}

	@Override
	public void toggle() 
	{
		if(m_radioButton != null)
		{
			m_radioButton.toggle();
		}
	}
}
