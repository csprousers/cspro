package gov.census.cspro.util

import android.content.Intent
import android.os.Parcelable
import kotlinx.parcelize.Parcelize
import java.util.*

/**
 * Used to pass data to Activities in an Intent
 * when the size of the data could be too big to save
 * in a Bundle. Instead of adding the data to the
 * Intent, call DataHolder.store() to get a key
 * and store the key in the Intent. Then to get
 * the data back, extract the key from the Intent
 * and pass it to DataHolder.retrieve().
 *
 * Note that data is stored in memory in the DataHolder
 * singleton and is removed once retrieved. This means that
 * if you try to get it from the Intent a second time e.g.
 * after screen rotation, it will be null. So once you
 * get the data when the Activity is first launched you
 * should stash the data in a ViewModel.
 *
 * Also if the process is killed by Android
 * due to low resources and later restored, the key
 * will still be in the Intent but the DataHolder will
 * be empty. This shouldn't be an issue in CSEntry since
 * the engine is restarted in that case anyway.
 */

@Parcelize
data class DataHolderKey(val id: UUID) : Parcelable

/**
 * Convenience methods for adding to data directly to the intent without
 * using the DataHolder methods directly.
 */
fun Intent.putDataHolderExtra(name:String, data: Any) = putExtra(name, DataHolder.store(data))

fun <T> Intent.getDataHolderExtra(name: String) = getParcelableExtra<DataHolderKey>(name)?.let {
    @Suppress("UNCHECKED_CAST")
    DataHolder.retrieve(it) as T
}

object DataHolder {

    private val bag: MutableMap<DataHolderKey, Any> = mutableMapOf()

    fun store(data: Any): DataHolderKey {
        val key = DataHolderKey(UUID.randomUUID())
        bag[key] = data
        return key
    }

    fun retrieve(key: DataHolderKey): Any? {
        val data = bag[key]
        bag.remove(key)
        return data
    }
}