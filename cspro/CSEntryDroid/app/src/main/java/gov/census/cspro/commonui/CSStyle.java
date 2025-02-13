/***************************************************************************************
 * 
 * CSEntry for Android
 * 
 * Module:		CSStyle.java
 * 
 * Description: This is the style class used for setting case tree and other
 * 				UI styles.
 * 
 **************************************************************************************/
package gov.census.cspro.commonui;

// Java Imports
import gov.census.cspro.csentry.CSEntry;

import java.io.InputStream;
import java.util.ArrayList;

// Android Imports
import android.content.Context;
import android.content.res.XmlResourceParser;
import android.util.Log;
import android.util.Xml;

// Project Imports
import org.xmlpull.v1.XmlPullParser;

public class CSStyle 
{
	private static final String ITEM_TAG					= "item";
    private static final String STYLE_TAG 					= "style";
    private static final String DESC_ATTR					= "desc";
    private static final String DIVIDER_COLOR_ATTR 			= "dividerColor";
    private static final String GROUP_BACKGROUND_COLOR_ATTR = "groupBackgroundColor";
    private static final String HEADER_TEXT_COLOR_ATTR 		= "headerTextColor";
    private static final String ITEM_BACKGROUND_COLOR_ATTR 	= "itemBackgroundColor";
    private static final String ITEM_VISITED_BACKGROUND_COLOR_ATTR 	= "itemVisitedBackgroundColor";
    private static final String ITEM_SKIPPED_BACKGROUND_COLOR_ATTR 	= "itemSkippedBackgroundColor";
    private static final String ITEM_CURRENT_BACKGROUND_COLOR_ATTR 	= "itemNeverVisitedBackgroundColor";
    private static final String ITEM_NEVERVISITED_BACKGROUND_COLOR_ATTR = "itemCurrentBackgroundColor";
    private static final String ITEM_PROTECTED_BACKGROUND_COLOR_ATTR = "itemProtectedBackgroundColor";
    private static final String ITEM_LABEL_TEXT_COLOR_ATTR 	= "itemLabelTextColor";
    private static final String ITEM_VALUE_TEXT_COLOR_ATTR 	= "itemValueTextColor";
    private static final String LIST_BACKGROUND_COLOR_ATTR 	= "listBackgroundColor";
    private static final String NAME_ATTR					= "name";
    private static final String ROW_COUNTER_TEXT_COLOR 		= "rowCounterTextColor";
    private static final String SELECTION_COLOR_ATTR 		= "selectionColor";
    
    private static ArrayList<CSStyle> m_styles 				= new ArrayList<CSStyle>();
    private static CSStyle		DEFAULT_STYLE				= null;
    
    private int	 	m_dividerColor 			= 0xFF423E34;
    private int 	m_groupBackgroundColor 	= 0xFFEFEFEF;
    private int 	m_headerTextColor 		= 0xFF2C2F2D;
    private int 	m_itemBackgroundColor 	= 0xFFEFEFEF;
    private int 	m_itemVisitedBackgroundColor 	= 0xFFD3D3D3;
    private int 	m_itemSkippedBackgroundColor 	= 0xFF696969;
    private int 	m_itemCurrentBackgroundColor 	= 0xFFEFEFEF;
    private int 	m_itemNeverVisitedBackgroundColor 	= 0xFFEFEFEF;
    private int		m_itemProtectedBackgroundColor = 0xFF696969;
    private int 	m_itemLabelTextColor 	= 0xFF2C2F2D;
    private int 	m_itemValueTextColor 	= 0xFF625368;
    private int 	m_listBackgroundColor 	= 0xFFBFBFBF;
    private int 	m_rowCounterTextColor 	= 0xFFFFFFFF;
    private int 	m_selectionColor 		= 0xFFA1A2B3;
    
    private String 	m_desc					= null;
    private String 	m_name					= null;

    private CSStyle()
    {
    	
    }
    
    public static CSStyle defaultStyle()
    {
    	// set the default name and description
    	if(DEFAULT_STYLE == null)
    	{
    		// the default style is Gray Elegy
    		String name = "Gray Elegy";
    		
    		if(m_styles.size() == 0)
    		{
    			// load the styles
    			try
    			{
        			getAllStyles(CSEntry.Companion.getContext());
    			}
    			
    			catch(Exception ex)
    			{
    			}
    		}
			// find the default
			for (CSStyle style : m_styles) 
			{
				if(style.getName().compareTo(name) == 0)
				{
					DEFAULT_STYLE = style;
					break;
				}
			}
			
			if( DEFAULT_STYLE == null )
			{
				Log.d("CSStyle", "An Error Occurred While Loading Styles, the default will be selected.");
				DEFAULT_STYLE = new CSStyle();
				DEFAULT_STYLE.m_name = "Gray Elegy";
				DEFAULT_STYLE.m_desc = "CSEntry Default";
			}
    	}
    	
    	return DEFAULT_STYLE;
    }
    	
	public static ArrayList<CSStyle> getAllStyles(Context context) throws Exception
	{
		InputStream inputStream = null;
		XmlPullParser xrp = null;

		if(m_styles.size() > 0)
		{
			m_styles.clear();
		}
		
		try 
		{			
			CSStyle style	= null;
			inputStream		= context.getResources().openRawResource(gov.census.cspro.csentry.R.raw.casetree_styles);
			xrp 			= Xml.newPullParser();
			xrp.setInput(inputStream, null);
		
			while (xrp.getEventType() != XmlResourceParser.END_DOCUMENT) 
			{
				// get the current tag
                String tagname = xrp.getName();

	            if (xrp.getEventType() == XmlResourceParser.START_TAG) 
	            {
	            	// this is an outer tag, check the value to know what we are 
	            	// parsing
	                if (tagname.equals(STYLE_TAG)) 
	                {
	                	// get the name of this style and create an object
	                	String name 	= xrp.getAttributeValue(null, NAME_ATTR);
	        			style 			= new CSStyle();

	                	style.m_name 	= name;
	                	style.m_desc 	= xrp.getAttributeValue(null, DESC_ATTR);	                		
	                }
	                else if(tagname.compareTo(ITEM_TAG) == 0)
	                {
            			// get the name of this attribute of CSStryle
            			String attrName = xrp.getAttributeValue(null, NAME_ATTR);
            			String t		= xrp.nextText();
            			// switch on the value we are setting
            			if(attrName.compareTo(DIVIDER_COLOR_ATTR) == 0)
            			{
            				style.m_dividerColor = (int)Long.parseLong(t, 16);
            			}
            			else if(attrName.compareTo(GROUP_BACKGROUND_COLOR_ATTR) == 0)
            			{
            				style.m_groupBackgroundColor = (int)Long.parseLong(t, 16);
            			}
            			else if(attrName.compareTo(HEADER_TEXT_COLOR_ATTR) == 0)
            			{
            				style.m_headerTextColor = (int)Long.parseLong(t, 16);
            			}
            			else if(attrName.compareTo(ITEM_BACKGROUND_COLOR_ATTR) == 0)
            			{
            				style.m_itemBackgroundColor = (int)Long.parseLong(t, 16);
            			}
            			else if(attrName.compareTo(ITEM_SKIPPED_BACKGROUND_COLOR_ATTR) == 0)
            			{
            				style.m_itemSkippedBackgroundColor= (int)Long.parseLong(t, 16);
            			}
            			else if(attrName.compareTo(ITEM_NEVERVISITED_BACKGROUND_COLOR_ATTR) == 0)
            			{
            				style.m_itemNeverVisitedBackgroundColor= (int)Long.parseLong(t, 16);
            			}
            			else if(attrName.compareTo(ITEM_VISITED_BACKGROUND_COLOR_ATTR) == 0)
            			{
            				style.m_itemVisitedBackgroundColor= (int)Long.parseLong(t, 16);
            			}
            			else if(attrName.compareTo(ITEM_CURRENT_BACKGROUND_COLOR_ATTR) == 0)
            			{
            				style.m_itemCurrentBackgroundColor= (int)Long.parseLong(t, 16);
            			}
            			else if(attrName.compareTo(ITEM_PROTECTED_BACKGROUND_COLOR_ATTR) == 0)
            			{
            				style.m_itemProtectedBackgroundColor= (int)Long.parseLong(t, 16);
            			}
            			else if(attrName.compareTo(ITEM_LABEL_TEXT_COLOR_ATTR) == 0)
            			{
            				style.m_itemLabelTextColor = (int)Long.parseLong(t, 16);
            			}
            			else if(attrName.compareTo(ITEM_VALUE_TEXT_COLOR_ATTR) == 0)
            			{
            				style.m_itemValueTextColor = (int)Long.parseLong(t, 16);
            			}
            			else if(attrName.compareTo(LIST_BACKGROUND_COLOR_ATTR) == 0)
            			{
            				style.m_listBackgroundColor = (int)Long.parseLong(t, 16);
            			}
            			else if(attrName.compareTo(ROW_COUNTER_TEXT_COLOR) == 0)
            			{
            				style.m_rowCounterTextColor = (int)Long.parseLong(t, 16);
            			}
            			else if(attrName.compareTo(SELECTION_COLOR_ATTR) == 0)
            			{
            				style.m_selectionColor = (int)Long.parseLong(t, 16);
            			}
	                }
	            }
	            else if(xrp.getEventType() == XmlResourceParser.END_TAG)
	            {
	            	if(tagname.compareTo(STYLE_TAG) == 0)
	            	{
	            		// finished parsing this style, add it to the list
	            		m_styles.add(style);
	            	}
	            }
	            
	            xrp.next();
	        }
		}
		catch (Exception ex) 
		{
			throw ex;
		}
		
		return m_styles;		
	}
	
	public String getDescription()
	{
		return m_desc;
	}
	
	public int getDividerColor()
	{
		return m_dividerColor;
	}

	public int getGroupBackgroundColor()
	{
		return m_groupBackgroundColor;
	}
	
	public int getHeaderTextColor()
	{
		return m_headerTextColor;
	}
	
	public int getItemBackgroundColor()
	{
		return m_itemBackgroundColor;
	}
	public int getItemSkippedBackgroundColor()
	{
		return m_itemSkippedBackgroundColor;
	}
	public int getItemVisitedBackgroundColor()
	{
		return m_itemVisitedBackgroundColor;
	}
	public int getItemNeverVisitedBackgroundColor()
	{
		return m_itemNeverVisitedBackgroundColor;
	}
	public int getItemCurrentBackgroundColor()
	{
		return m_itemCurrentBackgroundColor;
	}
	public int getItemProtectedBackgroundColor()
	{
		return m_itemProtectedBackgroundColor;
	}
	
	public int getItemLabelTextColor()
	{
		return m_itemLabelTextColor;
	}
	
	public int getItemValueTextColor()
	{
		return m_itemValueTextColor;
	}
	
	public int getListBackgroundColor()
	{
		return m_listBackgroundColor;
	}
	
	public String getName()
	{
		return m_name;
	}
	
	public int getRowCounterTextColor()
	{
		return m_rowCounterTextColor;
	}
	
	public int getSelectionColor()
	{
		return m_selectionColor;
	}
	
	public static CSStyle getStyleByName(String name)
	{
		CSStyle style = null;
		
		for (CSStyle aStyle : m_styles)
		{
			if(aStyle.getName().compareTo(name) == 0)
			{
				style = aStyle;
			}
		}
		
		return style;
	}
}
