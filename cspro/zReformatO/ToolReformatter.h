#pragma once

#include <zReformatO/zReformatO.h>

class BasicLogger;
class CDataDict;
struct DictionaryDifference;
class PFF;
class Reformatter;


class ZREFORMATO_API ToolReformatter
{
public:
    static void GetDictionaryDifferences(BasicLogger& differences_log, const Reformatter& reformatter,
                                         bool show_names, bool show_only_destructive_changes,
                                         size_t* out_display_name_length = nullptr);

    bool Run(const PFF& pff, bool silent, Reformatter& reformatter);

    bool Run(const PFF& pff, bool silent, std::shared_ptr<const CDataDict> input_dictionary = nullptr,
             std::shared_ptr<const CDataDict> output_dictionary = nullptr);

private:
    static std::vector<DictionaryDifference> GetSortedDifferences(const Reformatter& reformatter);

    template<typename GetReformatterCallback>
    bool Run(const PFF& pff, bool silent, GetReformatterCallback get_reformatter_callback);
};
