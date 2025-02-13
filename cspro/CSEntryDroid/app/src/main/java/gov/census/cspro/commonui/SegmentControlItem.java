/***************************************************************************************
 * 
 * CSEntry for Android
 * 
 * Module:		SegmentControlItem.java
 * 
 * Description: The child presentation class for SegmentControl.
 * 				This class handles selection and text for the control.
 * 
 **************************************************************************************/
package gov.census.cspro.commonui;

// Java Imports

// Android Imports
import android.content.Context;
import android.util.AttributeSet;
import android.widget.RelativeLayout;
import android.widget.TextView;

// Project Imports
import gov.census.cspro.csentry.R;

public class SegmentControlItem extends RelativeLayout 
{
	// segment positioning
	static final	int		LEFT_POSITION		= 0;
	static final	int		INTERIOR_POSITION	= 1;
	static final	int		RIGHT_POSITION		= 2;

	private boolean 		m_selected			= false;
	private int				m_segmentPosition	= 0;
	
	public SegmentControlItem(Context context) 
	{
		super(context);
	}

	public SegmentControlItem(Context context, AttributeSet attrs) 
	{
		super(context, attrs);
	}

	public SegmentControlItem(Context context, AttributeSet attrs, int defStyle) 
	{
		super(context, attrs, defStyle);
	}
	
	public CharSequence getText()
	{
		CharSequence result = null;
		TextView tv = (TextView)findViewById(gov.census.cspro.csentry.R.id.textview_segment_control_item);
		result = tv.getText();
		
		return result;
	}
	
	public boolean isSelected()
	{
		return m_selected;
	}
	
	public void setText(CharSequence text)
	{
		TextView tv = (TextView)findViewById(gov.census.cspro.csentry.R.id.textview_segment_control_item);
		tv.setText(text);
		// TODO: WLM3 - Add a set text alignmnet for API 14
		// tv.setTextAlignment(TEXT_ALIGNMENT_CENTER);
	}
	
	void setPosition(int segmentPosition)
	{
		m_segmentPosition = segmentPosition;
		updateSegmentBackground();
	}
	
	public void setSelected(boolean selected)
	{		
		m_selected = selected;
		updateSegmentBackground();
	}
	
	private void updateSegmentBackground()
	{
		int backgroundResource = -1;
		
		if(m_selected)
		{
			if(m_segmentPosition == LEFT_POSITION)
			{
				backgroundResource = R.drawable.segment_control_item_selected_border_left;
			}
			else if(m_segmentPosition == INTERIOR_POSITION)
			{
				backgroundResource = R.drawable.segment_control_item_selected_border_interior;
			}
			else if(m_segmentPosition == RIGHT_POSITION)
			{
				backgroundResource = R.drawable.segment_control_item_selected_border_right;
			}
		}
		else
		{
			if(m_segmentPosition == LEFT_POSITION)
			{
				backgroundResource = R.drawable.segment_control_item_nonselected_border_left;
			}
			else if(m_segmentPosition == INTERIOR_POSITION)
			{
				backgroundResource = R.drawable.segment_control_item_nonselected_border_interior;
			}
			else if(m_segmentPosition == RIGHT_POSITION)
			{
				backgroundResource = R.drawable.segment_control_item_nonselected_border_right;
			}
		}
		
		setBackgroundResource(backgroundResource);
	}
}
