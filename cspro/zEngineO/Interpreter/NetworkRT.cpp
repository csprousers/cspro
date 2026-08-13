#include "stdafx.h"
#include "IncludesRT.h"
#include "Nodes/Various.h"


Engine::Value LogicInterpreter::ex_connection(const int program_index)
{
    const auto& connection_node = GetNode<Nodes::Connection>(program_index);

    return Engine::Value::Bool(
        m_applicationInterface->IsNetworkConnected(
            ( connection_node.connection_type & Nodes::Connection::WiFi ),
            ( connection_node.connection_type & Nodes::Connection::Mobile )
        )
    );
}
