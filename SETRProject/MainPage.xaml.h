//
// MainPage.xaml.h
// Déclaration de la classe MainPage.
//

#pragma once

#include "MainPage.g.h"

namespace SETRProject
{
	/// <summary>
	/// Une page vide peut être utilisée seule ou constituer une page de destination au sein d'un frame.
	/// </summary>
	public ref class MainPage sealed
	{
	private:
		uint64_t readSensorsInstant;
		uint64_t findNearestInstant;
	public:
		MainPage();
		void updateUi();
		void onTick(Platform::Object^ sender, Platform::Object^ args);
	};
}
