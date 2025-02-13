package gov.census.cspro.csentry

import android.graphics.Color
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.DiffUtil.DiffResult
import androidx.recyclerview.widget.RecyclerView
import gov.census.cspro.data.CaseSummary
import gov.census.cspro.data.GetTrimmedKeyForDisplay
import gov.census.cspro.engine.Util
import java.util.*

class CaseListAdapter : RecyclerView.Adapter<CaseListAdapter.ViewHolder>() {
    private var m_items: List<CaseSummary> = ArrayList()
    private var m_showLabels: Boolean = true
    private var m_onItemClickListener: OnItemClickListener? = null
    private var m_onItemLongClickListener: OnItemClickListener? = null

    interface OnItemClickListener {
        fun OnClick(caseSummary: CaseSummary, view: View?)
    }

    fun setCases(cases: List<CaseSummary>) {
        val diffCallback = DiffCallback(m_items, cases)
        val diffResult: DiffResult = DiffUtil.calculateDiff(diffCallback)
        m_items = cases
        diffResult.dispatchUpdatesTo(this)
    }

    fun setShowLabels(showLabels: Boolean) {
        m_showLabels = showLabels
        notifyDataSetChanged()
    }

    fun setItemOnClickListener(listener: OnItemClickListener?) {
        m_onItemClickListener = listener
    }

    fun setItemOnLongClickListener(listener: OnItemClickListener?) {
        m_onItemLongClickListener = listener
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
        val v: View = LayoutInflater.from(parent.context)
            .inflate(R.layout.cases_list_item_layout, parent, false)
        return ViewHolder(v)
    }

    override fun onBindViewHolder(holder: ViewHolder, position: Int) {
        val caseSummary: CaseSummary = m_items[position]
        holder.m_titleTextView.text = caseSummary.GetTrimmedKeyForDisplay(m_showLabels)
        holder.m_titleTextView.setTextColor(TITLE_COLOR)
        holder.m_indicatorView.setBackgroundColor(if (caseSummary.isPartial) INCOMPLETE_STATUS_COLOR else NO_COLOR)
        if (caseSummary.caseNote.isEmpty()) {
            holder.m_notesTextView.visibility = View.GONE
        } else {
            holder.m_notesTextView.visibility = View.VISIBLE
            holder.m_notesTextView.text = caseSummary.caseNote
        }
    }

    override fun getItemCount(): Int {
        return m_items.size
    }

    inner class ViewHolder constructor(v: View) : RecyclerView.ViewHolder(v) {
        var m_titleTextView: TextView
        var m_notesTextView: TextView
        var m_indicatorView: View

        init {
            v.setOnClickListener { view -> onItemClick(adapterPosition, view) }
            v.setOnLongClickListener { view ->
                onItemLongClick(adapterPosition, view)
                true
            }
            m_titleTextView = v.findViewById(R.id.textview_cases_caseid)
            m_notesTextView = v.findViewById(R.id.textview_cases_note)
            m_indicatorView = v.findViewById(R.id.view_cases_status)
        }
    }

    private fun onItemClick(position: Int, view: View) {
        if ((m_onItemClickListener != null) && (position >= 0) && (position < m_items.size)) {
            m_onItemClickListener?.OnClick(m_items[position], view)
        }
    }

    private fun onItemLongClick(position: Int, view: View) {
        m_onItemLongClickListener?.OnClick(m_items[position], view)
    }

    private inner class DiffCallback(private val m_oldList: List<CaseSummary>, private val m_newList: List<CaseSummary>) : DiffUtil.Callback() {
        override fun getOldListSize(): Int {
            return m_oldList.size
        }

        override fun getNewListSize(): Int {
            return m_newList.size
        }

        override fun areItemsTheSame(oldItemPos: Int, newItemPos: Int): Boolean {
            return m_oldList.get(oldItemPos) === m_newList.get(newItemPos)
        }

        override fun areContentsTheSame(oldItemPos: Int, newItemPos: Int): Boolean {
            return areItemsTheSame(oldItemPos, newItemPos)
        }
    }

    companion object {
        private val INCOMPLETE_STATUS_COLOR: Int = Color.RED
        private val NO_COLOR: Int = 0x00000000
        private val TITLE_COLOR: Int = 0xFF32323A.toInt()
    }
}