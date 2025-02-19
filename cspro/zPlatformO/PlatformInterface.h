#pragma once

#ifndef WIN_DESKTOP

#include <zToolsO/zToolsO.h>
#include <engine/StandardSystemIncludes.h>
#include <zToolsO/ApplicationInterface.h>
#include <zUtilO/MediaStore.h>
#include <zUtilO/PortableColor.h>
#include <zAppO/MappingDefines.h>
#include <zParadataO/Logger.h>
#include <zSyncO/BluetoothDeviceInfo.h>
#include <zEngineF/EngineUINodes.h>
#include <zMapping/Geometry.h>

#ifdef _CONSOLE
#include <zEngineF/WindowsApplicationInterface.h>
using BaseApplicationInterfaceParent = WindowsApplicationInterface;
#else
using BaseApplicationInterfaceParent = ApplicationInterface;
#endif

class Application;
class ExceptionHolder;
class FtpConnection;
class HttpConnection;
class IBluetoothAdapter;
struct IMapUI;
class NavigationAddress;
class OAuth2Authorizer;
class OAuth2Token;
class ObjectTransporter;
class TemporaryFile;
class Userbar;
struct UsernamePassword;
class Viewer;

enum class RefreshPageContents { All, Notes };


class BaseApplicationInterface : public BaseApplicationInterfaceParent
{
public:
    virtual ~BaseApplicationInterface() {}

public:
    virtual ObjectTransporter* GetObjectTransporter() = 0;
    virtual void RefreshPage(RefreshPageContents contents) = 0;
    virtual void DisplayErrorMessage(cs::string_view_sz error_message_sv) = 0;
    virtual SharableString DisplayCSHtmlDlg(const NavigationAddress& navigation_address, const std::string* action_invoker_access_token_override, ExceptionHolder* exception_holder) = 0;
    virtual SharableString DisplayHtmlDialogFunctionDlg(const NavigationAddress& navigation_address, const std::string* action_invoker_access_token_override,
                                                        const SharableString& display_options_json, ExceptionHolder* exception_holder) = 0;
    virtual int ShowModalDialog(cs::string_view_sz title_sv, cs::string_view_sz message_sv, int mbType) = 0;
    virtual int ShowMessage(const CString& title, const CString& message, const std::vector<CString>& aButtons) = 0;
    virtual bool GpsOpen() = 0;
    virtual bool GpsClose() = 0;
    virtual CString GpsRead(int waitTime, int accuracy, const std::optional<std::string>& dialog_text) = 0;
    virtual CString GpsReadLast() = 0;
    virtual CString GpsReadInteractive(bool read_interactive_mode, const BaseMapSelection& base_map_selection, const std::optional<std::string>& message, double read_duration) = 0;
    virtual std::optional<UsernamePassword> ShowLoginDialog(const std::string& server, bool show_invalid_error) = 0;
    virtual std::optional<BluetoothDeviceInfo> ChooseBluetoothDevice(const GUID& service_uuid) = 0;
    virtual OAuth2Token OAuth2Authorize(OAuth2Authorizer& oauth2_authorizer) = 0;
    virtual void EngineAbort() = 0;
    virtual bool ExecSystem(const std::wstring& command, bool wait) = 0;
    virtual bool ExecPff(const std::wstring& pff_filename) = 0;
    virtual CString GetProperty(const CString& parameter) = 0;
    virtual void SetProperty(const CString& parameter, const CString& value) = 0;
    virtual void ShowProgressDialog(const std::string& message) = 0;
    virtual void HideProgressDialog() = 0;
    virtual bool UpdateProgressDialog(int progressPercent, const std::string* message) = 0;
    virtual bool PartialSave(bool bPartialSaveClearSkipped, bool bFromLogic) = 0;
    virtual int ShowChoiceDialog(const CString& title, const std::vector<std::vector<CString>*>& data) = 0;
    virtual int ShowShowDialog(const std::vector<CString>* column_titles, const std::vector<PortableColor>* row_text_colors,
        const std::vector<std::vector<CString>*>& data, const CString& heading) = 0;
    virtual int ShowSelcaseDialog(const std::vector<CString>* column_titles, const std::vector<std::vector<CString>*>& data, const CString& heading, std::vector<bool>* selections) = 0;
    virtual std::tuple<int, int> GetMaxDisplaySize() const = 0;
    virtual std::vector<std::string> GetMediaFilePaths(MediaStore::MediaType media_type) const = 0;
    virtual std::string GetUsername() const = 0;
    virtual std::string GetDeviceId() const = 0;
    virtual std::string GetLocaleLanguage() const = 0;
    virtual void StoreCredential(const std::string& attribute, const std::string& secret_value) = 0;
    virtual std::string RetrieveCredential(const std::string& attribute) = 0;
    virtual std::optional<std::string> GetPassword(const std::string& title, const std::string& description, bool file_exists) = 0;
    virtual void ParadataDriverManager(Paradata::PortableMessage msg, const Application* application) = 0;
    virtual void ParadataDeviceInfoQuery(Paradata::ApplicationEvent::DeviceInfo& device_info) = 0;
    virtual void ParadataDeviceStateQuery(Paradata::DeviceStateEvent::DeviceState& device_state) = 0;
    virtual double GetUpTime() = 0;

    virtual std::unique_ptr<IBluetoothAdapter> CreateBluetoothAdapter() = 0;
    virtual std::unique_ptr<HttpConnection> CreateHttpConnection() = 0;
    virtual std::unique_ptr<FtpConnection> CreateFtpConnection() = 0;

    virtual bool AudioPlay(const std::string& file_path, const std::string& message_text) = 0;
    virtual bool AudioStartRecording(const std::string& file_path, std::optional<double> seconds, std::optional<int> sampling_rate) = 0;
    virtual bool AudioStopRecording() = 0;
    virtual std::unique_ptr<TemporaryFile> AudioRecordInteractive(const std::string& message_text, std::optional<int> sampling_rate) = 0;

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
    virtual std::string CreateSharableUri(const std::string& path, bool add_write_permission) = 0;

    // copies a sharable URL to a file
    virtual void FileCopySharableUri(const std::string& sharable_uri, const std::string& destination_path) = 0;
#endif
};


///<summary>Singleton container for platform specific dependencies that are set by platform specific init code and can be used by framework</summary>
class CLASS_DECL_ZTOOLSO PlatformInterface
{
public:
    PlatformInterface();
    virtual ~PlatformInterface() { }

    static PlatformInterface* GetInstance();

    BaseApplicationInterface* GetApplicationInterface()               { return m_pApplicationInterface; }
    void SetApplicationInterface(BaseApplicationInterface* pAppIFace) { m_pApplicationInterface = pAppIFace; }

    const std::string& GetWorkingDirectory() const  { return m_workingDirectory; }
    void SetWorkingDirectory(std::string directory) { m_workingDirectory = std::move(directory); }

    const std::string& GetTempDirectory() const  { return m_tempDirectory.empty() ? m_workingDirectory : m_tempDirectory ; }
    void SetTempDirectory(std::string directory) { m_tempDirectory = std::move(directory); }

    const std::string& GetApplicationDirectory() const  { return m_applicationDirectory; }
    void SetApplicationDirectory(std::string directory) { m_applicationDirectory = std::move(directory); }

    const std::string& GetCSEntryDirectory() const  { return m_csentryDirectory; }
    void SetCSEntryDirectory(std::string directory) { m_csentryDirectory = std::move(directory); }

    const std::string& GetExternalMemoryCardDirectory() const  { return m_externalMemoryCardDirectory; }
    void SetExternalMemoryCardDirectory(std::string directory) { m_externalMemoryCardDirectory = std::move(directory); }

    const std::string& GetInternalStorageDirectory() const  { return m_internalStorageDirectory; }
    void SetInternalStorageDirectory(std::string directory) { m_internalStorageDirectory = std::move(directory); }

    const std::string& GetAssetsDirectory() const  { return m_assetsDirectory; }
    void SetAssetsDirectory(std::string directory) { m_assetsDirectory = std::move(directory); }

    const std::string& GetDownloadsDirectory() const  { return m_downloadsDirectory; }
    void SetDownloadsDirectory(std::string directory) { m_downloadsDirectory = std::move(directory); }

#ifdef ANDROID
    const std::string& GetVersionNumber() const { return m_versionNumber; }
    void SetVersionNumber(std::string version)  { m_versionNumber = std::move(version); }
#endif

private:
    BaseApplicationInterface* m_pApplicationInterface;

    std::string m_workingDirectory;
    std::string m_tempDirectory;
    std::string m_applicationDirectory;
    std::string m_csentryDirectory;
    std::string m_externalMemoryCardDirectory;
    std::string m_internalStorageDirectory;
    std::string m_assetsDirectory;
    std::string m_downloadsDirectory;

#ifdef ANDROID
    std::string m_versionNumber;
#endif
};

#endif
