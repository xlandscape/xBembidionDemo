import random
import logging
from xBembidion import TBembidion
import csv

# Configure logging
logging.basicConfig(
    filename="simulation_log.txt",
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s"
)

class TBembidionModel:
    def __init__(self, landscape_min_x, landscape_max_x, landscape_min_y, landscape_max_y, number_individuals, simulation_days):
        # Initialize global variables
        self.InitialNumberOfIndividuals = number_individuals
        self.LandscapeMinX = landscape_min_x
        self.LandscapeMaxX = landscape_max_x
        self.LandscapeMinY = landscape_min_y
        self.LandscapeMaxY = landscape_max_y
        self.SimulationDays = simulation_days
        self.RandomWalkTarget = 0
        self.JulianDay = 0
        self.Bembidions = []
        logging.info("Model initialized with %d individuals for %d days.", number_individuals, simulation_days)

    def run_model(self, temperature):
        # Run the simulation
        self.initialize()
        for day in range(self.SimulationDays):
            logging.info("Day %d: Population count: %d", day + 1, self.get_population_count())
            self.simulate_day(temperature)

    def initialize(self):
        # Create and distribute individuals in the landscape
        for _ in range(self.InitialNumberOfIndividuals):
            x, y = -1, -1
            while not self.habitat_accessible(x, y):
                x = random.randint(self.LandscapeMinX, self.LandscapeMaxX)
                y = random.randint(self.LandscapeMinY, self.LandscapeMaxY)

            bembid = TBembidion(x, y, self.LandscapeMinX, self.LandscapeMaxX, self.LandscapeMinY, self.LandscapeMaxY, 5)  # stADULT = 5
            self.Bembidions.append(bembid)
        logging.info("Initialized with %d individuals.", len(self.Bembidions))

    def add_eggs(self, x, y, number_eggs):
        for _ in range(number_eggs):
            bembid = TBembidion(x, y, self.LandscapeMinX, self.LandscapeMaxX, self.LandscapeMinY, self.LandscapeMaxY, 0)  # stEGG = 0
            self.Bembidions.append(bembid)
        logging.info("Added %d eggs at location (%d, %d).", number_eggs, x, y)

    def habitat_accessible(self, x, y):
        # Check if habitat is accessible
        return self.LandscapeMinX <= x <= self.LandscapeMaxX and self.LandscapeMinY <= y <= self.LandscapeMaxY

    def simulate_day(self, temperature):
        # Simulate the behavior of all individuals for a day.
        self.JulianDay += 1
        dead_count = 0
        for bembid in self.Bembidions[:]:  # Use a copy to safely remove items
            bembid.UpdateTemperature(temperature[self.JulianDay - 1])
            bembid.UpdateDegreeDays()
            bembid.Develop()
            number_eggs = bembid.Reproduce()
            if number_eggs > 0:
                self.add_eggs(bembid.X, bembid.Y, number_eggs)
            if bembid.Die():
                self.Bembidions.remove(bembid)
                dead_count += 1
        logging.info("Day %d complete. Population: %d, Deaths: %d", self.JulianDay, self.get_population_count(), dead_count)

    def get_population_count(self):
        # Return current number of individuals
        return len(self.Bembidions)


# Example usage
if __name__ == "__main__":
    weather_file_name = 'weather_mars-97100.csv'
    temperature_col_index = 6
    temperature = []
    
    # Open and read the CSV file
    with open(weather_file_name, mode='r') as file:
        reader = csv.reader(file)
        next(reader)  # Skip header
        for row in reader:
            temperature.append(float(row[temperature_col_index]))

    # Initialize model parameters
    landscape_min_x = 0
    landscape_max_x = 10000
    landscape_min_y = 0
    landscape_max_y = 10000
    number_bembidions = 10
    simulation_days = 365

    # Create and run the model
    model = TBembidionModel(landscape_min_x, landscape_max_x, landscape_min_y, landscape_max_y, number_bembidions, simulation_days)
    model.run_model(temperature)

    logging.info("Simulation finished!")
    print("\nSimulation complete! Check the log file for details.")
