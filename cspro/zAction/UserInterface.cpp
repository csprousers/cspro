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


ActionInvoker::Result ActionInvoker::Runtime::UI_getMaxDisplayDimensions(const JsonNode& /*json_node*/, Caller& /*caller*/)
{
    return Result::JsonText(AssertAndReturnValidJson(FormatText(R"({"width":%d,"height":%d})",
                                                                Screen::GetMaxDisplayWidth(), Screen::GetMaxDisplayHeight())));
}


ActionInvoker::Result ActionInvoker::Runtime::UI_getDisplayOptions(const JsonNode& /*json_node*/, Caller& caller)
{
    SharableString display_options;

    IterateOverListeners(
        [&](Listener& listener)
        {
            display_options = listener.OnGetDisplayOptions(caller);
            return !display_options.IsSet();
        });

    return display_options.IsSet() ? Result::JsonText(AssertAndReturnValidJson(std::move(display_options))) :
                                     Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::UI_setDisplayOptions(const JsonNode& json_node, Caller& caller)
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


ActionInvoker::Result ActionInvoker::Runtime::UI_setWebViewOptions(const JsonNode& json_node, Caller& caller)
{
    std::unique_ptr<std::vector<WebViewPermission>> permissions;

    const JsonNode permissions_json_node = json_node.GetOrEmpty(JK::permissions);

    if( !permissions_json_node.IsEmpty() )
    {
        permissions = std::make_unique<std::vector<WebViewPermission>>();

        auto add_permission = [&](const JsonNode& permission_json_node)
        {
            const std::string_view permission_sv = permission_json_node.Get<std::string_view>();

            permissions->emplace_back(
                ( permission_sv == "camera" )      ? WebViewPermission::Camera :
                ( permission_sv == "microphone" )  ? WebViewPermission::Microphone :
                throw CSProException(SO::Concatenate("Unknown permission: ", permission_sv))
            );
        };

        if( permissions_json_node.IsString() )
        {
            add_permission(permissions_json_node);
        }

        else
        {
            for( const JsonNode& permission_json_node : permissions_json_node.GetArray() )
                add_permission(permission_json_node);
        }
    }

    if( permissions != nullptr )
    {
        IterateOverListeners(caller,
            [&](Listener& listener)
            {
                listener.OnSetWebViewOptions(permissions.get());
            });
    }

    return Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::UI_getInputData(const JsonNode& /*json_node*/, Caller& caller)
{
    SharableString input_data;

    auto find_input_data = [&](const bool match_caller)
    {
        IterateOverListeners(
            [&](Listener& listener)
            {
                input_data = listener.OnGetInputData(caller, match_caller);
                return !input_data.IsSet();
            });
    };

    // first try to find input data specific to this caller
    find_input_data(true);

    // if not found, find any input data
    if( !input_data.IsSet() )
        find_input_data(false);

    return ( input_data.IsSet() && !input_data->empty() ) ? Result::JsonText(AssertAndReturnValidJson(std::move(input_data))) :
                                                            Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::UI_close(const JsonNode& json_node, Caller& caller)
{
    Listener::CloseResult close_result;

    if( json_node.Contains(JK::result) )
    {
        close_result.emplace<const JsonNode>(json_node.Get(JK::result));
    }

    else if( json_node.Contains(JK::exception) )
    {
        close_result = std::make_unique<ActionInvoker::Exception>(json_node.Get(JK::exception), false);
    }

    std::optional<bool> window_closed;

    IterateOverListeners(
        [&](Listener& listener)
        {
            // make sure that the close request is applicable to the caller
            window_closed = listener.OnClose(close_result, caller);
            return !window_closed.has_value();
        });

    // if the exception was not handled, throw the exception in this execution context
    if( std::holds_alternative<std::unique_ptr<const ActionInvoker::Exception>>(close_result) &&
        std::get<std::unique_ptr<const ActionInvoker::Exception>>(close_result) != nullptr )
    {
        throw *std::get<std::unique_ptr<const ActionInvoker::Exception>>(close_result);
    }

    return Result::Bool(window_closed.value_or(false));
}


ActionInvoker::Result ActionInvoker::Runtime::UI_closeDialog(const JsonNode& json_node, Caller& caller)
{
    static_assert(Versioning::Number <= 8.1, "Start adding runtime warnings when using UI.closeDialog as opposed to UI.close");
    return UI_close(json_node, caller);
}


std::string ActionInvoker::Runtime::GetHtmlDialogFilePath(const std::string& base_filename)
{
    // check if the file exists in an overridden HTML dialog directory or in CSPro's html/dialogs directory
    std::string file_path;

    auto create_file_path = [&](const std::string_view directory_sv)
    {
        file_path = MakeFullPath(directory_sv, base_filename);
        return PortableFunctions::FileIsRegular(file_path);
    };

    const PFF* const pff = GetPff(false);

    if( ( pff != nullptr && create_file_path(UTF8_TODO::GetUtf8(pff->GetHtmlDialogsDirectory())) ) ||
        create_file_path(Html::GetDirectory(Html::Subdirectory::Dialogs)) )
    {
        return file_path;
    }

    throw CSProException("The dialog could not be shown because the dialog source could not be found: " + base_filename);
}


ActionInvoker::Result ActionInvoker::Runtime::ShowHtmlDialog(const std::string& dialog_file_path, SharableString input_data, SharableString display_options/* = SharableString()*/)
{
    HtmlDialogFunctionRunner html_dialog_function_runner(NavigationAddress::CreateHtmlFilePathReference(dialog_file_path),
                                                         std::move(input_data),
                                                         std::move(display_options));

    html_dialog_function_runner.DoModalOnUIThread();

    html_dialog_function_runner.GetExceptionHolder().ThrowExceptions();

    SharableString results = html_dialog_function_runner.GetResultsText();

    return results.IsSet() ? Result::JsonText(std::move(results)) :
                             Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::UI_showDialog(const JsonNode& json_node, Caller& caller)
{
    const std::string base_dialog_filename_or_path = json_node.Get<std::string>(JK::path);
    std::string evaluated_dialog_file_path = caller.EvaluateAbsolutePath(base_dialog_filename_or_path);

    // if the HTML dialog does not exist, check if it exists in an overridden HTML dialog directory or in CSPro's html/dialogs directory
    if( !PortableFunctions::FileIsRegular(evaluated_dialog_file_path) )
    {
        evaluated_dialog_file_path = GetHtmlDialogFilePath(base_dialog_filename_or_path);
        ASSERT(PortableFunctions::FileIsRegular(evaluated_dialog_file_path));
    }

    SharableString input_data = json_node.Contains(JK::inputData) ? json_node.Get(JK::inputData).GetNodeAsSharableString() :
                                                                    SharableString();

    SharableString display_options = json_node.Contains(JK::displayOptions) ? json_node.Get(JK::displayOptions).GetNodeAsSharableString() :
                                                                              SharableString();

    return ShowHtmlDialog(evaluated_dialog_file_path, std::move(input_data), std::move(display_options));
}


ActionInvoker::Result ActionInvoker::Runtime::UI_alert(const JsonNode& json_node, Caller& /*caller*/)
{
    // use the errmsg dialog
    const std::string dialog_file_path = GetHtmlDialogFilePath(PortableFunctions::PathAppendFileExtension(std::string(ErrmsgDlg::DialogName), FileExtensions::HTML));

    // create the input data
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::title, json_node.Contains(JK::title) ? json_node.Get<std::string_view>(JK::title) : "Alert")
                .Write(JK::message, json_node.Get<std::string_view>(JK::text))
                .EndObject();

    ShowHtmlDialog(dialog_file_path, json_writer->ReleaseSharableString());

    return Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::UI_view(const JsonNode& json_node, Caller& caller)
{
    const char* const input_type = GetUniqueKeyFromChoices(json_node, JK::path, JK::url);
    Viewer viewer;
    std::string url;

    // path
    if( input_type == JK::path )
    {
        const std::string path = caller.EvaluateAbsolutePath(json_node.Get<std::string>(JK::path));

        if( !PortableFunctions::FileIsRegular(path) )
            throw FileIO::Exception::FileNotFound(path);

        // register an access token if the file is in the html directory
        if( SO::StartsWithNoCase(path, Html::GetDirectory()) )
            viewer.SetAccessInvokerAccessTokenOverride(AccessToken::CreateAccessTokenForHtmlDirectoryFile(path));

        url = PortableLocalhost::CreateFileUrl(path);
    }

    // url
    else
    {
        ASSERT(input_type == JK::url);

        url = json_node.Get<std::string>(JK::url);
    }

    if( json_node.Contains(JK::inputData) )
        viewer.GetOptions().action_invoker_ui_get_input_data = json_node.Get(JK::inputData).GetNodeAsSharableString();

    if( json_node.Contains(JK::displayOptions) )
        viewer.GetOptions().display_options_node = std::make_unique<const JsonNode>(json_node.Get(JK::displayOptions));

    auto exception_holder = std::make_shared<ExceptionHolder>();

    viewer.UseEmbeddedViewer()
          .UseSharedHtmlLocalFileServer()
          .UseExceptionHolder(exception_holder)
          .ViewHtmlUrl(url);

    exception_holder->ThrowExceptions();

    return Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::UI_enumerateWebViews(const JsonNode& /*json_node*/, Caller& caller)
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    if( caller.IsWebView() )
        json_writer->Write(JK::webViewId, caller.GetCallerId());

    json_writer->BeginArray(JK::webViews);

    IterateOverListeners(
        [&](Listener& listener)
        {
            const std::optional<int> web_view_caller_id = listener.OnGetAssociatedWebViewCallerId();

            if( web_view_caller_id.has_value() )
            {
                json_writer->BeginObject()
                            .Write(JK::webViewId, *web_view_caller_id)
                            .EndObject();
            }

            return true;
        });

    json_writer->EndArray();

    json_writer->EndObject();

    return Result::JsonText(*json_writer);
}


ActionInvoker::Result ActionInvoker::Runtime::UI_postWebMessage(const JsonNode& json_node, Caller& /*caller*/)
{
    const std::string message = json_node.Get<std::string>(JK::message);
    const std::optional<std::string> target_origin = json_node.GetOptional<std::string>(JK::targetOrigin);
    const std::optional<int> target_web_view_caller_id = json_node.GetOptional<int>(JK::webViewId);
    Listener* applicable_listener = nullptr;

    IterateOverListeners(
        [&](Listener& listener)
        {
            const std::optional<int> web_view_caller_id = listener.OnGetAssociatedWebViewCallerId();

            if( ( web_view_caller_id.has_value() ) &&
                ( !target_web_view_caller_id.has_value() || *web_view_caller_id == *target_web_view_caller_id ) )
            {
                applicable_listener = &listener;
                return false;
            }

            return true;
        });

    if( applicable_listener == nullptr )
    {
        if( target_web_view_caller_id.has_value() )
            throw CSProException("No web view exists with ID '%d'.", *target_web_view_caller_id);

        throw CSProException("No web view is currently showing.");
    }

    applicable_listener->OnPostWebMessage(message, target_origin);

    return Result::Undefined();
}
