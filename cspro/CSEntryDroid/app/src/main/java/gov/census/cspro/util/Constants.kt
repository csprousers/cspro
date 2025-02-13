package gov.census.cspro.util

class Constants {

    // Constants used to pass extra data in the intent
    companion object IntentConstants {

        const val INTENT_STARTACTIVITY_DEEPLINK: Int = 100
        const val INTENT_STARTACTIVITY_BARCODE: Int = 102
        const val INTENT_FINISHACTIVITY_BARCODE: Int = 104

        const val BARCODE_URI = "Barcode"
        const val EXTRA_BARCODE_VALUE = "BarcodeValue"
        const val EXTRA_BARCODE_KEY = "BarcodeKey"
        const val EXTRA_USER_MESSAGE_KEY = "UserMessageKey"
        const val EXTRA_BARCODE_DISPLAYVALUE_KEY = "BarcodeDisplayValueKey"
        const val EXTRA_CAPTURE_IMAGE_FILE_URL_KEY = "CameraImageFile"
        const val EXTRA_SIGNATURE_FILE_URL_KEY = "SignatureFile"
        const val EXTRA_SIGNATURE_MESSAGE_KEY = "SignatureMessageKey"
        const val EXTRA_RECORDING_FILE_URL_KEY = "RecordingAudioFile"
        const val EXTRA_RECORDING_SERVICE_FILE_URL_KEY = "ServiceRecordingAudioFile"
        const val EXTRA_PLAYER_FILE_URL_KEY = "RecordingAudioFile"
        const val EXTRA_RECORDING_MAX_TIME_SECONDS = "RecordingMaxTimeSeconds"
        const val EXTRA_RECORDING_SAMPLING_RATE = "RecordingSamplingRate"
        const val EXTRA_AUTOSTART = "Autostart"
    }


}