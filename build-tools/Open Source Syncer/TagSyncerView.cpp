#include "StdAfx.h"
#include "TagSyncerView.h"
#include "MarkdownViewer.h"
#include "ReleaseTag.h"
#include "Syncer.h"


namespace
{
    constexpr std::string_view EarliestVersion_sv = "8.0.1";

    constexpr std::string_view MessagePrefix_sv = "Open source version of: https://github.com/CSProDevelopment/cspro/commit/";
}


namespace UpdateAction
{
    constexpr WPARAM RefreshTags  = 1;
    constexpr WPARAM UpdateTagsUI = 2;
    constexpr WPARAM DisplayHtml  = 3;
}


IMPLEMENT_DYNCREATE(TagSyncerView, CFormView)


BEGIN_MESSAGE_MAP(TagSyncerView, CFormView)
    ON_MESSAGE(UWM::OpenSourceSyncer::UpdateUI, OnUpdateUI)
    ON_COMMAND(IDC_CREATE_OPEN_SOURCE_TAG, OnCreateOpenSourceTag)
    ON_COMMAND(IDC_VIEW_OPEN_SOURCE_HISTORY_LOG, OnViewOpenSourceHistoryLog)
    ON_COMMAND(IDC_CREATE_PRIVATE_HISTORY_LOG, OnCreatePrivateHistoryLog)
END_MESSAGE_MAP()


TagSyncerView::TagSyncerView()
    :   CFormView(IDD_TAG_SYNCER),
        m_controller(Controller::GetInstance())
{
}


void TagSyncerView::OnInitialUpdate()
{
    __super::OnInitialUpdate();

    ResizeParentToFit(FALSE);

    m_tagsListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
    m_tagsListCtrl.SetHeadings(L"Tag Name,250;Private Commit,250;Open Source Commit,250");
    m_tagsListCtrl.LoadColumnInfo();

    PostMessage(UWM::OpenSourceSyncer::UpdateUI, UpdateAction::RefreshTags);
}


void TagSyncerView::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_TAGS, m_tagsListCtrl);
}


LRESULT TagSyncerView::OnUpdateUI(const WPARAM wParam, const LPARAM lParam)
{
    if( wParam == UpdateAction::RefreshTags )
    {
        OnRefreshTags();
    }

    else if( wParam == UpdateAction::UpdateTagsUI )
    {
        OnUpdateTagsUI();
    }

    else if( wParam == UpdateAction::DisplayHtml )
    {
        const SharableString* const html = reinterpret_cast<const SharableString*>(lParam);
        ASSERT(html != nullptr);
        MarkdownViewer::ShowHtmlInDialog(*html);
    }

    else
    {
        return ReturnProgrammingError(0);
    }

    return 1;
}


struct TagSyncerView::Tag
{
    std::string tag_name;
    std::string version;
    std::string cs_commit_sha;
    std::string os_commit_sha;
    int64_t timestamp = std::numeric_limits<int64_t>::max(); // earliest between the open source and private commits
};


void TagSyncerView::OnRefreshTags()
{
    m_controller.RunOperation(GetParentFrame(), false,
        [this](Controller& controller)
        {
            RefreshTags(controller);
        });
}


void TagSyncerView::RefreshTags(Controller& controller)
{
    GitRepository& private_repo = controller.GetPrivateRepo();
    GitRepository& open_source_repo = controller.GetOpenSourceRepo();

    auto tags = std::make_unique<std::vector<Tag>>();

    auto process_release_tags = [&](GitRepository& repo, std::string Tag::* const commit_sha)
    {
        for( const ReleaseTag& release_tag : ReleaseTag::Populate(repo, EarliestVersion_sv, false) )
        {
            // try to link by tag name
            auto lookup = std::find_if(tags->begin(), tags->end(),
                [&](const Tag& tag) { return ( tag.tag_name == release_tag.tag_name ); }
            );

            // if there is no existing entry matching the name, see if there is a
            // linkage using a SHA specified in the tag message
            if( lookup == tags->end() &&
                SO::StartsWith(release_tag.tag_message, MessagePrefix_sv) )
            {
                const std::string cs_commit_sha(SO::Trim(release_tag.tag_message).substr(MessagePrefix_sv.size()));

                lookup = std::find_if(tags->begin(), tags->end(),
                    [&](const Tag& tag) { return ( cs_commit_sha == tag.cs_commit_sha ); }
                );
            }

            // if not, add the tag
            if( lookup == tags->end() )
                lookup = tags->emplace(tags->end(), Tag { release_tag.tag_name, release_tag.version });

            (*lookup).*commit_sha = release_tag.commit.GetObjectId().GetHexHash();

            lookup->timestamp = std::min(lookup->timestamp, release_tag.commit.GetAuthor().GetWhen().GetTimestamp());
        }
    };

    process_release_tags(private_repo, &Tag::cs_commit_sha);
    process_release_tags(open_source_repo, &Tag::os_commit_sha);

    // sort by reverse version and timestamp
    std::sort(tags->begin(), tags->end(),
        [](const Tag& tag1, const Tag& tag2)
        {
            const int version_compare = tag1.version.compare(tag2.version);

            return ( version_compare != 0 ) ? ( version_compare >  0 ) :
                                              ( tag1.timestamp > tag2.timestamp );
        }
    );

    m_tags = std::move(tags);

    PostMessage(UWM::OpenSourceSyncer::UpdateUI, UpdateAction::UpdateTagsUI);
}


void TagSyncerView::OnUpdateTagsUI()
{
    const std::shared_ptr<const std::vector<Tag>> tags = m_tags;
    ASSERT(tags != nullptr && !tags->empty());

    m_tagsListCtrl.DeleteAllItems();

    for( const Tag& tag : *tags )
    {
        m_tagsListCtrl.AddItem(
            TC::ToWide(tag.tag_name).c_str(),
            TC::ToWide(tag.cs_commit_sha).c_str(),
            TC::ToWide(tag.os_commit_sha).c_str()
        );
    }
}


void TagSyncerView::RunOperationWithTag(const bool validate_private_commit, const bool validate_open_source_commit,
                                        std::function<void(Controller&, const Tag&)> callback_function)
{
    ASSERT(callback_function);

    m_controller.RunOperation(GetParentFrame(),
        [tags = m_tags,
         callback_function_ = std::move(callback_function),
         validate_private_commit,
         validate_open_source_commit,
         selections = m_tagsListCtrl.GetSelectedCount(),
         index = m_tagsListCtrl.GetSelectionMark()]
        (Controller& controller)
        {
            if( index < 0 || selections != 1 )
                throw CSProException("Select a tag.");

            ASSERT(tags != nullptr && static_cast<size_t>(index) < tags->size());
            const Tag& tag = tags->at(index);

            if( validate_private_commit && tag.cs_commit_sha.empty() )
                throw CSProException("The operation requires a tag defined in the private repository.");

            if( validate_open_source_commit && tag.os_commit_sha.empty() )
                throw CSProException("The operation requires a tag defined in the open source repository.");

            callback_function_(controller, tag);
        }
    );
}


void TagSyncerView::OnCreateOpenSourceTag()
{
    RunOperationWithTag(true, false,
        [this](Controller& controller, const Tag& tag)
        {
            GitRepository& private_repo = controller.GetPrivateRepo();
            GitRepository& open_source_repo = controller.GetOpenSourceRepo();

            if( !tag.os_commit_sha.empty() )
                throw CSProException("There is already an open source tag associated with the private repository's tag.");

            // find the matching commit in the open source repository
            const GitCommit cs_commit = private_repo.LookupCommit(tag.cs_commit_sha);
            std::optional<GitCommit> os_matching_commit;

            GitRevisionWalker walker(open_source_repo);

            walker.WalkFromHead(
                [&](GitCommit os_commit)
                {
                    if( os_commit.Equals(cs_commit) )
                    {
                        os_matching_commit = std::move(os_commit);
                        return false;
                    }

                    return true;
                });

            if( !os_matching_commit.has_value() )
            {
                throw CSProException("Could not find a commit in the open source repository matching: " +
                                     cs_commit.GetMessage());
            }

            // create the tag, with the message referencing the private commit
            const std::string message = SO::Concatenate(MessagePrefix_sv, tag.cs_commit_sha, "\n");

            m_controller.LogText("Creating open source tag '%s' with the message:\n\n",
                                 tag.tag_name.c_str(), message.c_str());

            open_source_repo.CreateTag(
                Controller::GetCSProBotSignature(open_source_repo),
                *os_matching_commit,
                tag.tag_name,
                message
            );

            // refresh the UI to show this new tag
            RefreshTags(controller);
        });
}


void TagSyncerView::OnViewOpenSourceHistoryLog()
{
    RunOperationWithTag(false, true,
        [this](Controller& controller, const Tag& tag)
        {
            GitRepository& open_source_repo = controller.GetOpenSourceRepo();

            const GitCommit os_commit = open_source_repo.LookupCommit(tag.os_commit_sha);
            const GitTree tree = os_commit.GetTree();
            const GitTreeEntry tree_entry = tree.GetEntryByPath(Syncer::GetHistoryFilename());
            const GitBlob history_blob = tree_entry.GetObject().GetBlob();

            DisplayHistoryMarkdown(
                history_blob.as<std::string>()
            );
        }
    );}


void TagSyncerView::OnCreatePrivateHistoryLog()
{
    RunOperationWithTag(true, false,
        [this](Controller& controller, const Tag& tag)
        {
            GitRepository& private_repo = controller.GetPrivateRepo();
            Syncer& syncer = controller.GetSyncer();

            const GitCommit cs_commit = private_repo.LookupCommit(tag.cs_commit_sha);

            DisplayHistoryMarkdown(
                syncer.CreateHistoryLog(cs_commit)
            );
        }
    );
}


void TagSyncerView::DisplayHistoryMarkdown(const std::string& history_md)
{
    // convert the Markdown to HTML for viewing
    const std::unique_ptr<const SharableString>& history_html = m_historyHtml.emplace_back(
        std::make_unique<SharableString>(
            MarkdownViewer::MarkdownToHtmlDocument(Syncer::GetHistoryFilename(), history_md)
        )
    );

    PostMessage(UWM::OpenSourceSyncer::UpdateUI, UpdateAction::DisplayHtml, reinterpret_cast<LPARAM>(history_html.get()));
}
