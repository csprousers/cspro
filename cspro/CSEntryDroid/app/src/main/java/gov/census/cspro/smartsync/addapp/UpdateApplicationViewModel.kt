package gov.census.cspro.smartsync.addapp

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import java.util.ArrayList

class UpdateApplicationViewModel : ViewModel() {

    private val downloader: DeploymentPackageDownloader = DeploymentPackageDownloader()

    private val updatableApps: MutableLiveData<List<DeploymentPackage>> = MutableLiveData()
    private val canceled : MutableLiveData<Boolean> = MutableLiveData()
    var loaded = false

    fun getUpdatableApplications(): LiveData<List<DeploymentPackage>> {
        return updatableApps
    }

    fun getCanceled() : LiveData<Boolean>
    {
        return canceled
    }

    fun loadUpdatableApps() {
        loaded = true
        val packages = ArrayList<DeploymentPackage>()
        when (downloader.ListUpdatablePackages(packages)) {
            DeploymentPackageDownloader.resultOk -> updatableApps.postValue(packages)
            DeploymentPackageDownloader.resultCanceled -> canceled.postValue(true)
            DeploymentPackageDownloader.resultError -> updatableApps.postValue(emptyList())
        }
    }

    fun updateApp(deploymentPackage: DeploymentPackage): Boolean {
        return downloader.UpdatePackage(deploymentPackage) == DeploymentPackageDownloader.resultOk
    }

    fun cleanup() {
        downloader.Disconnect()
    }
}