#ifndef COMMANDUTILS_H
#define COMMANDUTILS_H

#include "remote/apilistener.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/host.hpp"

namespace icinga
{

    class CommandUtils
    {
    public:
        static void ExecuteCommandLocally(const Checkable::Ptr& checkable, const CheckCommand::Ptr& command, const Dictionary::Ptr& params);
        static void SendCommandMessageToEndpoints(const Endpoint::Ptr& endpoint, const ApiListener::Ptr& listener, const Dictionary::Ptr& message);

    private:
        CommandUtils();
    };

}

#endif /* COMMANDUTILS_H */
