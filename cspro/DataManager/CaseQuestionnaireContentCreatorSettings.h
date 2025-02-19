#pragma once

class CapiQuestionManager;
class CDEFormFile;


class CaseQuestionnaireContentCreatorSettings
{
public:
    const std::string& GetFormFilePath() const             { return m_formFilePath; }
    std::shared_ptr<const CDEFormFile> GetFormFile() const { return m_formFile; }

    const std::string& GetQuestionTextFilePath() const                        { return m_questionTextFilePath; }
    std::shared_ptr<const CapiQuestionManager> GetCapiQuestionManager() const { return m_capiQuestionManager; }

    void SetFormFile(std::string file_path, std::shared_ptr<const CDEFormFile> form_file);
    void SetCapiQuestionManager(std::string file_path, std::shared_ptr<const CapiQuestionManager> capi_question_manager);

    static CaseQuestionnaireContentCreatorSettings CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;

private:
    std::string m_formFilePath;
    std::string m_questionTextFilePath;

    std::shared_ptr<const CDEFormFile> m_formFile;
    std::shared_ptr<const CapiQuestionManager> m_capiQuestionManager;
};
