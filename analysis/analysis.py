"""Main analysis for the HX2.5 data. Data is in the form of pyEasyTransfer ETDataArray objects"""
from cProfile import label
import sys
from flask import testing
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import os 
import shutil
from pathlib import Path
import pandas as pd
from pandasai import PandasAI
from pandasai.llm.openai import OpenAI
import scipy as sp
import seaborn as sns
import matplotlib.pyplot as plt
import pickle
import math
import analysis_utils as utils

from collections import defaultdict


# Add the path of the main HX2.5 new microcontroller code to the path so that the pyEasyTransfer module can be imported
parent_dir = Path(__file__).parent.parent
sys.path.append(os.path.join(parent_dir))
python_EasyTransfer_dir = os.path.join(parent_dir, "python_EasyTransfer")
sys.path.append(python_EasyTransfer_dir)

from python_EasyTransfer.ETData import ETDataArrays, ETData
from python_EasyTransfer.utils import load_ETDataArrays
from analysis_logger import CustomLogger

class Analysis:
    
    def __init__(self, datasets: dict[str, list[str]], log: CustomLogger) -> None:
        """Initialize the class with the data filepaths and then load the data into a dictionary of ETDataArray objects
        
        Args:
            datasets: dict[str, list[str]]
                A dictionary of the datasets to analyze. The key is the name of the dataset and the value is a list of the filepaths to the data files
            log: CustomLogger
                The logger to use for logging the analysis                
        """
        self.datasets = datasets
        self.log = log
        
        # Set the numpy error handling to ignore divide by zero and invalid values
        np.seterr(divide='ignore', invalid='ignore')

    
    def main(self) -> None:
        """Run the main analysis"""
        # Create a dictionary to store the analyzed datasets in
        self.analyzed_datasets = defaultdict(dict)
        
        # For each dataset in the dictionary...
        for dataset in list(self.datasets.keys()):
            # Get the list of filepaths for the dataset
            self.data_filepaths = self.datasets[dataset]
            self.dataset_name = dataset
            
            # One-shot load the data into a dictionary of ETDataArray objects
            self.data = self.load_data()      # This is potentially a large dictionary of ETDataArray objects
            
            # Combine the driver and monitor dataclasses into one single dataclass with the set attributes and the time starting at 0 with each dataclass time being being synced
            self.combined_data = self.combine_data()
            #print(self.combined_data[list(self.combined_data.keys())[0]].head())        # Print the head of the first 5 rows of the data of the first key 
            
            # Run the pre-processing on the data, this creates filtered data and also calculates the average of the data. This is stored in the self.data dictionary with a key of "fitlered_{filename}"
            self.filtered_data = self.process_data()
            #print(self.combined_data[list(self.combined_data.keys())[0]].head())        # Print the head of the first 5 rows of the data of the first key
            
            # Then we run the analysis to find any potential interesting data points
            self.analyzed_data = self.analyze_data()
            
            # Append the analyzed dataset to the analyzed_datasets dictionary
            self.analyzed_datasets[self.dataset_name] = self.analyzed_data
            
    
    def load_data(self) -> dict[str, ETDataArrays]:
        """Load the data from the filepaths into a dictionary of ETDataArray objects
        
        Args:
            data_filepaths: list[str]
                The list of filepaths to load the data from

        Returns:
            data: dict[str, ETDataArrays]   
                The dictionary of ETDataArrays objects with the keys as the filenames and the values as the ETDataArrays objects
        """
        data = {}
        for filepath in self.data_filepaths:
            # Have to split into directory and filename because the load_ETDataArrays function takes a directory and filename 
            dir = os.path.dirname(filepath)
            filename = os.path.basename(filepath)
            data[filename] = load_ETDataArrays(dir, filename)   
            #self.log.debug(f"Loaded data from {filepath}")
        return data
    
    def combine_data(self) -> dict[str, pd.DataFrame]:
        """Combine the data from the driver and monitor dataclasses into a pandas dataframe"""
        # This will hold our final data.
        data_for_dataframe_dict = {}

        # Group by timestamp.
        grouped_by_timestamp = defaultdict(dict)
        for filename, et_data_array in self.data.items():
            # Extract timestamp from filename.
            timestamp = os.path.splitext(filename)[0].split('_')[-1]
            name = os.path.splitext(filename)[0].split('_')[0]
            grouped_by_timestamp[timestamp][name] = et_data_array       # format: {timestamp: {name: et_data_array, name2: et_data_array}}, example 

        # For each group...
        for timestamp, et_data_arrays in grouped_by_timestamp.items():
            # Merge the data arrays into a single dict.
            data_for_dataframe = {}
            
            # Find the last index of valid data for each array using the time_ms array
            samples_per_second = 20
            max_difference = ((1 / samples_per_second) / 1e3) * 1.1     # ~about the time between samples with 10% margin of error
            monitor_last_valid_index = self._find_last_valid_time_index(et_data_arrays['monitor'].time_ms, max_difference)
            driver_last_valid_index = self._find_last_valid_time_index(et_data_arrays['driver'].time_ms, max_difference)
            last_valid_index = max(monitor_last_valid_index, driver_last_valid_index)       # Should be the monitor since driver lags when turning on piezos
            self.log.debug(f"timestamp: {timestamp}, Last valid index: {last_valid_index}, monitor: {monitor_last_valid_index}, driver: {driver_last_valid_index}")
            # For each data array for the timestamp...
            for name, et_data_array in et_data_arrays.items():
                # Set the last index to the last valid index for the monitor or driver data
                if name == 'monitor':
                    last_index = monitor_last_valid_index
                elif name == 'driver':
                    last_index = driver_last_valid_index
                else:
                    raise ValueError(f"Unknown name: {name}")
                # For each attribute in the struct...
                for attr in et_data_array.struct_def.keys():
                    # Add to the final data, using the attribute name and the filename to create a unique column name.
                    column_name = f'{name}_{attr}'
                    column_data: np.ndarray = getattr(et_data_array, attr)[:last_index]         # Only add the data up to the last valid index
                    final_column_data = self._pad_with_nan(column_data, last_valid_index, column_name)
                    data_for_dataframe[column_name] = final_column_data        
                    #self.log.warning(f"Column name: {column_name}, number of rows in column: {final_column_data.shape[0]}")
                    
            # Plot the time differences for debugging the algorithms
            # monitor_time_us = data_for_dataframe['monitor_time_us']
            # driver_time_us = data_for_dataframe['driver_time_us']
            # self._plot_time_differences(monitor_time_us, driver_time_us, timestamp, monitor_last_valid_index, driver_last_valid_index)
            
            # Convert the final data dict to a DataFrame and add it to the dictionary.
            df = pd.DataFrame.from_dict(data_for_dataframe)
            
            # Remove the first 5 rows
            df = df.iloc[5:]

            # Shift 'time_us' and 'time_ms' columns to start from 0.
            for column in df.columns:
                if 'time' in column:                    # Does both time_us and time_ms
                    df[column] -= df[column].iloc[0]
                    pass

            data_for_dataframe_dict[timestamp] = df
        self.log.info(f"Combined data for '{self.dataset_name}' from same timestamps into a single dataframe, total of {len(data_for_dataframe_dict)} dataframes")
        return data_for_dataframe_dict
      
    def process_data(self) -> dict[str, pd.DataFrame]:
        """Filter and process the data"""
        # Go through each of the dataframes and filter the data
        filtered_data_for_dataframe_dict = {}
        
        for timestamp, df in self.combined_data.items():
            # Create a copy of the dataframe to filter
            df_filtered = df.copy()
            
            # Remove the time_ms columns
            #df_filtered = df_filtered.drop(columns=[col for col in df_filtered.columns if "time_ms" in col])
            
            # Process the sensor data
            df_filtered = self._process_thermistor_data(df_filtered)
            df_filtered = self._process_fluid_flow_data(df_filtered)
            df_filtered = self._process_inlet_fluid_temp_data(df_filtered)

            # Add the filtered data to the self.data dictionary with a key of "filtered_{timestamp}"
            filtered_data_for_dataframe_dict[f"filtered_{timestamp}"] = df_filtered
            self.log.debug(f"Processed data for {timestamp} combined data")
        return filtered_data_for_dataframe_dict
           
    def analyze_data(self) -> dict[str, pd.DataFrame]:
        """Analyze the data from individual datasets"""
        # We want to determine 
        
        # Print the head and tail of the driver_time_us column and the monitor_time_us column and convert us to s
        t_domain = 'us'
        to_seconds = 1e6
        # print(self.filtered_data[list(self.filtered_data.keys())[0]][f'driver_time_{t_domain}'].head() / to_seconds)
        # print(self.filtered_data[list(self.filtered_data.keys())[0]][f'driver_time_{t_domain}'].tail() / to_seconds)
        # print(self.filtered_data[list(self.filtered_data.keys())[0]][f'monitor_time_{t_domain}'].head() / to_seconds)
        # print(self.filtered_data[list(self.filtered_data.keys())[0]][f'monitor_time_{t_domain}'].tail() / to_seconds)
        
        # plot the time differenced
        filtered_data_keys = [key for key in self.filtered_data.keys() if "filtered" in key]
        for key in filtered_data_keys:
            #self._plot_time_differences(self.filtered_data[key])
            pass  
        
        # jsut for now return the filtered data
        return self.filtered_data

    def _plot_time_differences(self, t_monitor: np.ndarray, t_driver: np.ndarray, timestamp: str, li_monitor: int, li_driver: int) -> None:
        # First create the figure
        fig = plt.figure(figsize=(14, 8))
        
        # Get the x axis arrays and then mask the invalid values and convert to seconds
        x1 = np.linspace(0, t_monitor.shape[0], t_monitor.shape[0])
        x2 = np.linspace(0, t_driver.shape[0], t_driver.shape[0])
        t_monitor = np.ma.masked_invalid(t_monitor) / 1e6
        t_driver = np.ma.masked_invalid(t_driver) / 1e6
        
        # trim the x an y arrays to be the last 10% of the total length
        x1 = x1[-int(x1.shape[0] * 0.1):]
        x2 = x2[-int(x2.shape[0] * 0.1):]
        t_monitor = t_monitor[-int(t_monitor.shape[0] * 0.1):]
        t_driver = t_driver[-int(t_driver.shape[0] * 0.1):]
        
        # Plot the data
        sb = fig.add_subplot(1, 1, 1)
        sb.plot(x1, t_monitor, linestyle='-', linewidth=0.5, color='b', label='monitor')
        sb.plot(x2, t_driver, linestyle='-', linewidth=0.5, color='r', label='driver')
        # Draw a line at the last valid index
        sb.axvline(x=li_monitor, linestyle='--', linewidth=0.5, color='b')
        sb.axvline(x=li_driver, linestyle='--', linewidth=0.5, color='r')
        sb.set_title(f'Time of Driver and Monitor: {self.dataset_name}, {timestamp}')
        sb.set_xlabel('Index')
        sb.set_ylabel('Time (s)')
        sb.legend(loc='upper left')      
        
        fig.tight_layout()
        plt.draw()
    
    def _pad_with_nan(self, arr: np.ndarray, target_length: int, arr_name: str="") -> np.ndarray:
        """Pads a numpy array with nan values to the target length"""
        if len(arr) < target_length:
            pad_length = target_length - len(arr)
            return np.pad(arr, (0, pad_length), constant_values=np.nan)
        else:
            #self.log.warning(f"When padding with np.nan, the array is already longer than the target length. Array length: {len(arr)}, Target length: {target_length}, array name: {arr_name}")
            return arr

    def _find_last_valid_time_index(self, time_array: np.ndarray, max_time_diff: float) -> int:
        """Finds the index of the last valid time in the time array, used with np.empty array initialization (stupid method without filling with nan or None)"""
        # Get the approximate average time difference using the first 20 values   
        avg_time_diff = np.average((np.diff(time_array[:20])))
        time_diff = np.abs(np.diff(time_array))
        
        # Get where the time difference is greater than the average time difference
        valid_indices = np.where(time_diff > avg_time_diff*1.1)[0]      # 10% tolerance
        total_number_indices = time_array.size
        
        # We only want indices that are more than 55% of the total number of indices
        valid_indices = valid_indices[valid_indices > (total_number_indices * 0.55)]
        
        # If there are valid indices, return the first one, this should be where the time values are done recording
        if valid_indices.size > 0:
            return valid_indices[0] + 1 # +1 because diff() reduces the array size by 1
        else:
            return time_array.size - 1 # If there are no valid indices, return the last index
         
    def _process_thermistor_data(self, df: pd.DataFrame) -> pd.DataFrame:
        """Process the thermistor data in the dataframe"""
        # Get the column names that have 'thermistor' in them
        thermistor_column_names = [col for col in df.columns if 'thermistor' in col]
        # Thermistors 1-12 are the ones we really care about here, but we will filter all of them
        for therm_col_name in thermistor_column_names:
            # Get the thermistor data column name from the thermistor name list using the therm_number
            column_data = df[therm_col_name].to_numpy()
            # Filter the data using the moving average filter
            therm_data_filtered = utils.filter_data(column_data, filter_type='moving_average', filter_params={'window_size': 7})
            # Add the filtered data to the dataframe
            df[therm_col_name] = therm_data_filtered
        return df
    
    def _process_fluid_flow_data(self, df: pd.DataFrame) -> pd.DataFrame:
        """Process the fluid flow data in the dataframe"""
        # Filter the inlet flow data with the moving average filter since it isn't that noisy
        inlet_flow_name = [col for col in df.columns if 'inlet_flow' in col][0]
        inlet_flow_data = df[inlet_flow_name].to_numpy()
        inlet_flow_data_filtered = utils.filter_data(inlet_flow_data, filter_type='moving_average', filter_params={'window_size': 5})
        df[inlet_flow_name] = inlet_flow_data_filtered
        # Filter the outlet flow data with a savgol filter to remove the noise
        outlet_flow_name = [col for col in df.columns if 'outlet_flow' in col][0]
        outlet_flow_data = df[outlet_flow_name].to_numpy()
        outlet_flow_data_filtered = utils.filter_data(outlet_flow_data, filter_type='savgol', filter_params={'window_length': 7, 'polyorder': 3})
        df[outlet_flow_name] = outlet_flow_data_filtered
        return df
        
    def _process_inlet_fluid_temp_data(self, df: pd.DataFrame) -> pd.DataFrame:
        """Process the inlet fluid temp data in the dataframe"""
        # Filter the inlet fluid temp data with the moving average filter since it isn't that noisy
        inlet_fluid_temp_name = "monitor_inlet_fluid_temp_c"
        inlet_fluid_temp_data = df[inlet_fluid_temp_name].to_numpy()
        inlet_fluid_temp_data_filtered = utils.filter_data(inlet_fluid_temp_data, filter_type='moving_average', filter_params={'window_size': 5})
        df[inlet_fluid_temp_name] = inlet_fluid_temp_data_filtered
        return df
    
    def get_filename(self) -> str:
        """Creates and returns the filename for the analysis object given the datasets that were used to create it"""
        dataset_names = list(self.datasets.keys())
        dataset_names.sort()
        dataset_names_str = "_".join(dataset_names)
        return f"analysis_{dataset_names_str}.pkl"
        
    

class Plotter:
    """Take an Analysis object and plot the data"""
    def __init__(self, analysis: Analysis, save_plot_parent_dir: str, log: CustomLogger, clear_saved: bool=True) -> None:
        """Take an Analysis object and plot the data
        
        Args:
            analysis: Analysis
                The analysis object to plot
            save_plot_parent_dir: str
                The parent directory to save the plots in
            log: CustomLogger
                The logger to use
            clear_saved: bool
                Whether or not to clear the saved plots directory before saving the plots
        """
        # Bring the analysis datasets into scope and logger
        self.datasets = analysis.analyzed_datasets      # Dict of datasets that have been processed and analyzed
        self.log = log
        self.clear_saved = clear_saved

        # Create the directory to save the plots in
        self.save_plot_dir = Path(save_plot_parent_dir, analysis.get_filename())
        self._create_save_directory()
        
    def _optimal_subplot_grid(self, num_subplots: int, max_rows: int, max_cols: int) -> tuple[int, int]:
        """Find the optimal grid arrangement for the given number of subplots and the maximum number of rows and columns"""
        # Check if the desired subplots fit within the constraints
        if num_subplots > max_rows * max_cols:
            raise ValueError(f'Cannot fit {num_subplots} into a grid of {max_rows}x{max_cols}.')
        # Calculate the optimal grid arrangement
        for i in range(1, max_cols + 1):
            if num_subplots % i == 0:
                rows = num_subplots // i
                if rows <= max_rows:
                    return (rows, i)        # Return the optimal grid arrangement in the form (rows, cols)
        raise ValueError(f'Cannot find an optimal grid arrangement for {num_subplots} subplots.')

        
        
    def plot(self) -> None:
        """Plot data in the scope of ALL datasets"""
        # First we will plot the average temperature of the thermistors over time, with a vertical line at the point of when the piezos were turned on
        self._plot_thermistor_data()

    def _plot_thermistor_data(self) -> None:
        """Plot relevant data from the thermistors, such as the average temperature over time, and the temperature difference between the bottom and the top thermistors
        Each plot will have a vertical line at the point of when the piezos were turned on, and each dataset will have an individual plot for each timestamp, and there will be a combined plot for all datasets
        datasets:
            dataset_name (str): dataset (dict[str, pd.DataFrame])
            
        dataset: 
            timestamp (str): df (pd.DataFrame)
        """ 
        #sns.set_theme()
        sns.set_style("ticks", {'grid.color': '.9', 'axes.grid': True})
        for dataset_name, dataset in self.datasets.items():
            # Get the number of subplots we will need and set the rows and columns accordingly
            num_subplots = len(dataset)
            max_num_columns = 3
            max_num_rows = 5
            # Find the optimal grid configuration for the number of subplots and the set max columns and rows
            rows, cols = self._optimal_subplot_grid(num_subplots, max_num_rows, max_num_columns)
            #print(rows, cols)
            # Create figure with subplots for each timestamp in the dataset
            fig, axs = plt.subplots(rows, cols, figsize=(12, 12))
            sp_count = 0    # Subplot count
            fig.suptitle(f"Thermistor Data for {dataset_name}", fontsize=16)
            
            for timestamp, df in dataset.items():
                df: pd.DataFrame = df   # Set the type
                # Calculate the current row and column of the subplot and the axis
                row = sp_count // cols
                col = sp_count % cols
                if rows == 1:
                    ax = axs[col]
                elif cols == 1:
                    ax = axs[row]
                else:
                    ax = axs[row, col]
                
                # Skip the non-filtered data
                if 'filtered' not in timestamp:
                    continue
                
                # Get all of the thermistor data we want to plot over time
                thermistor_names: list[str] = [col for col in df.columns if 'thermistor' in col and '13' not in col and '14' not in col]       # remove the 13 and 14 from the list
                top_thermistor_names = [name for name in thermistor_names if int(name.split("_")[2]) % 2 == 0]   
                btm_thermistor_names = [name for name in thermistor_names if int(name.split("_")[2]) % 2 == 1]
                df.set_index('monitor_time_ms', inplace=True)
                
                # Find the index where the piezo was turned on
                piezo_1_enable_data = df['driver_piezo_1_enable'].to_numpy()
                piezo_on_index = np.where(piezo_1_enable_data == 1)[0][0]
                
                # Plot the data
                x = df.index / 1000     # ms to s
                y = df[thermistor_names]
                # Trim the data to get rid of the first and last 5 points
                x = x[5:-5]
                y = y[5:-5]
                
                # Get the average of the top, bottom, and total thermistors
                y_total_avg = y.mean(axis=1)
                y_top_avg = y[top_thermistor_names].mean(axis=1)
                y_btm_avg = y[btm_thermistor_names].mean(axis=1)
                
                # Create a dataframe of the data with hue and style columns
                df_plot = pd.DataFrame({'Seconds': x, 'Total Avg': y_total_avg, 'Top Avg': y_top_avg, 'Btm Avg': y_btm_avg})
                df_plot.set_index('Seconds', inplace=True)
                
                # Filter the thermistor data even more
                df_filtered = df_plot.apply(lambda x: utils.filter_data(x, 'savgol', {'window_length': 8, 'polyorder': 4}), axis=0)
                df_filtered = df_filtered.apply(lambda x: utils.filter_data(x, 'moving_average', {'window_length': 5}), axis=0)
                df_filtered = df_filtered.apply(lambda x: utils.filter_data(x, 'savgol', {'window_length': 8, 'polyorder': 4}), axis=0)
                df_melted = df_filtered.reset_index().melt(id_vars="Seconds", var_name="Measurements", value_name="Temperature")    
                temp_mean = df_melted['Temperature'].mean()
                temp_3_stdev = df_melted['Temperature'].std(ddof=0) * 3
                y_min = temp_mean - temp_3_stdev
                y_max = temp_mean + temp_3_stdev
                
                # Plot the data
                #sns.lineplot(data=df_plot, ax=ax, legend='full')
                sns.lineplot(data=df_melted, x="Seconds", y="Temperature", hue="Measurements", ax=ax, errorbar=('ci', 60), err_style='band', legend='brief')
                ax.axvline(x=x[piezo_on_index], color='k', linestyle='--', label='Piezo On')
                ax.set_xlim(x[5], x[-5])
                ax.set_ylim(y_min, y_max)
                ax.legend(loc='upper left')
                ax.set_xlabel('Time (seconds)')
                ax.set_ylabel('Temperature (C)')
                ax.set_title(f"Thermistor Data for {dataset_name} at {timestamp.replace('filtered', '')}")
                sp_count += 1
                
            fig.tight_layout()
            plt.savefig(Path(self.save_plot_dir, f"thermistor_data_{dataset_name}.png"), dpi=600)
            plt.draw()

        
    def _create_save_directory(self) -> None:
        """Create the directory to save the plots in given the current analysis being plotted"""
        if not os.path.isdir(self.save_plot_dir):
            os.mkdir(self.save_plot_dir)
            self.log.debug(f"Created directory for plots: '{self.save_plot_dir}'")
        else:
            if self.clear_saved:
                shutil.rmtree(self.save_plot_dir)       # Delete the directory and all its contents
                os.mkdir(self.save_plot_dir)            # Create the directory again
                self.log.debug(f"Cleared directory for plots: '{self.save_plot_dir}'")



def save_analysis(analysis: Analysis, analysis_save_dir: str, log: CustomLogger) -> str:
    """Use pickle to save the Analysis object to a pickle file"""
    # Confirm the save directory exists
    if not os.path.isdir(analysis_save_dir):
        raise FileNotFoundError(f"Save directory '{analysis_save_dir}' does not exist")
    
    # Get the filename using the dataset names and create the filepath
    filename = analysis.get_filename()
    file_path = Path(analysis_save_dir, filename)
    
    # Save the analysis object to the pickle file
    with open(file_path, 'wb') as f:
        pickle.dump(analysis, f)
        
    # Log the save
    log.info(f"Saved analysis to '{file_path}'")
    
    return filename
    
def load_analysis(analysis_save_dir: str, analysis_filename: str, log: CustomLogger) -> Analysis:
    """Load the Analysis object from the pickle file"""
    # Create the file path
    file_path = Path(analysis_save_dir, analysis_filename)
    
    # Confirm that the file exists
    if not os.path.isfile(file_path):
        raise FileNotFoundError(f"Analysis file '{file_path}' does not exist")
    
    # Load the analysis object
    with open(file_path, 'rb') as f:
        analysis = pickle.load(f)
    
    # Log the load
    log.info(f"Loaded analysis from '{file_path}'")
    return analysis


if __name__ == "__main__":
    # Get the logger
    log_dir = os.path.join(parent_dir, "analysis", "logs")
    logger = CustomLogger(log_dir=log_dir)
    
    # Main directories
    parent_dir = Path(__file__).parent.parent
    data_folder_name = "interest_data"
    
    # First dataset
    dataset_1_name = "single_vertical_piezo_10mm_wall_pyramids"
    data_folder_path = os.path.join(parent_dir, data_folder_name, dataset_1_name)
    dataset_1_filepaths = [os.path.join(data_folder_path, file) for file in os.listdir(data_folder_path) if file.endswith(".pkl")]
    
    # second dataset
    dataset_2_name = "single_vertical_piezo_15mm_wall"
    data_folder_path = os.path.join(parent_dir, data_folder_name, dataset_2_name)
    dataset_2_filepaths = [os.path.join(data_folder_path, file) for file in os.listdir(data_folder_path) if file.endswith(".pkl")]
    
    # Create datasets dictionary
    datasets = {dataset_1_name: dataset_1_filepaths,
                dataset_2_name: dataset_2_filepaths}
    
    # Create the save directory for analysis and plotter
    saved_analysis_dir = os.path.join(parent_dir, "analysis", "saved_analysis")
    saved_plotting_dir = os.path.join(parent_dir, "analysis", "saved_plots")
    
    
    # Run the analysis
    # analysis = Analysis(datasets=datasets, log=logger.log)
    # analysis.main()
    # save_filename = save_analysis(analysis=analysis, analysis_save_dir=saved_analysis_dir, log=logger.log)
    # print(save_filename)
    
    # Load the analysis
    analysis_filename = "analysis_single_vertical_piezo_10mm_wall_pyramids_single_vertical_piezo_15mm_wall.pkl"
    analysis = load_analysis(analysis_save_dir=saved_analysis_dir, analysis_filename=analysis_filename, log=logger.log)
    plotter = Plotter(analysis=analysis, save_plot_parent_dir=saved_plotting_dir, log=logger.log)
    plotter.plot()
    
    plt.show()