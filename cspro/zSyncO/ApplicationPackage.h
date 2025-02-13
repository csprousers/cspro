#pragma once

#include <zSyncO/zSyncO.h>
#include <zAppO/SyncTypes.h>


///<summary>Application deployment package stored on server that can be downloaded to install app(s) and related files.</summary>
class SYNC_API ApplicationPackage {
public:

	enum class DeploymentType
	{
		None, CSWeb, Dropbox, FTP, LocalFile, LocalFolder
	};

    struct FileSpec {
        CString Path;
        CString Signature;
        bool OnlyOnFirstInstall;
    };

	ApplicationPackage(CString name, CString description, time_t buildTime, DeploymentType deploymentType, CString serverUrl, const std::vector<FileSpec>& files);

	CString getName() const
	{
		return m_name;
	}

	void setName(CString name)
	{
		m_name = name;
	}

	CString getDescription() const
	{
		return m_description;
	}

	void setDescription(CString desc)
	{
		m_description = desc;
	}

	time_t getBuildTime() const
	{
		return m_buildTime;
	}

	void setBuildTime(time_t buildTime)
	{
		m_buildTime = buildTime;
	}

	time_t getInstalledVersionBuildTime() const
	{
		return m_installedVersionBuildTime;
	}

	void setInstalledVersionBuildTime(time_t buildTime)
	{
		m_installedVersionBuildTime = buildTime;
	}

	DeploymentType getDeploymentType() const
	{
		return m_deploymentType;
	}

	void setDeploymentType(DeploymentType t)
	{
		m_deploymentType = t;
	}

	CString getServerUrl() const
	{
		return m_deploymentType == DeploymentType::Dropbox ? _T("Dropbox") : m_serverUrl;
	}

	void setServerUrl(CString url)
	{
		m_serverUrl = url;
	}

    const std::vector<FileSpec> getFiles() const
    {
        return m_files;
    }

    void setFiles(const std::vector<FileSpec>& files)
    {
        m_files = files;
    }

	CString getInstallPath() const
	{
		return m_installPath;
	}

	void setInstallPath(CString path)
	{
		m_installPath = path;
	}
private:

	CString m_name;
	CString m_description;
	time_t m_buildTime;
	time_t m_installedVersionBuildTime;
	CString m_serverUrl;
	DeploymentType m_deploymentType;
    std::vector<FileSpec> m_files;
    CString m_installPath;
};
