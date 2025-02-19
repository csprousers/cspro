#pragma once


struct CSWebUser
{
    std::string id;
    std::string role_name;

    bool IsAdmin() const { return SO::EqualsNoCase(role_name, "Administrator"); }

    static CSWebUser CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline CSWebUser CSWebUser::CreateFromJson(const JsonNode& json_node)
{
    return { json_node.Get<std::string>(JK::id),
             json_node.Get<std::string>(JK::roleName) };
}


inline void CSWebUser::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::id, id)
               .Write(JK::roleName, role_name)
               .EndObject();
}
