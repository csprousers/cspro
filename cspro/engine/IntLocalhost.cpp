#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include <zEngineO/BinarySymbol.h>
#include <zUtilO/MimeType.h>
#include <zHtml/PortableLocalhost.h>


namespace
{
    struct BinaryContent
    {
        std::shared_ptr<const std::vector<std::byte>> content;
        std::wstring content_type; // content type will be blank when unknown
    };


    class LocalhostVirtualFileMappingHandler : public VirtualFileMappingHandler
    {
    public:
        LocalhostVirtualFileMappingHandler(const BinarySymbol& binary_symbol, std::optional<std::wstring> content_type_override)
            :   m_binarySymbol(binary_symbol),
                m_contentTypeOverride(std::move(content_type_override))
        {
        }

        bool ServeContent(void* response_object) override
        {
            if( !m_binarySymbol.HasContent() )
                return false;

            try
            {
                const BinaryContent binary_content = EvaluateContent(m_binarySymbol, m_contentTypeOverride);

                LocalFileServerSetResponse(response_object, *binary_content.content, binary_content.content_type);

                return true;
            }

            catch(...)
            {
                return false;
            }
        }

        // returns the content for the binary symbol (which must have content);
        // exceptions may be thrown when accessing the content
        static BinaryContent EvaluateContent(const BinarySymbol& binary_symbol, const std::optional<std::wstring>& content_type_override)
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

    private:
        const BinarySymbol& m_binarySymbol;
        const std::optional<std::wstring> m_contentTypeOverride;
    };
}


template<typename T/* = std::wstring*/>
T CIntDriver::LocalhostCreateMappingForBinarySymbol(const BinarySymbol& binary_symbol, std::optional<std::wstring> content_type_override/* = std::nullopt*/,
                                                    const bool evaluate_immediately/* = false*/)
{
    std::unique_ptr<VirtualFileMappingHandler> virtual_file_mapping_handler;

    if( evaluate_immediately )
    {
        if( binary_symbol.HasContent() )
        {
            try
            {
                BinaryContent binary_content = LocalhostVirtualFileMappingHandler::EvaluateContent(binary_symbol, content_type_override);
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
        virtual_file_mapping_handler = std::make_unique<LocalhostVirtualFileMappingHandler>(binary_symbol, std::move(content_type_override));
    }

    const std::wstring filename = binary_symbol.GetFilenameOnly();
    ASSERT(filename == PortableFunctions::PathGetFilename(filename));

    PortableLocalhost::CreateVirtualFile(*virtual_file_mapping_handler, filename);

    if constexpr(std::is_same_v<T, std::wstring>)
    {
        return m_localHostVirtualFileMappingHandlers.emplace_back(std::move(virtual_file_mapping_handler))->GetUrl();
    }

    else
    {
        return virtual_file_mapping_handler;
    }    
}

template std::wstring CIntDriver::LocalhostCreateMappingForBinarySymbol(const BinarySymbol& binary_symbol, std::optional<std::wstring> content_type_override/* = std::nullopt*/,
                                                                        const bool evaluate_immediately/* = false*/);
template std::unique_ptr<VirtualFileMappingHandler> CIntDriver::LocalhostCreateMappingForBinarySymbol(const BinarySymbol& binary_symbol, std::optional<std::wstring> content_type_override/* = std::nullopt*/,
                                                                                                      const bool evaluate_immediately/* = false*/);
