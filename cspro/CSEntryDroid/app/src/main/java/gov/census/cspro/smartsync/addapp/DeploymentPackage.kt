package gov.census.cspro.smartsync.addapp

import android.os.Parcelable
import kotlinx.parcelize.Parcelize
import java.util.*

/**
 * Application deployment package
 */
@Parcelize
data class DeploymentPackage(
    var name: String?,
    var description: String?,
    var buildTime: Date?,
    var currentInstalledBuildTime: Date?,
    var serverUrl: String?,
    var deploymentType: Int) : Parcelable {

    override fun describeContents(): Int {
        return 0
    }

    val installStatus: Int
        get() = when {
            currentInstalledBuildTime == null -> {
                NEW_INSTALL
            }
            buildTime?.after(currentInstalledBuildTime) == true -> {
                UPDATE_AVAILABLE
            }
            else -> {
                UP_TO_DATE
            }
        }

    companion object {
        const val DeploymentType_None = 0
        const val DeploymentType_CSWeb = 1
        const val DeploymentType_Dropbox = 2
        const val DeploymentType_FTP = 3
        const val DeploymentType_LocalFile = 4
        const val DeploymentType_LocalFolder = 5
        const val NEW_INSTALL = 1
        const val UPDATE_AVAILABLE = 2
        const val UP_TO_DATE = 3
    }
}