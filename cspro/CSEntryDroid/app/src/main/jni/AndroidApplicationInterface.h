#pragma once

#include <zPlatformO/PlatformInterface.h>
#include <zPlatformO/PortableMFC.h>
#include <zEngineF/EngineUI.h>
#include <zEntryO/CoreEntryEngineInterface.h>
#include <jni.h>

struct ActionInvokerData;


class AndroidApplicationInterface : public BaseApplicationInterface
{
public:
    AndroidApplicationInterface(CoreEntryEngineInterface* core_interface);

    void OnProgressDialogCancel();
    bool IsProgressDialogCanceled() const;

    // ApplicationInterface overrides
    SharableString BarcodeRead(const std::string& message_text) override;
    bool IsNetworkConnected(bool wifi, bool mobile) override;

    // BaseApplicationInterface overrides
    ObjectTransporter* GetObjectTransporter() override;
    void RefreshPage(RefreshPageContents contents) override;
    void DisplayErrorMessage(cs::string_view_sz error_message_sv) override;
    SharableString DisplayCSHtmlDlg(const NavigationAddress& navigation_address, const std::string* action_invoker_access_token_override, ExceptionHolder* exception_holder) override;
    SharableString DisplayHtmlDialogFunctionDlg(const NavigationAddress& navigation_address, const std::string* action_invoker_access_token_override,
                                                const SharableString& display_options_json, ExceptionHolder* exception_holder) override;
    int ShowModalDialog(cs::string_view_sz title_sv, cs::string_view_sz message_sv, int mbType) override;
    int ShowMessage(const CString& title, const CString& message, const std::vector<CString>& aButtons) override;
    bool GpsOpen() override;
    bool GpsClose() override;
    CString GpsRead(int waitTime, int accuracy, const std::optional<std::string>& dialog_text) override;
    CString GpsReadLast() override;
    CString GpsReadInteractive(bool read_interactive_mode, const BaseMapSelection& base_map_selection, const std::optional<std::string>& message, double read_duration) override;
    std::optional<UsernamePassword> ShowLoginDialog(const std::string& server, bool show_invalid_error) override;
    std::optional<BluetoothDeviceInfo> ChooseBluetoothDevice(const GUID& service_uuid) override;
    OAuth2Token OAuth2Authorize(OAuth2Authorizer& oauth2_authorizer) override;
    std::tuple<int, int> GetMaxDisplaySize() const override;
    std::vector<std::string> GetMediaFilePaths(MediaStore::MediaType media_type) const override;
    std::string GetUsername() const override;
    void SetUsername(std::string username);
    void StoreCredential(const std::string& attribute, const std::string& secret_value) override;
    std::string RetrieveCredential(const std::string& attribute) override;
    std::optional<std::string> GetPassword(const std::string& title, const std::string& description, bool file_exists) override;
    std::string GetDeviceId() const override;
    std::string GetLocaleLanguage() const override;
    void EngineAbort() override;
    bool ExecSystem(const std::string& command, bool wait) override;
    bool ExecPff(const std::string& pff_file_path) override;
    std::string GetProperty(const std::string& parameter) override;
    void SetProperty(const std::string& parameter, const std::string& value) override;
    void ShowProgressDialog(const std::string& message) override;
    void HideProgressDialog() override;
    bool UpdateProgressDialog(int progressPercent, const std::string* message) override;
    bool PartialSave(bool bPartialSaveClearSkipped, bool bFromLogic) override;
    int ShowChoiceDialog(const CString& title, const std::vector<std::vector<CString>*>& data) override;
    int ShowShowDialog(const std::vector<CString>* column_titles, const std::vector<PortableColor>* row_text_colors,
        const std::vector<std::vector<CString>*>& data, const CString& heading) override;
    int ShowSelcaseDialog(const std::vector<CString>* column_titles, const std::vector<std::vector<CString>*>& data, const CString& heading, std::vector<bool>* selections) override;
    void ParadataDriverManager(Paradata::PortableMessage msg, const Application* application) override;
    void ParadataDeviceInfoQuery(Paradata::ApplicationEvent::DeviceInfo& device_info) override;
    void ParadataDeviceStateQuery(Paradata::DeviceStateEvent::DeviceState& device_state) override;
    double GetUpTime() override;

    std::unique_ptr<IBluetoothAdapter> CreateBluetoothAdapter() override;
    std::unique_ptr<HttpConnection> CreateHttpConnection() override;
    std::unique_ptr<FtpConnection> CreateFtpConnection() override;

    void GetParadataCachedEvents();

    bool AudioPlay(const std::string& file_path, const std::string& message_text) override;
    bool AudioStartRecording(const std::string& file_path, std::optional<double> seconds, std::optional<unsigned int> sampling_rate) override;
    bool AudioStopRecording() override;
    std::unique_ptr<TemporaryFile> AudioRecordInteractive(const std::string& message_text, std::optional<unsigned int> sampling_rate) override;

    void CapturePolygonTrace(std::unique_ptr<Geometry::Polygon>& captured_polygon, const Geometry::Polygon* polygon, IMapUI* map) override;
    void CapturePolygonWalk(std::unique_ptr<Geometry::Polygon>& captured_polygon, const Geometry::Polygon* polygon, IMapUI* map) override;

    // for EngineUIProcessor
    LRESULT RunEngineUIProcessor(WPARAM wParam, LPARAM lParam) override;
    bool CaptureImage(EngineUI::CaptureImageNode& capture_image_node) override;
    void CreateMapUI(EngineUI::CreateMapUINode& create_map_ui_node) override;
    void CreateUserbar(std::unique_ptr<Userbar>& userbar) override;
    SharableString EditNote(const SharableString& note, const std::string& title, bool case_note) override;
    bool ExecSystemApp(EngineUI::ExecSystemAppNode& exec_system_app_node) override;
    std::string GetHtmlDialogsDirectory() override;
    void Prompt(EngineUI::PromptNode& options) override;
    bool RunPffExecutor(EngineUI::RunPffExecutorNode& run_pff_executor_node) override;
    bool View(const Viewer& viewer) override;

    // Android-only BaseApplicationInterface overrides
    void MediaScanFiles(const std::vector<CString>& paths) override;
    std::string CreateSharableUri(const std::string& path, bool add_write_permission) override;
    void FileCopySharableUri(const std::string& sharable_uri, const std::string& destination_path) override;

    // for the Action Invoker
    int ActionInvokerCreateWebController(SharableString access_token_override);
    std::shared_ptr<ActionInvokerData> ActionInvokerGetWebController(int caller_id, bool release_web_controller);
    ExceptionHolder* GetTopmostExceptionHolder();

    // other methods
    long GetThreadWaitId();
    void SetThreadWaitComplete(long thread_wait_id, SharableString response);

private:
    SharableString ThreadWaitForComplete(long thread_wait_id);

    void ViewWebPageWithJavaScriptInterface(const Viewer& viewer, const std::string& url);

    OAuth2Token OAuth2Authorize_Dropbox();
    OAuth2Token OAuth2Authorize_GoogleDrive(OAuth2Authorizer& oauth2_authorizer);

private:
    static std::string m_username;

    CoreEntryEngineInterface* m_pCoreEngineInterface;
    EngineUIProcessor m_engineUIProcessor;
    bool m_progressDialogCanceled;

    std::mutex m_threadWaitIdsMutex;
    std::map<long, std::unique_ptr<SharableString>> m_threadWaitIds;

    std::mutex m_actionInvokerWebControllersMutex;
    std::map<int, std::shared_ptr<ActionInvokerData>> m_actionInvokerWebControllers;
    std::vector<ExceptionHolder*> m_exceptionHolders;
};
