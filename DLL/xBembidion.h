//#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <algorithm>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <random>
#include <cmath>
#include <math.h>

using namespace std;

#ifndef BEMBIDION_MODEL_H
#define BEMBIDION_MODEL_H

/****************************************************************************
*******************************   Constants   *******************************
****************************************************************************/

// Life stages of the beetle
const unsigned int stEGG = 0;
const unsigned int stLARVA_1 = 1;
const unsigned int stLARVA_2 = 2;
const unsigned int stLARVA_3 = 3;
const unsigned int stPUPA = 4;
const unsigned int stADULT = 5;
const unsigned int stNUMBER_LIFE_STAGES = 6;

// Constants for DayDegree development for different life stages
const double devDEVELOPMENT_DAY_DEGREES[stNUMBER_LIFE_STAGES] =
{
	178.58,   	// Day degrees for egg development: egg --> larval stage 1
	101.4,   	// Day degrees for larval development: larval stage 1 --> larval stage 2
	107.4,   	// Day degrees for larval development: larval stage 2 --> larval stage 3
	190.4,   	// Day degrees for larval development: larval stage 3 --> pupa
	147.7,   	// Day degrees for pupa development: pupa --> adult
	0.0         // No day degrees for development needed for adults
};

const double devDEVELOPMENT_TEMPERATURE_THRESHOLD = 3.0;    // Threshold temperature to be subtracted for all day degree calculations for all life stages
const double devDEVELOPMENT_INFLECTION_TEMPERATURE = 12.0; 	// Temperature at which development speed changes

// Day degrees correction factors for fifferent life stages at higher temperatures
const double devDEVELOPMENT_DAY_DEGREES_CORRECTION_FACTOR[stNUMBER_LIFE_STAGES] =
{
	178.58 / 124.9,   // stEGG
	101.4 / 87.5,     // stLARVA_1
	107.4 / 95.2,     // stLARVA_2
	190.4 / 189.3,    // stLARVA_3
	147.7 / 132.3,    // stPUPA
	0.0             // stADULT (no developmental factor needed)
};


const int STOP_AGGREGATION_DAY = 280;

// reproduction constants
const double repMIN_EGG_LAYING_TEMPERATURE = 3.0;
const double repEGG_PRODUCTION_FACTOR = 0.6;
const int repMAX_TOTAL_EGGS_PER_FEMALE = 256;

// Mortality constants
const double morDAILY_MORTALITY[stNUMBER_LIFE_STAGES] =
{
	0.007,  // stEGG
	0.001,  // stLARVA_1
	0.001,  // stLARVA_2
	0.001,  // stLARVA_3
	0.001,  // stPUPA
	0.001   // stADULT
};

// Temperature-dependent mortality for larval stages (each specified for 6 temperature intervals)
const double morDAILY_LARVA_1_TEMPERATURE_MORTALITY[6] = { 0.0855,0.0855,0.1036,0.0563,0.0515,0.0657 };
const double morDAILY_LARVA_2_TEMPERATURE_MORTALITY[6] = { 0.0626,0.0626,0.0940,0.0529,0.0492,0.0633 };
const double morDAILY_LARVA_3_TEMPERATURE_MORTALITY[6] = { 0.0631,0.0631,0.0545,0.0268,0.0236,0.0295 };

// Density-dependent mortality for adults
const int morMIN_NUMBER_ADULTS_FOR_DENSITY_MORTALITY = 3;
const double morDAILY_ADULT_DENSITY_MORTALITY = 0.1;

// Constants for the movement
const int movMAX_DAILY_MOVEMENT_DISTANCE = 14;
const double movMINIMUM_TEMPERATURE_FOR_MOVEMENT = 1.0;
const double movADULT_TURNING_PROBABILITY = 0.8;

// Random walk constants
const int rwHIBERNATION_START_JULIAN_DAY = 270;
const int rwHIBERNATION_END_JULIAN_DAY = 90;
const int rwFORAGING_HABITAT = 0;
const int rwHIBERNATION_HABITAT = 1;

// Habitat constants
const int habHABITAT_NOT_SET = -1;
const int habHABITAT_NOT_SUITABLE_FOR_HIBERNATION = 0;
const int habHABITAT_SUITABLE_FOR_HIBERNATION = 1;
const int habHABITAT_NOT_SUITABLE_FOR_WALKING = 2;
const int habHABITAT_SUITABLE_FOR_WALKING = 3;
const int habHABITAT_NOT_SUITABLE_FOR_FORAGING = 4;
const int habHABITAT_SUITABLE_FOR_FORAGING = 5;

const int habHABITAT_INACCESSIBLE = 0;
const int habHABITAT_ACCESSIBLE_FOR_WALKING = 1;
const int habHABITAT_ACCESSIBLE_AS_TARGET = 2;

// Constants for pesticides
const int PESTICIDES_NO = 4;
const string PESTICIDE_NAMES[PESTICIDES_NO] =
{
	"Pesticide 1",
	"Pesticide 2",
	"Pesticide 3",
	"Pesticide 4",
};

const double PESTICIDE_BODY_BURDEN_THRESHOLDS[PESTICIDES_NO] =
{
	0.5,
	0.03,
	0.1,
	2.4,
};
//-------------------------------------------------------------------------------

// Landscape class
typedef struct
{
	double Temperature;
	int Habitat;
	int FarmEvent;	
} TLandscape;

//-------------------------------------------------------------------------------
// Beetle class declarations
class TBembidion {

public:

	/****************************************************************************
	*******************************   Variables   *******************************
	****************************************************************************/

	// Variables of the individual
	double X, Y;                	// Position of the beetle (coordinates in meter)
	int Direction;               	// Walking direction (int in 0-7, 0 = North, clockwise)
	int LifeStage;  				// Current life stage
	int Age;               			// Total age in days
	double DegreeDays;   			// Cumulative degree days for this individual
	double FrostDegreeDays;      	// Cumulative frost (negative) degree days for this individual (used for winter mortality)
	bool CanReproduce;              // Indicator for reproduction (young adults cannot reproduce)
	int NumberEggsLeftInLife;       // Number of eggs (decreases with egg laying)
	bool Hibernating;               // Is the beetle currently hibernating or not
	vector<double> BodyBurden;      // Pesticide burden level for a vector of different pesticides 
	vector<double> BodyBurdenThreshold;	// Pesticide burden level thresholds for a vector of different pesticides (to be provided by Python component)
	int Habitat;					// Habitat at current position
	int RandomWalkTarget;			// Target of random walk (e.g. search for hibernation place)
	int JulianDay;					// Current Julian day
	double Temperature;				// Current temperature around the individual

	// Variables of the landscape not directly linked to the individual
	unsigned int LandscapeWidth;	// width of landscape (in meter)
	unsigned int LandscapeHeight;	// height of landscape (in meter)
	const vector<double> *HabitatRaster;	// Pointer to habitat raster object to be provided by Python component
	const vector<int> *DensityRaster;	// Pointer to density raster object to be provided by Python component
	const vector<double> *PesticideAmountRaster;	// Pointer to pesticide exposure raster object (multiple rasters in one row possible) to be provided by Python component

	/****************************************************************************
	*******************************   Functions   *******************************
	****************************************************************************/

	// Constructor and Destructor
	TBembidion(double position_x, double position_y, int landscape_width, int landscape_height, int life_stage);
	~TBembidion();

	// Development-related functions
	void Develop();         	// Progress through life stages based on degree days

	// Reproduction-related functions (adults only)
	int Reproduce();			// Reproduction if conditions are met (i.e. lay eggs), returns number of eggs laid

	// Functions for mortality
	bool Die();					/* Check for mortality, mortality may depend on:
											life stage
											density
											temperature
											pesticides
											soil cultivation / harvest */
	// Sub-functions for mortality
	bool DieFromDailyBackgroundMortality();
	bool DieFromDensityMortality();
	bool DieFromTemperatureMortality();
	bool DieFromPesticides();
	bool DieFromCumulativeWinterMortality();

	// Other helper functions
	void UpdateBodyBurden();	// Update pesticide burden on beetle
	void UpdateDegreeDays();
	void UpdateJulianDay(int day);
	void UpdateTemperature(double temperature);
	bool HabitatSuitableForHibernation();
	int GetHabitatFromLandscapeRaster(int x, int y);
	int GetDensityFromLandscapeRaster(int x, int y);
	bool HabitatIsSuitable(int habitat);
	bool PositionOutOfLandscapeBounds(int x, int y);
	void SetBodyBurdenThreshold(const vector<double> body_burden);

	// Movement-related functions
	void Forage();		// Move to other locations to forage
	void Aggregate();	// Aggregate at some date to search for hibernation place
	void Move(const int max_distance, const bool dispersing);   // Move beetle based on life stage and distance

	// Temperature and environmental conditions
	vector<double> GetPesticideAmountFromLandscapeRaster(int x, int y);
	void UpdateHabitatLandscapeRaster(const vector<double> &raster);
	void UpdateDensityLandscapeRaster(const vector<int>& raster);
	void UpdatePesticideAmountLandscapeRaster(const vector<double>& raster);
};

// Function to get a random number between 0 and 1
inline double GetRandomFloat() {
	// Static local variables are initialized only once and persist across function calls
	static std::random_device rd;   // Seed generator
	static std::mt19937 gen(rd());	// Mersenne Twister generator seeded by rd()
	static std::uniform_real_distribution<> get_random_num(0.0, 1.0); // Distribution [0.0, 1.0)

	// Generate and return a random number between 0 and 1
	return get_random_num(gen);
}

// Function to get a random integer between min and max (inclusive)
inline int GetRandomInt(int min, int max) {
	static std::random_device rd;  // Seed generator
	static std::mt19937 gen(rd()); // Mersenne Twister generator seeded by rd()

	std::uniform_int_distribution<> get_random_num(min, max); // Integer distribution in [min, max]
	return get_random_num(gen);
}

#endif  // BEMBIDION_MODEL_H