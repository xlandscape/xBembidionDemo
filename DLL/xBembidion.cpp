#define BOOST_PYTHON_STATIC_LIB    
#define _USE_MATH_DEFINES

#include <boost/python.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <vector>
#include <string>
#include "xBembidion.h"

using namespace std;

//------------------------------------------------------------------------------

TBembidion::TBembidion(double position_x, double position_y, 
		int landscape_width, int landscape_height, int life_stage)  // constructor
{
	LifeStage = life_stage;
	Age = 0;
	DegreeDays = 0;
	FrostDegreeDays = 0;
	Habitat = 0;
	Hibernating = false;

	// set position and direction
	X = position_x;
	Y = position_y;
	Direction = GetRandomInt(0, 7);

	LandscapeWidth = landscape_width;
	LandscapeHeight = landscape_height;

	// reproduction variables
	if (life_stage == stADULT)  // allow reproduction in first year for initialised adults
		CanReproduce = true;
	else
		CanReproduce = false;
	NumberEggsLeftInLife = repMAX_TOTAL_EGGS_PER_FEMALE;

	Habitat= 0;
	Temperature = 0.0;

	HabitatRaster = NULL;
	PesticideAmountRaster = NULL;

}
//------------------------------------------------------------------------------

TBembidion::~TBembidion()  // destructor
{
}
//------------------------------------------------------------------------------

void TBembidion::UpdateDegreeDays()
{
	double degrees_to_add = Temperature - devDEVELOPMENT_TEMPERATURE_THRESHOLD;

	// correct developmental process for high temperature if necessary
	if (Temperature > devDEVELOPMENT_INFLECTION_TEMPERATURE)
		degrees_to_add *= devDEVELOPMENT_DAY_DEGREES_CORRECTION_FACTOR[LifeStage];

	if (LifeStage != stADULT)
		DegreeDays += degrees_to_add;
	else    // LifeStage == stADULT
	{
		// add-up negative degrees for winter mortality calculation
		if (Temperature < 0.0)
			FrostDegreeDays += Temperature;
	}

	return;
}
//------------------------------------------------------------------------------

bool TBembidion::HabitatSuitableForHibernation()
{
	return false;
	
	Habitat = GetHabitatFromLandscapeRaster(X, Y);
	if (Habitat == habHABITAT_SUITABLE_FOR_HIBERNATION)
		return true;

	return false;
}
//------------------------------------------------------------------------------

int TBembidion::GetHabitatFromLandscapeRaster(int x, int y)
// Progress through life stages based on degree days
{
	int index = y * LandscapeWidth + x;

	if (index > HabitatRaster->size())
	{
		std::wstring message = std::to_wstring(x) + L" ";
		message += std::to_wstring(y) + L" ";
		message += std::to_wstring(index) + L" ";
		message += std::to_wstring(HabitatRaster->size()) + L" ";
		MessageBox(NULL, message.c_str(), L"GetHabitatFromLandscapeRaster Error", MB_OK);
	}

	// look-up habitat in the habitat raster
	int habitat = HabitatRaster->at(index);

	// remember habitat
	Habitat = habitat;

	//std::wstring message = std::to_wstring(habitat);
	//MessageBox(NULL, message.c_str(), L"Habitat", MB_OK);

	if (habitat == 10)
	{
		//MessageBox(NULL, L"Yes, Habitat == 10", L"Habitat", MB_OK);
		return habHABITAT_ACCESSIBLE_AS_TARGET;
	}
	else
	{
		//MessageBox(NULL, L"No, Habitat != 10", L"Habitat", MB_OK);
		return habHABITAT_INACCESSIBLE;
	}

	/*	if (MainForm->CoordinatesOutsideOfLandscape(x, y))
			return habHABITAT_INACCESSIBLE;

		if (MainForm->IsInsideObstaclePanel(x, y))
			return habHABITAT_INACCESSIBLE;

		return habHABITAT_ACCESSIBLE_AS_TARGET;  */
}
//------------------------------------------------------------------------------

int TBembidion::GetDensityFromLandscapeRaster(int x, int y)
// Progress through life stages based on degree days
{
	int index = y * LandscapeWidth + x;

	if (index > DensityRaster->size())
	{
		std::wstring message = L"x:" + std::to_wstring(x) + L" - y:" + std::to_wstring(y) + 
				L" - index:" + std::to_wstring(index) + L" - raster size:" + std::to_wstring(DensityRaster->size()) + L" ";
		MessageBox(NULL, message.c_str(), L"GetDensityFromLandscapeRaster Error", MB_OK);
	}

	// look-up density in the density raster
	int density = DensityRaster->at(index);

	return density;
}
//------------------------------------------------------------------------------

void TBembidion::Develop()
// Progress through life stages based on degree days
{
	// no need for development if individual is already adult
	if (LifeStage == stADULT)
		return;

	// check if next life stage reached
	if (DegreeDays > devDEVELOPMENT_DAY_DEGREES[LifeStage])
	{
		// go to next life stage
		LifeStage++;

		// reset degree days for larvae (stage 1, not for stage 2 and 3), pupae and adults
		if (LifeStage == stLARVA_1 || LifeStage == stPUPA || LifeStage == stADULT)
			DegreeDays = 0;
	}

	return;
}
//------------------------------------------------------------------------------

bool TBembidion::DieFromDailyBackgroundMortality()
// Daily background mortality for each life stage independent from e.g. temperature
/*
	Compare: File Bembidion_all.cpp - Line  363 - void Bembidion_Egg_List::DailyMortality()
			 File Bembidion_all.cpp - Line 1004 - bool Bembidion_Larvae::DailyMortality()
			 File Bembidion_all.cpp - Line 1352 - bool Bembidion_Pupae::DailyMortality()
			 File Bembidion_all.cpp - Line 2306 - bool Bembidion_Adult::DailyMortality()
*/
{
	if (GetRandomFloat() < morDAILY_MORTALITY[LifeStage])
		return true;

	return false;
}
//------------------------------------------------------------------------------

bool TBembidion::DieFromDensityMortality()
{
	// in ALMaSS, density dependent mortality is implemented for adults and larvae but only applied to adults
	if (LifeStage != stADULT)
		return false;

	// do not die from density-dependent mortality during hibernation
	if (JulianDay >= rwHIBERNATION_START_JULIAN_DAY || JulianDay < rwHIBERNATION_END_JULIAN_DAY)
		return false;

	// get density at position (number of individuals within certain range)
	int density = GetDensityFromLandscapeRaster(X, Y);

	// check for mortality
	if (density >= morMIN_NUMBER_ADULTS_FOR_DENSITY_MORTALITY && GetRandomFloat() < morDAILY_ADULT_DENSITY_MORTALITY)
		return true;

	return false;
}
//------------------------------------------------------------------------------

bool TBembidion::DieFromTemperatureMortality()
/*
	Compare: File Bembidion_all.cpp - Line 864 - bool Bembidion_Larvae::TempRelatedLarvalMortality(int temp2)
*/
{
	/*
		Original comment from "File Bembidion_all.cpp":
		The idea here is that we calculate how long a larvae would expect to be
		at a temperature (based on today's temp)
		We also calculate the background mortality expected at that temperature
		then divide the total mortality by the number of days
		then apply that mortality today
		Problem is that we are dealing with percentages so this is not straight
		forward.
		The solution used here is to calculate a set of development and daily
		mort for 5 degree intervals then choose the closest.
	*/
	double temperature = Temperature;

	// stay in temperature range 0-25 degrees
	if (temperature < 0.0)
		temperature = 0.0;
	if (temperature > 25.0)
		temperature = 25.0;

	// get desired temperature interval
	int temperature_interval = (int)(floor(temperature + 2.0) * 0.2);

	if (LifeStage == stLARVA_1)
	{
		if (GetRandomFloat() < (morDAILY_LARVA_1_TEMPERATURE_MORTALITY[temperature_interval]))
			return true;
	}
	else if (LifeStage == stLARVA_2)
	{
		if (GetRandomFloat() < (morDAILY_LARVA_2_TEMPERATURE_MORTALITY[temperature_interval]))
			return true;
	}
	else if (LifeStage == stLARVA_3)
	{
		if (GetRandomFloat() < (morDAILY_LARVA_3_TEMPERATURE_MORTALITY[temperature_interval]))
			return true;
	}
	else
	{
		// do nothing
	}

	return false;
}
//------------------------------------------------------------------------------

bool TBembidion::DieFromPesticides()
{
	// update body burden
	UpdateBodyBurden();

	if (BodyBurden.size() != BodyBurdenThreshold.size())
		return false;

	// check if threshold reached
	for (unsigned int p = 0; p < BodyBurden.size(); p++)
		if (BodyBurden[p] > BodyBurdenThreshold[p])
			return true;

	return false;
}
//------------------------------------------------------------------------------

bool TBembidion::DieFromCumulativeWinterMortality()
/*
	Compare: File Bembidion_all.cpp - Line 1698 - bool Bembidion_Adult::WinterMort()
*/
{
	// Beetles are immobile during winter so only call this once at the end of overwintering
	double probability_to_survive;
	if (FrostDegreeDays > -40)
		probability_to_survive = 0.94925 + (FrostDegreeDays * 0.00426);
	else if (FrostDegreeDays > -80)
		probability_to_survive = 1.16913 + (FrostDegreeDays * 0.01149);
	else if (FrostDegreeDays > -107)
		probability_to_survive = 0.6665 + (FrostDegreeDays * 0.00528);
	else
		probability_to_survive = 0.1; 	// max 90% mortality

	if (GetRandomFloat() > probability_to_survive)
		return true;	// die

	return false;
}
//------------------------------------------------------------------------------

bool TBembidion::Die()
/* Check for mortality, mortality may depend on:
		life stage
		density
		temperature
		pesticides
		soil cultivation / harvest */
{
	// evaluate daily backgroud mortality depending on life stage
	if (DieFromDailyBackgroundMortality())
		return true;

	// evaluate daily backgroud mortality depending on life stage
	if (DieFromDensityMortality())
		return true;

	// evaluate daily backgroud mortality depending on life stage
	if (DieFromTemperatureMortality())
		return true;

	// evaluate daily backgroud mortality depending on life stage
	if (DieFromPesticides())
		return true;

	// evaluate cumulated winter mortality
	if (JulianDay == rwHIBERNATION_END_JULIAN_DAY && DieFromCumulativeWinterMortality())
		return true;

	return false;
}
//------------------------------------------------------------------------------

int TBembidion::Reproduce()
// Handle reproduction if conditions are met (i.e. lay eggs)
{
	// do not reproduce during hibernation
	if (JulianDay >= rwHIBERNATION_START_JULIAN_DAY || JulianDay < rwHIBERNATION_END_JULIAN_DAY)
	{	
		// allow reproduction after an adult individual hibernated
		if (LifeStage == stADULT)
			CanReproduce = true;
		return 0;
	}

	// return if individual cannot reproduce (e.g. due to life stage)
	if (!CanReproduce)
		return 0;

	// return if no eggs left
	if (NumberEggsLeftInLife < 1)
		return 0;

	// consider minimum temperature for egg laying
	if (Temperature < repMIN_EGG_LAYING_TEMPERATURE)
		return 0;

	// calculate number of eggs laid today based on function and max egg number
	int eggs_laid = (int)floor(0.5 +
		((Temperature - repMIN_EGG_LAYING_TEMPERATURE) *
			repEGG_PRODUCTION_FACTOR));

	if (eggs_laid > 10) // allow a maximum of 10 eggs to be laid per day
		eggs_laid = 10;

	if (eggs_laid > NumberEggsLeftInLife) // only lay eggs left in lifetime
		eggs_laid = NumberEggsLeftInLife;

/*	// lay eggs, i.e. create new bembidion objects
	for (int i = 0; i < eggs_laid; i++)
	{
		TBembidion* bembid = new TBembidion(X, Y, LandscapeMinX, LandscapeMaxX, LandscapeMinY, LandscapeMaxY, stEGG);
		//bembid->Model = Model;
		Bembidions.push_back(bembid);  // store bembidions in vector
	}*/

	// subtract number of eggs
	NumberEggsLeftInLife -= eggs_laid;

	return eggs_laid;
}
//------------------------------------------------------------------------------

void TBembidion::Forage()
// Move to other locations to forage
{
}
//------------------------------------------------------------------------------
void TBembidion::Aggregate()
// Aggregate at some date to search for hibernation place
{
}
//------------------------------------------------------------------------------

void TBembidion::Move(const int max_distance, const bool dispersing)
// Move beetle based on life stage and distance
{
	//std::wstring message = std::to_wstring(max_distance);
	//MessageBox(NULL, message.c_str(), L"Max distance", MB_OK);

	try {

		// No movement during hibernation
		if (JulianDay >= rwHIBERNATION_START_JULIAN_DAY || JulianDay < rwHIBERNATION_END_JULIAN_DAY)
			return;
		
		// No movement if temperature is too low
		if (Temperature < movMINIMUM_TEMPERATURE_FOR_MOVEMENT)
			return;

		// check if currently hibernating
		if (RandomWalkTarget == rwHIBERNATION_HABITAT && HabitatSuitableForHibernation())
		{
			Hibernating = true;
			CanReproduce = true;    // adults can reproduce after first hibernation
			return;
		}

		double x = X;
		double y = Y;
		int direction = Direction;
		int habitat = Habitat;
		double turning_probability = 0.0;
		if (dispersing)
			turning_probability = movADULT_TURNING_PROBABILITY;

		// Adjust direction
		if (dispersing && GetRandomFloat() < turning_probability)
		{
			// left or right each at 50% probability
			if (GetRandomFloat() < 0.5)
				direction = (direction + 1) % 8;
			else
				direction = (direction + 7) % 8;
		}

		// iterate through distance (one step per distance in m)
		for (int i = 0; i < max_distance; i++) {

			int tries = 0;

			while (habitat != habHABITAT_ACCESSIBLE_AS_TARGET && tries++ < 10) {

				// Calculate direction angle in radians (invert y-axis)
				double angle = direction * (M_PI / 4); // Each direction is 45 degrees

				// Calculate new position using trigonometry, flipping the y-axis movement
				double new_x = x + (double)max_distance * cos(angle);
				double new_y = y - (double)max_distance * sin(angle); // Subtracting since increasing y moves down

				// Check if position outside allowed bounds
				if (PositionOutOfLandscapeBounds(new_x, new_y))
				{
					continue;
				}

				habitat = GetHabitatFromLandscapeRaster(new_x, new_y);

				if (habitat == habHABITAT_INACCESSIBLE)
				{
					// change direction
					if (GetRandomFloat() < 0.5)
						direction = (direction + 1) % 8;
					else
						direction = (direction + 7) % 8;
				}
				else if (habitat == habHABITAT_ACCESSIBLE_FOR_WALKING && GetRandomFloat() < 0.6)    // if low-quality habitat, 60% skip
				{
					// change direction
					if (GetRandomFloat() < 0.5)
						direction = (direction + 1) % 8;
					else
						direction = (direction + 7) % 8;
				}
				else    // Valid move
				{
					// assign coordinates
					X = new_x;
					Y = new_y;
					Direction = direction;
				}
			}
		}

		if (!dispersing)
		{
			habitat = GetHabitatFromLandscapeRaster(X, Y);
			if (JulianDay > STOP_AGGREGATION_DAY && HabitatIsSuitable(habitat)) {
				return;  // Suitable habitat found, stop aggregating
			}
		}
	}
	catch (...) {
		X = 100;
		Y = 100;
	}

	return;
}
//------------------------------------------------------------------------------

bool TBembidion::HabitatIsSuitable(int habitat)
{

	return true;
}
//------------------------------------------------------------------------------

bool TBembidion::PositionOutOfLandscapeBounds(int x, int y)
// Check if position is out of landscape bounds
{
	if (x < 0 || y < 0 || x >= LandscapeWidth || y >= LandscapeHeight)	// >= because index is 1 less than width or height
		return true;

	return false;
}
//------------------------------------------------------------------------------

vector<double> TBembidion::GetPesticideAmountFromLandscapeRaster(int x, int y)
	// Get pesticide amount from landscape
{
	vector<double> pesticide_amount;
	int index, corrected_index, number_raster_pixel, number_pesticides;

	try {
		index = y * LandscapeWidth + x;

		if (index > PesticideAmountRaster->size())
		{
			std::wstring message = L"x:" + std::to_wstring(x) + L" - y:" + std::to_wstring(y) + L" - index:" +
				std::to_wstring(index) + L" - raster size:" + std::to_wstring(PesticideAmountRaster->size());
			MessageBox(NULL, message.c_str(), L"GetPesticideAmountFromLandscapeRaster Error", MB_OK);
		}

		// get pesticide amount from landscape

		number_raster_pixel = LandscapeWidth * LandscapeHeight;
		number_pesticides = PesticideAmountRaster->size() / number_raster_pixel;
		pesticide_amount.resize(number_pesticides);
		for (unsigned int p = 0; p < number_pesticides; p++)
		{
			corrected_index = index + p * number_raster_pixel;
			pesticide_amount[p] = PesticideAmountRaster->at(corrected_index);
		}
	}
	catch (...) {
		std::wstring message =	L"x:" + std::to_wstring(x) + 
								L" - y:" + std::to_wstring(y) + 
								L" - index:" + std::to_wstring(index) + 
								L" - corrected index:" + std::to_wstring(corrected_index) +
								L" - raster size:" + std::to_wstring(PesticideAmountRaster->size());
		MessageBox(NULL, message.c_str(), L"GetPesticideAmountFromLandscapeRaster Error", MB_OK);
	}
	
	return pesticide_amount;
}
//------------------------------------------------------------------------------

void TBembidion::UpdateJulianDay(int day)
// Handle reproduction if conditions are met (i.e. lay eggs)
{
	JulianDay = day;

	return;
}
//------------------------------------------------------------------------------

void TBembidion::UpdateTemperature(double temperature)
// Handle reproduction if conditions are met (i.e. lay eggs)
{
	Temperature = temperature;

	return;
}
//------------------------------------------------------------------------------

void TBembidion::UpdateBodyBurden()
	// Update pesticide burden on beetle
{
	// get pesticide amount
	vector<double> pesticide_amount = GetPesticideAmountFromLandscapeRaster(X, Y);

	if (pesticide_amount.size() != BodyBurden.size())
		return;

	// now update body burden
	for (unsigned int p = 0; p < pesticide_amount.size(); p++)
		BodyBurden[p] += pesticide_amount[p];

	return;
}
//------------------------------------------------------------------------------

void TBembidion::UpdateHabitatLandscapeRaster(const vector<double>& raster)
{
	// update the pointer to the landscape
	HabitatRaster = &raster;

	return;
}
//------------------------------------------------------------------------------

void TBembidion::UpdateDensityLandscapeRaster(const vector<int>& raster)
{
	// update the pointer to the landscape
	DensityRaster = &raster;

	return;
}
//------------------------------------------------------------------------------

void TBembidion::UpdatePesticideAmountLandscapeRaster(const vector<double>& raster)
{
	// update the pointer to the landscape
	PesticideAmountRaster = &raster;

	return;
}
//------------------------------------------------------------------------------

void TBembidion::SetBodyBurdenThreshold(const vector<double> body_burden)
{
	// set the body burden thresholds
	BodyBurdenThreshold = body_burden;

	// initialise body burden object
	BodyBurden.resize(body_burden.size());
	for (unsigned int p = 0; p < BodyBurden.size(); p++)
		BodyBurden[p] = 0.0;	

	return;
}
//------------------------------------------------------------------------------

#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <vector>

BOOST_PYTHON_MODULE(xBembidion)
{
	using namespace boost::python;

	// Expose vector formats
	class_<std::vector<int>>("PyIntVec")
		.def(vector_indexing_suite<std::vector<int>>());
	class_<std::vector<double> >("PyFloatVec")
		.def(vector_indexing_suite<std::vector<double>>());

	// Expose TBembidion class
	class_<TBembidion>("TBembidion", init<double, double, int, int, int>()) // Constructor
		.def("Develop", &TBembidion::Develop)
		.def("Reproduce", &TBembidion::Reproduce)
		.def("Die", &TBembidion::Die)
		.def("UpdateDegreeDays", &TBembidion::UpdateDegreeDays)
		.def("UpdateJulianDay", &TBembidion::UpdateJulianDay)
		.def("UpdateTemperature", &TBembidion::UpdateTemperature)
		.def("GetHabitatFromLandscapeRaster", &TBembidion::GetHabitatFromLandscapeRaster)
		.def("HabitatIsSuitable", &TBembidion::HabitatIsSuitable)
		.def("PositionOutOfLandscapeBounds", &TBembidion::PositionOutOfLandscapeBounds)
		.def("UpdateHabitatLandscapeRaster", &TBembidion::UpdateHabitatLandscapeRaster)
		.def("UpdateDensityLandscapeRaster", &TBembidion::UpdateDensityLandscapeRaster)
		.def("UpdatePesticideAmountLandscapeRaster", &TBembidion::UpdatePesticideAmountLandscapeRaster)
		.def("Forage", &TBembidion::Forage)
		.def("Aggregate", &TBembidion::Aggregate)
		.def("Move", &TBembidion::Move)
		.def("GetPesticideAmountFromLandscapeRaster", &TBembidion::GetPesticideAmountFromLandscapeRaster)
		.def("SetBodyBurdenThreshold", &TBembidion::SetBodyBurdenThreshold)
		.def_readwrite("LifeStage", &TBembidion::LifeStage)
		.def_readwrite("X", &TBembidion::X)
		.def_readwrite("Y", &TBembidion::Y);
}
