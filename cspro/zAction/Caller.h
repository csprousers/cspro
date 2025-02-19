#pragma once

#include <zToolsO/CancelFlag.h>

class HtmlViewCtrl;

namespace ActionInvoker { class Caller; }


class ActionInvoker::Caller
{
public:
    virtual ~Caller() { }

    // Adjusts relative paths to absolute paths (if possible) and normalizes any slashes.
    std::string EvaluateAbsolutePath(std::string path);
    std::string EvaluateAbsolutePath(std::string path, bool allow_special_directories);

    // --------------------------------------------------------------------------
    // methods that subclasses must override
    // --------------------------------------------------------------------------

    // Returns a unique ID that identifies this caller.
    virtual int GetCallerId() const = 0;

    // Returns the cancelation flag.
    virtual CancelFlag& GetCancelFlag() = 0;

    // Returns the caller's root directory.
    virtual std::string GetRootDirectory() = 0;

    // --------------------------------------------------------------------------
    // methods that subclasses can override
    // --------------------------------------------------------------------------

    // Indicates that this caller is a web view.
    virtual bool IsWebView() const { return false; }

    // Indicates that this caller is an external source (and likely requires the use of access tokens).
    virtual bool IsExternalCaller() const { return false; }

    // Returns whether the user has overridden the need to use access tokens (true == allowed access).
    virtual std::optional<bool> GetUserOverrodeAccessTokenRequirement() const { return std::nullopt; }

    // Indicates that, upon prompting, the user allowed or disallowed access (for this caller).
    virtual void SetUserOverrodeAccessTokenRequirement(bool /*allowed_access*/) { }

    // Returns whether the access token is valid (based on a set of access tokens maintained by the caller).
    virtual bool IsAccessTokenValid(const std::string& /*access_token*/) const { return false; }
};
