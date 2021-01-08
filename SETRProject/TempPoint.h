#pragma once

#include <string>

struct TempPoint {
	uint64_t instant;
	std::wstring moteId;
	double value;

	TempPoint() : instant(0), moteId(L""), value(0) {}
	TempPoint(const TempPoint& that) : instant(that.instant), moteId(that.moteId), value(that.value) {}
	TempPoint(uint64_t _instant, std::wstring _moteId, double _value) : instant(_instant), moteId(_moteId), value(_value) {}
};
