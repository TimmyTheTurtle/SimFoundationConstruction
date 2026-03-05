#pragma once
#include <Windows.h>
#include <string>
#include <debugapi.h>
class ScopedQpcTimer {
	const char* label_;
	LARGE_INTEGER freq_{}, start_{};
public:
	ScopedQpcTimer(const char* label);
	~ScopedQpcTimer() noexcept;
};