#include "stdafx.h"
#include "WindowsOAuth2Authorizer.h"
#include "WindowsOAuth2AuthorizerWaitDlg.h"
#include <zToolsO/Hash.h>
#include <zToolsO/UniqueId.h>
#include <zUtilO/MimeType.h>
#include <zUtilO/UIThreadRunner.h>
#include <zHtml/HtmlTemplates.h>


WindowsOAuth2Authorizer::WindowsOAuth2Authorizer(OAuth2Authorizer& oauth2_authorizer)
    :   m_oauth2Authorizer(oauth2_authorizer),
        m_parameters(m_oauth2Authorizer.GetParameters()),
        m_state(GenerateState())
{
    // the redirect URI is defined as http://localhost so it should not have a trailing slash
    m_redirectUri = Encoders::ToUriComponent(PortableFunctions::PathRemoveTrailingSlash(m_server.GetBaseUrl()));

    // create the authorization URL
    ASSERT(SO::StartsWith(m_parameters.oauth2_endpoint, "https://"));
    ASSERT(m_parameters.oauth2_endpoint.back() != '?' && m_parameters.oauth2_endpoint.back() != '/');

    ASSERT(m_parameters.client_id == Encoders::ToUriComponent(m_parameters.client_id));
    ASSERT(m_parameters.client_secret == Encoders::ToUriComponent(m_parameters.client_secret));
    ASSERT(m_state == Encoders::ToUriComponent(m_state));

    m_authorizationUrl = SO::Concatenate(m_parameters.oauth2_endpoint,
                                         "?response_type=code"
                                         "&redirect_uri=", m_redirectUri,
                                         "&client_id=", m_parameters.client_id,
                                         "&state=", m_state);

    for( const auto& [key, value] : m_parameters.extra_authorization_parameters )
    {
        m_authorizationUrl.push_back('&');
        m_authorizationUrl.append(Encoders::ToUriComponent(key));
        m_authorizationUrl.push_back('=');
        m_authorizationUrl.append(Encoders::ToUriComponent(value));
    }

    // set up the local server to read the authorization redirect response and then handle the redirect to get the access/refresh tokens
    m_server.AddMapping("/",      [&](SimpleServer::Handler& handler) { HandleAuthorizationRedirect(handler); });
    m_server.AddMapping("/token", [&](SimpleServer::Handler& handler) { HandleTokenPage(handler); });
}


WindowsOAuth2Authorizer::~WindowsOAuth2Authorizer()
{
}


std::string WindowsOAuth2Authorizer::GenerateState()
{
    // the state string is 16 random bytes (calculated with the not-particularly-secure rand) with two bytes then XORed with a unique ID;
    // the string is then encoded as hex
    constexpr size_t StateLength = 16;
    char state_bytes[StateLength];
    const int unique_id = UniqueId::CreateInt();

    srand(static_cast<unsigned int>(std::time(nullptr)));

    for( size_t i = 0; i < StateLength; ++i )
        state_bytes[i] = rand() % 256;

    for( int i = 0; i < 2; ++i )
        state_bytes[rand() % StateLength] ^= unique_id;

    return Hash::BytesToHexString(state_bytes, StateLength);
}


OAuth2Token WindowsOAuth2Authorizer::ShowAuthorizationDialog()
{
    if( AfxGetApp() != nullptr )
    {
        // open the authorization URL in an external web browser
        ShellExecute(nullptr, L"open", TC::ToWide(EscapeCommandLineArgument(m_authorizationUrl)).c_str(), nullptr, nullptr, SW_SHOWNORMAL);

        // display a dialog to indicate that we are waiting for a response that occurs in the web browser;
        // try to display the dialog on the UI thread, falling back to this thread if the program does not respond to the UWM::UtilF::RunOnUIThread message
        m_waitDlg = std::make_unique<WindowsOAuth2AuthorizerWaitDlg>(m_parameters.client_name);

        DialogUIThreadRunner ui_thread_runner(m_waitDlg.get());

        if( !ui_thread_runner.RunOnUIThread() )
            m_waitDlg->DoModal();
    }

    // when run from CSDeploy, the dialog is shown using OAuth2Authorizer.exe
    else
    {
        ShowAuthorizationDialogFromDotNet();
    }

    // if no token was received and no error occurred (e.g., the user closed the dialog), throw a cancelation exception
    if( !m_tokenOrExceptionMessage.has_value() )
    {
        throw SyncCancelException();
    }

    // return the token on success...
    else if( std::holds_alternative<OAuth2Token>(*m_tokenOrExceptionMessage) )
    {
        return std::move(std::get<OAuth2Token>(*m_tokenOrExceptionMessage));
    }

    // ...or throw the saved exception that came from authorization
    else
    {
        throw SyncConnectionError(std::get<std::string>(*m_tokenOrExceptionMessage));
    }
}


void WindowsOAuth2Authorizer::ShowTextAsHtml(SimpleServer::Handler& handler, const std::string_view text_sv)
{
    const std::string html = HtmlTemplates::CreateCenteredTextPage("CSPro OAuth 2.0 Authorizer", text_sv);
    handler.SetResponseContent(html.data(), html.size(), MimeType::Type::Html);
}


void WindowsOAuth2Authorizer::HandleAuthorizationRedirect(SimpleServer::Handler& handler)
{
    // process the query string
    const std::string& request_target = handler.GetRequestTarget();
    const auto [ignore_sv, query_string_sv] = SO::GetTextOnEitherSideOfCharacter(request_target, '?');

    std::vector<std::tuple<std::string_view, std::string_view>> keys_and_values;

    SO::ForeachSection(query_string_sv, '&',
        [&](const std::string_view key_and_value_sv)
        {
            keys_and_values.emplace_back(SO::GetTextOnEitherSideOfCharacter(key_and_value_sv, '='));
        });

    auto find_value = [&](const char* const key) -> const std::string_view*
    {
        const auto& lookup = std::find_if(keys_and_values.cbegin(), keys_and_values.cend(),
                                          [&](const std::tuple<std::string_view, std::string_view>& keys_and_value) { return SO::Equals(std::get<0>(keys_and_value), key); });

        return ( lookup != keys_and_values.cend() ) ? &std::get<1>(*lookup) :
                                                      nullptr;
    };

    try
    {
        // first check for an error
        const std::string_view* const error_sv = find_value("error");

        if( error_sv != nullptr )
        {
            const std::string_view* const detailed_error_sv =
                !m_parameters.authorization_error_description_key.empty() ? find_value(m_parameters.authorization_error_description_key.c_str()) :
                                                                            nullptr;

            throw CSProException(Encoders::FromUrlQueryString(( detailed_error_sv != nullptr ) ? *detailed_error_sv :
                                                                                                 *error_sv));
        }

        // otherwise get the code and validate the state
        const std::string_view* const code_sv = find_value("code");
        const std::string_view* const state_sv = find_value("state");

        if( code_sv == nullptr || state_sv == nullptr || *state_sv != m_state )
            throw CSProException("Unexpected response received.");

        m_authorizationCode = *code_sv;

        // with a proper authorization code, we can now query for the tokens
        handler.SetResponseRedirect(PortableFunctions::PathAppendForwardSlashToPath(m_server.GetBaseUrl(), "token"));
    }

    catch( const CSProException& exception )
    {
        m_tokenOrExceptionMessage = FormatText("There was an error authenticating with %s: %s",
                                               m_parameters.client_name.c_str(), exception.what());

        // show the error in the browser
        ShowTextAsHtml(handler, std::get<std::string>(*m_tokenOrExceptionMessage));

        // close the dialog on error
        if( m_waitDlg != nullptr )
            m_waitDlg->PostMessage(WM_CLOSE);
    }
}


void WindowsOAuth2Authorizer::HandleTokenPage(SimpleServer::Handler& handler)
{
    ASSERT(!m_authorizationCode.empty());

    try
    {
        m_tokenOrExceptionMessage = m_oauth2Authorizer.GetTokenFromAuthorizationCode(m_authorizationCode, m_redirectUri);

        // show a page indicating that we successfully retrieved the tokens
        ShowTextAsHtml(handler, FormatText("Successfully retrieved credentials from %s. You can close this tab and return to CSPro", m_parameters.client_name.c_str()));
    }

    catch( const CSProException& exception )
    {
        m_tokenOrExceptionMessage = exception.what();

        // show the error in the browser
        ShowTextAsHtml(handler, std::get<std::string>(*m_tokenOrExceptionMessage));
    }

    // close the dialog on success or error
    if( m_waitDlg != nullptr )
        m_waitDlg->PostMessage(WM_CLOSE);
}



// --------------------------------------------------------------------------
// remove all of the below if/when CSDeploy is no longer a C# program
// --------------------------------------------------------------------------

#include <zToolsO/Encryption.h>
#include <zUtilO/CSProExecutables.h>
#include <zUtilO/TemporaryFile.h>


void WindowsOAuth2Authorizer::ShowAuthorizationDialogFromDotNet()
{
    const std::string exe_path = Path::Combine(CSProExecutables::GetModuleDirectory(), "OAuth2Authorizer.exe");
    TemporaryFile result_temporary_file;

    const std::string command_line = SO::Concatenate(EscapeCommandLineArgument(exe_path), " ",
                                                     EscapeCommandLineArgument(m_parameters.client_name), " ",
                                                     EscapeCommandLineArgument(result_temporary_file.GetPath()));

    try
    {
        int return_code;
        RunProgram(TC::ToWide(command_line), &return_code, SW_SHOWNA, true, true);

        const std::string encrypted_result = FileIO::ReadText(result_temporary_file.GetPath());

        // the results are encrypted by the client name
        Encryptor encryptor(Encryptor::Type::RijndaelHex, m_parameters.client_name);
        const std::string json_result = encryptor.Decrypt(encrypted_result);

        const JsonNode json_node = Json::Parse(json_result);

        if( json_node.IsString() )
        {
            m_tokenOrExceptionMessage = json_node.Get<std::string>();
        }

        else
        {
            m_tokenOrExceptionMessage = json_node.Get<OAuth2Token>();
        }
    }

    catch( const CSProException& exception )
    {
        m_tokenOrExceptionMessage = FormatText("There was an error running the OAuth 2.0 Authorizer for %s: %s",
                                               m_parameters.client_name.c_str(), exception.what());
    }
}
