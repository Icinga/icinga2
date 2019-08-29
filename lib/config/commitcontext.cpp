/* Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+ */

#include "config/commitcontext.hpp"

using namespace icinga;

thread_local CommitContext* CommitContext::m_Current = nullptr;
