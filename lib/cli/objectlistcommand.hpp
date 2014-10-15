/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef OBJECTLISTCOMMAND_H
#define OBJECTLISTCOMMAND_H

#include "base/qstring.hpp"
#include "base/dictionary.hpp"
#include "base/array.hpp"
#include "base/clicommand.hpp"

namespace icinga
{

/**
 * The "object list" command.
 *
 * @ingroup cli
 */
class ObjectListCommand : public CLICommand
{
public:
        DECLARE_PTR_TYPEDEFS(ObjectListCommand);

        virtual String GetDescription(void) const;
        virtual String GetShortDescription(void) const;
        virtual void InitParameters(boost::program_options::options_description& visibleDesc,
            boost::program_options::options_description& hiddenDesc,
            ArgumentCompletionDescription& argCompletionDesc) const;
        virtual int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const;

private:
        static void ReadObject(const String& message, std::map<String, int>& type_count, const String& name_filter, const String& type_filter);
        static String FormatProperties(const Dictionary::Ptr& props, const Dictionary::Ptr& debug_hints, int indent = 0);
        static String FormatHints(const Dictionary::Ptr& hints, int indent = 0);
        static String FormatHint(const Array::Ptr& msg, int indent = 0);
        static String FormatTypeCounts(const std::map<String, int>& type_count);
        static String FormatValue(const Value& val);
        static String FormatArray(const Array::Ptr& arr);
};

}

#endif /* OBJECTLISTCOMMAND_H */
