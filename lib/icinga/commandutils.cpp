#include "icinga/commandutils.hpp"
#include "icinga/host.hpp"
#include "icinga/checkcommand.hpp"
#include "remote/apilistener.hpp"
#include "remote/httputility.hpp"

using namespace icinga;


void CommandUtils::ExecuteCommandLocally(const Checkable::Ptr& checkable, const CheckCommand::Ptr& command, const Dictionary::Ptr& params)
{

    Dictionary::Ptr macros;
    if (!params->Contains("macros"))
        macros = new Dictionary();
    else
        macros = params->Get("macros");

    Array::Ptr command_line_tmp = command->GetCommandLine();
    Value shutdown_command_value = params->Get("shutdown_command");
    Array::Ptr full_command;
    if (shutdown_command_value.IsObjectType<Array>()){
        full_command = shutdown_command_value;
    } else{
        Log(LogNotice, "CommandUtils")
                << "Shutdown command must be of type Array.";
        return;
    }

    if(full_command->GetLength() == 0){
        Log(LogNotice, "CommandUtils")
                << "Shutdown command must not be empty.";
        return;
    }
    CheckResult::Ptr cr = new CheckResult();
    command->SetCommandLine(full_command);
    command->Execute(checkable, cr, macros, true);
    command->SetCommandLine(command_line_tmp);
}


void CommandUtils::SendCommandMessageToEndpoints(const Endpoint::Ptr& endpoint, const ApiListener::Ptr& listener, const Dictionary::Ptr& message)
{

    Zone::Ptr local_zone = Zone::GetLocalZone();
    Endpoint::Ptr local_endpoint = Endpoint::GetLocalEndpoint();

    std::set<Endpoint::Ptr> target_endpoints;
    for (const Zone::Ptr& zone : ConfigType::GetObjectsByType<Zone>()) {
        if (zone->GetParent() == local_zone) {
            std::set<Endpoint::Ptr> endpoints = zone->GetEndpoints();
            target_endpoints.insert(endpoints.begin(), endpoints.end());
        }
    }

    for (const Endpoint::Ptr& e : target_endpoints) {
        if (e == local_endpoint || ! e->GetConnected())
            continue;
        if (e == endpoint) {
            Log(LogNotice, "CommandUtils") << "Sending message to target endpoint '" << e->GetName() << "'.";
            listener->SyncSendMessage(e, message);
            return;
        }
    }

    for (const Endpoint::Ptr& e : target_endpoints) {
        if (e == local_endpoint || ! e->GetConnected())
            continue;
        Log(LogNotice, "CommandUtils") << "Broadcasting message to connected endpoint '" << e->GetName() << "'.";
        listener->SyncSendMessage(e, message);
    }

}
