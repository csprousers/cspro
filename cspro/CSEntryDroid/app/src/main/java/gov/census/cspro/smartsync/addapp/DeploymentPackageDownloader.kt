package gov.census.cspro.smartsync.addapp

import java.util.*

class DeploymentPackageDownloader {

    private var m_nativeInstance: Long = 0

    fun ConnectToServer(serverUrl: String?): Int {
        return ConnectToServerCredentials(serverUrl, null, null)
    }

    fun ConnectToServerCredentials(serverUrl: String?, application_name: String?, credentials: String?): Int {
        if (m_nativeInstance == 0L) m_nativeInstance = CreateNativeInstance()
        return ConnectToServer(m_nativeInstance, serverUrl, application_name, credentials)
    }

    fun ListPackages(appPackages: ArrayList<DeploymentPackage?>): Int {
        return ListPackages(m_nativeInstance, appPackages)
    }

    fun InstallPackage(packageName: String?, forceFullUpdate: Boolean): Int {
        return InstallPackage(m_nativeInstance, packageName, forceFullUpdate)
    }

    fun Disconnect() {
        CleanupNativeInstance(m_nativeInstance)
        m_nativeInstance = 0
    }

    fun ListUpdatablePackages(packages: ArrayList<DeploymentPackage>): Int {
        if (m_nativeInstance == 0L) m_nativeInstance = CreateNativeInstance()
        return ListUpdatablePackages(m_nativeInstance, packages)
    }

    fun UpdatePackage(p: DeploymentPackage): Int {
        if (m_nativeInstance == 0L) m_nativeInstance = CreateNativeInstance()
        return UpdatePackage(p.name, p.serverUrl, m_nativeInstance)
    }

    private external fun CreateNativeInstance(): Long
    private external fun CleanupNativeInstance(nativeInstance: Long)
    private external fun ConnectToServer(nativeInstance: Long, serverUrl: String?, application_name: String?, credentials: String?): Int
    private external fun ListPackages(nativeInstance: Long, packages: ArrayList<DeploymentPackage?>): Int
    private external fun InstallPackage(nativeInstance: Long, packageName: String?, forceFullInstall: Boolean): Int
    private external fun ListUpdatablePackages(nativeInstance: Long, packages: ArrayList<DeploymentPackage>): Int
    private external fun UpdatePackage(name: String?, serverUrl: String?, m_nativeInstance: Long): Int

    companion object {
        const val resultOk = 1
        const val resultError = 0
        const val resultCanceled = -1
    }
}