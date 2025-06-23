#include "StdAfx.h"
#include "CapiQuestionYaml.h"
#include "CapiQuestionManager.h"
#include <yaml-cpp/yaml.h>


namespace
{
    // yaml-cpp library adds new line when reading "Literal" output and it does not have folding feature to eliminate new lines
    // as a work around we are trimming new lines added when using "Literal" style output
    constexpr bool RightTrimNewLines = true;
}


namespace YAML
{
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
    struct convert<CapiStyle>
    {
        static bool decode(const Node& node, CapiStyle& rhs)
        {
            if( !node.IsMap() )
                return false;

            rhs.name = node["name"].as<std::string>();
            rhs.class_name = node["className"].as<std::string>();
            rhs.css = node["css"].as<std::string>();

            return true;
        }
    };


    template<>
    struct convert<Language>
    {
        static Node encode(const Language& rhs)
        {
            Node node(NodeType::Map);
            node.force_insert("name", rhs.GetName());
            node.force_insert("label", rhs.GetLabel());
            return node;
        }

        static bool decode(const Node& node, Language& rhs)
        {
            if( !node.IsMap() )
                return false;

            rhs.SetName(node["name"].as<std::string>());
            rhs.SetLabel(node["label"].as<std::string>());

            return true;
        }
    };


    template<>
    struct convert<CapiText::Format>
    {
        static Node encode(const CapiText::Format& rhs)
        {
            ASSERT(static_cast<int>(rhs) < _countof(CapiText::FormatTexts));
            return Node(CapiText::FormatTexts[static_cast<int>(rhs)]);
        }

        static bool decode(const Node& node, CapiText::Format& rhs)
        {
            if( node.IsScalar() )
            {
                const std::string this_format_text = node.as<std::string>();

                for( int i = 0; i < _countof(CapiText::FormatTexts); ++i )
                {
                    if( this_format_text == CapiText::FormatTexts[i] )
                    {
                        rhs = static_cast<CapiText::Format>(i);
                        return true;
                    }
                }
            }

            return false;
        }
    };


    template<>
    struct convert<CapiText>
    {
        static bool decode(const Node& node, CapiText& rhs)
        {
            SharableString text;
            CapiText::Format format;

            if( node.IsScalar() ) // pre-CSPro 8.1
            {
                format = CapiText::Format::Html;
                text = node.as<std::string>();
            }

            else if( node.IsMap() ) // CSPro 8.1+
            {
                format = node["format"].as<CapiText::Format>();
                text = node["text"].as<std::string>();
            }

            else
            {
                return false;
            }

            if constexpr(RightTrimNewLines)
                text.MakeTrimRight('\n');

            rhs = CapiText(std::move(text), format);

            return true;
        }
    };


    template<>
    struct convert<CapiCondition>
    {
        static bool decode(const Node& node, CapiCondition& rhs)
        {
            static_assert(Serializer::GetEarliestSupportedVersion() < Serializer::Iteration_8_0_000_1, "when removing pre-8.0 support, remove 'logicExpression'");

            if( !node.IsMap() )
                return false;

            if( node["logic"] )
                rhs.m_logic = node["logic"].as<std::string>();

            if( node["logicExpression"] )
                rhs.m_programIndex = node["logicExpression"].as<int>();

            if( node["questionText"] )
                rhs.m_questionTexts = node["questionText"].as<std::map<std::string, CapiText>>();

            if( node["helpText"] )
                rhs.m_helpTexts = node["helpText"].as<std::map<std::string, CapiText>>();

            return true;
        }
    };


    template<>
    struct convert<CapiQuestion>
    {
        static bool decode(const Node& node, CapiQuestion& rhs)
        {
            static_assert(Serializer::GetEarliestSupportedVersion() < Serializer::Iteration_8_0_000_1, "when removing pre-8.0 support, remove 'fillExpressions'");

            if( !node.IsMap() )
                return false;

            rhs.SetItemName(node["name"].as<std::string>());

            if( node["conditions"] )
                rhs.m_conditions = node["conditions"].as<std::vector<CapiCondition>>();

            if( node["fillExpressions"] )
            {
                ASSERT(rhs.m_pre81FillExpressions == nullptr);
                rhs.m_pre81FillExpressions = std::make_unique<std::map<std::string, int>>(node["fillExpressions"].as<std::map<std::string, int>>());
            }

            return true;
        }
    };


    template <typename T>
    Emitter& operator<<(Emitter& emitter, const T& t)
    {
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

    if( question_manager.GetDefaultCapiTextFormat().has_value() )
    {
        out << YAML::Key << "defaultFormat";
        out << YAML::Value << *question_manager.GetDefaultCapiTextFormat();
    }

    out << YAML::Key << "languages";
    out << YAML::Value << question_manager.GetLanguages();

    // styles
    out << YAML::Key << "styles";
    out << YAML::BeginSeq;

    for( const CapiStyle& style : question_manager.GetStyles() )
    {
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

    for( const CapiQuestion& question : question_manager.GetQuestionsSortedInFormOrder() )
    {
        out << YAML::BeginMap;

        out << YAML::Key << "name";
        out << YAML::Value << question.GetItemName();

        const std::vector<CapiCondition>& conditions = question.GetConditions();

        if( !conditions.empty() )
        {
            out << YAML::Key << "conditions";
            out << YAML::BeginSeq;

            for( const CapiCondition& condition : conditions )
            {
                out << YAML::BeginMap;

                if( !condition.GetLogic().empty() )
                {
                    out << YAML::Key << "logic";
                    out << YAML::Value << condition.GetLogic();
                }

                auto write_texts = [&](const char* const key, const std::map<std::string, CapiText>& texts)
                {
                    if( texts.empty() )
                        return;

                    out << YAML::Key << key;
                    out << YAML::BeginMap;

                    for( const auto& [language_name, capi_text] : texts )
                    {
                        out << YAML::Key << language_name;

                        out << YAML::BeginMap;

                        out << YAML::Key << "format";
                        out << YAML::Value << capi_text.GetFormat();

                        out << YAML::Key << "text";
                        out << YAML::Literal << capi_text.GetText();

                        out << YAML::EndMap;
                    }

                    out << YAML::EndMap;
                };

                write_texts("questionText", condition.GetAllQuestionText());
                write_texts("helpText", condition.GetAllHelpText());

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
    const std::string file_type = yaml["fileType"].as<std::string>();

    if( file_type != "Question Text" )
        throw CSProException("Invalid file type");

    if( yaml["defaultFormat"] )
        question_manager.SetDefaultCapiTextFormat(yaml["defaultFormat"].as<CapiText::Format>());

    std::vector<Language> languages = yaml["languages"].as<std::vector<Language>>();

    for( Language& language : languages )
        question_manager.AddLanguage(std::move(language));

    if( yaml["styles"] )
    {
        std::vector<CapiStyle> styles = yaml["styles"].as<std::vector<CapiStyle>>();

        if constexpr(RightTrimNewLines)
        {
            for( CapiStyle& style : styles )
                SO::MakeTrimRight(style.css, '\n');
        }

        question_manager.SetStyles(std::move(styles));
    }

    if( yaml["questions"] )
    {
        std::vector<CapiQuestion> questions = yaml["questions"].as<std::vector<CapiQuestion>>();

        for( CapiQuestion& question : questions )
            question_manager.SetQuestion(std::move(question));
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
