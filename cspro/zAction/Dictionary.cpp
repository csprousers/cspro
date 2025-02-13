#include "stdafx.h"
#include <zDictO/DDClass.h>
#include <zHtml/AccessUrlSerializer.h>


ActionInvoker::Result ActionInvoker::Runtime::Dictionary_getDictionary(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    std::shared_ptr<const CDataDict> dictionary;

    if( json_node.Contains(JK::path) )
    {
        const std::wstring path = caller.EvaluateAbsolutePath(json_node.Get<std::wstring>(JK::path));
        dictionary = CDataDict::InstantiateAndOpen(path, true);
    }

    else
    {
        dictionary = std::get<2>(GetApplicationComponents<std::shared_ptr<const CDataDict>>(json_node.GetOptional<wstring_view>(JK::name)));
    }

    ASSERT(dictionary != nullptr);

    auto json_writer = Json::CreateStringWriter();
    json_writer->SetVerbose();

    // create access URLs for things like value set images
    auto access_url_serializer_helper_holder = json_writer->GetSerializerHelper().Register(std::make_unique<AccessUrl::SerializerHelper>());

    json_writer->Write(*dictionary);

    return Result::JsonText(json_writer);
}
