#include "stdafx.h"
#include "ConnectResponse.h"
#include "OAuth2Token.h"
#include "UsernamePassword.h"


CREATE_JSON_KEY(apiVersion)
CREATE_JSON_KEY(client_id)
CREATE_JSON_KEY(client_secret)
CREATE_JSON_KEY(expires_in)
CREATE_JSON_KEY(grant_type)
CREATE_JSON_KEY(lastModified)
CREATE_JSON_KEY(path_lower)
CREATE_JSON_KEY(refresh_token)
CREATE_JSON_KEY(scope)
CREATE_JSON_KEY(server_modified)
CREATE_JSON_KEY(serverName)
CREATE_JSON_KEY(token_type)
CREATE_JSON_KEY(userName)

CREATE_JSON_VALUE(file)
CREATE_JSON_VALUE(folder)


// --------------------------------------------------------------------------
// ConnectResponse
// --------------------------------------------------------------------------

ConnectResponse ConnectResponse::CreateFromJson(const JsonNode& json_node)
{
    return ConnectResponse(json_node.Get<std::string>(JK::deviceId),
                           json_node.GetOrDefault(JK::apiVersion, 0.0));
}


void ConnectResponse::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::deviceId, m_serverDeviceId)
               .WriteIfNotBlank(JK::serverName, m_serverName)
               .WriteIfNotBlank(JK::userName, m_username)
               .WriteIfNot(JK::apiVersion, m_apiVersion, 0.0)
               .EndObject();
}



// --------------------------------------------------------------------------
// FileInfo
// --------------------------------------------------------------------------

CREATE_ENUM_JSON_SERIALIZER(FileInfo::FileType,
    { FileInfo::FileType::File,      JV::file },
    { FileInfo::FileType::Directory, JK::directory })


FileInfo FileInfo::CreateFromJson(const JsonNode& json_node)
{
    const FileType type = json_node.Get<FileType>(JK::type);
    std::string name = json_node.Get<std::string>(JK::name);
    std::string directory = json_node.Get<std::string>(JK::directory);

    if( type == FileType::File )
    {
        return FileInfo(type, std::move(name), std::move(directory),
                        json_node.Get<int64_t>(JK::size),
                        json_node.Contains(JK::lastModified) ? json_node.GetDate(JK::lastModified) : 0,
                        json_node.GetOrConstruct<std::string>(JK::md5));
    }

    else
    {
        ASSERT(type == FileType::Directory);

        return FileInfo(type, std::move(name), std::move(directory));
    }
}


FileInfo FileInfo::CreateFromDropboxJson(const JsonNode& json_node)
{
    constexpr std::string_view TagKey_sv = ".tag";
    const JsonNode tag_node = json_node.GetOrEmpty(TagKey_sv);
    FileInfo::FileType type;

    // if the .tag is missing, it is a file
    if( tag_node.IsEmpty() )
    {
        type = FileInfo::FileType::File;
    }

    else
    {
        static_assert(static_cast<size_t>(FileInfo::FileType::File) == 0 &&
                      static_cast<size_t>(FileInfo::FileType::Directory) == 1);

        type = static_cast<FileInfo::FileType>(tag_node.GetFromStringOptions({ JV::file, JV::folder }));
    }

    std::string name = json_node.Get<std::string>(JK::name);
    std::string directory = PortableFunctions::PathGetDirectory(json_node.Get<std::string>(JK::path_lower));

    if( type == FileType::File )
    {
        return FileInfo(type, std::move(name), std::move(directory),
                        json_node.Get<int64_t>(JK::size),
                        json_node.GetDate(JK::server_modified));
    }

    else
    {
        ASSERT(type == FileType::Directory);

        return FileInfo(type, std::move(name), std::move(directory));
    }
}


void FileInfo::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::type, m_type)
               .Write(JK::name, m_name)
               .Write(JK::directory, m_directory);

    if( m_type == FileType::File )
    {
        json_writer.Write(JK::size, m_size)
                   .WriteDate(JK::lastModified, m_lastModified)
                   .WriteIfHasValue(JK::md5, m_md5);
    }

    json_writer.EndObject();
}



// --------------------------------------------------------------------------
// OAuth2Token
// --------------------------------------------------------------------------

OAuth2Token OAuth2Token::CreateFromJson(const JsonNode& json_node)
{
    const JsonNode scope_node = json_node.GetOrEmpty(JK::scope);

    return OAuth2Token(json_node.Get<std::string>(JK::access_token),
                       json_node.GetOrDefault<int>(JK::expires_in, -1),
                       json_node.GetOrConstruct<std::string>(JK::token_type),
                       scope_node.IsString() ? scope_node.Get<std::string>() : std::string(),
                       json_node.GetOrConstruct<std::string>(JK::refresh_token));
}


void OAuth2Token::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::access_token, m_accessToken)
               .WriteIfNot(JK::expires_in, m_expiresIn, -1)
               .WriteIfNotBlank(JK::token_type, m_tokenType)
               .WriteIfNotBlank(JK::scope, m_scope)
               .WriteIfNotBlank(JK::refresh_token, m_refreshToken)
               .EndObject();
}



// --------------------------------------------------------------------------
// OAuth2TokenRequest
// --------------------------------------------------------------------------

void OAuth2TokenRequest::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::client_id, m_clientId)
               .Write(JK::client_secret, m_clientSecret);

    if( m_grantType == OAuth2TokenRequest::GrantType::Password )
    {
        json_writer.Write(JK::grant_type, "password")
                   .Write(JK::username, m_username)
                   .Write(JK::password, m_password);
    }

    else
    {
        json_writer.Write(JK::grant_type, "refresh_token")
                   .Write(JK::refresh_token, m_refreshToken);
    }

    json_writer.EndObject();
}



// --------------------------------------------------------------------------
// UsernamePassword
// --------------------------------------------------------------------------

UsernamePassword UsernamePassword::CreateFromJson(const JsonNode& json_node)
{
    return UsernamePassword
    {
        json_node.Get<std::string>(JK::username),
        json_node.Get<std::string>(JK::password)
    };
}


void UsernamePassword::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::username, username)
               .Write(JK::password, password)
               .EndObject();
}
