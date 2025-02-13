package gov.census.cspro.csentry.ui

import android.app.AlertDialog
import android.content.DialogInterface
import android.os.Bundle
import android.view.View
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import gov.census.cspro.csentry.R
import gov.census.cspro.csentry.ui.ReviewNotesActivity
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.engine.Util
import gov.census.cspro.form.FieldNote
import java.util.*

class ReviewNotesActivity : AppCompatActivity() {

    private var m_fieldNotes: ArrayList<FieldNote>? = null

    private inner class OccurrenceLabelNoteCounter(occRow: TableRow?) {
        var OccRow: TableRow? = null
        var Notes = 0

        init {
            OccRow = occRow
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.notes_review_layout)
        m_fieldNotes = EngineInterface.getInstance().allNotes
        if (m_fieldNotes?.size == 0) {
            Toast.makeText(this, getString(R.string.notes_no_notes), Toast.LENGTH_LONG).show()
            finish()
            return
        }


        // fill in the table with the notes
        val tableLayout = findViewById<View>(R.id.tableLayoutNotes) as TableLayout
        val layoutParams = TableRow.LayoutParams(TableRow.LayoutParams.MATCH_PARENT, TableRow.LayoutParams.MATCH_PARENT)
        var lastGroupSymbolIndex = -1
        var lastGroupLabel = ""
        var currentOccLabelNoteCounter: OccurrenceLabelNoteCounter? = null
        val mapNotesToOccurrenceLabelNoteCounter = HashMap<TableRow, OccurrenceLabelNoteCounter?>()
        m_fieldNotes?.forEach { fieldNote->

            // add a new occurrence label if necessary
            if (fieldNote.groupSymbolIndex != lastGroupSymbolIndex || fieldNote.groupLabel != lastGroupLabel) {
                val occRow = TableRow(this)
                occRow.layoutParams = layoutParams
                val occView = layoutInflater.inflate(R.layout.notes_review_label, null)
                val tvOccLabel = occView.findViewById<View>(R.id.textViewNotesOccurrenceLabel) as TextView
                tvOccLabel.text = fieldNote.groupLabel
                occRow.addView(occView)
                tableLayout.addView(occRow)
                lastGroupSymbolIndex = fieldNote.groupSymbolIndex
                lastGroupLabel = fieldNote.groupLabel
                currentOccLabelNoteCounter = OccurrenceLabelNoteCounter(occRow)
            }

            // add the note
            val noteRow = TableRow(this)
            noteRow.layoutParams = layoutParams
            currentOccLabelNoteCounter?.let{ ++it.Notes }
            mapNotesToOccurrenceLabelNoteCounter[noteRow] = currentOccLabelNoteCounter
            val noteView = layoutInflater.inflate(R.layout.notes_review_row, null)
            val tvFieldLabel = noteView.findViewById<View>(R.id.textViewNotesFieldLabel) as TextView
            tvFieldLabel.text = fieldNote.label
            val tvNote = noteView.findViewById<View>(R.id.textViewNotesFieldNote) as TextView
            tvNote.text = fieldNote.note

            // handle the "go to field" button
            val buttonGoto = noteView.findViewById<View>(R.id.buttonNotesGoto) as ImageButton
            if (!fieldNote.isFieldNote) buttonGoto.visibility = View.INVISIBLE else {
                buttonGoto.setOnClickListener(View.OnClickListener {
                    val intent = this@ReviewNotesActivity.intent
                    intent.putExtra(GotoFieldNoteIndex, fieldNote.index)
                    setResult(RESULT_OK, intent)
                    finish()
                })
            }

            // handle the "delete" button
            val buttonDelete = noteView.findViewById<View>(R.id.buttonNotesDelete) as ImageButton
            buttonDelete.setOnClickListener(object : View.OnClickListener {
                override fun onClick(v: View) {
                    // confirm the deletion
                    val builder = AlertDialog.Builder(this@ReviewNotesActivity)
                    builder.setMessage(getString(R.string.notes_delete_prompt))
                        .setPositiveButton(getString(R.string.modal_dialog_helper_yes_text), object : DialogInterface.OnClickListener {
                            override fun onClick(dialog: DialogInterface, id: Int) {
                                // if this is the only note under an occurrence label, we also need to delete the label
                                val occLabelNoteCounter = mapNotesToOccurrenceLabelNoteCounter[noteRow]
                                if (occLabelNoteCounter != null) {
                                    if (--occLabelNoteCounter.Notes == 0) tableLayout.removeView(occLabelNoteCounter.OccRow)
                                }

                                // delete the note
                                EngineInterface.getInstance().deleteNote(fieldNote.index)
                                m_fieldNotes?.remove(fieldNote)
                                tableLayout.removeView(noteRow)

                                // if they deleted all the notes, quit out of the activity
                                if (m_fieldNotes?.size == 0) finish()
                            }
                        })
                        .setNegativeButton(getString(R.string.modal_dialog_helper_no_text), null)
                        .show()
                }
            })
            noteRow.addView(noteView)
            tableLayout.addView(noteRow)
        }
    }

    companion object {
        val GotoFieldNoteIndex = "GOTO_NOTE_INDEX"
    }
}