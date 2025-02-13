#pragma once

#ifndef WIN_DESKTOP

#include <zPlatformO/zPlatformO.h>
#include <engine/StandardSystemIncludes.h>
#include <zUtilO/MediaStore.h>
#include <zUtilO/PortableColor.h>
#include <zAppO/MappingDefines.h>
#include <zParadataO/Logger.h>
#include <zSyncO/BluetoothDeviceInfo.h>
#include <zEngineF/EngineUINodes.h>
#include <zMapping/Geometry.h>

class Application;
struct IBluetoothAdapter;
struct IFtpConnection;
struct IHttpConnection;
struct IMapUI;
class NavigationAddress;
class ObjectTransporter;
class TemporaryFile;
class Userbar;
class Viewer;

enum class RefreshPageContents { All, Notes };


class CLASS_DECL_ZPLATFORMO_IMPL BaseApplicationInterface
{
public:
    BaseApplicationInterface() {}
    virtual ~BaseApplicationInterface() {}

public:
    virtual ObjectTransporter* GetObjectTransporter() = 0;
    virtual void RefreshPage(RefreshPageContents contents) = 0;
    virtual void DisplayErrorMessage(const TCHAR* error_message) = 0;
    virtual std::optional<std::wstring> DisplayCSHtmlDlg(const NavigationAddress& navigation_address, const std::wstring* action_invoker_access_token_override) = 0;
    virtual std::optional<std::wstring> DisplayHtmlDialogFunctionDlg(const NavigationAddress& navigation_address, const std::wstring* action_invoker_access_token_override, const std::optional<std::wstring>& display_options_json) = 0;
    virtual int ShowModalDialog(NullTerminatedString title, NullTerminatedString message, int mbType) = 0;
    virtual int ShowMessage(const CString& title, const CString& message, const std::vector<CString>& aButtons) = 0;
    virtual bool GpsOpen() = 0;
    virtual bool GpsClose() = 0;
    virtual CString GpsRead(int waitTime, int accuracy, const CString& dialog_text) = 0;
    virtual CString GpsReadLast() = 0;
    virtual CString GpsReadInteractive(bool read_interactive_mode, const BaseMapSelection& base_map_selection, const CString& message, double read_duration) = 0;
    struct LoginCredentials { CString username; CString password; };
    virtual std::optional<LoginCredentials> ShowLoginDialog(const CString& server, bool show_invalid_error) = 0;
    virtual std::optional<BluetoothDeviceInfo> ChooseBluetoothDevice(const GUID& service_uuid) = 0;
    virtual CString AuthorizeDropbox(const CString& clientId) = 0;
    virtual void EngineAbort() = 0;
    virtual bool ExecSystem(const std::wstring& command, bool wait) = 0;
    virtual bool ExecPff(const std::wstring& pff_filename) = 0;
    virtual CString GetProperty(const CString& parameter) = 0;
    virtual void SetProperty(const CString& parameter, const CString& value) = 0;
    virtual void ShowProgressDialog(const CString& message) = 0;
    virtual void HideProgressDialog() = 0;
    virtual bool UpdateProgressDialog(int progressPercent, const CString* message) = 0;
    virtual bool PartialSave(bool bPartialSaveClearSkipped, bool bFromLogic) = 0;
    virtual int ShowChoiceDialog(const CString& title, const std::vector<std::vector<CString>*>& data) = 0;
    virtual int ShowShowDialog(const std::vector<CString>* column_titles, const std::vector<PortableColor>* row_text_colors,
        const std::vector<std::vector<CString>*>& data, const CString& heading) = 0;
    virtual int ShowSelcaseDialog(const std::vector<CString>* column_titles, const std::vector<std::vector<CString>*>& data, const CString& heading, std::vector<bool>* selections) = 0;
    virtual bool IsNetworkConnected(int connectionType) = 0;
    virtual std::tuple<int, int> GetMaxDisplaySize() const = 0;
    virtual std::vector<std::wstring> GetMediaFilenames(MediaStore::MediaType media_type) const = 0;
    virtual CString GetUsername() const = 0;
    virtual CString GetDeviceId() const = 0;
    virtual CString GetLocaleLanguage() const = 0;
    virtual void StoreCredential(const std::wstring& attribute, const std::wstring& secret_value) = 0;
    virtual std::wstring RetrieveCredential(const std::wstring& attribute) = 0;
    virtual std::optional<std::wstring> GetPassword(const std::wstring& title, const std::wstring& description, bool file_exists) = 0;
    virtual void ParadataDriverManager(Paradata::PortableMessage msg, const Application* application) = 0;
    virtual void ParadataDeviceInfoQuery(Paradata::ApplicationEvent::DeviceInfo& device_info) = 0;
    virtual void ParadataDeviceStateQuery(Paradata::DeviceStateEvent::DeviceState& device_state) = 0;
    virtual double GetUpTime() = 0;
    virtual IBluetoothAdapter* CreateAndroidBluetoothAdapter() = 0;
    virtual IHttpConnection* CreateAndroidHttpConnection() = 0;
    virtual IFtpConnection* CreateAndroidFtpConnection() = 0;
    virtual std::wstring BarcodeRead(const std::wstring& message_text) = 0;

    virtual bool AudioPlay(const std::wstring& filename, const std::wstring& message_text) = 0;
    virtual bool AudioStartRecording(const std::wstring& filename, std::optional<double> seconds, std::optional<int> sampling_rate) = 0;
    virtual bool AudioStopRecording() = 0;
    virtual std::unique_ptr<TemporaryFile> AudioRecordInteractive(const std::wstring& message_text, std::optional<int> sampling_rate) = 0;

    virtual void CapturePolygonTrace(std::unique_ptr<Geometry::Polygon>& captured_polygon, const Geometry::Polygon* polygon, IMapUI* map) = 0;
    virtual void CapturePolygonWalk(std::unique_ptr<Geometry::Polygon>& captured_polygon, const Geometry::Polygon* polygon, IMapUI* map) = 0;

    // for EngineUIProcessor
    virtual long RunEngineUIProcessor(WPARAM wParam, LPARAM lParam) = 0;
    virtual bool CaptureImage(EngineUI::CaptureImageNode& capture_image_node) = 0;
    virtual void CreateMapUI(std::unique_ptr<IMapUI>& map_ui) = 0;
    virtual void CreateUserbar(std::unique_ptr<Userbar>& userbar) = 0;
    virtual CString EditNote(const CString& note, const CString& title, bool case_note) = 0;
    virtual bool ExecSystemApp(EngineUI::ExecSystemAppNode& exec_system_app_node) = 0;
    virtual std::wstring GetHtmlDialogsDirectory() = 0;
    virtual void Prompt(EngineUI::PromptNode& options) = 0;
    virtual bool RunPffExecutor(EngineUI::RunPffExecutorNode& run_pff_executor_node) = 0;
    virtual long View(const Viewer& viewer) = 0;

#ifdef ANDROID
    // Android only updates files exposed to PC via USB connection after
    // they are scanned by media scanner so if you create a new file need
    // to scan it first for it to show up when connecting to PC.
    virtual void MediaScanFiles(const std::vector<CString>& paths) = 0;

    // creates a sharable URI for the specified file
    virtual std::wstring CreateSharableUri(const std::wstring& path, bool add_write_permission) = 0;

    // copies a sharable URL to a file
    virtual void FileCopySharableUri(const std::wstring& sharable_uri, const std::wstring& destination_path) = 0;
#endif
};


///<summary>Singleton container for platform specific dependencies that are set by platform specific init code and can be used by framework</summary>
class CLASS_DECL_ZPLATFORMO_IMPL PlatformInterface
{
public:
    PlatformInterface();
    virtual ~PlatformInterface() { }

    static PlatformInterface* GetInstance();

    BaseApplicationInterface* GetApplicationInterface()               { return m_pApplicationInterface; }
    void SetApplicationInterface(BaseApplicationInterface* pAppIFace) { m_pApplicationInterface = pAppIFace; }

    const std::wstring& GetWorkingDirectory() const  { return m_workingDirectory; }
    void SetWorkingDirectory(std::wstring directory) { m_workingDirectory = std::move(directory); }

    const std::wstring& GetTempDirectory() const  { return m_tempDirectory.empty() ? m_workingDirectory : m_tempDirectory ; }
    void SetTempDirectory(std::wstring directory) { m_tempDirectory = std::move(directory); }

    const std::wstring& GetApplicationDirectory() const  { return m_applicationDirectory; }
    void SetApplicationDirectory(std::wstring directory) { m_applicationDirectory = std::move(directory); }

    const std::wstring& GetCSEntryDirectory() const  { return m_csentryDirectory; }
    void SetCSEntryDirectory(std::wstring directory) { m_csentryDirectory = std::move(directory); }

    const CString& GetExternalMemoryCardDirectory() const { return m_strExternalMemoryCardDirectory; }
    void SetExternalMemoryCardDirectory(LPCTSTR pszDir)   { m_strExternalMemoryCardDirectory = pszDir; }

    const std::wstring& GetInternalStorageDirectory() const  { return m_internalStorageDirectory; }
    void SetInternalStorageDirectory(std::wstring directory) { m_internalStorageDirectory = std::move(directory); }

    const std::wstring& GetAssetsDirectory() const  { return m_assetsDirectory; }
    void SetAssetsDirectory(std::wstring directory) { m_assetsDirectory = std::move(directory); }

    const std::wstring& GetDownloadsDirectory() const  { return m_downloadsDirectory; }
    void SetDownloadsDirectory(std::wstring directory) { m_downloadsDirectory = std::move(directory); }

#ifndef _CONSOLE
    const std::wstring& GetVersionNumber() const { return m_versionNumber; }
    void SetVersionNumber(std::wstring version)  { m_versionNumber = std::move(version); }
#endif

private:
    BaseApplicationInterface* m_pApplicationInterface;

    std::wstring m_workingDirectory;
    std::wstring m_tempDirectory;
    std::wstring m_applicationDirectory;
    std::wstring m_csentryDirectory;
    CString m_strExternalMemoryCardDirectory;
    std::wstring m_internalStorageDirectory;
    std::wstring m_assetsDirectory;
    std::wstring m_downloadsDirectory;

#ifndef _CONSOLE
    std::wstring m_versionNumber;
#endif
};

#endif
