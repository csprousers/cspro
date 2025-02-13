package gov.census.cspro.engine.functions

import android.app.Activity
import android.content.Intent
import androidx.activity.result.ActivityResult
import androidx.documentfile.provider.DocumentFile
import gov.census.cspro.engine.Messenger


class SelectDocumentDialog(private val m_mimeTypes: Array<String>, private val m_multiple: Boolean) : EngineFunction {
    override fun runEngineFunction(activity: Activity) {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT)
        intent.addCategory(Intent.CATEGORY_OPENABLE)

        if( m_multiple ) {
            intent.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, true)
        }

        intent.type  = "*/*"
        intent.putExtra(Intent.EXTRA_MIME_TYPES, m_mimeTypes)

        Messenger.getInstance().startActivityForResultFromEngineFunction(activity,
            { result -> Messenger.getInstance().engineFunctionComplete(generateResult(activity, result)) },
            intent)
    }

    private fun generateResult(activity: Activity, result: ActivityResult): Array<String>? {
        if( result.resultCode == Activity.RESULT_OK ) {
            val pathsAndNames = ArrayList<String>()

            // a single selection will come in via Intent.data
            if( result.data?.data != null ) {
                val df = DocumentFile.fromSingleUri(activity, result.data!!.data!!)
                if( df?.name != null ) {
                    pathsAndNames.add(df.uri.toString())
                    pathsAndNames.add(df.name!!)
                }
            }

            // otherwise we must use Intent.ClipData
            else if( result.data?.clipData != null ) {
                for( i in 0 until result.data!!.clipData!!.itemCount ) {
                    result.data!!.clipData!!.getItemAt(i)?.let {
                        val df = DocumentFile.fromSingleUri(activity, it.uri)
                        if( df?.name != null ) {
                            pathsAndNames.add(df.uri.toString())
                            pathsAndNames.add(df.name!!)
                        }
                    }
                }
            }

            if( pathsAndNames.isNotEmpty() ) {
                return pathsAndNames.toTypedArray()
            }
        }

        return null
    }
}