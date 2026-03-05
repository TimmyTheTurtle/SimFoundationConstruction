// Scoped QPC timer (debug output)
#include "ScopedQpcTimer.h"

ScopedQpcTimer::ScopedQpcTimer(const char* label) : label_(label) {
		static const LARGE_INTEGER s_freq = []{
			LARGE_INTEGER f{};
			QueryPerformanceFrequency(&f);
			return f;
		}();
		freq_ = s_freq;
		QueryPerformanceCounter(&start_);
	}
ScopedQpcTimer::~ScopedQpcTimer() noexcept {
		LARGE_INTEGER end{}; QueryPerformanceCounter(&end);
		double ms = (end.QuadPart - start_.QuadPart) * 1000.0 / double(freq_.QuadPart);
		std::string s = std::string(label_) + ": " + std::to_string(ms) + " ms\n";
		OutputDebugStringA(s.c_str());
	}