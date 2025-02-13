/***************************************************************************************
 *
 * CSEntry for Android
 *
 * Module:		SelectStyleActivity.java
 *
 * Description: GUI used for selecting Case Tree styles.
 *
 */
package gov.census.cspro.csentry.ui

import android.os.Bundle
import android.view.View
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import gov.census.cspro.commonui.CSStyle
import gov.census.cspro.commonui.StyleListAdapter
import gov.census.cspro.commonui.StyleListAdapter.OnStyleItemChanged
import gov.census.cspro.csentry.CSEntry
import gov.census.cspro.csentry.R

//Android Imports
//Project Imports
class SelectStyleActivity : AppCompatActivity(), View.OnClickListener, AdapterView.OnItemClickListener, OnStyleItemChanged {

    private var m_selectedIndex: Int = -1

    override fun onCreate(savedInstanceState: Bundle?) {

        super.onCreate(savedInstanceState)
        setFinishOnTouchOutside(false)
        setContentView(R.layout.activity_select_style)

        // populate the style list
        val styleListview: ListView = findViewById<View>(R.id.listview_styles) as ListView
        val adapter = StyleListAdapter(this)
        adapter.setOnStyleItemChanged(this)
        styleListview.choiceMode = ListView.CHOICE_MODE_SINGLE
        styleListview.adapter = adapter
        styleListview.onItemClickListener = this

        // wireup the button event handling
        val okButton: Button = findViewById<View>(R.id.button_select_style_ok) as Button
        okButton.setOnClickListener(this)
        // select the current operating style
        CSEntry.style?.name?.let { styleName->
            for (i in 0 until adapter.count) {
                val aStyle: CSStyle = adapter.getItem(i) as CSStyle
                if (aStyle.name.compareTo(styleName) == 0) {
                    // set the listview selection
                    styleListview.setSelection(i)
                    styleListview.setItemChecked(i, true)
                    m_selectedIndex = i
                    break
                }
            }
        }

    }

    public override fun onClick(v: View) {
        if (v.id == R.id.button_select_style_ok) {
            // apply the style then dismiss
            val styleListview: ListView = findViewById<View>(R.id.listview_styles) as ListView
            val adapter: StyleListAdapter = styleListview.adapter as StyleListAdapter
            val selectedStyle: CSStyle? = adapter.getItem(m_selectedIndex) as CSStyle?
            if (selectedStyle != null) {
                // save it
                CSEntry.style = selectedStyle
            }
            finish()
        }
    }

    public override fun onItemClick(arg0: AdapterView<*>?, view: View, position: Int, id: Long) {
        val styleListview: ListView = findViewById<View>(R.id.listview_styles) as ListView
        styleListview.setItemChecked(position, true)
        m_selectedIndex = position
    }

    public override fun onStyleItemChanged(position: Int) {
        val styleListview: ListView = findViewById<View>(R.id.listview_styles) as ListView
        styleListview.setItemChecked(position, true)
        m_selectedIndex = position
    }
}