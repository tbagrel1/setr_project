#pragma once

#define M_PI 3.14159265358979323846
#include <cmath>

struct GeoPoint
{
	double latitude;
	double longitude;

	double distanceWith(const GeoPoint& that) {
		double earthRadius = 6371.0;
		double radDeltaLatitude = (that.latitude - latitude) * (M_PI / 180.0);
		double radDeltaLongitude = (that.longitude - longitude) * (M_PI / 180.0);
		double haversine = sin(radDeltaLatitude / 2) * sin(radDeltaLatitude / 2) +
			cos(latitude * (M_PI / 180.0)) * cos(that.latitude * (M_PI / 180.0)) * sin(radDeltaLongitude / 2) * sin(radDeltaLongitude / 2);
		return 2.0 * earthRadius * asin(sqrt(haversine));
	}

	GeoPoint() : latitude(0), longitude(0) {}
	GeoPoint(const GeoPoint& that) : latitude(that.latitude), longitude(that.longitude) {}
	GeoPoint(double _latitude, double _longitude) : latitude(_latitude), longitude(_longitude) {}
};
