#pragma once

#include <zMapping/zMapping.h>
#include <zMapping/IMapUI.h>
#include <zToolsO/PointerClasses.h>

class MappingProperties;


// --------------------------------------------------------------------------
// HTML-based implementation of mapping.
// --------------------------------------------------------------------------

class ZMAPPING_API HtmlMapUI : public IMapUI
{
public:
    HtmlMapUI(cs::non_null_shared_or_raw_ptr<const MappingProperties> mapping_properties);
    ~HtmlMapUI();

    // IMapUI overrides
    void Clear() override;

    bool SetTitle(SharableString title) override;

protected:
    std::string GetUrlOfMapHtml() const;
    std::string GetUrlForUrlOrFile(const std::string& url_or_file_path);

    void PostActionMessage(cs::string_sz action);
    void PostActionMessage(cs::string_sz action, const std::function<void(JsonWriter&)>& callback_function);

    void OnWebMessageReceived(std::string_view message_sv);

    // the following methods must be overridden by subclasses:
    virtual bool IsMapShowing() = 0;

    // OnPostActionMessage will only be called when the map is showing.
    virtual void OnPostActionMessage(SharableString action_message_json) = 0;

    virtual void OnSetWindowTitle(const std::string& title) = 0;

private:
    // The following methods, with the suffix IMIS ("if map is showing"), are mostly companion
    // functions to the IMapUI overrides that will only be called when the map is showing.
    void SetUpInitialMapIMIS();

    void SetTitleIMIS();

protected:
    cs::non_null_shared_or_raw_ptr<const MappingProperties> m_mappingProperties;

private:
    struct Data;
    std::unique_ptr<Data> m_data;
};
