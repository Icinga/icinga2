#include "icinga/wip.hpp"

Wip::IncDec::IncDec() {
	Inc.store(0);
	Dec.store(0);
}

Wip::Avg::Avg() {
	Count.store(0);
	Sum.store(0);
}

Wip l_Wip;
