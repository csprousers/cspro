#include "stdafx.h"
#include "IncludesRT.h"
#include <BinarySymbol.h>
#include <zHtml/PortableLocalhost.h>

namespace LocalhostRT { struct BinaryContent; class LocalhostVirtualFileMappingHandler; }


struct LocalhostRT::BinaryContent
{
    std::shared_ptr<const std::vector<std::byte>> content;
    std::string content_type; // content type will be blank when unknown
};


class LocalhostRT::LocalhostVirtualFileMappingHandler : public VirtualFileMappingHandler
{
public:
    LocalhostVirtualFileMappingHandler(const BinarySymbol& binary_symbol, std::optional<std::string> content_type_override);

    bool ServeContent(VirtualFileMappingResponse& response) override;

    // Returns the content for the binary symbol (which must have content).
    // Exceptions may be thrown when accessing the content.
    static BinaryContent EvaluateContent(const BinarySymbol& binary_symbol, const std::optional<std::string>& content_type_override);

private:
    const BinarySymbol& m_binarySymbol;
    std::optional<std::string> m_contentTypeOverride;
};


LocalhostRT::LocalhostVirtualFileMappingHandler::LocalhostVirtualFileMappingHandler(const BinarySymbol& binary_symbol,
                                                                                    std::optional<std::string> content_type_override)
    :   m_binarySymbol(binary_symbol),
        m_contentTypeOverride(std::move(content_type_override))
{
}


bool LocalhostRT::LocalhostVirtualFileMappingHandler::ServeContent(VirtualFileMappingResponse& response)
{
    if( m_binarySymbol.HasContent() )
    {
        try
        {
            const BinaryContent binary_content = EvaluateContent(m_binarySymbol, m_contentTypeOverride);
            response.SetContent(binary_content.content, binary_content.content_type);
            return true;
        }
        catch(...) { }
    }

    return false;
}


LocalhostRT::BinaryContent LocalhostRT::LocalhostVirtualFileMappingHandler::EvaluateContent(const BinarySymbol& binary_symbol,
                                                                                            const std::optional<std::string>& content_type_override)
{
    const BinarySymbolData& binary_symbol_data = binary_symbol.GetBinarySymbolData();
    ASSERT(binary_symbol_data.IsDefined());

    BinaryContent binary_content
    {
        binary_symbol_data.GetSharedContent(),
        ValueOrDefault(binary_symbol_data.GetMetadata().GetEvaluatedMimeType())
    };

    if( content_type_override.has_value() )
    {
        binary_content.content_type = *content_type_override;
    }

    else if( binary_content.content_type == MimeType::Type::Text )
    {
        binary_content.content_type = MimeType::ServerType::TextUtf8;
    }

    return binary_content;
}


template<typename T/* = std::string*/>
T LogicInterpreter::LocalhostCreateMappingForBinarySymbol(const BinarySymbol& binary_symbol,
                                                          std::optional<std::string> content_type_override/* = std::nullopt*/,
                                                          const bool evaluate_immediately/* = false*/)
{
    std::unique_ptr<VirtualFileMappingHandler> virtual_file_mapping_handler;

    if( evaluate_immediately )
    {
        if( binary_symbol.HasContent() )
        {
            try
            {
                LocalhostRT::BinaryContent binary_content = LocalhostRT::LocalhostVirtualFileMappingHandler::EvaluateContent(binary_symbol, content_type_override);
                virtual_file_mapping_handler = std::make_unique<DataVirtualFileMappingHandler<std::shared_ptr<const std::vector<std::byte>>>>(std::move(binary_content.content),
                                                                                                                                              binary_content.content_type);
            }
            catch(...) { }
        }

        if( virtual_file_mapping_handler == nullptr )
            virtual_file_mapping_handler = std::make_unique<FourZeroFourVirtualFileMappingHandler>();
    }

    else
    {
        // BINARY_TYPES_TO_ENGINE_TODO this will work for now, but when the EngineItem wraps a CaseItem, we will have to
        // evaluate the occurrences to determine if they are valid at the point of evaluation
        virtual_file_mapping_handler = std::make_unique<LocalhostRT::LocalhostVirtualFileMappingHandler>(binary_symbol, std::move(content_type_override));
    }

    const std::string filename = binary_symbol.GetFilenameOnly();
    ASSERT(filename == Path::GetFilename(filename));

    PortableLocalhost::CreateVirtualFile(*virtual_file_mapping_handler, filename);

    if constexpr(std::is_same_v<T, std::string>)
    {
        return m_localHostVirtualFileMappingHandlers.emplace_back(std::move(virtual_file_mapping_handler))->GetUrl();
    }

    else
    {
        return virtual_file_mapping_handler;
    }
}

template ZENGINEO_API std::string LogicInterpreter::LocalhostCreateMappingForBinarySymbol(const BinarySymbol& binary_symbol, std::optional<std::string> content_type_override, bool evaluate_immediately);
template ZENGINEO_API std::unique_ptr<VirtualFileMappingHandler> LogicInterpreter::LocalhostCreateMappingForBinarySymbol(const BinarySymbol& binary_symbol, std::optional<std::string> content_type_override, bool evaluate_immediately);
