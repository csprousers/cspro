#pragma once


enum class OAuth2ClientType
{
    Dropbox,
    GoogleDrive
};


struct OAuth2AuthenticationParameters
{
    OAuth2ClientType client_type;
    std::string client_name;
    std::string client_id;
    std::string client_secret;

    std::string oauth2_endpoint;
    std::string token_endpoint;

    // if empty, defaults to "error"
    std::string authorization_error_description_key;

    // keys and values should not be pre-encoded as URI components
    std::vector<std::tuple<std::string, std::string>> extra_authorization_parameters;
};
