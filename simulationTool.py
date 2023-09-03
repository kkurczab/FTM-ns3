import os
import numpy as np
import matplotlib.pyplot as plt
import random

# Initialize dictionaries to store results - for Graphs
mean_rtt_results = {}
mean_signal_strength_results = {}
num_measurements_results = {}

# Define the CLI command template
cli_command = './waf --run "ftm-example.cc {params}"'

# Define the range of parameters
parameters = {
    # "numberOfBurstsExponent": range(1, 5),
            # "burstDuration": range(6, 12),
    # "minDeltaFtm": range(0, 27),
            # "ftmsPerBurst": range(0, 8),
    # "frequency": range(0, 2),
    # "rxGain": range(0, 2),
    # "propagationLossModel": range(0, 2),
    "numberOfStations": (2**p for p in range(0, 1)),
    # "channelBandwidth": [20, 40, 80, 160],
    # "distance": range(5, 26, 5)
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

# Loop over parameter combinations
for combination in param_combinations:
    mean_rtt_values = []
    mean_signal_strength_values = []
    num_measurements_values = []

    for times in range(1, 4): # number of runs
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
    
        print(f"Run {times} with {output_file_name} completed.")
       
        # ----------For the graph creation----------
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
                mean_rtt = float(line.split(":")[1].strip())
                if mean_rtt is not None:
                    mean_rtt_values.append(mean_rtt)
            elif "Mean Signal Strength [dBm]:" in line:
                mean_signal_strength = float(line.split(":")[1].strip())
                if mean_signal_strength is not None:
                    mean_signal_strength_values.append(mean_signal_strength)
            elif "Number of Measurements:" in line:
                num_measurements = int(line.split(":")[1].strip())
                if num_measurements is not None:
                    num_measurements_values.append(num_measurements)

    # Calculate averages and standard deviations
    mean_rtt_avg = np.mean(mean_rtt_values)
    mean_rtt_std = np.std(mean_rtt_values)
    mean_signal_strength_avg = np.mean(mean_signal_strength_values)
    mean_signal_strength_std = np.std(mean_signal_strength_values)
    num_measurements_avg = np.mean(num_measurements_values)
    num_measurements_std = np.std(num_measurements_values)
    
    # Store results
    combination_str = "_".join([f"{param_name}={param_value}" for param_name, param_value in combination])
    mean_rtt_results[combination_str] = {'mean': mean_rtt_avg, 'std': mean_rtt_std}
    mean_signal_strength_results[combination_str] = {'mean': mean_signal_strength_avg, 'std': mean_signal_strength_std}
    num_measurements_results[combination_str] = {'mean': num_measurements_avg, 'std': num_measurements_std}


# Process the results and create line graphs
for result_dict, ylabel in zip([mean_rtt_results, mean_signal_strength_results, num_measurements_results],
                                ['Mean RTT [ps]', 'Mean Signal Strength [dBm]', 'Number of Measurements']):
    plt.figure(figsize=(10, 6))
    param_combinations = list(result_dict.keys())

    for param_combination, values in result_dict.items():
        mean_value = values['mean']
        std_value = values['std']
        
        x_values = []
        y_values = []
        for param in parameters:
            if param in param_combination:
                param_value = int(param_combination.split(f"{param}=")[1].split("_")[0])
                x_values.append(param_value)
                y_values.append(mean_value)
        
        # Exclude values of 0 when calculating the mean
        y_values_filtered = [value for value in y_values if value != 0]

        # Calculate the mean of non-zero values
        if len(y_values_filtered) > 0:
            mean_value_filtered = np.mean(y_values_filtered)
        else:
            mean_value_filtered = None     

        plt.errorbar(x_values, y_values, yerr=std_value, marker='o', label=param_combination, linestyle='-', linewidth=2)
    
    # Add a constant line at a specific Y-axis position
    if ylabel == 'Mean RTT [ps]':
        real_distance = 1
        plt.axhline(y=(real_distance * 2 / 0.0003), color='r', linestyle='--', label=f'RTT of the Real distance {real_distance} [m]')

    plt.xlabel("Number of Stations")
    plt.ylabel(ylabel)
    plt.legend()
    plt.grid()
    plt.savefig(os.path.join(output_dir_base, ylabel.replace(" ", "_") + "_line_graph_with_std.png"))
    plt.show()
