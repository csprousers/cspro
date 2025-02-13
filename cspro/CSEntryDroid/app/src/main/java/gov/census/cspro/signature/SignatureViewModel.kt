package gov.census.cspro.signature

import android.graphics.Bitmap
import android.graphics.Rect
import androidx.lifecycle.ViewModel

class SignatureViewModel : ViewModel() {

    var bitmap : Bitmap? = null
    var bounds: Rect = Rect()
}