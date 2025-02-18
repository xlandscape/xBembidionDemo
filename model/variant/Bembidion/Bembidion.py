import datetime
import numpy as np
import random
import base
import attrib
import typing

import matplotlib.pyplot as plt
from osgeo import gdal, ogr, osr

import xBembidion
from xBembidion import TBembidion
import logging


# Configure logging
logging.basicConfig(
    filename="simulation_log.txt",
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s"
)

class Bembidion(base.Component):
    """
    Version 1.0.0 - Initial model version
    
    This is the first version of the Bembidion model, including basic functionality. Some features need to be implemented in future versions:
    - Debugging of non-reasonable behavior/population development
    - Integration of field events (e.g., ploughing)
    - Special movement patterns for hibernation/aggregation
    - Comparison with the ALMaSS system
    - Use of a configuration file for defining input parameters (as in ALMaSS)
    - Conversion of hardcoded variables into configurable inputs
    """

    # RELEASES
    VERSION = base.VersionCollection(
        base.VersionInfo("1.0.0", None)
    )

    # CHANGELOG
    VERSION.added("1.0.0", "`components.Bembidion` component")

    def __init__(self, name: str, default_observer: base.Observer, default_store: typing.Optional[base.Store]) -> None:
        """
        Initializes the Bembidion simulation component.
        
        Args:
            name (str): Name of the component.
            default_observer (base.Observer): Observer for logging and simulation monitoring.
            default_store (typing.Optional[base.Store]): Storage for simulation outputs.
        """
        super(Bembidion, self).__init__(name, default_observer, default_store)
        self._module = base.Module("Bembidion", "1.0", r"module\README.md", None, None) 
        self._inputs = base.InputContainer(self, [
            base.Input(
                "SimulationStart",
                (attrib.Class(datetime.date, 1), attrib.Unit(None, 1), attrib.Scales("global", 1)),
                self.default_observer,
                description="""The first day of the simulation. A `datetime.date` of global scale. Value has no unit."""
            ),
            base.Input(
                "SimulationEnd",
                (attrib.Class(datetime.date, 1), attrib.Unit(None, 1), attrib.Scales("global", 1)),
                self.default_observer,
                description="""The last day of the simulation. A `datetime.date` of global scale. Value has no unit."""
            ),
            base.Input(
                "Fields",
                (attrib.Class(list[int], 1), attrib.Unit(None, 1), attrib.Scales("space/base_geometry", 1)),
                self.default_observer,
                description="A list of identifiers of individual geometries. A list[int] of scale "
                            "space/base_geometry. Values have no unit."
            ),
            base.Input(
                "GeometryCrs",
                (attrib.Class(str), attrib.Scales("global"), attrib.Unit(None)),
                self.default_observer,
                description="""The coordinate reference system in which the [Fields_Geometries](#Fields_Geometries) 
                are projected. The coordinate reference system needs to be in Proj4 notation."""
            ),
            base.Input(
                "Extent",
                (attrib.Class(tuple[float]), attrib.Scales("space/extent"), attrib.Unit("metre")),
                self.default_observer,
                description="The extent of the simulated landscape. This value has to be consistent "
                            "with the [Fields_Geometries](#Fields_Geometries) and the "
                            "[Fields_FlowGrid](#Fields_FlowGrid) and is projected in the "
                            "[GeometryCrs](#GeometryCrs). The landscape scenario normally takes "
                            "care of that."
            ),
            base.Input(
                "FieldGeometries",
                (attrib.Class(list[bytes]), attrib.Scales("space/base_geometry"), attrib.Unit(None)),
                self.default_observer,
                description="The geometries of in-field areas in WKB representation. Each element "
                            "refers to a field with its according identifier from the list of "
                            "[Fields_Ids](#Fields_Ids)."
            ),
            base.Input(
                "LandUseLandCoverTypes",
                (attrib.Class(list[int], 1), attrib.Unit(None, 1), attrib.Scales("space/base_geometry", 1)), 
                self.default_observer,
                description="The land-use and land-cover type of spatial units. A list[int] of scale "
                            "space/base_geometry. Values have no unit."
            ),
            base.Input(
                "AirTemperature",
                (attrib.Class(np.ndarray), attrib.Scales("time/day"), attrib.Unit("°C")),
                self.default_observer,
                description="A timeseries of daily average air temperatures. Water temperatures will be output for the "
                            "temporal extent spanned by the air temperatures."
            )            
        ])
        self._outputs = base.OutputContainer(self, [
            base.Output("NumberBembids", default_store, self, 
                description="A test output. A numpy-array of scale other/application.")
        ])
        
    def run_model(self, lulc_vec, exposure_vec, temperature):
        """
        Runs the Bembidion simulation model.
        
        Args:
            lulc_vec (list[float]): Flattened LULC raster data.
            exposure_vec (list[float]): Flattened pesticide exposure data.
            temperature (np.ndarray): Array of daily average temperatures.
        """
        
        #---------- convert to DLL-readable format ----------#
        
        self.default_observer.write_message(5, f"Raster conversion for C++ DLL started.")
        
        # Make the LULC array usable for the C++ DLL (using PyFloatVec defined in the DLL code)
        lulc_vec_for_dll = xBembidion.PyFloatVec()
        lulc_vec_for_dll.extend(np.array(lulc_vec, dtype=np.float64))
        
        # Make the density array usable for the C++ DLL (using PyIntVec defined in the DLL code)
        density_map = self.create_density_map()
        density_vec = [value for row in density_map for value in row]
        density_vec_for_dll = xBembidion.PyIntVec()
        density_vec_for_dll.extend(np.array(density_vec, dtype=np.int64).tolist())
   
        # Make the exposure (e.g. pesticide) array usable for the C++ DLL (using PyFloatVec defined in the DLL code)
        exposure_vec_for_dll = xBembidion.PyFloatVec()
        exposure_vec_for_dll.extend(np.array(exposure_vec, dtype=np.float64))
        
        self.default_observer.write_message(5, f"Raster conversion for C++ DLL finished.")
        
        # remove individuals that were not placed in a valid habitat (can only be checked after individuals have been created)
        for bembid in self.Bembidions[:]:            
            bembid.UpdateHabitatLandscapeRaster(lulc_vec_for_dll) # provide pointer to landscape
            if bembid.GetHabitatFromLandscapeRaster(int(bembid.X), int(bembid.Y)) != 2:   # habHABITAT_ACCESSIBLE_AS_TARGET = 2
                self.Bembidions.remove(bembid)
        
        #---------- Run model ----------#
        # Start simulation
        self.default_observer.write_message(5, f"Simulation started.")
        for day in range(self.SimulationDays):
            #logging.info("Day %d: Population count: %d", day + 1, self.get_population_count())
            self.simulate_day(lulc_vec_for_dll, density_vec_for_dll, exposure_vec_for_dll, temperature)

    def initialize(self, lulc_rows, lulc_cols):
        """
        Initializes environmental and population parameters.
        Args:
            lulc_rows: The landscape height.
            lulc_cols: The landscape width.
        Returns:
            Nothing.
        """
        
        # Initialize global variables
        self.InitialNumberOfIndividuals = 100
        self.SimulationDays = 365
        self.RandomWalkTarget = 0
        self.JulianDay = 0
        self.Bembidions = []
        self.BembidionCount = []
        
        # Assign landscape parameters
        self.LandscapeWidth = lulc_cols
        self.LandscapeHeight = lulc_rows
        
        print(f"Width: {lulc_cols} - Height: {lulc_rows}")
        
        # prepare thresholds for pesticide body burden
        pesticide_threshold_vec_for_dll = xBembidion.PyFloatVec()
        pesticide_threshold_vec_for_dll.extend(np.array([1, 1], dtype=np.float64))
        
        # Create and distribute individuals in the landscape
        for _ in range(self.InitialNumberOfIndividuals):
            x, y = -1, -1
            while not self.habitat_accessible(x, y):
                x = random.randint(0, self.LandscapeWidth-1)
                y = random.randint(0, self.LandscapeHeight-1)

            bembid = TBembidion(x, y, self.LandscapeWidth, self.LandscapeHeight, 5)  # stADULT = 5
            bembid.SetBodyBurdenThreshold(pesticide_threshold_vec_for_dll)
            bembid.x_trace = []
            bembid.x_trace.append(x)
            bembid.y_trace = []
            bembid.y_trace.append(y)
            self.Bembidions.append(bembid)
        logging.info("Initialized with %d individuals.", len(self.Bembidions))

    def add_eggs(self, x, y, number_eggs, raster_vec, exposure_vec, temperature):
        """
        Adding eggs to the Bembidions object.
        Args:
            x, : X/Y coordinates.
            number_eggs: The number of eggs (newly introduced individuals) that shall be laid.
            raster_vec: Pointer to the landscape raster with LULC information the individuals shall be able to access
            exposure_vec: Pointer to the landscape raster with exposure information the individuals shall be able to access
            temperature: Vector with temperature values.
        Returns:
            Nothing.
        """
        
        # prepare thresholds for pesticide body burden
        pesticide_threshold_vec_for_dll = xBembidion.PyFloatVec()
        pesticide_threshold_vec_for_dll.extend(np.array([1, 1], dtype=np.float64))
        
        # produce eggs
        for _ in range(number_eggs):
            bembid = TBembidion(x, y, self.LandscapeWidth, self.LandscapeHeight, 0)  # stEGG = 0
            bembid.UpdateHabitatLandscapeRaster(raster_vec) # provide pointer to landscape
            bembid.UpdatePesticideAmountLandscapeRaster(exposure_vec) # provide pointer to landscape
            bembid.SetBodyBurdenThreshold(pesticide_threshold_vec_for_dll)
            bembid.UpdateJulianDay(int(self.JulianDay))
            bembid.UpdateTemperature(float(temperature.values[self.JulianDay - 1]))
            bembid.UpdateDegreeDays()
            bembid.x_trace = []
            bembid.x_trace.append(x)
            bembid.y_trace = []
            bembid.y_trace.append(y)
            self.Bembidions.append(bembid)
        logging.info("Added %d eggs at location (%d, %d).", number_eggs, x, y)

    def habitat_accessible(self, x, y):
        """
        Check for habitat accessibility.
        Args:
            x, : X/Y coordinates.
        Returns:
            Boolean if habitat can be accessed or not.
        """
        # Check if habitat is accessible
        return 0 <= x < self.LandscapeWidth and 0 <= y < self.LandscapeHeight

    def simulate_day(self, raster_vec, density_vec_for_dll, exposure_vec, temperature):
        """
        Simulate one day.
        Args:
            raster_vec: Pointer to the landscape raster with LULC information the individuals shall be able to access
            density_vec_for_dll: Pointer to the landscape raster with beetle density information the individuals shall be able to access
            exposure_vec: Pointer to the landscape raster with exposure information the individuals shall be able to access
            temperature: Vector with temperature values.
        Returns:
            Nothing.
        """
        
        # Simulate the behavior of all individuals for a day.
        self.JulianDay += 1
        if self.JulianDay > 365:
            self.JulianDay = 1
        print(f"Day: {self.JulianDay} - {len(self.Bembidions)} individuals")
        dead_count = 0
                
        for bembid in self.Bembidions[:]:
            
            # Update environmental conditions
            bembid.UpdateHabitatLandscapeRaster(raster_vec) # provide pointer to landscape
            bembid.UpdateDensityLandscapeRaster(density_vec_for_dll) # provide pointer to landscape
            bembid.UpdatePesticideAmountLandscapeRaster(exposure_vec) # provide pointer to landscape
            bembid.UpdateJulianDay(int(self.JulianDay))
            bembid.UpdateTemperature(float(temperature.values[self.JulianDay - 1]))
            bembid.UpdateDegreeDays()
            
            # Development and reproduction
            bembid.Develop()
            number_eggs = bembid.Reproduce()
            if number_eggs > 0:
                self.add_eggs(bembid.X, bembid.Y, number_eggs, raster_vec, exposure_vec, temperature)
            
            # Apply mortality
            if bembid.Die():
                self.Bembidions.remove(bembid)
                dead_count += 1
            
            # Movement
            #print(f"1 Position X: {bembid.X}, Position Y: {bembid.Y}")
            bembid.Move(14, True)   # 14 is the maximum distance in meter per day (defined in ALMaSS), True is for dispersing
                                    # True has to be replaced lateron (in C++ code to not always disperse)
            bembid.x_trace.append(bembid.X)
            bembid.y_trace.append(bembid.Y)
            #print(f"2 Position X: {bembid.X}, Position Y: {bembid.Y}")
        
        # population size at end of the day
        self.BembidionCount.append(len(self.Bembidions))
        
        logging.info("Day %d complete. Population: %d, Deaths: %d", self.JulianDay, self.get_population_count(), dead_count)

    def get_population_count(self):
        """
        Adding eggs to the Bembidions object.
        Args:
            None.
        Returns:
            Current population size.
        """
        # Return current number of individuals
        return len(self.Bembidions)

    def create_density_map(self, spread_range=3):
        """
        Creating a beetle density map.
        Args:
            spread_range: Range for counting individuals in the proximity (in number of pixels, 1 pixel = 1 m).
        Returns:
            The created density map.
        """
        
        # Create empty density map
        density_map = np.zeros((self.LandscapeHeight, self.LandscapeWidth), dtype=np.int64)
        
        # count bembidions
        for bembid in self.Bembidions:
            if bembid.LifeStage != 5:   # only consider adults (LifeStage=5)
                continue
            
            x, y = int(bembid.X), int(bembid.Y)  # cast to integers

            for dx in range(-spread_range, spread_range + 1):
                for dy in range(-spread_range, spread_range + 1):
                    new_x, new_y = x + dx, y + dy

                    # check for valid position
                    if 0 <= new_x < self.LandscapeWidth and 0 <= new_y < self.LandscapeHeight:
                        density_map[new_y, new_x] += 1  # count the bembidions

        return density_map

    def get_spatial_LULC(self):
        """
        Provides LULC values from rasterized map.
        Args:
            None.
        Returns:
            A flattened raster with LULC values for each pixel (array with row after row).
        """
        # ggf look-up table laden
        # Note: xLandscape can only provide geometries, not rastered landscapes. Hence we read geometries and have to raster ourselves.
        # This can be done with GDAL (not supported by newer python versions) or Shapely (in which the same can be done as below, but in very few lines)
     
        # 1. read geometries and LULC types:
        field_geometries = self.inputs["FieldGeometries"].read()  # list of all geometries
        type_ids = self.inputs["LandUseLandCoverTypes"].read()  # Landuse info. This vector has the length of all geometries (e.g. 10,2,0,0,10,2,...)
                                                   # e.g. field_geometries[0] has LandUseLandCoverTypes[0]
        # read landscape dimensions and cooridinate system:
        extent = self.inputs["Extent"].read().values
        crs = self.inputs["GeometryCrs"].read()  # coordinate system
        # begin rasterizing:
        # a) Preparations:
        spatial_reference = osr.SpatialReference()
        spatial_reference.ImportFromWkt(crs.values)
        ogr_driver = ogr.GetDriverByName("MEMORY")
        ogr_data_set = ogr_driver.CreateDataSource("memory") # create in memory, not in a file
        ogr_layer = ogr_data_set.CreateLayer("filtered", spatial_reference, ogr.wkbPolygon) # create layer. The word "filtered" has no importance here
        ogr_layer.CreateField(ogr.FieldDefn("HabitatType", ogr.OFTInteger)) # add entry "HabitatType"
        ogr_layer_definition = ogr_layer.GetLayerDefn() # needed below
        # iterating through all geometries and set habitat type
        for i in range(len(type_ids.values)):   
            field = ogr.Feature(ogr_layer_definition)
            field.SetGeometry(ogr.CreateGeometryFromWkb(field_geometries.values[i]))
            field.SetField("HabitatType", type_ids.values[i])    # !!!!!!!!!!!!!!! type IDs müssen mit look-up table konvertiert werden
            ogr_layer.CreateFeature(field)  # Note: should also be saved as shape file to degug in QGIS 
        # prepare GeoTiff:
        raster_cols = int(round(extent[1] - extent[0]))
        raster_rows = int(round(extent[3] - extent[2]))
        raster_driver = gdal.GetDriverByName("GTiff")        
        raster_data_set = raster_driver.Create("MEMORY", raster_cols, raster_rows, 1, 2) # 1=Layer Zahl
        raster_data_set.SetGeoTransform((extent[0], 1, 0, extent[3], 0, -1))    # spiegeln/nicht spiegeln (visuell prüfen)
        raster_band = raster_data_set.GetRasterBand(1)  # one band, LULC
        raster_band.SetNoDataValue(99)   # NA value, may need to be adapted for Bembidion (use id for areas where Bembidion can't go) e.g. 65535 (highest value possible)
        raster_data_set.SetProjection(crs.values)
        gdal.RasterizeLayer(raster_data_set, [1], ogr_layer, burn_values=[0], options=["ATTRIBUTE=HabitatType"]) # this rasters the layer
        
        # get layer values
        band = raster_data_set.GetRasterBand(1).ReadAsArray()
 
        # clear memory:
        del raster_data_set
        
        # return flattened raster
        return band        

    def run(self):        
        """
        Run the model.
        Args:
            None.
        Returns:
            Nothing.
        """
               
        #----------- GET GLOBAL VARIABLES ----------#
        # read spatial information
        lulc_band = self.get_spatial_LULC()
        # Get the shape of the LULC band (number rows and cols)
        lulc_rows, lulc_cols = lulc_band.shape
        # flatten raster data band
        lulc_vec = [value for row in lulc_band for value in row]
        self.default_observer.write_message(5, f"LULC raster loaded and prepared.")
        
        # read temperature
        temperature = self.inputs["AirTemperature"].read(select={"time/day": "all"})
        
        # constuct pesticide raster object
        exposure_vec = np.array(lulc_vec) # use LULC values as a basis
        death_zone = int(len(exposure_vec) * 0.2)    # define death zone (a stripe at the top)
        exposure_vec[:death_zone] = 1
        exposure_vec[death_zone:] = 0
        exposure_vec_2 = np.concatenate((exposure_vec, exposure_vec))
        self.default_observer.write_message(5, f"Exposure raster prepared.")
        
        #----------- INITIALISATION ----------#
        # Initialize
        self.default_observer.write_message(5, f"Initialisation started.")
        self.initialize(lulc_rows, lulc_cols)
        self.default_observer.write_message(5, f"Initialisation finished.")
               
        #ctypes.windll.user32.MessageBoxW(0, f"Temperature: {temperature.values}", "Array Value", 1)
        
        #----------- RUN THE MODEL ----------#
        
        # Run the model
        self.run_model(lulc_vec, exposure_vec_2, temperature)
        self.default_observer.write_message(5, f"Simulation finished.")
        
        # create last density map for plotting
        density_map = self.create_density_map()
        
        # Display the raster using matplotlib
        plt.figure(figsize=(10, 8))
        plt.title("Visualization")
        plt.imshow(lulc_band, cmap="gray")
        #plt.imshow(density_map, cmap='hot', alpha=1)  # density map overlay
        plt.colorbar(label="LULC value")
        #plt.colorbar(label="# Individuals")
        plt.xlabel("X Coordinate")
        plt.ylabel("Y Coordinate")
        
        for bembid in self.Bembidions[:]:
            plt.plot(bembid.x_trace, bembid.y_trace, color='red', linewidth=2, label='Walking Route')
        
        #plt.savefig('D:/_Git_/24019_BAY_Bembidion/Test/bembid/analysis/output_plot.png')
        
        self.default_observer.write_message(5, f"Output plot created.")
        
        # set outputs:
        self.outputs["NumberBembids"].set_values(self.BembidionCount)
        self.default_observer.write_message(5, f"Output written.")
        