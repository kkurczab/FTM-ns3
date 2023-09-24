import os
import numpy as np
import matplotlib.pyplot as plt
import random
import csv

# Initialize dictionaries to store results - for Graphs
mean_rtt_results = {}
mean_signal_strength_results = {}
num_measurements_results = {}

# Define the CLI command template
cli_command = './waf --run "ftm-example.cc {params}"'

# Default values
# numberOfBurstsExponent = 1
# burstDuration = 10
# minDeltaFtm = 1
# asap = True
# ftmsPerBurst = 1
# burstPeriod = 10
# frequency = 0
# numberOfStations = 1
# propagationLossModel = 0
# channelBandwidth = 20
# distance = 5

currentSimulationNumber = 0

# Define the range of parameters
parameters = {
    # "numberOfBurstsExponent": [1, 2, 3, 4],
    # "burstDuration": [5, 11], # min 5, max 11
    # "minDeltaFtm": [5, 10, 320, 640],
    # "asap": [0, 1], # 0, 1
    # "ftmsPerBurst": [1, 2, 3],
    # "burstPeriod": [1, 10],
    # "frequency": [0, 1],
    "numberOfStations": (2**p for p in range(0, 8)),
    # "propagationLossModel": [0, 1],
    # "channelBandwidth": [20, 40],
    "distance": [5, 30]
}
# Create a directory to save outputs (delete the old outputs)
output_dir_base = "outputs"

if os.path.exists(output_dir_base):
    import shutil
    shutil.rmtree(output_dir_base)

os.makedirs(output_dir_base, exist_ok=True)

# Generate all possible parameter combinations
param_combinations = [[]]  # Start with an empty combination
for param_name, param_values in parameters.items():
    new_combinations = []
    for param_value in param_values:
        for combo in param_combinations:
            new_combinations.append(combo + [(param_name, param_value)])
    param_combinations = new_combinations

# Create a CSV file to store the results
csv_file_path = "simulation_results.csv"

# Open the CSV file for writing
with open(csv_file_path, mode='w', newline='') as csv_file:
    csv_writer = csv.writer(csv_file)
    
    # Write the header row to the CSV file
    csv_writer.writerow(['numberOfStations', 'distance', 'measurementError', 'numberOfBurstsExponent', 'burstDuration', 
                        'minDeltaFtm', 'asap', 'ftmsPerBurst', 'burstPeriod', 'frequency', 
                        'propagationLossModel', 'channelBandwidth', 'RTT', 'stdDev', 'meanSignalStrength', 'stdDevSS', 'numMeasurements', 'stdDevnumM'])

# Loop over parameter combinations
    for combination in param_combinations:      
        # Check conditions to skip certain combinations
        combination_dict = dict(combination)
        if ((combination_dict.get("burstDuration") == 5 and combination_dict.get("minDeltaFtm") in [320, 640]) or
            (combination_dict.get("burstDuration") == 5 and combination_dict.get("ftmsPerBurst") in [2, 3] and combination_dict.get("minDeltaFtm") == 10) or
            (combination_dict.get("burstDuration") == 11 and combination_dict.get("minDeltaFtm") in [5, 10]) or
            (combination_dict.get("burstDuration") == 11 and combination_dict.get("ftmsPerBurst") in [2, 3] and combination_dict.get("minDeltaFtm") == 640)):
            continue

        mean_rtt_values = []
        mean_signal_strength_values = []
        num_measurements_values = []

        number_of_runs = 5
        for times in range(1, number_of_runs + 1): # number of runs
            seed = random.randint(100000, 999999)
    
            # Create a directory for this run
            output_file_name = "_".join([f"{param_name}={param_value}" for param_name, param_value in combination])
            output_dir = os.path.join(output_dir_base, output_file_name)
            os.makedirs(output_dir, exist_ok=True)
        
            run_dir = os.path.join(output_dir, str(times))  # Create subdirectory "1", "2", etc.
            os.makedirs(run_dir, exist_ok=True)
    
            params_str = " ".join([f"--{param_name}={param_value} --pcapPath={run_dir}/{times} --RngRun={seed}" for param_name, param_value in combination])
            formatted_command = cli_command.format(params=params_str)
        
            # Run the command and capture the output
            output = os.popen(formatted_command).read()
        
            # Save the output to a file in the run's directory
            output_file = os.path.join(run_dir, "output.txt")
            with open(output_file, "w") as f:
                f.write(output)
        
            # print(f"Run {times} with {output_file_name} completed.")
            currentSimulationNumber += 1
            print(f"Current simulation: {currentSimulationNumber}/{(len(param_combinations) * number_of_runs)}")
           
            # Collect results from the output.txt file
            output_file_path = os.path.join(run_dir, "output.txt")
            with open(output_file_path, "r") as f:
                lines = f.readlines()
    
            # Initialize variables to store parsed values
            mean_rtt = None
            mean_signal_strength = None
            num_measurements = None
    
            # Iterate through the lines to find the relevant values
            for line in lines:
                if "Mean RTT [ps]:" in line:
                    mean_rtt = int(line.split(":")[1].strip())
                    # if mean_rtt != 0:
                    mean_rtt_values.append(mean_rtt)
                elif "Mean Signal Strength [dBm]:" in line:
                    mean_signal_strength = float(line.split(":")[1].strip())
                    # if mean_signal_strength != 0.0:
                    mean_signal_strength_values.append(mean_signal_strength)
                elif "Number of Measurements:" in line:
                    num_measurements = int(line.split(":")[1].strip())
                    # if num_measurements != 0:
                    num_measurements_values.append(num_measurements)
    
        # Calculate averages and standard deviations
        mean_rtt_avg = np.mean(mean_rtt_values)
        mean_rtt_std = np.std(mean_rtt_values)
        mean_signal_strength_avg = np.mean(mean_signal_strength_values)
        mean_signal_strength_std = np.std(mean_signal_strength_values)
        num_measurements_avg = np.mean(num_measurements_values)
        num_measurements_std = np.std(num_measurements_values)
                
        if mean_rtt_avg == 0:
            continue

        # Write the results to the CSV file
        numberOfStations_csv = None
        distance_csv = None
        numberOfBurstsExponent_csv = None
        burstDuration_csv = None
        minDeltaFtm_csv = None
        asap_csv = None
        ftmsPerBurst_csv = None
        burstPeriod_csv = None
        frequency_csv = None
        propagationLossModel_csv = None
        channelBandwidth_csv = None

        for param_name, param_value in combination:
            if param_name == 'numberOfStations':
                numberOfStations_csv = param_value
            elif param_name == 'distance':
                distance_csv = param_value
                if mean_rtt_avg != 0:
                    measurementError = round(abs((mean_rtt_avg * 0.0003 / 2) - (distance_csv)), 2) # in meters
                else:
                    measurementError = distance_csv
            elif param_name == 'numberOfBurstsExponent':
                numberOfBurstsExponent_csv = param_value
            elif param_name == 'burstDuration':
                burstDuration_csv = param_value
            elif param_name == 'minDeltaFtm':
                minDeltaFtm_csv = param_value
            elif param_name == 'asap':
                asap_csv = param_value
            elif param_name == 'ftmsPerBurst':
                ftmsPerBurst_csv = param_value
            elif param_name == 'burstPeriod':
                burstPeriod_csv = param_value
            elif param_name == 'frequency':
                frequency_csv = param_value
            elif param_name == 'propagationLossModel':
                propagationLossModel_csv = param_value
            elif param_name == 'channelBandwidth':
                channelBandwidth_csv = param_value
    
        csv_writer.writerow([numberOfStations_csv, distance_csv, measurementError, numberOfBurstsExponent_csv, burstDuration_csv, minDeltaFtm_csv, asap_csv, ftmsPerBurst_csv, burstPeriod_csv, frequency_csv, propagationLossModel_csv, channelBandwidth_csv, int(mean_rtt_avg), int(mean_rtt_std), round(mean_signal_strength_avg, 1), round(mean_signal_strength_std, 1), int(num_measurements_avg), int(num_measurements_std)])

        # if os.path.exists(output_dir_base):
        #     import shutil
        #     shutil.rmtree(output_dir_base)

        # os.makedirs(output_dir_base, exist_ok=True)

        # Store results
        combination_str = "_".join([f"{param_name}={param_value}" for param_name, param_value in combination])
        mean_rtt_results[combination_str] = {'mean': mean_rtt_avg, 'std': mean_rtt_std}
        mean_signal_strength_results[combination_str] = {'mean': mean_signal_strength_avg, 'std': mean_signal_strength_std}
        num_measurements_results[combination_str] = {'mean': num_measurements_avg, 'std': num_measurements_std}    

# GRAPH CREATION - a small modification/fix could be necessary, due to the CSV creation process above
# # Specify the parameters to include in the plot
# y_axis_parameters = ['Mean RTT [ps]']
# # , 'Mean Signal Strength [dBm]', 'Number of Measurements']

# # Filter the result dictionaries based on the specified parameters
# result_dicts_to_plot = [mean_rtt_results, mean_signal_strength_results, num_measurements_results]

# for param_name, param_values in parameters.items():
#     if param_name == 'numberOfStations':
#         continue  # Skip 'numberOfStations' parameter
#     for result_dict, ylabel in zip(result_dicts_to_plot, y_axis_parameters):
#     # Process the results and create line graphs
#     # for result_dict, ylabel in zip([mean_rtt_results, mean_signal_strength_results, num_measurements_results],
#                                     # ['Mean RTT [ps]']):
#                                     #, 'Mean Signal Strength [dBm]', 'Number of Measurements']):
#         plt.figure(figsize=(10, 6))
#         # param_combinations = list(result_dict.keys())
    
#         for param_combination, values in result_dict.items():
#             mean_value = values['mean']
#             std_value = values['std']
            
#             x_values = []
#             y_values = []
#             # for param in parameters:
#             #     if param in param_combination:
#             param_name = 'numberOfStations'
#             if param_name in param_combination:
#                 param_value = int(param_combination.split(f"{param_name}=")[1].split("_")[0])
#                 x_values.append(param_value)
#                 y_values.append(mean_value)
            
#             # Exclude values of 0 when calculating the mean
#             y_values_filtered = [value for value in y_values if value != 0]
    
#             # Calculate the mean of non-zero values
#             if len(y_values_filtered) > 0:
#                 mean_value_filtered = np.mean(y_values_filtered)
#             else:
#                 mean_value_filtered = None     
            
#             # If mean_value_filtered is not None, plot it; otherwise, ignore
#             if mean_value_filtered is not None:
#                 plt.errorbar(x_values, y_values, yerr=std_value, marker='o', label=param_combination, linestyle='-', linewidth=2)
        
#         # Add a constant line at a specific Y-axis position
#         if ylabel == 'Mean RTT [ps]':
#             plt.axhline(y=(distance * 2 / 0.0003), color='r', linestyle='--', label=f'RTT of the Real distance {distance} [m]')
    
#         plt.xlabel("Number of Stations")
#         plt.ylabel(ylabel)
#         plt.legend()
#         plt.grid()
#         plt.savefig(os.path.join(output_dir_base, ylabel.replace(" ", "_") + "_graph.png"))
#         # plt.show()
