#pragma once

class HtmlViewCtrl;


namespace ActionInvoker
{
    class Caller
    {
    public:
        using WebViewTag = typename std::conditional<OnWindowsDesktop(), HtmlViewCtrl*, int>::type;

        virtual ~Caller() { }

        // sets the cancelation flag
        void SetCancelFlag(bool flag) { GetCancelFlag() = flag; }

        bool IsFromWebView(WebViewTag web_view_tag) const { return ( web_view_tag == GetWebViewTag() ); }
        bool IsFromWebView(const Caller& caller) const    { return ( this == &caller || caller.GetWebViewTag() == GetWebViewTag() ); }

        // adjusts relative paths to absolute (if possible) and normalizes any slashes
        std::wstring EvaluateAbsolutePath(std::wstring path);
        std::wstring EvaluateAbsolutePath(std::wstring path, bool allow_special_directories);

        // --------------------------------------------------------------------------
        // methods that subclasses must override
        // --------------------------------------------------------------------------

        virtual bool& GetCancelFlag() = 0;

        virtual std::wstring GetRootDirectory() = 0;

        // --------------------------------------------------------------------------
        // methods that subclasses can override
        // --------------------------------------------------------------------------

        // indicates that this caller is an external source (and likely requires the use of access tokens)
        virtual bool IsExternalCaller() const { return false; }

        // returns whether the user has overriden the need to use access tokens (true == allowed access)
        virtual std::optional<bool> GetUserOverrodeAccessTokenRequirement() const { return std::nullopt; }

        // indicates that, upon prompting, the user allowed or disallowed access (for this caller)
        virtual void SetUserOverrodeAccessTokenRequirement(bool /*allowed_access*/) { }

        // returns whether the access token is valid (based on a set of access tokens maintained by the caller)
        virtual bool IsAccessTokenValid(const std::wstring& /*access_token*/) const { return false; }

        // a web view should be able to identify itself with the tag
        virtual std::optional<WebViewTag> GetWebViewTag() const { return std::nullopt; }
    };
}
