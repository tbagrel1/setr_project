//
// MainPage.xaml.h
// Déclaration de la classe MainPage.
//

#pragma once

#include "MainPage.g.h"
#include "UserPositionResult.h"

namespace SETRProject
{
	public ref class Gps sealed {

	private:
		UserPositionResult userPositionRes;
		Windows::Devices::Geolocation::Geolocator^ geolocator;

	public:
		void init();
		void positionUpdated(Windows::Devices::Geolocation::Geolocator^ geolocator, Windows::Devices::Geolocation::PositionChangedEventArgs^ args);
	};

	/// <summary>
	/// Une page vide peut être utilisée seule ou constituer une page de destination au sein d'un frame.
	/// </summary>
	public ref class MainPage sealed
	{
	private:
		Gps gps;
		uint64_t computeInstant;
	public:
		MainPage();
		void updateUi();
		void onTick(Platform::Object^ sender, Platform::Object^ args);
	};
}
