package gov.census.cspro.commonui;

import java.util.ArrayList;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.TextView;
import android.widget.AdapterView.OnItemClickListener;

import gov.census.cspro.csentry.R;
import timber.log.Timber;

public class StyleListAdapter extends BaseAdapter implements OnItemClickListener
{
	private ArrayList<CSStyle> 	m_styles 			= null;
	private Context 			m_context 			= null;
	private OnStyleItemChanged	m_styleChanged		= null;
	
	public StyleListAdapter() 
	{
		m_styles = new ArrayList<CSStyle>();
	}
	
	public StyleListAdapter(Context context)
	{
		super();
		
		try 
		{
			m_context	= context;
			m_styles 	= CSStyle.getAllStyles(context);
		} 
		catch (Exception ex) 
		{
			Timber.d(ex, "An Error Occurred While Loading Styles");
		}
	}

	@Override
	public int getCount() 
	{
		return m_styles.size();
	}

	@Override
	public Object getItem(int index) 
	{
		return m_styles.get(index);
	}

	@Override
	public long getItemId(int arg0) 
	{
		return 0;
	}

	@Override
	public View getView(int position, View convertView, ViewGroup parent) 
	{
		if(convertView == null)
		{
			LayoutInflater infalInflater= (LayoutInflater) m_context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
			convertView 				= infalInflater.inflate(R.layout.style_list_item, null);
		}

		// set the properties in the view for the style at this index
		CSStyle	style 					= (CSStyle)getItem(position);
		// background
		View 	navfragView				= convertView.findViewById(R.id.style_item_navfrag_hero);
		navfragView.setBackgroundColor(style.getItemBackgroundColor());
		// name textcolor
		View 	textColorView 			= convertView.findViewById(R.id.style_item_textcolor_hero);
		textColorView.setBackgroundColor(style.getItemLabelTextColor());
		// value textcolor
		View 	subTextColorView 		= convertView.findViewById(R.id.style_item_subtextcolor_hero);
		subTextColorView.setBackgroundColor(style.getItemValueTextColor());
		// selection color
		View 		selectionColorView	= convertView.findViewById(R.id.style_item_selectioncolor_hero);
		selectionColorView.setBackgroundColor(style.getSelectionColor());
		// set the display info
		TextView	nameTextview	 	= (TextView)convertView.findViewById(R.id.style_item_name_textview);
		nameTextview.setText(style.getName());
		TextView 	descTextview 		= (TextView)convertView.findViewById(R.id.style_item_description_textview);
		descTextview.setText(style.getDescription());
		descTextview.setTextColor(style.getListBackgroundColor());
		// wireup event handling and set position for registering taps properly
		CheckableRelativeLayout layout	= (CheckableRelativeLayout)convertView;
		layout.setPosition(position);
		layout.setOnItemClickListener(this);
		
		return convertView;
	}

	@Override
	public void onItemClick(AdapterView<?> arg0, View arg1, int position, long id) 
	{
		// notify when the style changed
		if(m_styleChanged != null)
		{
			m_styleChanged.onStyleItemChanged(position);
		}
	}
	
	public void setOnStyleItemChanged(OnStyleItemChanged styleItemChanged)
	{
		m_styleChanged = styleItemChanged;
	}
	
	public interface OnStyleItemChanged
	{
		public void onStyleItemChanged(int position);
	}
}
