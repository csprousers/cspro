#pragma once

#include <zUtilO/FromString.h>


class DocBuildSettings
{
public:
    static DocBuildSettings SettingsForCSDocBuildToClipboard();
    static DocBuildSettings SettingsForCSDocQuickCompilation();
    static DocBuildSettings DefaultSettingsForCSDocBuildToHtml();
    static DocBuildSettings DefaultSettingsForCSDocBuildToPdf();

    enum class BuildType;
    static DocBuildSettings DefaultSettingsForBuildType(BuildType build_type);

    void FixIncompatibleSettingsForBuildType(bool throw_exceptions_for_serious_issues);

    bool HasCustomSettings() const;

    static DocBuildSettings ApplySettings(DocBuildSettings base_build_settings, const DocBuildSettings& other_build_settings);

    // output directory:
    // the root directory for outputs
    // --------------------------------------------------------------------------
    const std::wstring& GetOutputDirectory() const  { return m_outputDirectory; }
    void SetOutputDirectory(std::wstring directory) { m_outputDirectory = std::move(directory); }

    // output name:
    // the name to override the default name
    // (which is based on the Document Set filename)
    // --------------------------------------------------------------------------
    const std::wstring& GetOutputName() const { return m_outputName; }
    std::wstring GetEvaluatedOutputName(const DocSetSpec& doc_set_spec) const;

    // build type:
    // the way to build Document Sets
    // --------------------------------------------------------------------------
    enum class BuildType { HtmlPages, HtmlWebsite, Chm, Pdf };

    const std::optional<BuildType>& GetBuildType() const { return m_buildType; }
    void SetBuildType(BuildType build_type)              { m_buildType = build_type; }

    // path adjustments:
    // if a source file starts with the directory, its path is adjusted;
    // the path may be adjusted relative to the source, or to an absolute
    // path based on the location of the build settings
    // the longest matching directory is used for the adjustment
    // --------------------------------------------------------------------------
    const std::vector<std::tuple<std::wstring, std::wstring, bool>>& GetPathAdjustments() const { return m_pathAdjustments; }

    // build documents using relative paths:
    // if false, all documents will be built into the same directory;
    // if true, all documents must be in the same directory, or a default document must be set
    // --------------------------------------------------------------------------
    bool BuildDocumentsUsingRelativePaths() const { return m_buildDocumentsUsingRelativePaths.value_or(true); }

    // stylesheet output:
    // where to put stylesheets;
    // if a directory, it is evaluated relative to the output directory
    // --------------------------------------------------------------------------
    enum class StylesheetAction { Embed, SourceAbsolute, SourceRelative, Directory };

    StylesheetAction GetStylesheetAction() const       { return m_stylesheetAction.value_or(StylesheetAction::Embed); }
    const std::wstring& GetStylesheetDirectory() const { ASSERT(m_stylesheetAction == StylesheetAction::Directory); return m_stylesheetDirectory; }

    // title links:
    // how to create links around titles
    // --------------------------------------------------------------------------
    enum class TitleLinkageAction { Suppress, Prefix, OutputNamePrefix };

    TitleLinkageAction GetTitleLinkageAction() const { return m_titleLinkageAction.value_or(TitleLinkageAction::Suppress); }
    const std::wstring& GetTitleLinkPrefix() const   { ASSERT(m_titleLinkageAction != TitleLinkageAction::Suppress); return m_titleLinkPrefix; }

    // Document Set links:
    // how to link to other documents in the same Document Set
    // --------------------------------------------------------------------------
    enum class DocSetLinkageAction { Suppress, Link };

    DocSetLinkageAction GetDocSetLinkageAction() const { return m_docSetLinkageAction.value_or(DocSetLinkageAction::Suppress); }

    // project links:
    // how to link to other documents in the same project (but not the same Document Set)
    // --------------------------------------------------------------------------
    enum class ProjectLinkageAction { Suppress, Link };

    ProjectLinkageAction GetProjectLinkageAction() const { return m_projectLinkageAction.value_or(ProjectLinkageAction::Suppress); }

    // external links:
    // how to link to other documents outside of the Document Set or project;
    // if a directory, it is evaluated relative to the output directory
    // --------------------------------------------------------------------------
    enum class ExternalLinkageAction { Suppress, Forbid, SourceAbsolute, SourceRelative, RelativeToOutput, Directory };

    ExternalLinkageAction GetExternalLinkageAction() const { return m_externalLinkageAction.value_or(ExternalLinkageAction::Suppress); }
    const std::wstring& GetExternalLinkDirectory() const { ASSERT(m_externalLinkageAction == ExternalLinkageAction::Directory); return m_externalLinkDirectory; }

    // logic links:
    // how to link to documents related to CSPro logic
    // --------------------------------------------------------------------------
    enum class LogicLinkageAction { Suppress, CSProUsers, Project };

    LogicLinkageAction GetLogicLinkageAction() const { return m_logicLinkageAction.value_or(LogicLinkageAction::CSProUsers); }

    // image output:
    // where to put images;
    // if a directory, it is evaluated relative to the output directory
    // --------------------------------------------------------------------------
    enum class ImageAction { DataUrl, SourceAbsolute, SourceRelative, RelativeToOutput, Directory };

    ImageAction GetImageAction() const            { return m_imageAction.value_or(ImageAction::DataUrl); }
    const std::wstring& GetImageDirectory() const { ASSERT(m_imageAction == ImageAction::Directory); return m_imageDirectory; }

    // CHM flags
	// --------------------------------------------------------------------------
    const std::vector<std::tuple<std::wstring, std::wstring>>& GetChmButtons() const { return m_chmButtons; }
	
	// PDF flags
	// --------------------------------------------------------------------------
    enum class WkhtmltopdfFlagType { Global, Cover, TableOfContents, Page };

	const std::vector<std::tuple<WkhtmltopdfFlagType, std::wstring, std::wstring>>& GetWkhtmltopdfFlags() const { return m_wkhtmltopdfFlags; }

    // serialization
    // --------------------------------------------------------------------------
    void Compile(DocSetCompiler& doc_set_compiler, const JsonNode<wchar_t>& json_node);
    void WriteJson(JsonWriter& json_writer, bool write_to_new_json_object = true) const;

private:
    std::tuple<std::wstring, std::wstring, bool>* FindPathAdjustment(const std::wstring& path);

    bool ContainsChmButton(const std::tuple<std::wstring, std::wstring>& link_and_text) const;

    std::tuple<WkhtmltopdfFlagType, std::wstring, std::wstring>* FindWkhtmltopdfFlag(WkhtmltopdfFlagType type, const std::wstring& flag);

private:
    std::wstring m_outputDirectory;
    std::wstring m_outputName;

    std::optional<BuildType> m_buildType;

    std::vector<std::tuple<std::wstring, std::wstring, bool>> m_pathAdjustments; // directory / adjustment / "relative to path" flag

    std::optional<bool> m_buildDocumentsUsingRelativePaths;

    std::optional<StylesheetAction> m_stylesheetAction;
    std::wstring m_stylesheetDirectory;

    std::optional<TitleLinkageAction> m_titleLinkageAction;
    std::wstring m_titleLinkPrefix;

    std::optional<DocSetLinkageAction> m_docSetLinkageAction;

    std::optional<ProjectLinkageAction> m_projectLinkageAction;

    std::optional<ExternalLinkageAction> m_externalLinkageAction;
    std::wstring m_externalLinkDirectory;

    std::optional<LogicLinkageAction> m_logicLinkageAction;

    std::optional<ImageAction> m_imageAction;
    std::wstring m_imageDirectory;

    std::vector<std::tuple<std::wstring, std::wstring>> m_chmButtons; // link / text
	
	std::vector<std::tuple<WkhtmltopdfFlagType, std::wstring, std::wstring>> m_wkhtmltopdfFlags; // type / flag / value
};


template<> std::optional<DocBuildSettings::BuildType> FromString<DocBuildSettings::BuildType>(wstring_view text_sv);
