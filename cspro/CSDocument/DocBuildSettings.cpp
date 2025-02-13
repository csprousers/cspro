#include "StdAfx.h"
#include "DocBuildSettings.h"


DocBuildSettings DocBuildSettings::SettingsForCSDocBuildToClipboard()
{
    DocBuildSettings build_settings;

    ASSERT(build_settings.GetStylesheetAction() == StylesheetAction::Embed);
    ASSERT(build_settings.GetTitleLinkageAction() == TitleLinkageAction::Suppress);
    ASSERT(build_settings.GetDocSetLinkageAction() == DocSetLinkageAction::Suppress);
    ASSERT(build_settings.GetProjectLinkageAction() == ProjectLinkageAction::Suppress);
    ASSERT(build_settings.GetExternalLinkageAction() == ExternalLinkageAction::Suppress);
    ASSERT(build_settings.GetLogicLinkageAction() == LogicLinkageAction::CSProUsers);
    ASSERT(build_settings.GetImageAction() == ImageAction::DataUrl);

    return build_settings;
}


DocBuildSettings DocBuildSettings::SettingsForCSDocQuickCompilation()
{
    DocBuildSettings build_settings;

    build_settings.m_stylesheetAction = StylesheetAction::SourceAbsolute;
    ASSERT(build_settings.GetTitleLinkageAction() == TitleLinkageAction::Suppress);
    ASSERT(build_settings.GetDocSetLinkageAction() == DocSetLinkageAction::Suppress);
    ASSERT(build_settings.GetProjectLinkageAction() == ProjectLinkageAction::Suppress);
    ASSERT(build_settings.GetExternalLinkageAction() == ExternalLinkageAction::Suppress);
    build_settings.m_logicLinkageAction = LogicLinkageAction::Suppress;
    build_settings.m_imageAction = ImageAction::SourceAbsolute;

    return build_settings;
}


DocBuildSettings DocBuildSettings::DefaultSettingsForCSDocBuildToHtml()
{
    DocBuildSettings build_settings;

    // even the default settings will be set so that they are serialized (so the user can see the settings)
    build_settings.m_buildType = BuildType::HtmlPages;
    build_settings.m_buildDocumentsUsingRelativePaths = true;
    build_settings.m_stylesheetAction = StylesheetAction::Embed;
    build_settings.m_titleLinkageAction = TitleLinkageAction::Suppress;
    build_settings.m_docSetLinkageAction = DocSetLinkageAction::Link;
    build_settings.m_projectLinkageAction = ProjectLinkageAction::Link;
    build_settings.m_externalLinkageAction = ExternalLinkageAction::RelativeToOutput;
    build_settings.m_logicLinkageAction = LogicLinkageAction::CSProUsers;
    build_settings.m_imageAction = ImageAction::DataUrl;

    return build_settings;
}


DocBuildSettings DocBuildSettings::DefaultSettingsForCSDocBuildToPdf()
{
    DocBuildSettings build_settings;

    build_settings.m_buildType = BuildType::Pdf;
    build_settings.m_buildDocumentsUsingRelativePaths = false;
    build_settings.m_stylesheetAction = StylesheetAction::SourceAbsolute;
    build_settings.m_titleLinkageAction = TitleLinkageAction::Suppress;
    build_settings.m_docSetLinkageAction = DocSetLinkageAction::Link;
    build_settings.m_projectLinkageAction = ProjectLinkageAction::Link;
    build_settings.m_externalLinkageAction = ExternalLinkageAction::Suppress;
    build_settings.m_imageAction = ImageAction::SourceAbsolute;

    return build_settings;
}


DocBuildSettings DocBuildSettings::DefaultSettingsForBuildType(BuildType build_type)
{
    if( build_type == BuildType::Pdf )
        return DefaultSettingsForCSDocBuildToPdf();

    DocBuildSettings build_settings;

    build_settings.m_buildType = build_type;

    if( build_type == BuildType::HtmlPages || build_type == BuildType::HtmlWebsite )
    {
        build_settings.m_buildDocumentsUsingRelativePaths = true;
        build_settings.m_stylesheetAction = StylesheetAction::Directory;
        build_settings.m_stylesheetDirectory = _T("css");
        build_settings.m_titleLinkageAction = TitleLinkageAction::Suppress;
        build_settings.m_docSetLinkageAction = DocSetLinkageAction::Link;
        build_settings.m_projectLinkageAction = ProjectLinkageAction::Link;
        build_settings.m_externalLinkageAction = ExternalLinkageAction::RelativeToOutput;
        build_settings.m_logicLinkageAction = LogicLinkageAction::CSProUsers;
        build_settings.m_imageAction = ImageAction::RelativeToOutput;
    }

    else 
    {
        ASSERT(build_type == BuildType::Chm);

        build_settings.m_buildDocumentsUsingRelativePaths = false;
        build_settings.m_stylesheetAction = StylesheetAction::Directory;
        build_settings.m_stylesheetDirectory = _T(".");
        build_settings.m_titleLinkageAction = TitleLinkageAction::Suppress;
        build_settings.m_docSetLinkageAction = DocSetLinkageAction::Link;
        build_settings.m_projectLinkageAction = ProjectLinkageAction::Link;
        build_settings.m_externalLinkageAction = ExternalLinkageAction::Suppress;
        build_settings.m_logicLinkageAction = LogicLinkageAction::CSProUsers;
        build_settings.m_imageAction = ImageAction::Directory;
        build_settings.m_imageDirectory = _T(".");
    }

    return build_settings;
}


void DocBuildSettings::FixIncompatibleSettingsForBuildType(bool throw_exceptions_for_serious_issues)
{
    if( GetBuildType() == BuildType::Chm ||
        GetBuildType() == BuildType::Pdf )
    {
        if( BuildDocumentsUsingRelativePaths() )
            m_buildDocumentsUsingRelativePaths = false;

        if( GetExternalLinkageAction() != ExternalLinkageAction::Suppress &&
            GetExternalLinkageAction() != ExternalLinkageAction::Forbid )
        {
            if( throw_exceptions_for_serious_issues )
                throw CSProException("It is not possible to link to external documents when building to PDF format.");

            m_externalLinkageAction = ExternalLinkageAction::Suppress;
        }
    }

    if( GetBuildType() == BuildType::Pdf )
    {
        if( GetStylesheetAction() == StylesheetAction::Directory )
            m_stylesheetAction = StylesheetAction::SourceAbsolute;

        if( GetImageAction() != ImageAction::SourceAbsolute &&
            GetImageAction() != ImageAction::SourceRelative )
        {
            m_imageAction = ImageAction::SourceAbsolute;
        }
    }
}


bool DocBuildSettings::HasCustomSettings() const
{
    return ( !m_outputDirectory.empty() ||
             !m_outputName.empty() ||
             m_buildType.has_value() ||
             !m_pathAdjustments.empty() ||
             m_buildDocumentsUsingRelativePaths.has_value() ||
             m_stylesheetAction.has_value() || !m_stylesheetDirectory.empty() ||
             m_titleLinkageAction.has_value() || !m_titleLinkPrefix.empty() ||
             m_docSetLinkageAction.has_value() ||
             m_projectLinkageAction.has_value() ||
             m_externalLinkageAction.has_value() || !m_externalLinkDirectory.empty() ||
             m_logicLinkageAction.has_value() ||
             m_imageAction.has_value() || !m_imageDirectory.empty() ||
             !m_chmButtons.empty() ||
             !m_wkhtmltopdfFlags.empty() );
}


std::tuple<std::wstring, std::wstring, bool>* DocBuildSettings::FindPathAdjustment(const std::wstring& path)
{
    for( std::tuple<std::wstring, std::wstring, bool>& path_and_adjustment : m_pathAdjustments )
    {
        if( SO::EqualsNoCase(std::get<0>(path_and_adjustment), path) )
            return &path_and_adjustment;
    }

    return nullptr;
}


bool DocBuildSettings::ContainsChmButton(const std::tuple<std::wstring, std::wstring>& link_and_text) const
{
    return ( std::find(m_chmButtons.cbegin(), m_chmButtons.cend(), link_and_text) != m_chmButtons.cend() );
}


std::tuple<DocBuildSettings::WkhtmltopdfFlagType, std::wstring, std::wstring>* DocBuildSettings::FindWkhtmltopdfFlag(WkhtmltopdfFlagType type, const std::wstring& flag)
{
    for( std::tuple<WkhtmltopdfFlagType, std::wstring, std::wstring>& type_flag_and_value : m_wkhtmltopdfFlags )
    {
        if( std::get<0>(type_flag_and_value) == type &&
            std::get<1>(type_flag_and_value) == flag )
        {
            return &type_flag_and_value;
        }
    }

    return nullptr;
}


DocBuildSettings DocBuildSettings::ApplySettings(DocBuildSettings base_build_settings, const DocBuildSettings& other_build_settings)
{
    auto assign_if_not_empty = [](auto& lhs_value, const auto& rhs_value) { if( !rhs_value.empty() ) lhs_value = rhs_value; };
    auto assign_if_has_value = [](auto& lhs_value, const auto& rhs_value) { if( rhs_value.has_value() ) lhs_value = *rhs_value; };

    assign_if_not_empty(base_build_settings.m_outputDirectory, other_build_settings.m_outputDirectory);
    assign_if_not_empty(base_build_settings.m_outputName, other_build_settings.m_outputName);

    assign_if_has_value(base_build_settings.m_buildType, other_build_settings.m_buildType);

    for( const auto& [other_path, other_adjustment, other_relative_to_path] : other_build_settings.m_pathAdjustments )
    {
        std::tuple<std::wstring, std::wstring, bool>* existing_path_and_adjustment = base_build_settings.FindPathAdjustment(other_path);

        if( existing_path_and_adjustment != nullptr )
        {
            std::get<1>(*existing_path_and_adjustment) = other_adjustment;
        }

        else
        {
            base_build_settings.m_pathAdjustments.emplace_back(other_path, other_adjustment, other_relative_to_path);
        }
    }

    assign_if_has_value(base_build_settings.m_buildDocumentsUsingRelativePaths, other_build_settings.m_buildDocumentsUsingRelativePaths);

    assign_if_has_value(base_build_settings.m_stylesheetAction, other_build_settings.m_stylesheetAction);
    assign_if_not_empty(base_build_settings.m_stylesheetDirectory, other_build_settings.m_stylesheetDirectory);

    assign_if_has_value(base_build_settings.m_titleLinkageAction, other_build_settings.m_titleLinkageAction);
    assign_if_not_empty(base_build_settings.m_titleLinkPrefix, other_build_settings.m_titleLinkPrefix);

    assign_if_has_value(base_build_settings.m_docSetLinkageAction, other_build_settings.m_docSetLinkageAction);

    assign_if_has_value(base_build_settings.m_projectLinkageAction, other_build_settings.m_projectLinkageAction);

    assign_if_has_value(base_build_settings.m_externalLinkageAction, other_build_settings.m_externalLinkageAction);
    assign_if_not_empty(base_build_settings.m_externalLinkDirectory, other_build_settings.m_externalLinkDirectory);

    assign_if_has_value(base_build_settings.m_logicLinkageAction, other_build_settings.m_logicLinkageAction);

    assign_if_has_value(base_build_settings.m_imageAction, other_build_settings.m_imageAction);
    assign_if_not_empty(base_build_settings.m_imageDirectory, other_build_settings.m_imageDirectory);

    for( const std::tuple<std::wstring, std::wstring>& other_link_and_text : other_build_settings.m_chmButtons )
    {
        if( !base_build_settings.ContainsChmButton(other_link_and_text) )
            base_build_settings.m_chmButtons.emplace_back(other_link_and_text);
    }

    for( const auto& [other_type, other_flag, other_value] : other_build_settings.m_wkhtmltopdfFlags )
    {
        std::tuple<DocBuildSettings::WkhtmltopdfFlagType, std::wstring, std::wstring>* existing_type_flag_and_value = base_build_settings.FindWkhtmltopdfFlag(other_type, other_flag);

        if( existing_type_flag_and_value != nullptr )
        {
            std::get<2>(*existing_type_flag_and_value) = other_value;
        }

        else
        {
            base_build_settings.m_wkhtmltopdfFlags.emplace_back(other_type, other_flag, other_value);
        }
    }

    return base_build_settings;
}


std::wstring DocBuildSettings::GetEvaluatedOutputName(const DocSetSpec& doc_set_spec) const
{
    return m_outputName.empty() ? PortableFunctions::PathGetFilenameWithoutExtension(doc_set_spec.GetFilename()) : 
                                  m_outputName;                                                                                          
}



// --------------------------------------------------------------------------
// serialization
// --------------------------------------------------------------------------

CREATE_JSON_KEY(adjustment)
CREATE_JSON_KEY(buildDocumentsUsingRelativePaths)
CREATE_JSON_KEY(buildType)
CREATE_JSON_KEY(chmButtons)
CREATE_JSON_KEY(documentSetLinkage)
CREATE_JSON_KEY(externalLinkage)
CREATE_JSON_KEY(externalLinkDirectory)
CREATE_JSON_KEY(imageOutput)
CREATE_JSON_KEY(imageOutputDirectory)
CREATE_JSON_KEY(logicLinkage)
CREATE_JSON_KEY(outputDirectory)
CREATE_JSON_KEY(outputName)
CREATE_JSON_KEY(pathAdjustments)
CREATE_JSON_KEY(projectLinkage)
CREATE_JSON_KEY(relativeToPath)
CREATE_JSON_KEY(stylesheetOutput)
CREATE_JSON_KEY(stylesheetOutputDirectory)
CREATE_JSON_KEY(titleLinkage)
CREATE_JSON_KEY(titleLinkPrefix)
CREATE_JSON_KEY(wkhtmltopdfFlags)

CREATE_JSON_VALUE(CHM)
CREATE_JSON_VALUE(cover)
CREATE_JSON_VALUE_TEXT_OVERRIDE(CSProUsers, CSPro Users)
CREATE_JSON_VALUE(dataUrl)
CREATE_JSON_VALUE(directory)
CREATE_JSON_VALUE(embed)
CREATE_JSON_VALUE(forbid)
CREATE_JSON_VALUE(global)
CREATE_JSON_VALUE(HTML)
CREATE_JSON_VALUE(link)
CREATE_JSON_VALUE(page)
CREATE_JSON_VALUE(PDF)
CREATE_JSON_VALUE(prefix)
CREATE_JSON_VALUE(outputNamePrefix)
CREATE_JSON_VALUE(relativeToOutput)
CREATE_JSON_VALUE(sourceAbsolute)
CREATE_JSON_VALUE(sourceRelative)
CREATE_JSON_VALUE(suppress)
CREATE_JSON_VALUE(toc)
CREATE_JSON_VALUE(website)

CREATE_ENUM_JSON_SERIALIZER(DocBuildSettings::BuildType,
    { DocBuildSettings::BuildType::HtmlPages,   JV::HTML },
    { DocBuildSettings::BuildType::HtmlWebsite, JV::website },
    { DocBuildSettings::BuildType::Chm,         JV::CHM },
    { DocBuildSettings::BuildType::Pdf,         JV::PDF })

CREATE_ENUM_JSON_SERIALIZER(DocBuildSettings::StylesheetAction,
    { DocBuildSettings::StylesheetAction::Embed,          JV::embed },
    { DocBuildSettings::StylesheetAction::SourceAbsolute, JV::sourceAbsolute },
    { DocBuildSettings::StylesheetAction::SourceRelative, JV::sourceRelative },
    { DocBuildSettings::StylesheetAction::Directory,      JV::directory})

CREATE_ENUM_JSON_SERIALIZER(DocBuildSettings::TitleLinkageAction,
    { DocBuildSettings::TitleLinkageAction::Suppress,         JV::suppress },
    { DocBuildSettings::TitleLinkageAction::Prefix,           JV::prefix },
    { DocBuildSettings::TitleLinkageAction::OutputNamePrefix, JV::outputNamePrefix })

CREATE_ENUM_JSON_SERIALIZER(DocBuildSettings::DocSetLinkageAction,
    { DocBuildSettings::DocSetLinkageAction::Suppress, JV::suppress },
    { DocBuildSettings::DocSetLinkageAction::Link,     JV::link })

CREATE_ENUM_JSON_SERIALIZER(DocBuildSettings::ProjectLinkageAction,
    { DocBuildSettings::ProjectLinkageAction::Suppress, JV::suppress },
    { DocBuildSettings::ProjectLinkageAction::Link,     JV::link })

CREATE_ENUM_JSON_SERIALIZER(DocBuildSettings::ExternalLinkageAction,
    { DocBuildSettings::ExternalLinkageAction::Suppress,         JV::suppress },
    { DocBuildSettings::ExternalLinkageAction::Forbid,           JV::forbid },
    { DocBuildSettings::ExternalLinkageAction::SourceAbsolute,   JV::sourceAbsolute },
    { DocBuildSettings::ExternalLinkageAction::SourceRelative,   JV::sourceRelative },
    { DocBuildSettings::ExternalLinkageAction::RelativeToOutput, JV::relativeToOutput },
    { DocBuildSettings::ExternalLinkageAction::Directory,        JV::directory })

CREATE_ENUM_JSON_SERIALIZER(DocBuildSettings::LogicLinkageAction,
    { DocBuildSettings::LogicLinkageAction::Suppress,   JV::suppress },
    { DocBuildSettings::LogicLinkageAction::CSProUsers, JV::CSProUsers },
    { DocBuildSettings::LogicLinkageAction::Project,    JK::project })

CREATE_ENUM_JSON_SERIALIZER(DocBuildSettings::ImageAction,
    { DocBuildSettings::ImageAction::DataUrl,          JV::dataUrl },
    { DocBuildSettings::ImageAction::SourceAbsolute,   JV::sourceAbsolute },
    { DocBuildSettings::ImageAction::SourceRelative,   JV::sourceRelative },
    { DocBuildSettings::ImageAction::RelativeToOutput, JV::relativeToOutput},
    { DocBuildSettings::ImageAction::Directory,        JV::directory })

CREATE_ENUM_JSON_SERIALIZER(DocBuildSettings::WkhtmltopdfFlagType,
    { DocBuildSettings::WkhtmltopdfFlagType::Global,          JV::global},
    { DocBuildSettings::WkhtmltopdfFlagType::Cover,           JV::cover },
    { DocBuildSettings::WkhtmltopdfFlagType::TableOfContents, JV::toc },
    { DocBuildSettings::WkhtmltopdfFlagType::Page,            JV::page })


void DocBuildSettings::Compile(DocSetCompiler& doc_set_compiler, const JsonNode<wchar_t>& json_node)
{
    // output directory
    if( json_node.Contains(JK::outputDirectory) )
    {
        std::wstring output_directory = json_node.GetAbsolutePath(JK::outputDirectory);

        if( doc_set_compiler.CheckIfOverridesWithNewValue(JK::outputDirectory, output_directory, m_outputDirectory) )
            m_outputDirectory = std::move(output_directory);
    }

    // output name
    if( json_node.Contains(JK::outputName) )
    {
        std::wstring output_name = json_node.Get<std::wstring>(JK::outputName);

        if( doc_set_compiler.CheckIfOverridesWithNewValue(JK::outputName, output_name, m_outputName) )
            m_outputName = std::move(output_name);
    }

    // build type
    if( json_node.Contains(JK::buildType) )
        m_buildType = json_node.Get<BuildType>(JK::buildType);

    // path adjustments
    if( json_node.Contains(JK::pathAdjustments) )
    {
        for( const auto& path_adjustment_node : json_node.GetArray(JK::pathAdjustments) )
        {
            std::wstring path = path_adjustment_node.GetAbsolutePath(JK::path);

            if( !doc_set_compiler.CheckIfDirectoryExists(_T("path adjustment"), path) )
                continue;

            const bool relative_to_path = path_adjustment_node.GetOrDefault(JK::relativeToPath, true);
            std::wstring adjustment = relative_to_path ? path_adjustment_node.Get<std::wstring>(JK::adjustment) :
                                                         path_adjustment_node.GetAbsolutePath(JK::adjustment);

            std::tuple<std::wstring, std::wstring, bool>* existing_path_and_adjustment = FindPathAdjustment(path);

            if( existing_path_and_adjustment != nullptr )
            {
                if( doc_set_compiler.CheckIfOverridesWithNewValue(JK::adjustment, adjustment, std::get<1>(*existing_path_and_adjustment)) )
                {
                    std::get<1>(*existing_path_and_adjustment) = std::move(adjustment);
                    std::get<2>(*existing_path_and_adjustment) = relative_to_path;
                }
            }

            else
            {
                m_pathAdjustments.emplace_back(std::move(path), std::move(adjustment), relative_to_path);
            }
        }
    }

    // build documents using relative paths
    if( json_node.Contains(JK::buildDocumentsUsingRelativePaths) )
        m_buildDocumentsUsingRelativePaths = json_node.Get<bool>(JK::buildDocumentsUsingRelativePaths);

    // stylesheet output
    if( json_node.Contains(JK::stylesheetOutput) )
        m_stylesheetAction = json_node.Get<StylesheetAction>(JK::stylesheetOutput);

    if( m_stylesheetAction == StylesheetAction::Directory )
    {
        if( json_node.Contains(JK::stylesheetOutputDirectory) )
        {
            std::wstring stylesheet_directory = json_node.Get<std::wstring>(JK::stylesheetOutputDirectory);

            if( doc_set_compiler.CheckIfOverridesWithNewValue(JK::stylesheetOutputDirectory, stylesheet_directory, m_stylesheetDirectory) )
                m_stylesheetDirectory = std::move(stylesheet_directory);
        }

        if( m_stylesheetDirectory.empty() )
        {
            doc_set_compiler.AddError(FormatTextCS2WS(_T("When copying stylesheets to a specific directory, you must specify the directory with the key '%s'."),
                                                      JK::stylesheetOutputDirectory));
        }
    }

    // title links
    if( json_node.Contains(JK::titleLinkage) )
        m_titleLinkageAction = json_node.Get<TitleLinkageAction>(JK::titleLinkage);

    if( m_titleLinkageAction.has_value() && *m_titleLinkageAction != TitleLinkageAction::Suppress )
    {
        if( json_node.Contains(JK::titleLinkPrefix) )
        {
            std::wstring title_link_prefix = json_node.Get<std::wstring>(JK::titleLinkPrefix);

            if( doc_set_compiler.CheckIfOverridesWithNewValue(JK::titleLinkPrefix, title_link_prefix, m_titleLinkPrefix) )
                m_titleLinkPrefix = std::move(title_link_prefix);
        }

        if( m_titleLinkPrefix.empty() )
        {
            doc_set_compiler.AddError(FormatTextCS2WS(_T("When creating links around titles, you must specify the prefix with the key '%s'."),
                                                      JK::titleLinkPrefix));
        }
    }

    // Document Set links
    if( json_node.Contains(JK::documentSetLinkage) )
        m_docSetLinkageAction = json_node.Get<DocSetLinkageAction>(JK::documentSetLinkage);

    // project links
    if( json_node.Contains(JK::projectLinkage) )
        m_projectLinkageAction = json_node.Get<ProjectLinkageAction>(JK::projectLinkage);

    // external links
    if( json_node.Contains(JK::externalLinkage) )
        m_externalLinkageAction = json_node.Get<ExternalLinkageAction>(JK::externalLinkage);

    if( m_externalLinkageAction == ExternalLinkageAction::Directory )
    {
        if( json_node.Contains(JK::externalLinkDirectory) )
        {
            std::wstring external_link_directory = json_node.Get<std::wstring>(JK::externalLinkDirectory);

            if( doc_set_compiler.CheckIfOverridesWithNewValue(JK::externalLinkDirectory, external_link_directory, m_externalLinkDirectory) )
                m_externalLinkDirectory = std::move(external_link_directory);
        }

        if( m_externalLinkDirectory.empty() )
        {
            doc_set_compiler.AddError(FormatTextCS2WS(_T("When indicating that external CSPro Documents will be built to specific directory, you must specify the directory with the key '%s'."),
                                                      JK::externalLinkDirectory));
        }
    }

    // logic links
    if( json_node.Contains(JK::logicLinkage) )
        m_logicLinkageAction = json_node.Get<LogicLinkageAction>(JK::logicLinkage);

    // image output
    if( json_node.Contains(JK::imageOutput) )
        m_imageAction = json_node.Get<ImageAction>(JK::imageOutput);

    if( m_imageAction == ImageAction::Directory )
    {
        if( json_node.Contains(JK::imageOutputDirectory) )
        {
            std::wstring image_directory = json_node.Get<std::wstring>(JK::imageOutputDirectory);

            if( doc_set_compiler.CheckIfOverridesWithNewValue(JK::imageOutputDirectory, image_directory, m_imageDirectory) )
                m_imageDirectory = std::move(image_directory);
        }

        if( m_imageDirectory.empty() )
        {
            doc_set_compiler.AddError(FormatTextCS2WS(_T("When copying images to a specific directory, you must specify the directory with the key '%s'."),
                                                      JK::imageOutputDirectory));
        }
    }

    // CHM buttons
    if( json_node.Contains(JK::chmButtons) )
    {
        const auto& chm_buttons_node = json_node.Get(JK::chmButtons);

        if( doc_set_compiler.EnsureJsonNodeIsArray(chm_buttons_node, JK::chmButtons) )
        {
            for( const auto& chm_button_node : chm_buttons_node.GetArray() )
            {
                std::tuple<std::wstring, std::wstring> link_and_text(doc_set_compiler.JsonNodeGetStringWithWhitespaceCheck(chm_button_node, JK::link),
                                                                     doc_set_compiler.JsonNodeGetStringWithWhitespaceCheck(chm_button_node, JK::text));

                if( !ContainsChmButton(link_and_text) )
                    m_chmButtons.emplace_back(std::move(link_and_text));
            }
        }

        while( m_chmButtons.size() > 2 )
        {
            doc_set_compiler.AddError(FormatTextCS2WS(_T("Only two buttons can be added to a Compiled HTML Help file so the button with text '%s' will be removed."),
                                                      std::get<1>(m_chmButtons.back()).c_str()));
            m_chmButtons.pop_back();
        }
    }    

    // wkhtmltopdf flags
    if( json_node.Contains(JK::wkhtmltopdfFlags) )
    {
        const auto& flags_node = json_node.Get(JK::wkhtmltopdfFlags);

        if( doc_set_compiler.EnsureJsonNodeIsArray(flags_node, JK::wkhtmltopdfFlags) )
        {
            for( const auto& flag_node : flags_node.GetArray() )
            {
                // flags can be specified as strings (which default to the Global type),
                // or as objects, with an optional type and value
                WkhtmltopdfFlagType type = WkhtmltopdfFlagType::Global;
                std::wstring flag = flag_node.IsObject() ? flag_node.Get<std::wstring>(JK::flag) :
                                                           flag_node.Get<std::wstring>();
                std::wstring value;
                bool value_was_null = false;

                if( flag_node.IsObject() )
                {
                    if( flag_node.Contains(JK::type) )
                        type = flag_node.Get<WkhtmltopdfFlagType>(JK::type);

                    if( flag_node.Contains(JK::value) )
                    {
                        const auto& value_node = flag_node.Get(JK::value);
                        value_was_null = value_node.IsNull();

                        if( !value_was_null )
                            value = value_node.Get<std::wstring>();
                    }
                }

                std::tuple<DocBuildSettings::WkhtmltopdfFlagType, std::wstring, std::wstring>* existing_type_flag_and_value = FindWkhtmltopdfFlag(type, flag);

                // add a new flag
                if( existing_type_flag_and_value == nullptr )
                {
                    m_wkhtmltopdfFlags.emplace_back(type, std::move(flag), std::move(value));
                }

                // clear an existing one
                else if( value_was_null )
                {
                    size_t index_to_delete = existing_type_flag_and_value - m_wkhtmltopdfFlags.data();
                    m_wkhtmltopdfFlags.erase(m_wkhtmltopdfFlags.cbegin() + index_to_delete);
                }

                // or modify an existing one
                else if( doc_set_compiler.CheckIfOverridesWithNewValue(JK::value, value, std::get<2>(*existing_type_flag_and_value)) )
                {
                    std::get<2>(*existing_type_flag_and_value) = std::move(value);
                }
            }
        }
    }
}


void DocBuildSettings::WriteJson(JsonWriter& json_writer, bool write_to_new_json_object/* = true*/) const
{
    if( write_to_new_json_object )
        json_writer.BeginObject();

    if( !m_outputDirectory.empty() )
        json_writer.WriteRelativePathWithDirectorySupport(JK::outputDirectory, m_outputDirectory);

    json_writer.WriteIfNotBlank(JK::outputName, m_outputName);

    json_writer.WriteIfHasValue(JK::buildType, m_buildType);

    json_writer.WriteArrayIfNotEmpty(JK::pathAdjustments, m_pathAdjustments,
        [&](const std::tuple<std::wstring, std::wstring, bool>& path_and_adjustment)
        {
            json_writer.BeginObject()
                       .WriteRelativePathWithDirectorySupport(JK::path, std::get<0>(path_and_adjustment));

            if( std::get<2>(path_and_adjustment) )
            {
                json_writer.Write(JK::adjustment, std::get<1>(path_and_adjustment));
            }

            else
            {
                json_writer.WriteRelativePathWithDirectorySupport(JK::adjustment, std::get<1>(path_and_adjustment));
            }

            json_writer.Write(JK::relativeToPath, std::get<2>(path_and_adjustment))
                       .EndObject();
        });

    json_writer.WriteIfHasValue(JK::buildDocumentsUsingRelativePaths, m_buildDocumentsUsingRelativePaths);
    
    if( m_stylesheetAction.has_value() )
    {
        json_writer.Write(JK::stylesheetOutput, *m_stylesheetAction);

        if( *m_stylesheetAction == StylesheetAction::Directory )
            json_writer.WriteIfNotBlank(JK::stylesheetOutputDirectory, m_stylesheetDirectory);
    }

    if( m_titleLinkageAction.has_value() )
    {
        json_writer.Write(JK::titleLinkage, *m_titleLinkageAction);

        if( *m_titleLinkageAction != TitleLinkageAction::Suppress )
            json_writer.WriteIfNotBlank(JK::titleLinkPrefix, m_titleLinkPrefix);
    }

    json_writer.WriteIfHasValue(JK::documentSetLinkage, m_docSetLinkageAction);
    json_writer.WriteIfHasValue(JK::projectLinkage, m_projectLinkageAction);

    if( m_externalLinkageAction.has_value() )
    {
        json_writer.Write(JK::externalLinkage, *m_externalLinkageAction);

        if( *m_externalLinkageAction == ExternalLinkageAction::Directory )
            json_writer.WriteIfNotBlank(JK::externalLinkDirectory, m_externalLinkDirectory);
    }

    json_writer.WriteIfHasValue(JK::logicLinkage, m_logicLinkageAction);

    if( m_imageAction.has_value() )
    {
        json_writer.Write(JK::imageOutput, *m_imageAction);

        if( *m_imageAction == ImageAction::Directory )
            json_writer.WriteIfNotBlank(JK::imageOutputDirectory, m_imageDirectory);
    }

    json_writer.WriteArrayIfNotEmpty(JK::chmButtons, m_chmButtons,
        [&](const std::tuple<std::wstring, std::wstring>& link_and_text)
        {
            const JsonWriter::FormattingHolder json_formatting_holder = json_writer.SetFormattingType(JsonFormattingType::ObjectArraySingleLineSpacing);
            json_writer.BeginObject();
            json_writer.SetFormattingAction(JsonFormattingAction::TopmostObjectLineSplitSameLine);
            
            json_writer.Write(JK::text, std::get<1>(link_and_text))
                       .Write(JK::link, std::get<0>(link_and_text))
                       .EndObject();
        });

    json_writer.WriteArrayIfNotEmpty(JK::wkhtmltopdfFlags, m_wkhtmltopdfFlags,
        [&](const std::tuple<WkhtmltopdfFlagType, std::wstring, std::wstring>& type_flag_and_value)
        {
            const JsonWriter::FormattingHolder json_formatting_holder = json_writer.SetFormattingType(JsonFormattingType::ObjectArraySingleLineSpacing);
            json_writer.BeginObject();
            json_writer.SetFormattingAction(JsonFormattingAction::TopmostObjectLineSplitSameLine);

            json_writer.Write(JK::type, std::get<0>(type_flag_and_value))
                       .Write(JK::flag, std::get<1>(type_flag_and_value))
                       .WriteIfNotBlank(JK::value, std::get<2>(type_flag_and_value))
                       .EndObject();
        });

    if( write_to_new_json_object )
        json_writer.EndObject();
}


template<> std::optional<DocBuildSettings::BuildType> FromString<DocBuildSettings::BuildType>(wstring_view text_sv)
{
    return SO::Equals(text_sv, JV::HTML)    ? std::make_optional(DocBuildSettings::BuildType::HtmlPages) :
           SO::Equals(text_sv, JV::website) ? std::make_optional(DocBuildSettings::BuildType::HtmlWebsite) : 
           SO::Equals(text_sv, JV::CHM)     ? std::make_optional(DocBuildSettings::BuildType::Chm) : 
           SO::Equals(text_sv, JV::PDF)     ? std::make_optional(DocBuildSettings::BuildType::Pdf) :
                                              std::nullopt;
}
