/***************************************************************************************
 * 
 * CSEntry for Android
 * 
 * Module:		SegmentControl.java
 * 
 * Description: A custom control designed to mimic the behavior of an iOS
 * 				UISegmentControl.  This control uses child SegmentControlItem classes
 * 				for presentation.
 * 
 **************************************************************************************/
package gov.census.cspro.commonui;

// Java imports
import android.content.Context;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;

// Android Imports

// Project Imports

public class SegmentControl extends LinearLayout implements android.view.View.OnClickListener
{
	private int							m_selectedIndex				= -1;
	private OnSegmentClickedListener	m_segmentClickedListener	= null;
	private SegmentControlItem			m_currentlySelectedItem		= null;
	
	public SegmentControl(Context context) 
	{
		super(context);
	}

	public SegmentControl(Context context, AttributeSet attrs) 
	{
		super(context, attrs);
	}

	public SegmentControl(Context context, AttributeSet attrs, int defStyle) 
	{
		super(context, attrs, defStyle);
	}
	
	public void addSegment(String title)
	{
		LayoutInflater 				inflater 		= (LayoutInflater)getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		SegmentControlItem			item			= (SegmentControlItem)inflater.inflate(gov.census.cspro.csentry.R.layout.segment_control_item, null);
		LinearLayout.LayoutParams	layoutParams	= new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT);
		// set the segment item's gravity and weight specifics
		layoutParams.gravity 						= Gravity.FILL;
		layoutParams.weight							= 1;
		// setup the item and add
		item.setText(title);
		item.setOnClickListener(this);
		
		// we need to setup the background, the segment has exterior and interior segments
		if(getChildCount() == 0)
		{
			// the first one, this should be on the left side
			item.setPosition(SegmentControlItem.LEFT_POSITION);
		}
		else if(getChildCount() > 1)
		{
			// there's an item to the rear of us, we need to set the previous item
			// to an interior segment
			SegmentControlItem sibling = (SegmentControlItem)getChildAt(getChildCount() - 1);
			sibling.setPosition(SegmentControlItem.INTERIOR_POSITION);
			item.setPosition(SegmentControlItem.RIGHT_POSITION);
		}
		else
		{
			// set this item to the end
			item.setPosition(SegmentControlItem.RIGHT_POSITION);
		}
		
		addView(item, layoutParams);
	}
	
	public SegmentControlItem findSegmentWithTitle(String title)
	{
		SegmentControlItem result = null;
		
		for(int i = 0; i < getChildCount(); i++)
		{
			View v = getChildAt(i);
			
			if(v instanceof SegmentControlItem)
			{
				SegmentControlItem item = (SegmentControlItem)v;
				
				if(item.getText().equals(title))
				{
					result = item;
					break;
				}
			}
		}

		return result;
	}
	
	public SegmentControlItem getSegmentAtIndex(int index)
	{
		return (SegmentControlItem)getChildAt(index);
	}
	
	public int getSelectedIndex()
	{
		return m_selectedIndex;
	}
	
	public SegmentControlItem getSelectedItem()
	{
		return m_currentlySelectedItem;
	}
	
	public void removeSegment(String title)
	{
		int segindex = -1;
		SegmentControlItem item = null;
		
		for(int i = 0; i < getChildCount(); i++)
		{
			View v = getChildAt(i);
			
			if(v instanceof SegmentControlItem)
			{
				item = (SegmentControlItem)v;
				
				if(item.getText().equals(title))
				{
					segindex = i;
					break;
				}
			}
		}
		
		if(segindex != -1 && item != null)
		{
			item.setOnClickListener(null);
			removeViewAt(segindex);
		}
	}
	
	public void setOnSegmentClickedListener(OnSegmentClickedListener listener)
	{
		m_segmentClickedListener = listener;
	}
	
	public void selectSegmentWithTitle(String title)
	{
		int 				segindex	= -1;
		SegmentControlItem	item 		= null;
		
		for(int i = 0; i < getChildCount(); i++)
		{
			View v = getChildAt(i);
			
			if(v instanceof SegmentControlItem)
			{
				item = (SegmentControlItem)v;
				
				if(item.getText().equals(title))
				{
					segindex = i;
					break;
				}
			}
		}
		
		if(segindex != -1 && item != null)
		{
			setSelectedSegmentIndex(segindex);
		}
	}
	
	public void setSelectedSegmentIndex(int index)
	{
		if(m_currentlySelectedItem != null)
		{
			m_currentlySelectedItem.setSelected(false);
		}
		
		for(int i = 0; i < getChildCount(); i++)
		{
			SegmentControlItem item = (SegmentControlItem)getChildAt(i);
			
			if(i == index)
			{
				item.setSelected(true);
				m_currentlySelectedItem = item;
				m_selectedIndex			= i;
				break;
			}
		}
		
		// notify listeners of this event
		if(m_segmentClickedListener != null)
		{
			m_segmentClickedListener.onClick(m_currentlySelectedItem);
		}
	}

	// onClickListener Implementation
	@Override
	public void onClick(View v) 
	{
		if(v instanceof SegmentControlItem)
		{
			SegmentControlItem item = (SegmentControlItem)v;
			
			// deselect the current item
			if(m_currentlySelectedItem != null && !m_currentlySelectedItem.equals(item))
			{
				m_currentlySelectedItem.setSelected(false);
				
				// select the current item
				item.setSelected(true);
				// set the currently selected item
				m_currentlySelectedItem = item;
			}
			else if(m_currentlySelectedItem != null)
			{
				// do nothing, it's the same selected item
			}
			else 
			{
				// select the current item
				item.setSelected(true);
				// set the currently selected item
				m_currentlySelectedItem = item;
			}
			
			// we need to compute the index
			for(int i = 0; i < getChildCount(); i++)
			{
				SegmentControlItem t = (SegmentControlItem)getChildAt(i);
				
				if(item.equals(t))
				{
					m_selectedIndex			= i;
					break;
				}
			}

			// notify listeners of this event
			if(m_segmentClickedListener != null)
			{
				m_segmentClickedListener.onClick(m_currentlySelectedItem);
			}
		}
	}
	
	public interface OnSegmentClickedListener
	{
		void onClick(SegmentControlItem item);
	}
}
