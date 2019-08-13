#pragma once

#include <atomic>
#include <cstdint>
#include "base/utility.hpp"

class Wip {
public:
	struct IncDec {
		std::atomic<uint_fast64_t> Inc, Dec;

		IncDec();
	};

	struct Avg {
		std::atomic<uint_fast64_t> Count, Sum;

		Avg();

		inline void Add(uint_fast64_t x) {
			Count.fetch_add(1);
			Sum.fetch_add(x);
		}

		inline double Calc() {
			return (double)Sum.load() / Count.load();
		}
	};

	IncDec CC, CE, PT;

	struct {
		struct {
			Avg GetNextPending, AquireSlot, Prepare, IncreaseSlot, Enqueue, HaveTurn, FireCheck, DecreaseSlot, PostProcess;
		} CheckerComponent;
	} Lantencies;
};

class Bench {
public:
	inline Bench() : start(icinga::Utility::GetTime()) {
	}

	inline double Stop() {
		return icinga::Utility::GetTime() - start;
	}

private:
	double start;
};

const double MinSecFrac = 1000000;

extern Wip l_Wip;
