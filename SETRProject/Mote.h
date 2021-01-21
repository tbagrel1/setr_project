#pragma once

#pragma once

#include <string>
#include "json.hpp"
#include "StringUtils.h"
#include "GeoPoint.h"

struct Mote {
	uint64_t instant;
	std::wstring moteId;
	double temperature;
	GeoPoint position;
	std::wstring positionName;
	bool active;

	Mote() : instant(0), moteId(L""), temperature(0), positionName(L""), active(false) {}
	Mote(const Mote& that) : instant(that.instant), moteId(that.moteId), temperature(that.temperature), position(that.position), positionName(that.positionName), active(that.active) {}

	void updateWithSensorsData(const Mote& that) {
		instant = that.instant;
		temperature = that.temperature;
		active = true;
	}

	// ReadSensors JSON constructor
	Mote(uint64_t _instant, std::wstring _moteId, double _temperature) : instant(_instant), moteId(_moteId), temperature(_temperature), positionName(L""), active(false) {}
	// Mote static list constructor
	Mote(std::wstring _moteId, GeoPoint _position, std::wstring _positionName) : instant(0), moteId(_moteId), temperature(0), position(_position), positionName(_positionName), active(false) {}
	// Full constructor
	Mote(uint64_t _instant, std::wstring _moteId, double _temperature, GeoPoint _position, std::wstring _positionName, bool _active) : instant(_instant), moteId(_moteId), temperature(_temperature), position(_position), positionName(_positionName), active(_active) {}

	static Mote fromJson(nlohmann::json const& obj) {
		return Mote(
			obj.at("timestamp").get<uint64_t>(),
			toWstring(obj.at("mote").get<std::string>()),
			obj.at("value").get<double>()
		);
	}
};
