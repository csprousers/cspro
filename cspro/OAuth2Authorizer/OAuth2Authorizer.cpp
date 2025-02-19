#include "stdafx.h"
#include "OAuth2Authorizer.h"
#include <zToolsO/Encryption.h>
#include <zToolsO/FileIO.h>
#include <zToolsO/Tools.h>
#include <zJson/Json.h>
#include <zNetwork/HttpConnection.h>
#include <zNetwork/OAuth2Authorizer.h>


/////////////////////////////////////////////////////////////////////////////
// The one and only OAuth2AuthorizerApp object
OAuth2AuthorizerApp theApp;


OAuth2AuthorizerApp::OAuth2AuthorizerApp()
{
    InitializeCSProEnvironment();
}


BOOL OAuth2AuthorizerApp::InitInstance()
{
    CWinApp::InitInstance();

    AfxEnableControlContainer();

    SetRegistryKey(L"U.S. Census Bureau");

    try
    {
        if( __argc != 3 )
            throw CSProException("Invalid command line arguments");

        const std::string client_name = TC::ToUtf8(__wargv[1]);
        const std::wstring result_file_path = MakeFullPath(GetWorkingFolder(), __wargv[2]);

        const OAuth2ClientType client_type =
            ( client_name == "Dropbox" )      ? OAuth2ClientType::Dropbox :
            ( client_name == "Google Drive" ) ? OAuth2ClientType::GoogleDrive :
                                                throw CSProException("Unknown client name: " + client_name);

        const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

        try
        {
            OAuth2Authorizer oauth2_authorizer(client_type, HttpConnection::Create());
            const OAuth2Token oauth2_token = oauth2_authorizer.Authorize();
            json_writer->Write(oauth2_token);
        }

        catch( const CSProException& exception )
        {
            json_writer->Write(exception.what());
        }

        // write the result encrypted by the client name
        Encryptor encryptor(Encryptor::Type::RijndaelHex, client_name);
        const std::string encrypted_result = encryptor.Encrypt(json_writer->GetString());

        FileIO::WriteText(result_file_path, encrypted_result, false);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }

    return FALSE;
}
