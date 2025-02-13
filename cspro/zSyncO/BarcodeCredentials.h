#pragma once

#include <zToolsO/Encryption.h>


class BarcodeCredentials
{
private:
    static const TCHAR FormatVersion      = '1';
    static const TCHAR DecryptionVerifier = '@';
    static const TCHAR LengthOffset       = 32;

    static Encryptor CreateEncryptor(const CString& application_name)
    {
        // they encryptor key will be the upper-case application name
        CString key = application_name;
        key.MakeUpper();

        return Encryptor(Encryptor::Type::RijndaelBase64, key);
    }

public:
    static CString Encode(const CString& application_name, const CString& username, const CString& password)
    {
        // the credentials will be encrypted as: <verifier><length of username><length of password><username><password><verifier>
        CString credential_string;
        credential_string.Format(_T("%c%c%c%s%s%c"), DecryptionVerifier, (TCHAR)( LengthOffset + username.GetLength() ),
           (TCHAR)( LengthOffset + password.GetLength() ), (LPCTSTR)username, (LPCTSTR)password, DecryptionVerifier);

        ASSERT(credential_string.GetLength() == ( 4 + username.GetLength() + password.GetLength() ));

        // the credential version will be prepended to the Base64 representation of the credential string
        return FormatText(_T("%c%s"), FormatVersion, CreateEncryptor(application_name).Encrypt(credential_string).c_str());
    }

    static std::optional<std::tuple<std::wstring, std::wstring>> Decode(const CString& application_name, const CString& encrypted_credential_string)
    {
        // check the credential version
        const TCHAR* encrypted_credential_string_buffer = (LPCTSTR)encrypted_credential_string;

        if( *encrypted_credential_string_buffer == 0 || *encrypted_credential_string_buffer != FormatVersion )
            return std::nullopt;

        std::wstring credential_string = CreateEncryptor(application_name).Decrypt(encrypted_credential_string_buffer + 1);

        // check the credential string
        if( credential_string.length() < 4 || credential_string[0] != DecryptionVerifier ||
            credential_string[credential_string.length() - 1] != DecryptionVerifier )
        {
            return std::nullopt;
        }

        TCHAR username_length = credential_string[1] - LengthOffset;
        TCHAR password_length = credential_string[2] - LengthOffset;

        if( credential_string.length() != (size_t)( 4 + username_length + password_length ) )
            return std::nullopt;
        
        return std::make_tuple(credential_string.substr(3, username_length),
                               credential_string.substr(3 + username_length, password_length));
    }
};
