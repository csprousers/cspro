#include "StdAfx.h"
#include "GitMerge.h"


GitMerge::Result GitMerge::Merge(const std::string_view ancestor_sv, const std::string_view ours_sv, const std::string_view theirs_sv)
{
    git_merge_file_input cs_mfi_before = GIT_MERGE_FILE_INPUT_INIT;
    cs_mfi_before.ptr = ancestor_sv.data();
    cs_mfi_before.size = ancestor_sv.size();

    git_merge_file_input cs_mfi_after = GIT_MERGE_FILE_INPUT_INIT;
    cs_mfi_after.ptr = ours_sv.data();
    cs_mfi_after.size = ours_sv.size();

    git_merge_file_input os_mfi_now = GIT_MERGE_FILE_INPUT_INIT;
    os_mfi_now.ptr = theirs_sv.data();
    os_mfi_now.size = theirs_sv.size();

    git_merge_file_result result;

    if( git_merge_file(&result, &cs_mfi_before, &cs_mfi_after, &os_mfi_now, nullptr) != 0 )
        throw GitException();

    Result merge_result
    {
        std::string(result.ptr, result.len),
        ( result.automergeable != 0 )
    };

    git_merge_file_result_free(&result);

    return merge_result;
}
