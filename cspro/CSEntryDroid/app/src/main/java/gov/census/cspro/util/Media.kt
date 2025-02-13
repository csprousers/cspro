package gov.census.cspro.util

import android.app.Activity
import android.os.Build
import android.provider.MediaStore
import timber.log.Timber
import java.util.ArrayList

class Media {
    companion object {
        fun getMediaFilenames(mediaType: Int, activity: Activity?): ArrayList<String> {
            val mediaFilenames = ArrayList<String>()

            try {
                val collection =
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                        when (mediaType) {
                            MediaTypeAudio  -> MediaStore.Audio.Media.getContentUri(MediaStore.VOLUME_EXTERNAL)
                            MediaTypeImages -> MediaStore.Images.Media.getContentUri(MediaStore.VOLUME_EXTERNAL)
                            MediaTypeVideo  -> MediaStore.Video.Media.getContentUri(MediaStore.VOLUME_EXTERNAL)
                            else            -> throw IllegalArgumentException()
                        }
                    } else {
                        when (mediaType) {
                            MediaTypeAudio  -> MediaStore.Audio.Media.EXTERNAL_CONTENT_URI
                            MediaTypeImages -> MediaStore.Images.Media.EXTERNAL_CONTENT_URI
                            MediaTypeVideo  -> MediaStore.Video.Media.EXTERNAL_CONTENT_URI
                            else            -> throw IllegalArgumentException()
                        }
                    }

                val projection = arrayOf(
                    when (mediaType) {
                        MediaTypeAudio  -> MediaStore.Audio.Media.DATA
                        MediaTypeImages -> MediaStore.Images.Media.DATA
                        MediaTypeVideo  -> MediaStore.Video.Media.DATA
                        else            -> throw IllegalArgumentException()
                    }
                )

                val query = activity?.contentResolver?.query(
                    collection,
                    projection,
                    null,
                    null,
                    null
                )

                query?.use { cursor ->
                    while (cursor.moveToNext()) {
                        mediaFilenames.add(cursor.getString(0))
                    }
                }

            } catch (e: Exception) {
                Timber.e(e)
            }

            return mediaFilenames
        }

        // values defined in MediaStore.h
        const val MediaTypeAudio  = 0
        const val MediaTypeImages = 1
        const val MediaTypeVideo  = 2
    }
}