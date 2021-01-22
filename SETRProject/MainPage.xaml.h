//
// MainPage.xaml.h
// Déclaration de la classe MainPage.
//

#pragma once

#include "MainPage.g.h"
#include "UserPositionResult.h"

namespace SETRProject
{
	public delegate void GpsStatusChangedCallback(Windows::Devices::Geolocation::PositionStatus newStatus);
	public ref class Gps sealed {

	private:
		UserPositionResult userPositionRes;
		Windows::Devices::Geolocation::Geolocator^ geolocator;
		GpsStatusChangedCallback^ statusChangedCallback;
	public:
		void init(GpsStatusChangedCallback^ _statusChangedCallback);
		double getLastLatitude();
		double getLastLongitude();
		void positionUpdated(Windows::Devices::Geolocation::Geolocator^ geolocator, Windows::Devices::Geolocation::PositionChangedEventArgs^ args);
		void _statusChanged(Windows::Devices::Geolocation::Geolocator^ geolocator, Windows::Devices::Geolocation::StatusChangedEventArgs^ args);
		void statusChanged(Windows::Devices::Geolocation::PositionStatus newStatus);
	};

	/// <summary>
	/// Une page vide peut être utilisée seule ou constituer une page de destination au sein d'un frame.
	/// </summary>
	public ref class MainPage sealed
	{
	private:
		Gps^ gps;
		uint64_t computeInstant;
	public:
		MainPage();
		void updateUi();
		void onTick(Platform::Object^ sender, Platform::Object^ args);
		void switchToGps();
		void switchToManualGeolocation();
		void showDisabledLocationMessages();
		void hideDisabledLocationMessages();
		void gpsStatusChanged(Windows::Devices::Geolocation::PositionStatus newStatus);
	private:
		void Button_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void UserLongitudeTextBlock_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
		void UserLatitudeTextBlock_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
		void CheckBox_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void CheckBox_Unchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
	};
}
