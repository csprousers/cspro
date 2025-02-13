#include "stdafx.h"
#include "AccessToken.h"
#include <zToolsO/Screen.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/Viewers.h>
#include <zUtilF/HtmlDialogFunctionRunner.h>
#include <zHtml/PortableLocalhost.h>
#include <zAppO/PFF.h>
#include <zEngineF/ErrmsgDlg.h>


CREATE_JSON_KEY(targetOrigin)
CREATE_JSON_KEY(webViews)


#ifdef WIN_DESKTOP

namespace
{
    template<>
    struct JsonSerializer<ActionInvoker::Caller::WebViewTag>
    {
        static_assert(sizeof(ActionInvoker::Caller::WebViewTag) == sizeof(int));

        static ActionInvoker::Caller::WebViewTag CreateFromJson(const JsonNode<wchar_t>& json_node)
        {
            return reinterpret_cast<ActionInvoker::Caller::WebViewTag>(json_node.Get<int>());
        }

        static void WriteJson(JsonWriter& json_writer, const ActionInvoker::Caller::WebViewTag& web_view_tag)
        {
            json_writer.Write(reinterpret_cast<int>(web_view_tag));
        }
    };
}

#endif // WIN_DESKTOP


ActionInvoker::Result ActionInvoker::Runtime::UI_getMaxDisplayDimensions(const JsonNode<wchar_t>& /*json_node*/, Caller& /*caller*/)
{
    return Result::JsonText(AssertAndReturnValidJson(FormatTextCS2WS(LR"({"width":%d,"height":%d})",
                                                                     Screen::GetMaxDisplayWidth(), Screen::GetMaxDisplayHeight())));
}


ActionInvoker::Result ActionInvoker::Runtime::UI_getDisplayOptions(const JsonNode<wchar_t>& /*json_node*/, Caller& caller)
{
    std::optional<std::wstring> display_options;

    IterateOverListeners(
        [&](Listener& listener)
        {
            display_options = listener.OnGetDisplayOptions(caller);
            return !display_options.has_value();
        });

    return display_options.has_value() ? Result::JsonText(AssertAndReturnValidJson(*display_options)) :
                                         Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::UI_setDisplayOptions(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    std::optional<bool> display_options_set;

    IterateOverListeners(
        [&](Listener& listener)
        {
            display_options_set = listener.OnSetDisplayOptions(json_node, caller);
            return !display_options_set.has_value();
        });

    return Result::Bool(display_options_set.value_or(false));
}


ActionInvoker::Result ActionInvoker::Runtime::UI_getInputData(const JsonNode<wchar_t>& /*json_node*/, Caller& caller)
{
    std::optional<std::wstring> input_data;

    auto find_input_data = [&](const bool match_caller)
    {
        IterateOverListeners(
            [&](Listener& listener)
            {
                input_data = listener.OnGetInputData(caller, match_caller);
                return !input_data.has_value();
            });
    };

    // first try to find input data specific to this caller
    find_input_data(true);

    // if not found, find any input data
    if( !input_data.has_value() )
        find_input_data(false);

    return ( input_data.has_value() && !input_data->empty() ) ? Result::JsonText(AssertAndReturnValidJson(*input_data)) :
                                                                Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::UI_closeDialog(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    const JsonNode<wchar_t> result_node = json_node.GetOrEmpty(JK::result);
    std::optional<bool> dialog_closed;

    IterateOverListeners(
        [&](Listener& listener)
        {
            // make sure that the closeDialog request is applicable to the caller
            dialog_closed = listener.OnCloseDialog(result_node, caller);
            return !dialog_closed.has_value();
        });

    return Result::Bool(dialog_closed.value_or(false));
}


std::wstring ActionInvoker::Runtime::GetHtmlDialogFilename(const std::wstring& base_filename)
{
    // check if the file exists in an overriden HTML dialog directory or in CSPro's html/dialogs directory
    std::wstring filename;

    auto create_filename = [&](const wstring_view directory_sv)
    {
        filename = MakeFullPath(directory_sv, base_filename);
        return PortableFunctions::FileIsRegular(filename);
    };

    const PFF* pff = GetPff(false);

    if( ( pff != nullptr && create_filename(pff->GetHtmlDialogsDirectory()) ) ||
        create_filename(Html::GetDirectory(Html::Subdirectory::Dialogs)) )
    {
        return filename;
    }

    throw CSProException(_T("The dialog could not be shown because the dialog source could not be found: ") + base_filename);
}


ActionInvoker::Result ActionInvoker::Runtime::ShowHtmlDialog(const std::wstring& dialog_path, std::wstring input_data, std::optional<std::wstring> display_options/* = std::nullopt*/)
{
    HtmlDialogFunctionRunner html_dialog_function_runner(NavigationAddress::CreateHtmlFilenameReference(dialog_path),
                                                         std::move(input_data),
                                                         std::move(display_options));

    html_dialog_function_runner.DoModalOnUIThread();

    const std::optional<std::wstring>& results = html_dialog_function_runner.GetResultsText();

    return results.has_value() ? Result::JsonText(*results) :
                                 Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::UI_showDialog(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    const std::wstring base_dialog_filename = json_node.Get<std::wstring>(JK::path);
    std::wstring evaluated_dialog_filename = caller.EvaluateAbsolutePath(base_dialog_filename);

    // if the HTML dialog does not exist, check if it exists in an overriden HTML dialog directory or in CSPro's html/dialogs directory
    if( !PortableFunctions::FileIsRegular(evaluated_dialog_filename) )
    {
        evaluated_dialog_filename = GetHtmlDialogFilename(base_dialog_filename);
        ASSERT(PortableFunctions::FileIsRegular(evaluated_dialog_filename));
    }

    std::wstring input_data = json_node.Contains(JK::inputData) ? json_node.Get(JK::inputData).GetNodeAsString() :
                                                                  std::wstring();

    std::optional<std::wstring> display_options = json_node.Contains(JK::displayOptions) ? std::make_optional(json_node.Get(JK::displayOptions).GetNodeAsString()) :
                                                                                           std::nullopt;

    return ShowHtmlDialog(evaluated_dialog_filename, std::move(input_data), std::move(display_options));
}


ActionInvoker::Result ActionInvoker::Runtime::UI_alert(const JsonNode<wchar_t>& json_node, Caller& /*caller*/)
{
    // use the errmsg dialog
    const std::wstring dialog_path = GetHtmlDialogFilename(PortableFunctions::PathAppendFileExtension<std::wstring>(ErrmsgDialogName, FileExtensions::HTML));

    // create the input data 
    auto json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::title, json_node.Contains(JK::title) ? json_node.Get<wstring_view>(JK::title) : _T("Alert"))
                .Write(JK::message, json_node.Get<wstring_view>(JK::text))
                .EndObject();

    ShowHtmlDialog(dialog_path, json_writer->GetString());

    return Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::UI_view(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    const TCHAR* const input_type = GetUniqueKeyFromChoices(json_node, JK::path, JK::url);
    Viewer viewer;
    std::wstring url;

    // path
    if( input_type == JK::path )
    {
        const std::wstring path = caller.EvaluateAbsolutePath(json_node.Get<std::wstring>(JK::path));

        if( !PortableFunctions::FileIsRegular(path) )
            throw FileIO::Exception::FileNotFound(path);
        
        // register an access token if the file is in the html directory
        if( SO::StartsWithNoCase(path, Html::GetDirectory()) )
            viewer.SetAccessInvokerAccessTokenOverride(AccessToken::CreateAccessTokenForHtmlDirectoryFile(path));

        url = PortableLocalhost::CreateFilenameUrl(path);        
    }

    // url
    else
    {
        ASSERT(input_type == JK::url);

        url = json_node.Get<std::wstring>(JK::url);
    }

    if( json_node.Contains(JK::inputData) )
        viewer.GetOptions().action_invoker_ui_get_input_data = std::make_shared<std::wstring>(json_node.Get(JK::inputData).GetNodeAsString());

    if( json_node.Contains(JK::displayOptions) )
        viewer.GetOptions().display_options_node = std::make_unique<const JsonNode<wchar_t>>(json_node.Get(JK::displayOptions));

    viewer.UseEmbeddedViewer()
          .UseSharedHtmlLocalFileServer()
          .ViewHtmlUrl(url);

    return Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::UI_enumerateWebViews(const JsonNode<wchar_t>& /*json_node*/, Caller& caller)
{
    auto json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    json_writer->WriteIfHasValue<const TCHAR*, Caller::WebViewTag>(JK::webViewId, caller.GetWebViewTag());

    json_writer->BeginArray(JK::webViews);

    IterateOverListeners(
        [&](Listener& listener)
        {
            std::optional<Caller::WebViewTag> web_view_tag = listener.OnGetAssociatedWebViewDetails();

            if( web_view_tag.has_value() )
            {
                json_writer->BeginObject()
                            .Write(JK::webViewId, *web_view_tag)
                            .EndObject();
            }

            return true;
        }); 
    
    json_writer->EndArray();

    json_writer->EndObject();

    return Result::JsonText(json_writer);
}


ActionInvoker::Result ActionInvoker::Runtime::UI_postWebMessage(const JsonNode<wchar_t>& json_node, Caller& /*caller*/)
{
    const std::wstring message = json_node.Get<std::wstring>(JK::message);
    const std::optional<std::wstring> target_origin = json_node.GetOptional<std::wstring>(JK::targetOrigin);
    const std::optional<Caller::WebViewTag> target_web_view_tag = json_node.GetOptional<Caller::WebViewTag>(JK::webViewId);
    Listener* applicable_listener = nullptr;

    IterateOverListeners(
        [&](Listener& listener)
        {
            std::optional<Caller::WebViewTag> web_view_tag = listener.OnGetAssociatedWebViewDetails();

            if( ( web_view_tag.has_value() ) &&
                ( !target_web_view_tag.has_value() || *web_view_tag == *target_web_view_tag ) )
            {
                applicable_listener = &listener;
                return false;
            }

            return true;
        }); 

    if( applicable_listener == nullptr )
    {
        if( target_web_view_tag.has_value() )
        {
#ifdef WIN_DESKTOP
            const int id = reinterpret_cast<int>(*target_web_view_tag);
#else
            const int id = static_cast<int>(*target_web_view_tag);
#endif
            throw CSProException(_T("No web view exists with ID '%d'."), id);
        }

        else
        {
            throw CSProException("No web view is currently showing.");
        }
    }

    applicable_listener->OnPostWebMessage(message, target_origin);

    return Result::Undefined();
}
