#pragma once

#include <zPlatformO/PlatformInterface.h>
#include <zPlatformO/PortableMFC.h>
#include <zEngineF/EngineUI.h>
#include <Zentryo/CoreEntryEngineInterface.h>
#include <jni.h>

struct ActionInvokerData;
struct IBluetoothAdapter;
struct IHttpConnection;
struct IFtpConnection;


class AndroidApplicationInterface : public BaseApplicationInterface
{
public:
    AndroidApplicationInterface(CoreEntryEngineInterface* core_interface);

    void OnProgressDialogCancel();
    bool IsProgressDialogCancelled() const;

    // overrides from BaseAndroidApplicationInterface
    ObjectTransporter* GetObjectTransporter() override;
    void RefreshPage(RefreshPageContents contents) override;
    void DisplayErrorMessage(const TCHAR* error_message) override;
    std::optional<std::wstring> DisplayCSHtmlDlg(const NavigationAddress& navigation_address, const std::wstring* action_invoker_access_token_override) override;
    std::optional<std::wstring> DisplayHtmlDialogFunctionDlg(const NavigationAddress& navigation_address, const std::wstring* action_invoker_access_token_override, const std::optional<std::wstring>& display_options_json) override;
    int ShowModalDialog(NullTerminatedString title, NullTerminatedString message, int mbType) override;
    int ShowMessage(const CString& title, const CString& message, const std::vector<CString>& aButtons) override;
    bool GpsOpen() override;
    bool GpsClose() override;
    CString GpsRead(int waitTime, int accuracy, const CString& dialog_text) override;
    CString GpsReadLast() override;
    CString GpsReadInteractive(bool read_interactive_mode, const BaseMapSelection& base_map_selection, const CString& message, double read_duration) override;
    std::optional<LoginCredentials> ShowLoginDialog(const CString& server, bool show_invalid_error) override;
    std::optional<BluetoothDeviceInfo> ChooseBluetoothDevice(const GUID& service_uuid) override;
    CString AuthorizeDropbox(const CString& clientId) override;
	std::tuple<int, int> GetMaxDisplaySize() const override;
    std::vector<std::wstring> GetMediaFilenames(MediaStore::MediaType media_type) const override;
    CString GetUsername() const override;
    void SetUsername(const CString& username);
    void StoreCredential(const std::wstring& attribute, const std::wstring& secret_value) override;
    std::wstring RetrieveCredential(const std::wstring& attribute) override;
    std::optional<std::wstring> GetPassword(const std::wstring& title, const std::wstring& description, bool file_exists) override;
    CString GetDeviceId() const override;
    CString GetLocaleLanguage() const override;
    void EngineAbort() override;
    bool ExecSystem(const std::wstring& command, bool wait) override;
    bool ExecPff(const std::wstring& pff_filename) override;
    CString GetProperty(const CString& parameter) override;
    void SetProperty(const CString& parameter, const CString& value) override;
    void ShowProgressDialog(const CString& message) override;
    void HideProgressDialog() override;
    bool UpdateProgressDialog(int progressPercent, const CString* message) override;
    bool PartialSave(bool bPartialSaveClearSkipped, bool bFromLogic) override;
    int ShowChoiceDialog(const CString& title, const std::vector<std::vector<CString>*>& data) override;
    int ShowShowDialog(const std::vector<CString>* column_titles, const std::vector<PortableColor>* row_text_colors,
        const std::vector<std::vector<CString>*>& data, const CString& heading) override;
    int ShowSelcaseDialog(const std::vector<CString>* column_titles, const std::vector<std::vector<CString>*>& data, const CString& heading, std::vector<bool>* selections) override;
    void ParadataDriverManager(Paradata::PortableMessage msg, const Application* application) override;
    void ParadataDeviceInfoQuery(Paradata::ApplicationEvent::DeviceInfo& device_info) override;
    void ParadataDeviceStateQuery(Paradata::DeviceStateEvent::DeviceState& device_state) override;
    double GetUpTime() override;
    bool IsNetworkConnected(int connectionType) override;
    IBluetoothAdapter* CreateAndroidBluetoothAdapter() override;
    IHttpConnection* CreateAndroidHttpConnection() override;
    IFtpConnection* CreateAndroidFtpConnection() override;
    std::wstring BarcodeRead(const std::wstring& message_text) override;
    void GetParadataCachedEvents();

    bool AudioPlay(const std::wstring& filename, const std::wstring& message_text) override;
    bool AudioStartRecording(const std::wstring& filename, std::optional<double> seconds, std::optional<int> sampling_rate) override;
    bool AudioStopRecording() override;
    std::unique_ptr<TemporaryFile> AudioRecordInteractive(const std::wstring& message_text, std::optional<int> sampling_rate) override;

    void CapturePolygonTrace(std::unique_ptr<Geometry::Polygon>& captured_polygon, const Geometry::Polygon* polygon, IMapUI* map) override;
    void CapturePolygonWalk(std::unique_ptr<Geometry::Polygon>& captured_polygon, const Geometry::Polygon* polygon, IMapUI* map) override;

    // for EngineUIProcessor
    long RunEngineUIProcessor(WPARAM wParam, LPARAM lParam) override;
    bool CaptureImage(EngineUI::CaptureImageNode& capture_image_node) override;
    void CreateMapUI(std::unique_ptr<IMapUI>& map_ui) override;
    void CreateUserbar(std::unique_ptr<Userbar>& userbar) override;
    CString EditNote(const CString& note, const CString& title, bool case_note) override;
    bool ExecSystemApp(EngineUI::ExecSystemAppNode& exec_system_app_node) override;
    std::wstring GetHtmlDialogsDirectory() override;
    void Prompt(EngineUI::PromptNode& options) override;
    bool RunPffExecutor(EngineUI::RunPffExecutorNode& run_pff_executor_node) override;
    long View(const Viewer& viewer) override;

    // Android-only BaseApplicationInterface overrides
    void MediaScanFiles(const std::vector<CString>& paths) override;
    std::wstring CreateSharableUri(const std::wstring& path, bool add_write_permission) override;
    void FileCopySharableUri(const std::wstring& sharable_uri, const std::wstring& destination_path) override;

    // for the Action Invoker
    int ActionInvokerCreateWebController(const std::wstring* access_token_override);
    std::shared_ptr<ActionInvokerData> ActionInvokerGetWebController(int web_controller_key, bool release_web_controller);

    // other methods
    long GetThreadWaitId();
    void SetThreadWaitComplete(long thread_wait_id, std::optional<std::wstring> response);

private:
    std::optional<std::wstring> ThreadWaitForComplete(long thread_wait_id);

    void ViewWebPageWithJavaScriptInterface(const Viewer& viewer, wstring_view url_sv);

private:
    static CString m_username;

    CoreEntryEngineInterface* m_pCoreEngineInterface;
    EngineUIProcessor m_engineUIProcessor;
    bool m_progressDialogCancelled;

    std::mutex m_threadWaitIdsMutex;
    std::map<long, std::unique_ptr<std::optional<std::wstring>>> m_threadWaitIds;

    std::mutex m_actionInvokerWebControllersMutex;
    std::map<int, std::shared_ptr<ActionInvokerData>> m_actionInvokerWebControllers;
};
