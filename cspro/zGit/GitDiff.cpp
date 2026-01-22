#include "StdAfx.h"
#include "GitDiff.h"


GitDiff::GitDiff(git_diff& diff) noexcept
    :   m_diff(&diff)
{
}


GitDiff::GitDiff(GitDiff&& rhs) noexcept
    :   m_diff(rhs.m_diff)
{
    rhs.m_diff = nullptr;
}


GitDiff::~GitDiff() noexcept
{
    if( m_diff != nullptr )
        git_diff_free(m_diff);
}


size_t GitDiff::GetNumberDeltas() const noexcept
{
    return git_diff_num_deltas(m_diff);
}


void GitDiff::ForeachDifference(const std::function<bool(const void* delta)>& callback_function) const
{
    ASSERT(m_diff != nullptr);

    struct Payload
    {
        const std::function<bool(const void* delta)>& callback_function;
        std::exception_ptr caught_exception;
    };

    Payload this_payload { callback_function };

    struct CB
    {
        static int file_cb(const git_diff_delta* const delta, float /*progress*/, void* const payload)
        {
            ASSERT(delta != nullptr && delta->new_file.path != nullptr && payload != nullptr);

            Payload& this_payload = *reinterpret_cast<Payload*>(payload);
            ASSERT(!this_payload.caught_exception);

            try
            {
                return this_payload.callback_function(delta) ? 0 : 1;
            }

            catch(...)
            {
                this_payload.caught_exception = std::current_exception();
                return 1;
            }
        }
    };

    git_diff_foreach(m_diff, CB::file_cb, nullptr, nullptr, nullptr, &this_payload);

    if( this_payload.caught_exception )
        std::rethrow_exception(this_payload.caught_exception);
}


void GitDiff::ForeachDifference(const std::function<bool(std::string path, unsigned int diff_flag)>& callback_function) const
{
    ForeachDifference(
        [&](const void* const delta)
        {
            const git_diff_delta* diff_delta = static_cast<const git_diff_delta*>(delta);
            return callback_function(diff_delta->new_file.path, diff_delta->status);
        });
}
