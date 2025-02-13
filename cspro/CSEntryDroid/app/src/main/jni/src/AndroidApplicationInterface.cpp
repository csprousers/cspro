#include <engine/StandardSystemIncludes.h>
#include "AndroidApplicationInterface.h"
#include "AndroidBluetoothAdapter.h"
#include "AndroidFtpConnection.h"
#include "AndroidHttpConnection.h"
#include "AndroidMapUI.h"
#include "AndroidUserbar.h"
#include "GeometryJni.h"
#include "JNIHelpers.h"

// NDK Includes
#include <android/log.h>
#include <sys/stat.h>

// Project Includes
#include <zPlatformO/PlatformInterface.h>
#include <zToolsO/Tools.h>
#include <zUtilO/AppLdr.h>
#include <zUtilO/imsaStr.h>
#include <zUtilO/MemoryHelpers.h>
#include <zUtilO/TemporaryFile.h>
#include <zUtilO/Viewers.h>
#include <zHtml/PortableLocalhost.h>
#include <zHtml/NavigationAddress.h>
#include <zHtml/WebViewSyncOperationMarker.h>
#include <zAppO/Application.h>
#include <zAppO/Properties/ApplicationProperties.h>
#include <zDictO/DDClass.h>
#include <ZBRIDGEO/npff.h>
#include <Zentryo/Runaple.h>
#include <engine/IntDrive.h>
#include <zEngineO/PffExecutor.h>
#include <zEngineO/SystemApp.h>
#include <zMessageO/Messages.h>
#include <Zentryo/CoreEntryEngineInterface.h>
#include <Zentryo/CoreEntryPage.h>
#include <Zentryo/CaseTreeNode.h>
#include <Zentryo/CaseTreeUpdate.h>
#include <zSyncO/BluetoothDeviceInfo.h>
#include <zParadataO/Logger.h>

#include <mapbox/geometry.hpp>
#include <typeinfo>

CString AndroidApplicationInterface::m_username;

namespace {

    bool SplitNewlineSeparatedPair(const CString& pair, CString& first, CString& second)
    {
        int iNewline = pair.Find(_T('\n'));
        if (iNewline > 0) {
            first = pair.Left(iNewline);
            second = pair.Mid(iNewline + 1);
            return true;
        } else {
            first = CString();
            second = CString();
            return false;
        }
    }
}

AndroidApplicationInterface::AndroidApplicationInterface(CoreEntryEngineInterface* core_interface)
    :   m_pCoreEngineInterface(core_interface),
        m_engineUIProcessor(*this),
        m_progressDialogCancelled(false)
{
}


ObjectTransporter* AndroidApplicationInterface::GetObjectTransporter()
{
    return ( m_pCoreEngineInterface != nullptr ) ? m_pCoreEngineInterface->GetObjectTransporter() :
                                                   nullptr;
}


void AndroidApplicationInterface::RefreshPage(RefreshPageContents contents)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    if( contents == RefreshPageContents::All || contents == RefreshPageContents::Notes )
        pEnv->CallStaticVoidMethod(JNIReferences::classApplicationInterface, JNIReferences::methodApplicationInterfaceRefreshNotes);
}


void AndroidApplicationInterface::DisplayErrorMessage(const TCHAR* error_message)
{
    ShowModalDialog(_T(""), error_message, MB_OK);
}


std::optional<std::wstring> AndroidApplicationInterface::DisplayCSHtmlDlg(const NavigationAddress& navigation_address,
                                                                          const std::wstring* const action_invoker_access_token_override)
{
    // action_invoker_access_token_override is null when the HTML dialog is not in the assets folder, meaning that it is a custom
    // dialog, which may not properly be using access tokens; this method relies on the dialog not calling back into JNI to display
    // any UI, but that may not be the case for a custom dialog (if nothing else, ShowModalDialog may display a message asking the user
    // to allow access to the Action Invoker); so without an access token, use the code path used by htmlDialog
    if( action_invoker_access_token_override == nullptr )
        return DisplayHtmlDialogFunctionDlg(navigation_address, action_invoker_access_token_override, std::nullopt);

    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    ASSERT(navigation_address.IsHtmlFilename());
    const std::wstring filename_url = PortableLocalhost::CreateFilenameUrl(navigation_address.GetHtmlFilename());
    JNIReferences::scoped_local_ref<jstring> jUrl(pEnv, WideToJava(pEnv, filename_url));

    JNIReferences::scoped_local_ref<jstring> jActionInvokerAccessTokenOverride(pEnv);

    if( action_invoker_access_token_override != nullptr )
        jActionInvokerAccessTokenOverride.reset(WideToJava(pEnv, *action_invoker_access_token_override));

    WebViewSyncOperationMarker::DisplayErrorIfOperationInProcess();

    if( WebViewSyncOperationMarker::IsOperationInProgress() )
    {
        return std::nullopt; // HTML_TODO why is this necessary to keep our dialogs from locking the JavaScript?
    }

    jstring jJsonResultsText = (jstring)pEnv->CallStaticObjectMethod(
        JNIReferences::classApplicationInterface,
        JNIReferences::methodApplicationInterfaceDisplayCSHtmlDlg,
        jUrl.get(),
        jActionInvokerAccessTokenOverride.get());

    return JavaToOptionalWSZ(pEnv, jJsonResultsText);
}


std::optional<std::wstring> AndroidApplicationInterface::DisplayHtmlDialogFunctionDlg(const NavigationAddress& navigation_address,
                                                                                      const std::wstring* const action_invoker_access_token_override,
                                                                                      const std::optional<std::wstring>& display_options_json)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    ASSERT(navigation_address.IsHtmlFilename());
    std::wstring filename_url = PortableLocalhost::CreateFilenameUrl(navigation_address.GetHtmlFilename());
    JNIReferences::scoped_local_ref<jstring> jUrl(pEnv, WideToJava(pEnv, filename_url));

    JNIReferences::scoped_local_ref<jstring> jActionInvokerAccessTokenOverride(pEnv);

    if( action_invoker_access_token_override != nullptr )
        jActionInvokerAccessTokenOverride.reset(WideToJava(pEnv, *action_invoker_access_token_override));

    JNIReferences::scoped_local_ref<jstring> jDisplayOptionsJson(pEnv, OptionalWideToJava(pEnv, display_options_json));

    WebViewSyncOperationMarker::DisplayErrorIfOperationInProcess();

    long thread_wait_id = pEnv->CallStaticLongMethod(
        JNIReferences::classApplicationInterface,
        JNIReferences::methodApplicationInterfaceDisplayHtmlDialogFunctionDlg,
        jUrl.get(),
        jActionInvokerAccessTokenOverride.get(),
        jDisplayOptionsJson.get());

    // return if there was an error launching the HTML dialog
    if( thread_wait_id < 0 )
        return _T("");

    return ThreadWaitForComplete(thread_wait_id);
}


int AndroidApplicationInterface::ShowModalDialog(const NullTerminatedString title, const NullTerminatedString message, const int mbType)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    JNIReferences::scoped_local_ref<jstring> jTitle(pEnv, WideToJava(pEnv, title));
    JNIReferences::scoped_local_ref<jstring> jMessage(pEnv, WideToJava(pEnv, message));

    // call the function
    const int result = pEnv->CallStaticIntMethod(JNIReferences::classApplicationInterface,
                                                 JNIReferences::methodApplicationInterfaceShowModalDialog,
                                                 jTitle.get(), jMessage.get(), mbType);
    __android_log_print(ANDROID_LOG_INFO, "APPLIFACE", "ShowModalDialog - Returned Button ID: %d", result);

    // Translate from Android button code to Win32 codes
    // On Android positive button (yes/ok) is -1 and negative button (no/cancel) is -2.
    if( mbType == MB_YESNO )
    {
        return ( result == -1 ) ? IDYES : IDNO;
    }

    else
    {
        ASSERT(mbType == MB_OK || mbType == MB_OKCANCEL);
        return ( result == -1 ? IDOK : IDCANCEL );
    }
}


int AndroidApplicationInterface::ShowMessage(const CString& title, const CString& message,
                                             const std::vector<CString> &buttons)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    jstring errmsgTitle = WideToJava(pEnv, title);
    jstring errmsgMessage = WideToJava(pEnv, message);

    jobjectArray stringArray = pEnv->NewObjectArray(buttons.size(), JNIReferences::classString, pEnv->NewStringUTF(""));

    for( int i = 0; i < buttons.size(); i++ )
    {
        jstring jButtonText = WideToJava(pEnv, buttons[i]);
        pEnv->SetObjectArrayElement(stringArray, i, jButtonText);
        pEnv->DeleteLocalRef(jButtonText);
    }

    return pEnv->CallStaticLongMethod(JNIReferences::classApplicationInterface, JNIReferences::methodApplicationInterfaceErrmsg, errmsgTitle, errmsgMessage, stringArray);
}

std::tuple<int, int> AndroidApplicationInterface::GetMaxDisplaySize() const
{
    auto env = GetJNIEnvForCurrentThread();

    auto get_dimension = [&](bool width) -> int
    {
        return (int)env->CallStaticIntMethod(JNIReferences::classApplicationInterface,
                                             JNIReferences::methodApplicationInterfaceGetMaxDisplaySize, width);
    };

	return std::make_tuple(get_dimension(true), get_dimension(false));
}


std::vector<std::wstring> AndroidApplicationInterface::GetMediaFilenames(MediaStore::MediaType media_type) const
{
    auto pEnv = GetJNIEnvForCurrentThread();

    std::vector<std::wstring> media_filenames;

    jobject jMediaFilenamesList = pEnv->CallStaticObjectMethod(JNIReferences::classApplicationInterface,
        JNIReferences::methodApplicationInterfaceGetMediaFilenames, (int)media_type);

    if( jMediaFilenamesList != nullptr )
    {
        int number_filenames = pEnv->CallIntMethod(jMediaFilenamesList, JNIReferences::methodListSize);

        for( int i = 0; i < number_filenames; ++i )
        {
            auto jFilename = (jstring)pEnv->CallObjectMethod(jMediaFilenamesList, JNIReferences::methodListGet, i);
            media_filenames.emplace_back(JavaToWSZ(pEnv, jFilename));
            pEnv->DeleteLocalRef(jFilename);
        }

        pEnv->DeleteLocalRef(jMediaFilenamesList);
    }

    return media_filenames;
}


CString AndroidApplicationInterface::GetUsername() const
{
    return m_username;
}

IBluetoothAdapter* AndroidApplicationInterface::CreateAndroidBluetoothAdapter()
{
    return AndroidBluetoothAdapter::create();
}

IHttpConnection* AndroidApplicationInterface::CreateAndroidHttpConnection()
{
    return new AndroidHttpConnection();
}

IFtpConnection* AndroidApplicationInterface::CreateAndroidFtpConnection()
{
    return new AndroidFtpConnection();
}

void AndroidApplicationInterface::CreateMapUI(std::unique_ptr<IMapUI>& map_ui)
{
    map_ui = std::make_unique<AndroidMapUI>();
}

void AndroidApplicationInterface::EngineAbort()
{
    //Call Java layer and put up an Alert for engine terminating message.
    ShowModalDialog(MGF::GetMessageText(MGF::AbortTitle), MGF::GetMessageText(MGF::ErrorEngine), MB_OK);
}


bool AndroidApplicationInterface::ExecSystem(const std::wstring& command, bool wait)
{
    auto env = GetJNIEnvForCurrentThread();
    return (bool)env->CallStaticLongMethod(JNIReferences::classApplicationInterface,
                                           JNIReferences::methodApplicationInterfaceExecsystem,
                                           WideToJava(env, command),
                                           wait);
}


int AndroidApplicationInterface::ShowChoiceDialog(const CString& title, const std::vector<std::vector<CString>*>& data)
{
    auto pEnv = GetJNIEnvForCurrentThread();
    JNIReferences::scoped_local_ref<jstring> jDialogTitle(pEnv, WideToJava(pEnv, title));

    JNIReferences::scoped_local_ref<jobjectArray> jStringArray(pEnv, pEnv->NewObjectArray(data.size(),JNIReferences::classString,pEnv->NewStringUTF("")));

    for( int i = 0; i < data.size(); i++ )
    {
        CString option = data.at(i)->at(0);
        option.TrimRight();
        JNIReferences::scoped_local_ref<jstring> jOption(pEnv, WideToJava(pEnv, option));

        pEnv->SetObjectArrayElement(jStringArray.get(),i,jOption.get());
    }

    return (int) pEnv->CallStaticLongMethod(JNIReferences::classApplicationInterface,JNIReferences::methodApplicationInterfaceChoiceDialog, jDialogTitle.get(), jStringArray.get());
}


CString AndroidApplicationInterface::EditNote(const CString& note, const CString& title, bool case_note)
{
    auto env = GetJNIEnvForCurrentThread();

    JNIReferences::scoped_local_ref<jstring> jFieldNote(env, WideToJava(env, note));
    JNIReferences::scoped_local_ref<jstring> jDialogTitle(env, WideToJava(env, title));

    JNIReferences::scoped_local_ref<jstring> newNote(env, (jstring)env->CallStaticObjectMethod(JNIReferences::classApplicationInterface,JNIReferences::methodApplicationInterfaceEditnote,
                                                                                               jFieldNote.get(), jDialogTitle.get(), case_note));

    if( newNote.get() != nullptr ) {
        return JavaToWSZ(env, newNote.get());
    }
    else {
        return note;
    }
}


void AndroidApplicationInterface::CreateUserbar(std::unique_ptr<Userbar>& userbar)
{
    userbar = std::make_unique<AndroidUserbar>();
}


int AndroidApplicationInterface::ShowShowDialog(const std::vector<CString>* column_titles, const std::vector<PortableColor>* row_text_colors,
    const std::vector<std::vector<CString>*>& data, const CString& heading)
{
    auto pEnv = GetJNIEnvForCurrentThread();
    int iNumColumns = data.at(0)->size();

    JNIReferences::scoped_local_ref<jobjectArray> jHeadersArray(pEnv, pEnv->NewObjectArray(iNumColumns,
        JNIReferences::classString, pEnv->NewStringUTF("")));

    if( column_titles != nullptr ) // the columns can be NULL in showarray calls
    {
        for( int i = 0; i < column_titles->size(); i++ )
        {
            JNIReferences::scoped_local_ref<jstring> jHeaderText(pEnv, WideToJava(pEnv,column_titles->at(i)));
            pEnv->SetObjectArrayElement(jHeadersArray.get(),i,jHeaderText.get());
        }
    }


    JNIReferences::scoped_local_ref<jintArray> jrowTextColorsArray(pEnv);

    if( row_text_colors != nullptr )
    {
        jrowTextColorsArray.reset(pEnv->NewIntArray(row_text_colors->size()));

        for( size_t i = 0; i < row_text_colors->size(); ++i )
        {
            int colorint = row_text_colors->at(i).ToColorInt();
            pEnv->SetIntArrayRegion(jrowTextColorsArray.get(), i, 1, &colorint);
        }
    }


    JNIReferences::scoped_local_ref<jobjectArray> jLinesArray(pEnv, pEnv->NewObjectArray(data.size() * iNumColumns,
        JNIReferences::classString, pEnv->NewStringUTF("")));

    int iArrayItr = 0;

    for( int i = 0; i < data.size(); i++ )
    {
        for( int j = 0; j < data.at(i)->size(); j++ )
        {
            JNIReferences::scoped_local_ref<jstring> jLineText(pEnv, WideToJava(pEnv,data.at(i)->at(j)));
            pEnv->SetObjectArrayElement(jLinesArray.get(),iArrayItr,jLineText.get());
            iArrayItr++;
        }
    }


    JNIReferences::scoped_local_ref<jstring> jHeadingText(pEnv, WideToJava(pEnv,heading));

    return (int) pEnv->CallStaticLongMethod(JNIReferences::classApplicationInterface, JNIReferences::methodApplicationInterfaceShow,
        jHeadersArray.get(), jrowTextColorsArray.get(), jLinesArray.get(), jHeadingText.get());
}

int AndroidApplicationInterface::ShowSelcaseDialog(const std::vector<CString>* column_titles, const std::vector<std::vector<CString>*>& data, const CString& heading, std::vector<bool>* selections)
{
    auto pEnv = GetJNIEnvForCurrentThread();

    JNIReferences::scoped_local_ref<jobjectArray> jHeadersArray(pEnv, pEnv->NewObjectArray(column_titles->size(),JNIReferences::classString,pEnv->NewStringUTF("")));

    for( int i = 0; i < column_titles->size(); i++ )
    {
        JNIReferences::scoped_local_ref<jstring> jHeaderText(pEnv, WideToJava(pEnv,column_titles->at(i)));
        pEnv->SetObjectArrayElement(jHeadersArray.get(), i, jHeaderText.get());
    }

    JNIReferences::scoped_local_ref<jobjectArray> jLinesArray(pEnv, pEnv->NewObjectArray(data.size() * data.at(0)->size(),JNIReferences::classString,pEnv->NewStringUTF("")));

    int iArrayItr = 0;

    for( int i = 0; i < data.size(); i++ )
    {
        for( int j = 0; j < data.at(i)->size(); j++ )
        {
            JNIReferences::scoped_local_ref<jstring> jLineText(pEnv, WideToJava(pEnv,data.at(i)->at(j)));
            pEnv->SetObjectArrayElement(jLinesArray.get(),iArrayItr,jLineText.get());
            iArrayItr++;
        }
    }

    int iRet = 0;
    bool bMultipleSelect = ( selections != nullptr );

    JNIReferences::scoped_local_ref<jstring> jHeading(pEnv, WideToJava(pEnv, heading));

    auto retVal = (jstring)pEnv->CallStaticObjectMethod(JNIReferences::classApplicationInterface,JNIReferences::methodApplicationInterfaceSelect,jHeadersArray.get(),jLinesArray.get(),jHeading.get(),bMultipleSelect);

    if( retVal != nullptr )
    {
        CString csSelectedValues = JavaToWSZ(pEnv,retVal);

        if( !bMultipleSelect ) // single selection
        {
            ASSERT(csSelectedValues.GetLength() == 1);
            iRet = csSelectedValues[0];
        }

        else
        {
            for( int i = 0; i < csSelectedValues.GetLength(); i++ ) // each character of the string will correspond to a selected value (minus one)
                selections->at(csSelectedValues[i] - 1) = true;

            iRet = 1;
        }

        pEnv->DeleteLocalRef(retVal);
    }

    return iRet;
}


bool AndroidApplicationInterface::ExecPff(const std::wstring& pff_filename)
{
    auto pEnv = GetJNIEnvForCurrentThread();
    JNIReferences::scoped_local_ref<jstring> jpff(pEnv, WideToJava(pEnv, pff_filename));
    return pEnv->CallStaticBooleanMethod(JNIReferences::classApplicationInterface, JNIReferences::methodApplicationInterfaceExecPFF, jpff.get());
}


bool AndroidApplicationInterface::ExecSystemApp(EngineUI::ExecSystemAppNode& exec_system_app_node)
{
    auto pEnv = GetJNIEnvForCurrentThread();

    JNIReferences::scoped_local_ref<jstring> jpackage(pEnv, WideToJava(pEnv, exec_system_app_node.package_name));
    JNIReferences::scoped_local_ref<jstring> jactivity(pEnv, exec_system_app_node.activity_name ? WideToJava(pEnv, *exec_system_app_node.activity_name) : nullptr );

    JNIReferences::scoped_local_ref<jobject> jarguments(pEnv, pEnv->NewObject(JNIReferences::classBundle, JNIReferences::methodBundleConstructor));

    for( const SystemApp::Argument& argument : exec_system_app_node.system_app.GetArguments() )
    {
        JNIReferences::scoped_local_ref<jstring> jarg_name(pEnv, WideToJava(pEnv, argument.name));

        if( !argument.value.has_value() ) {
            pEnv->CallObjectMethod(jarguments.get(), JNIReferences::methodBundlePutString, jarg_name.get(),
                                   nullptr);
        } else if( std::holds_alternative<std::wstring>(*argument.value) ) {
            JNIReferences::scoped_local_ref<jstring> jarg_value(pEnv, WideToJava(pEnv, std::get<std::wstring>(*argument.value)));
            pEnv->CallVoidMethod(jarguments.get(), JNIReferences::methodBundlePutString, jarg_name.get(),
                                   jarg_value.get());
        }
        else {
            pEnv->CallVoidMethod(jarguments.get(), JNIReferences::methodBundlePutDouble, jarg_name.get(), std::get<double>(*argument.value));
        }
    }

    JNIReferences::scoped_local_ref<jobject> jresult(pEnv, pEnv->CallStaticObjectMethod(JNIReferences::classApplicationInterface,
        JNIReferences::methodApplicationInterfaceRunSystemApp,
        jpackage.get(),
        jactivity.get(),
        jarguments.get()));

    if (jresult.get() == nullptr)
        return false;

    std::map<std::wstring, std::wstring> bundle_extras = JavaBundleToMap(pEnv, jresult.get());
    for (const auto& [key, value]: bundle_extras) {
        exec_system_app_node.system_app.SetResult(key, value);
    }

    return true;
}


std::wstring AndroidApplicationInterface::GetHtmlDialogsDirectory()
{
    return CS2WS(m_pCoreEngineInterface->GetPifFile()->GetHtmlDialogsDirectory());
}


bool AndroidApplicationInterface::PartialSave(bool bPartialSaveClearSkipped, bool bFromLogic)
{
    return m_pCoreEngineInterface->PartialSave(bPartialSaveClearSkipped, bFromLogic);
}

CString AndroidApplicationInterface::GetDeviceId() const
{
    auto env = GetJNIEnvForCurrentThread();

    JNIReferences::scoped_local_ref<jstring> retVal(env, (jstring) env->CallStaticObjectMethod(JNIReferences::classApplicationInterface,JNIReferences::methodApplicationInterfaceGetDeviceID));
    return JavaToWSZ(env,retVal.get());
}

bool AndroidApplicationInterface::IsNetworkConnected(int connectionType)
{
    auto pEnv = GetJNIEnvForCurrentThread();
    return pEnv->CallStaticBooleanMethod(JNIReferences::classApplicationInterface,JNIReferences::methodApplicationInterfaceIsNetworkConnected,connectionType) != 0;
}

CString AndroidApplicationInterface::GetProperty(const CString& parameter)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    JNIReferences::scoped_local_ref<jstring> jparameter(pEnv, WideToJava(pEnv, parameter));

    JNIReferences::scoped_local_ref<jstring> jresult(pEnv, (jstring)pEnv->CallStaticObjectMethod(JNIReferences::classApplicationInterface,JNIReferences::methodApplicationInterfaceGetProperty, jparameter.get()));

    CString result;
    if (jresult.get() != nullptr)
        result = JavaToWSZ(pEnv, jresult.get());

    return result;
}

void AndroidApplicationInterface::SetProperty(const CString& parameter, const CString& value)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    JNIReferences::scoped_local_ref<jstring> jparameter(pEnv, WideToJava(pEnv, parameter));
    JNIReferences::scoped_local_ref<jstring> jvalue(pEnv, WideToJava(pEnv, value));

    pEnv->CallStaticVoidMethod(JNIReferences::classApplicationInterface,JNIReferences::methodApplicationInterfaceSetProperty, jparameter.get(), jvalue.get());
}

void AndroidApplicationInterface::ShowProgressDialog(const CString& message)
{
    auto pEnv = GetJNIEnvForCurrentThread();

    m_progressDialogCancelled = false;

    jstring dialogText = WideToJava(pEnv,message);

    pEnv->CallStaticVoidMethod(JNIReferences::classApplicationInterface,
            JNIReferences::methodApplicationInterfaceShowProgressDialog,
            dialogText);

    pEnv->DeleteLocalRef(dialogText);
}

void AndroidApplicationInterface::HideProgressDialog()
{
    auto pEnv = GetJNIEnvForCurrentThread();

    pEnv->CallStaticVoidMethod(JNIReferences::classApplicationInterface,
            JNIReferences::methodApplicationInterfaceHideProgressDialog);
}

bool AndroidApplicationInterface::UpdateProgressDialog(int progressPercent, const CString* message)
{
    if (!m_progressDialogCancelled) {
        auto pEnv = GetJNIEnvForCurrentThread();

        jstring dialogText = message ? WideToJava(pEnv, *message) : nullptr;

        if (dialogText != nullptr || progressPercent >= -1) // Don't call for -2 (NO_PROGRESS_UPDATE)
            pEnv->CallStaticVoidMethod(JNIReferences::classApplicationInterface,
                    JNIReferences::methodApplicationInterfaceUpdateProgressDialog,
                    progressPercent, dialogText);

        pEnv->DeleteLocalRef(dialogText);
    }

    return m_progressDialogCancelled;
}

std::optional<BluetoothDeviceInfo> AndroidApplicationInterface::ChooseBluetoothDevice(const GUID& service_uuid)
{
    auto pEnv = GetJNIEnvForCurrentThread();
    auto retVal = (jstring)pEnv->CallStaticObjectMethod(JNIReferences::classApplicationInterface, JNIReferences::methodApplicationInterfaceChooseBluetoothDevice);
    CString nameAndAddress = JavaToWSZ(pEnv, retVal);

    // Name and address separated by newline
    BluetoothDeviceInfo device_info;
    if (SplitNewlineSeparatedPair(nameAndAddress, device_info.csName, device_info.csAddress))
        return device_info;
    else
        return {};
}

CString AndroidApplicationInterface::AuthorizeDropbox(const CString& clientId)
{
    auto pEnv = GetJNIEnvForCurrentThread();
    auto retVal = (jstring) pEnv->CallStaticObjectMethod(JNIReferences::classApplicationInterface, JNIReferences::methodApplicationInterfaceAuthorizeDropbox);
    return JavaToWSZ(pEnv, retVal);
}

std::optional<BaseApplicationInterface::LoginCredentials> AndroidApplicationInterface::ShowLoginDialog(const CString& server, bool show_invalid_error)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    auto jserver =  WideToJava(pEnv, server);

    auto retVal = (jstring) pEnv->CallStaticObjectMethod(JNIReferences::classApplicationInterface, JNIReferences::methodApplicationInterfaceLoginDialog, jserver, show_invalid_error);
    // Username and password separated by newline
    CString usernameAndPassword = JavaToWSZ(pEnv, retVal);
    LoginCredentials credentials;
    if (SplitNewlineSeparatedPair(usernameAndPassword, credentials.username, credentials.password))
        return credentials;
    else
        return {};
}


void AndroidApplicationInterface::StoreCredential(const std::wstring& attribute, const std::wstring& secret_value)
{
    auto env = GetJNIEnvForCurrentThread();
    JNIReferences::scoped_local_ref<jstring> jAttribute(env, WideToJava(env, attribute));
    JNIReferences::scoped_local_ref<jstring> jSecretValue(env, WideToJava(env, secret_value));

    env->CallStaticVoidMethod(JNIReferences::classApplicationInterface, JNIReferences::methodApplicationInterfaceStoreCredential, jAttribute.get(), jSecretValue.get());
}


std::wstring AndroidApplicationInterface::RetrieveCredential(const std::wstring& attribute)
{
    auto env = GetJNIEnvForCurrentThread();

    JNIReferences::scoped_local_ref<jstring> jAttribute(env, WideToJava(env, attribute));

    JNIReferences::scoped_local_ref<jstring> retVal(env, (jstring)env->CallStaticObjectMethod(JNIReferences::classApplicationInterface, JNIReferences::methodApplicationInterfaceRetrieveCredential, jAttribute.get()));

    return JavaToWSZ(env, retVal.get());
}


std::optional<std::wstring> AndroidApplicationInterface::GetPassword(const std::wstring& title, const std::wstring& description, bool file_exists)
{
    auto env = GetJNIEnvForCurrentThread();

    JNIReferences::scoped_local_ref<jstring> jTitle(env, WideToJava(env, title));
    JNIReferences::scoped_local_ref<jstring> jDescription(env, WideToJava(env, description));

    JNIReferences::scoped_local_ref<jstring> jPassword(env, (jstring)env->CallStaticObjectMethod(JNIReferences::classApplicationInterface,
        JNIReferences::methodApplicationInterfaceGetPassword, jTitle.get(), jDescription.get(), file_exists));

    return JavaToOptionalWSZ(env, jPassword.get());
}


void AndroidApplicationInterface::OnProgressDialogCancel()
{
    __android_log_print(ANDROID_LOG_INFO, "AndroidApplicationInterface", "OnProgressDialogCancel");
    m_progressDialogCancelled = true;
}

bool AndroidApplicationInterface::IsProgressDialogCancelled() const
{
    return m_progressDialogCancelled;
}

void AndroidApplicationInterface::ParadataDriverManager(Paradata::PortableMessage eMessage, const Application* application)
{
    auto env = GetJNIEnvForCurrentThread();

    if( eMessage == Paradata::PortableMessage::QueryCachedEvents )
    {
        GetParadataCachedEvents();
    }

    else
    {
        int iExtra = 0;

        if( eMessage == Paradata::PortableMessage::UpdateBackgroundCollectionParameters )
        {
            iExtra = application->GetApplicationProperties().GetParadataProperties().GetGpsLocationIntervalMinutes();
        }

        env->CallStaticObjectMethod(JNIReferences::classApplicationInterface,
            JNIReferences::methodApplicationInterfaceParadataDriverManager, (int)eMessage, iExtra);
    }
}

void AndroidApplicationInterface::GetParadataCachedEvents()
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    std::vector<CString> aEventStrings;

    jobject object = pEnv->CallStaticObjectMethod(JNIReferences::classApplicationInterface,
        JNIReferences::methodApplicationInterfaceParadataDriverManager,(int)Paradata::PortableMessage::QueryCachedEvents,0);

    if( object != nullptr )
    {
        int iEvents = pEnv->CallIntMethod(object,JNIReferences::methodListSize);

        for( int i = 0; i < iEvents; i++ )
        {
            auto jEvent = (jstring)pEnv->CallObjectMethod(object,JNIReferences::methodListGet,i);
            aEventStrings.emplace_back(JavaToWSZ(pEnv,jEvent));
            pEnv->DeleteLocalRef(jEvent);
        }

        pEnv->DeleteLocalRef(object);

        m_pCoreEngineInterface->ProcessParadataCachedEvents(aEventStrings);
    }
}

void AndroidApplicationInterface::ParadataDeviceInfoQuery(Paradata::ApplicationEvent::DeviceInfo& device_info)
{
    auto env = GetJNIEnvForCurrentThread();

    const int iValuesToFill = Paradata::ApplicationEvent::DeviceInfo::ValuesToFill;

    JNIReferences::scoped_local_ref<jobjectArray> valuesArray(env, env->NewObjectArray(iValuesToFill,JNIReferences::classString,env->NewStringUTF("")));
    env->CallStaticVoidMethod(JNIReferences::classApplicationInterface,JNIReferences::methodApplicationInterfaceParadataDeviceQuery,1,valuesArray.get());

    for( int i = 0; i < iValuesToFill; ++i ) {
        CString* pcsValue =
        ( i == 0 ) ? &device_info.screen_width :
        ( i == 1 ) ? &device_info.screen_height :
        ( i == 2 ) ? &device_info.screen_inches :
        ( i == 3 ) ? &device_info.memory_ram :
        ( i == 4 ) ? &device_info.battery_capacity :
        ( i == 5 ) ? &device_info.device_brand :
        ( i == 6 ) ? &device_info.device_device :
        ( i == 7 ) ? &device_info.device_hardware :
        ( i == 8 ) ? &device_info.device_manufacturer :
        ( i == 9 ) ? &device_info.device_model :
        ( i == 10 ) ? &device_info.device_processor :
        ( i == 11 ) ? &device_info.device_product :
        nullptr;

        ASSERT(pcsValue != nullptr);

        JNIReferences::scoped_local_ref<jstring> jval(env, (jstring) env->GetObjectArrayElement(valuesArray.get(),i));
        *pcsValue = JavaToWSZ(env, jval.get());
    }
}

void AndroidApplicationInterface::ParadataDeviceStateQuery(Paradata::DeviceStateEvent::DeviceState& device_state)
{
    auto env = GetJNIEnvForCurrentThread();
    const int iValuesToFill = Paradata::DeviceStateEvent::DeviceState::ValuesToFill;

    JNIReferences::scoped_local_ref<jobjectArray> valuesArray(env, env->NewObjectArray(iValuesToFill,JNIReferences::classString,env->NewStringUTF("")));
    env->CallStaticVoidMethod(JNIReferences::classApplicationInterface,JNIReferences::methodApplicationInterfaceParadataDeviceQuery,3,valuesArray.get());

    for( int i = 0; i < iValuesToFill; ++i ) {
        JNIReferences::scoped_local_ref<jstring> jvalue(env, (jstring) env->GetObjectArrayElement(
                valuesArray.get(), i));
        CString value = JavaToWSZ(env, jvalue.get());

        if (value.IsEmpty())
            continue;

        auto convert_boolean = [&value]() -> bool   { return ( value.Compare(_T("1")) == 0 ); };
        auto convert_double  = [&value]() -> double { return wcstod(value, nullptr); };

        switch (i) {
            case 0:
                device_state.bluetooth_enabled = convert_boolean();
                break;
            case 1:
                device_state.gps_enabled = convert_boolean();
                break;
            case 2:
                device_state.wifi_enabled = convert_boolean();
                break;
            case 3:
                device_state.wifi_ssid = value;
                break;
            case 4:
                device_state.mobile_network_enabled = convert_boolean();
                break;
            case 5:
                device_state.mobile_network_type = value;
                break;
            case 6:
                device_state.mobile_network_name = value;
                break;
            case 7:
                device_state.mobile_network_strength = convert_double();
                break;
            case 8:
                device_state.battery_level = convert_double();
                break;
            case 9:
                device_state.battery_charging = convert_boolean();
                break;
            case 10:
                device_state.screen_brightness = convert_double();
                break;
            case 11:
                device_state.screen_orientation_portrait = convert_boolean();
                break;
        }
    }
}

double AndroidApplicationInterface::GetUpTime()
{
    auto env = GetJNIEnvForCurrentThread();

    JNIReferences::scoped_local_ref<jobjectArray> valuesArray(env, env->NewObjectArray(1, JNIReferences::classString,env->NewStringUTF("")));
    env->CallStaticVoidMethod(JNIReferences::classApplicationInterface,JNIReferences::methodApplicationInterfaceParadataDeviceQuery,2,valuesArray.get());
    JNIReferences::scoped_local_ref<jstring> jval(env, (jstring) env->GetObjectArrayElement(valuesArray.get(), 0));
    CString uptime = JavaToWSZ(env, jval.get());
    return _tstof(uptime)/1000.0;
}

CString AndroidApplicationInterface::GetLocaleLanguage() const
{
    auto env = GetJNIEnvForCurrentThread();
    JNIReferences::scoped_local_ref<jstring> jLanguageName(env, (jstring) env->CallStaticObjectMethod(JNIReferences::classApplicationInterface,JNIReferences::methodApplicationInterfaceGetLocaleLanguage));
    return JavaToWSZ(env,jLanguageName.get());
}


void AndroidApplicationInterface::MediaScanFiles(const std::vector<CString>& paths)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    JNIReferences::scoped_local_ref<jobjectArray> jpaths(pEnv, pEnv->NewObjectArray(paths.size(),JNIReferences::classString,pEnv->NewStringUTF("")));

    for( int i = 0; i < paths.size(); i++ )
    {
        JNIReferences::scoped_local_ref<jstring> jpath(pEnv, WideToJava(pEnv,paths[i]));

        pEnv->SetObjectArrayElement(jpaths.get(),i,jpath.get());
    }

    pEnv->CallStaticVoidMethod(JNIReferences::classApplicationInterface,JNIReferences::methodApplicationInterfaceMediaScanFiles,jpaths.get());
}


std::wstring AndroidApplicationInterface::CreateSharableUri(const std::wstring& path, const bool add_write_permission)
{
    auto env = GetJNIEnvForCurrentThread();
    JNIReferences::scoped_local_ref<jstring> jPath(env, WideToJava(env, path));

    jstring jSharableUri = (jstring)env->CallStaticObjectMethod(JNIReferences::classApplicationInterface,
                                                                JNIReferences::methodApplicationInterfaceCreateSharableUri,
                                                                jPath.get(), add_write_permission);

    ThrowJavaExceptionAsCSProException(env);

    ASSERT(jSharableUri != nullptr);
    return JavaToWSZ(env, jSharableUri);
}


void AndroidApplicationInterface::FileCopySharableUri(const std::wstring& sharable_uri, const std::wstring& destination_path)
{
    auto env = GetJNIEnvForCurrentThread();
    JNIReferences::scoped_local_ref<jstring> jSharableUri(env, WideToJava(env, sharable_uri));
    JNIReferences::scoped_local_ref<jstring> jDestinationPath(env, WideToJava(env, destination_path));

    env->CallStaticVoidMethod(JNIReferences::classApplicationInterface,
                              JNIReferences::methodApplicationInterfaceFileCopySharableUri,
                              jSharableUri.get(), jDestinationPath.get());

    ThrowJavaExceptionAsCSProException(env);
}


long AndroidApplicationInterface::View(const Viewer& viewer)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    const auto& data = viewer.GetData();

    if( data.content_type == Viewer::Data::Type::Filename )
    {
        // if the file has a local file server specified, it is a HTML file
        if( !data.local_file_server_root_directory.empty() )
        {
            std::wstring filename_url = PortableLocalhost::CreateFilenameUrl(data.content);
            ViewWebPageWithJavaScriptInterface(viewer, filename_url);
        }

        // otherwise view as a file
        else
        {
            JNIReferences::scoped_local_ref<jstring> java_filename(pEnv, WideToJava(pEnv, data.content));
            pEnv->CallStaticVoidMethod(JNIReferences::classApplicationInterface,
                JNIReferences::methodApplicationInterfaceViewFile, java_filename.get());
        }
    }

    else
    {
        ASSERT(data.content_type == Viewer::Data::Type::HtmlUrl);
        ViewWebPageWithJavaScriptInterface(viewer, data.content);
    }

    return 1;
}


void AndroidApplicationInterface::ViewWebPageWithJavaScriptInterface(const Viewer& viewer, wstring_view url_sv)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    JNIReferences::scoped_local_ref<jstring> jTitle(pEnv, OptionalWideToJava(pEnv, viewer.GetOptions().title));
    JNIReferences::scoped_local_ref<jstring> jUrl(pEnv, WideToJava(pEnv, url_sv));

    JNIReferences::scoped_local_ref<jstring> jActionInvokerAccessTokenOverride(pEnv);

    if( viewer.GetData().action_invoker_access_token_override != nullptr )
        jActionInvokerAccessTokenOverride.reset(WideToJava(pEnv, *viewer.GetData().action_invoker_access_token_override));

    WebViewSyncOperationMarker::DisplayErrorIfOperationInProcess();

    long thread_wait_id = pEnv->CallStaticLongMethod(JNIReferences::classApplicationInterface,
        JNIReferences::methodApplicationInterfaceViewWebPageWithJavaScriptInterface,
        jTitle.get(), jUrl.get(), jActionInvokerAccessTokenOverride.get());

    // return if there was an error launching WebViewWithJavaScriptInterfaceActivity
    if( thread_wait_id < 0 )
        return;

    ThreadWaitForComplete(thread_wait_id);
}


long AndroidApplicationInterface::GetThreadWaitId()
{
    std::scoped_lock<std::mutex> lock(m_threadWaitIdsMutex);

    constexpr long FirstThreadWaitId = 1001;
    long thread_wait_id = m_threadWaitIds.empty() ? FirstThreadWaitId :
                                                    ( m_threadWaitIds.rbegin()->first + 1 );

    m_threadWaitIds.try_emplace(thread_wait_id, nullptr);

    return thread_wait_id;
}


void AndroidApplicationInterface::SetThreadWaitComplete(long thread_wait_id, std::optional<std::wstring> response)
{
    std::scoped_lock<std::mutex> lock(m_threadWaitIdsMutex);

    ASSERT(m_threadWaitIds.find(thread_wait_id) != m_threadWaitIds.cend());
    ASSERT(!m_threadWaitIds[thread_wait_id].has_value());

    // fill the response
    m_threadWaitIds[thread_wait_id] = std::make_unique<std::optional<std::wstring>>(std::move(response));
}


std::optional<std::wstring> AndroidApplicationInterface::ThreadWaitForComplete(long thread_wait_id)
{
    constexpr useconds_t SleepInterval = 100 * 1000; // 100 milliseconds
    std::optional<std::wstring> response;

    // wait until the thread is marked as complete
    while( true )
    {
        usleep(SleepInterval);

        std::scoped_lock<std::mutex> lock(m_threadWaitIdsMutex);
        auto lookup = m_threadWaitIds.find(thread_wait_id);

        if( lookup == m_threadWaitIds.cend() )
        {
            ASSERT(false);
            break;
        }

        if( lookup->second != nullptr )
        {
            response = std::move(*lookup->second);
            m_threadWaitIds.erase(lookup);
            break;
        }
    }

    return response;
}


void AndroidApplicationInterface::Prompt(EngineUI::PromptNode& prompt_node)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    jstring title = WideToJava(pEnv, prompt_node.title);
    jstring initial_value = WideToJava(pEnv, prompt_node.initial_value);

    auto return_value = (jstring)pEnv->CallStaticObjectMethod(JNIReferences::classApplicationInterface, JNIReferences::methodApplicationInterfacePrompt,
        title, initial_value, prompt_node.numeric, prompt_node.password, prompt_node.upper_case, prompt_node.multiline);

    if( return_value != nullptr )
        prompt_node.return_value = JavaToWSZ(pEnv, return_value);
}

bool AndroidApplicationInterface::RunPffExecutor(EngineUI::RunPffExecutorNode& run_pff_executor_node)
{
	try
	{
		return run_pff_executor_node.pff_executor->Execute(run_pff_executor_node.pff);
	}

	catch(...)
	{
		run_pff_executor_node.thrown_exception = std::current_exception();
		return false;
	}
}

void AndroidApplicationInterface::SetUsername(const CString& username)
{
    m_username = username;
}

bool AndroidApplicationInterface::GpsOpen()
{
    auto env = GetJNIEnvForCurrentThread();
    return (bool) env->CallStaticBooleanMethod(JNIReferences::classApplicationInterface,JNIReferences::methodApplicationInterfaceGpsOpen);
}

bool AndroidApplicationInterface::GpsClose()
{
    auto env = GetJNIEnvForCurrentThread();
    return (bool) env->CallStaticBooleanMethod(JNIReferences::classApplicationInterface,JNIReferences::methodApplicationInterfaceGpsClose);
}

CString AndroidApplicationInterface::GpsRead(int wait_time, int accuracy, const CString& dialog_text)
{
    auto env = GetJNIEnvForCurrentThread();

    JNIReferences::scoped_local_ref<jstring> jdialog_text(env, WideToJava(env,dialog_text));

    JNIReferences::scoped_local_ref<jstring> return_value(env, (jstring)env->CallStaticObjectMethod(JNIReferences::classApplicationInterface,
        JNIReferences::methodApplicationInterfaceGpsRead, wait_time, accuracy, jdialog_text.get()));

    return JavaToWSZ(env, return_value.get());
}

CString AndroidApplicationInterface::GpsReadLast()
{
    auto env = GetJNIEnvForCurrentThread();
    JNIReferences::scoped_local_ref<jstring> return_value(env, (jstring)env->CallStaticObjectMethod(JNIReferences::classApplicationInterface,
        JNIReferences::methodApplicationInterfaceGpsReadLast));
    return JavaToWSZ(env, return_value.get());
}

CString AndroidApplicationInterface::GpsReadInteractive(bool read_interactive_mode, const BaseMapSelection& base_map_selection,
    const CString& message, double read_duration)
{
    auto env = GetJNIEnvForCurrentThread();

    JNIReferences::scoped_local_ref<jstring> jmessage(env, WideToJava(env, message));

    JNIReferences::scoped_local_ref<jstring> return_value(env, (jstring)env->CallStaticObjectMethod(JNIReferences::classApplicationInterface,
        JNIReferences::methodApplicationInterfaceGpsReadInteractive, read_interactive_mode,
        AndroidMapUI::CreateJavaBaseMapSelection(env, base_map_selection), jmessage.get(), read_duration));

    return JavaToWSZ(env, return_value.get());
}


std::wstring AndroidApplicationInterface::BarcodeRead(const std::wstring& message_text)
{
    auto env = GetJNIEnvForCurrentThread();

    jstring message = WideToJava(env, message_text);

    jstring barcode = (jstring)env->CallStaticObjectMethod(JNIReferences::classApplicationInterface,
        JNIReferences::methodApplicationInterfaceBarcodeRead, message);

    return JavaToWSZ(env, barcode);
}


bool AndroidApplicationInterface::AudioPlay(const std::wstring& filename, const std::wstring& message_text)
{
    auto env = GetJNIEnvForCurrentThread();

    auto jfilename = JNIReferences::make_local_ref(env, WideToJava(env, filename));
    auto jmessage = JNIReferences::make_local_ref(env, WideToJava(env, message_text));

    return (bool) env->CallStaticBooleanMethod(JNIReferences::classApplicationInterface,JNIReferences::methodApplicationInterfaceAudioPlay, jfilename.get(), jmessage.get());
}

bool AndroidApplicationInterface::AudioStartRecording(const std::wstring& filename, std::optional<double> seconds, std::optional<int> sampling_rate)
{
    auto env = GetJNIEnvForCurrentThread();

    auto jfilename = JNIReferences::make_local_ref(env, WideToJava(env, filename));

    return (bool) env->CallStaticBooleanMethod(JNIReferences::classApplicationInterface,
            JNIReferences::methodApplicationInterfaceAudioStartRecording, jfilename.get(),
            seconds.value_or(-1), sampling_rate.value_or(-1));
}


bool AndroidApplicationInterface::AudioStopRecording()
{
    auto env = GetJNIEnvForCurrentThread();
    return (bool) env->CallStaticBooleanMethod(JNIReferences::classApplicationInterface,JNIReferences::methodApplicationInterfaceAudioStopRecording);
}


std::unique_ptr<TemporaryFile> AndroidApplicationInterface::AudioRecordInteractive(const std::wstring& message_text, std::optional<int> sampling_rate)
{
    auto env = GetJNIEnvForCurrentThread();

    auto temporary_file = std::make_unique<TemporaryFile>();

    auto jfilename = JNIReferences::make_local_ref(env, WideToJava(env, temporary_file->GetPath()));
    auto jmessage = JNIReferences::make_local_ref(env, WideToJava(env, message_text));

    if( env->CallStaticBooleanMethod(JNIReferences::classApplicationInterface,JNIReferences::methodApplicationInterfaceAudioRecordInteractive,
                                     jfilename.get(), jmessage.get(), sampling_rate.value_or(-1)) )
    {
        return temporary_file;
    }

    return nullptr;
}


long AndroidApplicationInterface::RunEngineUIProcessor(WPARAM wParam, LPARAM lParam)
{
    return m_engineUIProcessor.ProcessMessage(wParam, lParam);
}


bool AndroidApplicationInterface::CaptureImage(EngineUI::CaptureImageNode& capture_image_node)
{
    auto env = GetJNIEnvForCurrentThread();

    auto joverlay_message = JNIReferences::make_local_ref(env, capture_image_node.overlay_message ? WideToJava(env, *capture_image_node.overlay_message) : nullptr);

    JNIReferences::scoped_local_ref<jstring> filename(env);
    switch (capture_image_node.action) {
        case EngineUI::CaptureImageNode::Action::TakePhoto:
            filename.reset((jstring) env->CallStaticObjectMethod(JNIReferences::classApplicationInterface, JNIReferences::methodApplicationInterfaceImageTakePhoto, joverlay_message.get()));
            break;
        case EngineUI::CaptureImageNode::Action::CaptureSignature:
            filename.reset((jstring) env->CallStaticObjectMethod(JNIReferences::classApplicationInterface, JNIReferences::methodApplicationInterfaceImageCaptureSignature, joverlay_message.get()));
            break;
    }

    if (filename.get()) {
        capture_image_node.output_filename = JavaToWSZ(env, filename.get());
        return true;
    } else {
        return false;
    }
}


void AndroidApplicationInterface::CapturePolygonTrace(std::unique_ptr<Geometry::Polygon>& captured_polygon, const Geometry::Polygon* polygon, IMapUI* map) {

    auto env = GetJNIEnvForCurrentThread();
    jobject jmap = map ? dynamic_cast<AndroidMapUI*>(map)->GetAndroidMapUI() : nullptr;
    JNIReferences::scoped_local_ref<jobject> jexisting_polygon(env);
    if (polygon)
        jexisting_polygon = GeometryJni::polygonToJava(env, *polygon);
    JNIReferences::scoped_local_ref<jobject> jpolygon(env,
            env->CallStaticObjectMethod(JNIReferences::classApplicationInterface,JNIReferences::methodApplicationInterfaceGeometryTracePolygon, jexisting_polygon.get(), jmap));
    captured_polygon = GeometryJni::polygonFromJavaArrayList(env, jpolygon.get());
}


void AndroidApplicationInterface::CapturePolygonWalk(std::unique_ptr<Geometry::Polygon>& captured_polygon, const Geometry::Polygon* polygon, IMapUI* map) {
    JNIEnv* env = GetJNIEnvForCurrentThread();
    jobject jmap = map ? dynamic_cast<AndroidMapUI*>(map)->GetAndroidMapUI() : nullptr;
    JNIReferences::scoped_local_ref<jobject> jexisting_polygon(env);
    if (polygon)
        jexisting_polygon = GeometryJni::polygonToJava(env, *polygon);

    JNIReferences::scoped_local_ref<jobject> jpolygon(env,
                                                      env->CallStaticObjectMethod(JNIReferences::classApplicationInterface,
                                                                                  JNIReferences::methodApplicationInterfaceGeometryWalkPolygon, jexisting_polygon.get(), jmap));
    captured_polygon = GeometryJni::polygonFromJavaArrayList(env, jpolygon.get());
}
