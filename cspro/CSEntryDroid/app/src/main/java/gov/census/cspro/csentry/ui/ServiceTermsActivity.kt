/***************************************************************************************
 *
 * CSEntry for Android
 *
 * Module:		ServiceTermsActivity.java
 *
 * Description: Activity UI class for displaying and confirming terms of service
 * to the user.
 *
 */
package gov.census.cspro.csentry.ui

import android.os.Bundle
import android.text.method.ScrollingMovementMethod
import android.view.View
import android.widget.Button
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import gov.census.cspro.csentry.R
import timber.log.Timber
import java.io.BufferedReader
import java.io.IOException
import java.io.InputStream
import java.io.InputStreamReader

// Java Imports
// Android Imports
// Project Imports
class ServiceTermsActivity : AppCompatActivity(), View.OnClickListener {

    override fun onCreate(savedInstance: Bundle?) {
        super.onCreate(savedInstance)

        // set the view
        setContentView(R.layout.activity_service_terms)
        // setup buttons, events, load RTF
        val cancelButton: Button = findViewById(R.id.button_terms_cancel)
        cancelButton.setOnClickListener(this)
        val doneButton: Button = findViewById(R.id.button_terms_accept)
        doneButton.setOnClickListener(this)
        loadTermsOfService()
        val textview: TextView = findViewById(R.id.textview_terms_text)
        textview.movementMethod = ScrollingMovementMethod()
    }

    private fun loadTermsOfService() {
        var c: Int
        val buf = CharArray(1)
        var reader: BufferedReader? = null
        val inputStream: InputStream
        val terms: StringBuilder = StringBuilder()
        try {
            // open a stream to the terms of service
            inputStream = resources.openRawResource(R.raw.cspro_terms_of_service)
            reader = BufferedReader(InputStreamReader(inputStream, "UTF-8"))

            // read content
            do {
                c = reader.read(buf)
                if (c != -1) {
                    terms.append(buf[0])
                }
            } while (c != -1)
            val termsTextview: TextView = findViewById(R.id.textview_terms_text)
            termsTextview.text = terms
            termsTextview.scrollTo(0, 0)
        } catch (ex: Exception) {
            Timber.d(ex, "Could not display Terms of Service.")
        } finally {
            if (reader != null) {
                try {
                    reader.close()
                } catch (ioe: IOException) {
                    Timber.d(ioe, "Could not close input reader for Terms of Service.")
                }
            }
        }
    }

    override fun onClick(v: View) {
        var result: Int = RESULT_CANCELED
        if (v.id == R.id.button_terms_accept) {
            // user agreed
            result = RESULT_OK
            setResult(result)
            onBackPressed()
        } else {
            setResult(result)
            finish()
        }
    }
}