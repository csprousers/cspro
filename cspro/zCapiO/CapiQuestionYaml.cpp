#include "StdAfx.h"
#include "CapiQuestionYaml.h"
#include "CapiQuestionManager.h"
#include <yaml-cpp/yaml.h>


namespace YAML {

    template<>
    struct convert<CString> {
        static Node encode(const CString& rhs) {
            return Node(UTF8_TODO::GetUtf8(rhs));
        }

        static bool decode(const Node& node, CString& rhs) {
            if (!node.IsScalar())
                return false;
            rhs = UTF8_TODO::GetCString(node.Scalar());
            return true;
        }
    };

    template<>
    struct convert<std::wstring> {
        static Node encode(const std::wstring& rhs) {
            return Node(UTF8_TODO::GetUtf8(rhs));
        }

        static bool decode(const Node& node, std::wstring& rhs) {
            if (!node.IsScalar())
                return false;
            rhs = TC::ToWide(node.Scalar());
            return true;
        }
    };


    template<>
    struct convert<SharableString>
    {
        static Node encode(const SharableString& rhs)
        {
            return Node(rhs.GetString());
        }

        static bool decode(const Node& node, SharableString& rhs)
        {
            if( !node.IsScalar() )
                return false;

            rhs = node.Scalar();
            return true;
        }
    };


    template<>
    struct convert<CapiStyle> {
        static Node encode(const CapiStyle& rhs) {
            Node node(NodeType::Map);
            node.force_insert("name", rhs.name);
            node.force_insert("className", rhs.class_name);
            node.force_insert("css", rhs.css);
            return node;
        }

        static bool decode(const Node& node, CapiStyle& rhs) {
            if (!node.IsMap()) {
                return false;
            }
            rhs.name = node["name"].as<std::string>();
            rhs.class_name = node["className"].as<std::string>();
            rhs.css = node["css"].as<std::string>();
            return true;
        }
    };

    template<>
    struct convert<Language> {
        static Node encode(const Language& rhs) {
            Node node(NodeType::Map);
            node.force_insert("name", rhs.GetName());
            node.force_insert("label", rhs.GetLabel());
            return node;
        }

        static bool decode(const Node& node, Language& rhs) {
            if (!node.IsMap()) {
                return false;
            }
            rhs.SetName(node["name"].as<std::string>());
            rhs.SetLabel(node["label"].as<std::string>());
            return true;
        }
    };

    template<>
    struct convert<CapiText> {
        static Node encode(const CapiText& rhs) { return Node(rhs.GetText()); }

        static bool decode(const Node& node, CapiText& rhs) {
            if (!node.IsScalar())
                return false;
            rhs = CapiText(node.as<std::string>());
            return true;
        }
    };

    template<>
    struct convert<CapiCondition> {
        static Node encode(const CapiCondition& /*rhs*/) {
            ASSERT(false);
            return Node();
        }

        static bool decode(const Node& node, CapiCondition& rhs) {
            static_assert(Serializer::GetEarliestSupportedVersion() < Serializer::Iteration_8_0_000_1, "when removing pre-8.0 support, remove 'logicExpression'");
            if (!node.IsMap()) {
                return false;
            }
            if (node["logic"])
                rhs.m_logic = node["logic"].as<CString>();
            if (node["logicExpression"])
                rhs.m_logicExpression = node["logicExpression"].as<int>();
            if (node["questionText"])
                rhs.m_questionTexts = node["questionText"].as<std::map<std::wstring, CapiText>>();
            if (node["helpText"])
                rhs.m_helpTexts = node["helpText"].as<std::map<std::wstring, CapiText>>();
            return true;
        }
    };

    template<>
    struct convert<CapiQuestion> {
        static Node encode(const CapiQuestion& /*rhs*/) {
            ASSERT(false);
            return Node();
        }

        static bool decode(const Node& node, CapiQuestion& rhs) {
            static_assert(Serializer::GetEarliestSupportedVersion() < Serializer::Iteration_8_0_000_1, "when removing pre-8.0 support, remove 'fillExpressions'");
            if (!node.IsMap()) {
                return false;
            }
            rhs.SetItemName(node["name"].as<CString>());
            if (node["conditions"])
                rhs.m_conditions = node["conditions"].as<std::vector<CapiCondition>>();
            if (node["fillExpressions"])
                rhs.m_fillExpressions = node["fillExpressions"].as<std::map<CString, int>>();
            return true;
        }
    };

    template <typename T>
    Emitter& operator<<(Emitter& emitter, const T& t) {
        emitter << convert<T>::encode(t);
        return emitter;
    }
};


std::string WriteToYaml(const CapiQuestionManager& question_manager)
{
    // Ideally we would use the convert<> overrides above rather than emit
    // each field individually but we want to use literal notation
    // for the question text and we need to write out everything field by field
    // to do that.
    YAML::Emitter out;
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::Key << "fileType";
    out << YAML::Value << "Question Text";
    out << YAML::Key << "version";
    out << Versioning::CSProVersionText;
    out << YAML::Key << "languages";
    out << YAML::Value << question_manager.GetLanguages();

    // styles
    out << YAML::Key << "styles";
    out << YAML::BeginSeq;
    for (const CapiStyle& style : question_manager.GetStyles()) {
        out << YAML::BeginMap;
        out << YAML::Key << "name";
        out << YAML::Value << style.name;
        out << YAML::Key << "className";
        out << YAML::Value << style.class_name;
        out << YAML::Key << "css";
        out << YAML::Value << YAML::Literal << style.css;
        out << YAML::EndMap;
    }
    out << YAML::EndSeq;

    // questions
    out << YAML::Key << "questions";
    out << YAML::BeginSeq;

    for (const CapiQuestion& question : question_manager.GetQuestionsSortedInFormOrder()) {
        out << YAML::BeginMap;
        out << YAML::Key << "name";
        out << YAML::Value << question.GetItemName();
        const std::vector<CapiCondition>& conditions = question.GetConditions();
        if (!conditions.empty()) {
            out << YAML::Key << "conditions";
            out << YAML::BeginSeq;
            for (const CapiCondition& condition : conditions) {
                out << YAML::BeginMap;
                if (!condition.GetLogic().IsEmpty()) {
                    out << YAML::Key << "logic";
                    out << YAML::Value << condition.GetLogic();
                }
                if (!condition.GetAllQuestionText().empty()) {
                    out << YAML::Key << "questionText";
                    out << YAML::BeginMap;
                    for (const auto& text : condition.GetAllQuestionText()) {
                        out << YAML::Key << text.first;
                        out << YAML::Literal << text.second;
                    }
                    out << YAML::EndMap;
                }
                if (!condition.GetAllHelpText().empty()) {
                    out << YAML::Key << "helpText";
                    out << YAML::BeginMap;
                    for (const auto& text : condition.GetAllHelpText()) {
                        out << YAML::Key << text.first;
                        out << YAML::Literal << text.second;
                    }
                    out << YAML::EndMap;
                }
                out << YAML::EndMap;
            }
            out << YAML::EndSeq;
        }
        out << YAML::EndMap;
    }
    out << YAML::EndSeq;

    out << YAML::EndMap;
    out << YAML::EndDoc;

    return out.c_str();
}


void ReadFromYaml(CapiQuestionManager& question_manager, const YAML::Node& yaml)
{
    CString fileType = yaml["fileType"].as<CString>();
    if (fileType != L"Question Text")
        throw CSProException("Invalid file type");
    auto languages = yaml["languages"].as<std::vector<Language>>();
    for (Language& language : languages)
        question_manager.AddLanguage(std::move(language));

    //yaml-cpp library adds new line when reading "Literal" output and it does not have folding feature to eliminate new lines
    //as a work around we are trimming new lines added when using "Literal" style output
    if (yaml["styles"]) {
        auto styles = yaml["styles"].as<std::vector<CapiStyle>>();
        for (CapiStyle& style : styles) {
            SO::MakeTrimRight(style.css, '\n');
        }
        question_manager.SetStyles(std::move(styles));
    }

    if( yaml["questions"] )
    {
        std::vector<CapiQuestion> questions = yaml["questions"].as<std::vector<CapiQuestion>>();

        for( CapiQuestion& question : questions )
        {
            std::vector<CapiCondition>& conditions = question.GetConditions();

            for( CapiCondition& condition : conditions )
            {
                for( const auto& [language_name, capi_text] : condition.GetAllQuestionText() )
                {
                    condition.SetQuestionText(UTF8_TODO::GetCString(SO::TrimRight(capi_text.GetText().GetString(), '\n')),
                                              language_name);
                }

                for( const auto& [language_name, capi_text] : condition.GetAllHelpText() )
                {
                    condition.SetHelpText(UTF8_TODO::GetCString(SO::TrimRight(capi_text.GetText().GetString(), '\n')),
                                          language_name);
                }
            }

            question_manager.SetQuestion(std::move(question));
        }
    }
}


void ReadFromYaml(CapiQuestionManager& question_manager, std::istream& input)
{
    ReadFromYaml(question_manager, YAML::Load(input));
}

void ReadFromYaml(CapiQuestionManager& question_manager, const std::string& input)
{
    ReadFromYaml(question_manager, YAML::Load(input));
}
