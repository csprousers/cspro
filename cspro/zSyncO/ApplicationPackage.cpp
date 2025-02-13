#include "stdafx.h"
#include "ApplicationPackage.h"

ApplicationPackage::ApplicationPackage(CString name, CString description, time_t buildTime,
		DeploymentType deploymentType, CString serverUrl,
        const std::vector<ApplicationPackage::FileSpec>& files):
	m_name(name),
	m_buildTime(buildTime),
	m_description(description),
	m_installedVersionBuildTime(0),
	m_deploymentType(deploymentType),
	m_serverUrl(serverUrl),
    m_files(files)
{
}
