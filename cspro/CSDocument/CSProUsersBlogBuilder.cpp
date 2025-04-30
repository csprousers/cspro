#include "StdAfx.h"
#include "CSProUsersBlogBuilder.h"
#include "CSDocCompiler.h"
#include "HtmlTags.h"
#include <zToolsO/File.h>
#include <zReportO/HtmlTagModifier.h>


// --------------------------------------------------------------------------
// CSDocCompilerSettingsForCSProUsersBlog
// --------------------------------------------------------------------------

class CSDocCompilerSettingsForCSProUsersBlog : public CSDocCompilerSettings
{
public:
    CSDocCompilerSettingsForCSProUsersBlog(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec);

    // Resets the title / author / tags.
    void Reset();

    const std::string& GetTitle() const  { return m_title; }
    const std::string& GetAuthor() const { return m_author; }
    const std::string& GetTags() const   { return m_tags; }

protected:
    void SetTitleForCompilationFilePath(const std::string& title) override;

    void AddMetadata(std::string_view attribute_sv, std::string_view value_sv) override;

    bool AddHtmlHeader() const override { return false; }
    bool AddHtmlFooter() const override { return false; }

    bool TitleIsRequired() const override    { return true; }
    bool AddTitleToDocument() const override { return false; }

    std::string CreateUrlForTopic(const std::string& project, const std::string& path) override;
    std::string CreateUrlForImageFile(const std::string& path) override;
    std::string CreateUrlForResource(const std::string& resource) override;

    bool OpenExternalLinksInSeparateWindow() const override { return false; }

private:
    static std::string GetBlogDate(const std::string& file_path);

    std::string CreateUrlForFile(const std::string& file_path, const char* type) const;

private:
    std::string m_blogDirectory;
    std::string m_title;
    std::string m_author;
    std::string m_tags;
};


CSDocCompilerSettingsForCSProUsersBlog::CSDocCompilerSettingsForCSProUsersBlog(cs::non_null_shared_or_raw_ptr<DocSetSpec> doc_set_spec)
    :   CSDocCompilerSettings(std::move(doc_set_spec)),
        m_blogDirectory(PortableFunctions::PathGetDirectory(m_docSetSpec->GetFilePath()))
{
}


void CSDocCompilerSettingsForCSProUsersBlog::Reset()
{
    m_title.clear();
    m_author.clear();
    m_tags.clear();
}


void CSDocCompilerSettingsForCSProUsersBlog::SetTitleForCompilationFilePath(const std::string& title)
{
    m_title = title;

    __super::SetTitleForCompilationFilePath(title);
}


void CSDocCompilerSettingsForCSProUsersBlog::AddMetadata(const std::string_view attribute_sv, const std::string_view value_sv)
{
    std::string* const destination_value =
        ( attribute_sv == "author" ) ? &m_author :
        ( attribute_sv == "tags" )   ? &m_tags :
                                       throw CSProException("Unknown metadata attribute: '%s'", std::string(attribute_sv).c_str());

    *destination_value = value_sv;
}


std::string CSDocCompilerSettingsForCSProUsersBlog::GetBlogDate(const std::string& file_path)
{
    constexpr size_t DateLength = 10;

    // filenames look like: 2017-08-02-getting-started-with-cspro.csdoc
    std::string filename = Path::GetFilename(file_path);

    if( filename.length() < DateLength )
        throw ProgrammingErrorException();

    return filename.erase(DateLength);
}


std::string CSDocCompilerSettingsForCSProUsersBlog::CreateUrlForTopic(const std::string& project, const std::string& path)
{
    if( !project.empty() )
        throw CSProException("You cannot link to other projects.");

    const std::string blog_date = GetBlogDate(path);

    const std::string filename_without_blog_date = Path::GetFilenameWithoutExtension(path).substr(blog_date.length());
    ASSERT(!filename_without_blog_date.empty() && filename_without_blog_date.front() == '-');

    return FormatText("{{ site.baseurl }}/blog/%s/%s.html",
                      blog_date.c_str(),
                      filename_without_blog_date.c_str() + 1);
}


std::string CSDocCompilerSettingsForCSProUsersBlog::CreateUrlForFile(const std::string& file_path, const char* const type) const
{
    if( !SO::StartsWith(file_path, m_blogDirectory) )
        throw CSProException("You cannot link to %s not contained within the blog directory: %s", type, file_path.c_str());

    return Path::CombineForwardSlash("{{ site.baseurl }}/blog",
                                     Path::ToForwardSlash(file_path.substr(m_blogDirectory.length())));
}


std::string CSDocCompilerSettingsForCSProUsersBlog::CreateUrlForImageFile(const std::string& path)
{
    return CreateUrlForFile(path, "an image");
}


std::string CSDocCompilerSettingsForCSProUsersBlog::CreateUrlForResource(const std::string& resource)
{
    if( m_compilationFilePaths.empty() )
        throw ProgrammingErrorException();

    // look for resources in the directory with the blog date
    const std::string resource_file_path = Path::Combine(m_blogDirectory,
                                                         GetBlogDate(m_compilationFilePaths.back()),
                                                         resource);

    if( !PortableFunctions::FileIsRegular(resource_file_path) )
        throw CSProException("The resource could not be located: %s", resource_file_path.c_str());

    return CreateUrlForFile(resource_file_path, "a resource");
}



// --------------------------------------------------------------------------
// CSProUsersBlogHtmlTagModifier
// --------------------------------------------------------------------------

class CSProUsersBlogHtmlTagModifier : public HtmlTagModifier
{
protected:
    void ProcessTag(std::string& start_tag, std::string* end_tag) override;
};


void CSProUsersBlogHtmlTagModifier::ProcessTag(std::string& start_tag, std::string* const end_tag)
{
    // only elements with end tags will be processed
    if( end_tag == nullptr )
        return;

    // paragraph div tags -> p tags
    if( start_tag == HT::ParagraphDiv_sv[0] && *end_tag == "</div>" )
    {
        start_tag = "<p>";
        *end_tag = "</p>";
    }

    // subheader div tags -> h2 tags
    else if( start_tag == HT::Subheader[0] && *end_tag == HT::Subheader[1] )
    {
        start_tag = "<h2>";
        *end_tag = "</h2>";
    }

    // bold b tags -> strong
    else if( start_tag == HT::Bold[0] && *end_tag == HT::Bold[1] )
    {
        start_tag = "<strong>";
        *end_tag = "</strong>";
    }

    // italics i tags -> em
    else if( start_tag == HT::Italics[0] && *end_tag == HT::Italics[1] )
    {
        start_tag = "<em>";
        *end_tag = "</em>";
    }

    // code blocks -> a style used on the blog
    else if( start_tag == "<div class=\"code_colorization indent\">" && *end_tag == "</div>" )
    {
        start_tag = "<div class=\"code_colorization_block\">";
    }
}



// --------------------------------------------------------------------------
// CSProUsersBlogBuilder
// --------------------------------------------------------------------------

CSProUsersBlogBuilder::CSProUsersBlogBuilder(GlobalSettings& global_settings, std::string blog_doc_set_spec_file_path, std::string output_directory)
    :   m_outputDirectory(std::move(output_directory)),
        m_htmlTagModifier(std::make_unique<CSProUsersBlogHtmlTagModifier>()),
        m_docSetSpec(std::move(blog_doc_set_spec_file_path))
{
    FileIO::CreateDirectories(m_outputDirectory);

    // compile the doc set
    DocSetCompiler doc_set_compiler(global_settings, DocSetCompiler::ThrowErrors { });
    doc_set_compiler.CompileSpec(m_docSetSpec, FileIO::ReadText(m_docSetSpec.GetFilePath()), DocSetCompiler::SpecCompilationType::SpecAndComponents);

    m_settings = std::make_unique<CSDocCompilerSettingsForCSProUsersBlog>(&m_docSetSpec);
}


CSProUsersBlogBuilder::~CSProUsersBlogBuilder()
{
}


void CSProUsersBlogBuilder::Build()
{
    for( const DocSetComponent& doc_set_component : VI_V(m_docSetSpec.GetComponents()) )
    {
        if( doc_set_component.type == DocSetComponent::Type::Document )
        {
            const std::string& blog_file_path = doc_set_component.file_path;
            const std::string output_filename = Path::AppendExtension(Path::GetFilenameWithoutExtension(blog_file_path), FileExtensions::HTML);

            Build(blog_file_path,
                  FileIO::ReadText(blog_file_path),
                  Path::Combine(m_outputDirectory, output_filename));
        }
    }
}


void CSProUsersBlogBuilder::Build(const std::string& blog_file_path, const std::string& blog_text, const std::string& output_file_path)
{
    m_settings->Reset();

    CSDocCompiler csdoc_compiler;
    std::string html = csdoc_compiler.CompileToHtml(*m_settings, blog_file_path, blog_text);

    // convert tags, making them suitable for the blog
    html = m_htmlTagModifier->Process(html);

    FileIO::TextFile text_file;
    text_file.SetTextEncoding(TextEncoding::Type::Utf8);
    text_file.SetWriteNewlineAsCRLF(false);
    text_file.OpenForTextWriting(output_file_path, false);

    // write out the YAML frontmatter
    text_file.WriteLine("---");
    text_file.WriteLine("layout: post");

    ASSERT(!m_settings->GetTitle().empty());
    text_file.WriteFormattedLine("title: \"%s\"", Encoders::ToEscapedString(m_settings->GetTitle()).c_str());

    if( !m_settings->GetAuthor().empty() )
    {
        ASSERT(m_settings->GetAuthor() == Encoders::ToEscapedString(m_settings->GetAuthor()));
        text_file.WriteLine("author: " + m_settings->GetAuthor());
    }

    if( !m_settings->GetTags().empty() )
    {
        ASSERT(m_settings->GetTags() == Encoders::ToEscapedString(m_settings->GetTags()));
        text_file.WriteLine("tags:   " + m_settings->GetTags());
    }

    text_file.WriteLine("---");

    // write out the blog post
    text_file.Write(html);

    text_file.Close();
}
