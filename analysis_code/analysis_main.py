"""Main analysis for the HX2.5 data. Data is in the form of pyEasyTransfer ETDataArray objects"""
import sys
from more_itertools import last
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import os 
from pathlib import Path
import pandas as pd
import matplotlib.pyplot as plt
from collections import defaultdict
import pickle


# # Add the path of the main HX2.5 new microcontroller code to the path so that the pyEasyTransfer module can be imported
# project_dir = Path(__file__).absolute().parent.parent
# sys.path.append(os.path.join(project_dir))
# python_EasyTransfer_dir = os.path.join(project_dir, "python_EasyTransfer")
# sys.path.append(python_EasyTransfer_dir)

from python_EasyTransfer.ETData import ETDataArrays
from python_EasyTransfer.utils import load_ETDataArrays
from analysis_logger import CustomLogger
import analysis_utils as utils





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
            samples_per_second = 25
            # Get the fixed_dt in ms given the samples per second
            fixed_dt_ms = 1000 / samples_per_second
            monitor_time_ms = et_data_arrays['monitor'].time_ms
            driver_time_ms = et_data_arrays['driver'].time_ms
            monitor_last_valid_index = np.where(monitor_time_ms[5:] == 0)[0][0]     # Where there is a 0 in the monitor time_ms array, the first 5 values are always 0 so we skip those
            driver_last_valid_index = np.where(driver_time_ms[5:] == 0)[0][0]       # Where there is a 0 in the driver time_ms array, the first 5 values are always 0 so we skip those
            last_valid_index = max(monitor_last_valid_index, driver_last_valid_index)       # Should be the monitor since driver lags when turning on piezos
            self.log.debug(f"timestamp: {timestamp}, monitor: {monitor_last_valid_index}/{len(monitor_time_ms)}; {monitor_time_ms[monitor_last_valid_index]}, driver: {driver_last_valid_index}/{len(driver_time_ms)}; {driver_time_ms[driver_last_valid_index]} ms, final: {last_valid_index}")
            
            #self.log.debug(f"timestamp: {timestamp}, Last valid index: {last_valid_index}, monitor: {monitor_last_valid_index}, driver: {driver_last_valid_index}")
            # For each data array for the timestamp...
            for name, et_data_array in et_data_arrays.items():
                # Set the last index to the last valid index for the monitor or driver data
                if name == 'monitor':
                    last_index = monitor_last_valid_index
                elif name == 'driver':
                    last_index = driver_last_valid_index
                else:
                    raise ValueError(f"Unknown name: {name}")
                start_index = 1    # Start at 1 to skip the first 0 value   
                # Get the time_ms array and subtract the first value from all values to start at 0
                time_ms: np.ndarray = et_data_array.time_ms
                time_ms = time_ms[start_index:last_index]
                time_ms = time_ms - time_ms[0]
                # Get the last time_ms value and round down to the nearest second
                last_time_ms = round(time_ms[-1], -5)
                # Create a new time_ms array starting at 0 and ending at the last time_ms value with a fixed_dt
                new_time_ms = self._get_new_time_array(0, last_time_ms, fixed_dt_ms)
                
                # For each attribute in the struct...
                column_names = et_data_array.struct_def.keys()
                column_names = [col for col in column_names if 'time' not in col]     # Remove the time_ms column
                num_cols = len(column_names)
                num_rows = last_index - start_index
                column_data = np.full(shape=(num_rows, num_cols), fill_value=np.nan)
                for i in range(num_cols):
                    # Get the attribute name
                    attr = column_names[i]
                    
                    # Get the column data from the et_data_array and add to the column_data array
                    column_names[i] = f'{name}_{attr}'
                    col_data: np.ndarray = getattr(et_data_array, attr)[start_index:last_index]        # Only add the data up to the last valid index
                    column_data[:, i] = col_data     
                
                # Get interpolated data for the new time_ms array      
                new_data = np.full((new_time_ms.shape[0], num_cols), fill_value=np.nan)
                new_column_data = self._interpolate_data(column_data, time_ms, new_data, new_time_ms)
                
                # Add columns to the data_for_dataframe dict
                data_for_dataframe[f'{name}_time_ms'] = new_time_ms
                #self.log.debug(f"new_time_ms shape: {new_time_ms.shape}")
                for i in range(num_cols):
                    #self.log.debug(f"{column_names[i]} shape: {new_column_data[:, i].shape}")
                    data_for_dataframe[column_names[i]] = new_column_data[:, i]

                    
            # Plot the time differences for debugging the algorithms
            # monitor_time_us = data_for_dataframe['monitor_time_us']
            # driver_time_us = data_for_dataframe['driver_time_us']
            # self._plot_time_differences(monitor_time_us, driver_time_us, timestamp, monitor_last_valid_index, driver_last_valid_index)
            
            # Convert the final data dict to a DataFrame and add it to the dictionary.
            df = pd.DataFrame.from_dict(data_for_dataframe)
            
            # Remove the first 5 rows and last 5 rows
            df = df.iloc[5:-5]

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
            
            # Remove the time_us columns
            #df_filtered = df_filtered.drop(columns=[col for col in df_filtered.columns if "time_us" in col])
            
            # Process the sensor data
            df_filtered = self._process_thermistor_data(df_filtered)
            df_filtered = self._process_fluid_flow_data(df_filtered)
            df_filtered = self._process_inlet_fluid_temp_data(df_filtered)
            df_filtered = self._remove_outliers(df_filtered, skip_kws=['time', 'thermistor', 'outlet'])

            # Add the filtered data to the self.data dictionary with a key of "filtered_{timestamp}"
            filtered_data_for_dataframe_dict[f"filtered_{timestamp}"] = df_filtered
            self.log.debug(f"Processed data for {timestamp} combined data")
        return filtered_data_for_dataframe_dict
           
    def analyze_data(self) -> dict[str, pd.DataFrame]:
        """Analyze the data from individual datasets"""
        # Currently don't do anything here
        return self.filtered_data
    
    def _get_new_time_array(self, start_time_ms: float, end_time_ms: float, dt_ms: float) -> np.ndarray:
        """Get the new time array for the interpolated data"""
        self.log.debug(f"Getting new ms time array from {start_time_ms} to {end_time_ms} with dt of {dt_ms}")
        return np.arange(start_time_ms, end_time_ms, dt_ms)
    
    def _interpolate_data(self, column_data: np.ndarray, time: np.ndarray, new_data: np.ndarray, new_time: np.ndarray) -> np.ndarray:
        """Interpolate the data in the column with the dt in the time column equal to the supplied dt"""
        return utils.interpolate_to_fixed_dt_2D(column_data, time, new_data, new_time)
        
    
    def _remove_outliers(self, df: pd.DataFrame, skip_kws: list[str]=['time']) -> pd.DataFrame:
        """Identifies outliers for the data in the dataframe and replaces them with np.nan"""
        for column in df.columns:
            if not any(skip_kw in column for skip_kw in skip_kws):
                # If the column name does not have any of the skip keywords in it, then remove outliers
                df[column] = self._remove_outliers_from_column(df[column], column)
        return df
    
    def _remove_outliers_from_column(self, data: np.ndarray, column_name: str) -> np.ndarray:
        """Identifies outliers for the data and replaces them with np.nan"""
        # Calculate the mean and standard deviation
        mean = np.mean(data)
        std = np.std(data)
        # Replace the outliers with np.nan
        data = np.where(np.abs(data - mean) > 3 * std, np.nan, data)
        # Log the indices where the outliers were removed
        indices = np.argwhere(np.isnan(data)).transpose().tolist()
        self.log.debug(f"Removed {len(indices)} outliers from column '{column_name}'")
        return data

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
            column_data = df[therm_col_name]
            # Filter the data using the moving average filter and the savgol filter
            therm_data_filtered = utils.filter_data(column_data, filter_type='moving_average', filter_params={'window_size': 27})
            #therm_data_filtered = utils.filter_data(therm_data_filtered, filter_type='savgol', filter_params={'window_length': 9, 'polyorder': 3})
            # Add the filtered data to the dataframe
            df[therm_col_name] = therm_data_filtered
        return df
    
    def _process_fluid_flow_data(self, df: pd.DataFrame) -> pd.DataFrame:
        """Process the fluid flow data in the dataframe"""
        # Filter the inlet flow data with the moving average filter since it isn't that noisy
        inlet_flow_name = [col for col in df.columns if 'inlet_flow' in col][0]
        inlet_flow_data = df[inlet_flow_name].to_numpy()
        inlet_flow_data_filtered = utils.filter_data(inlet_flow_data, filter_type='moving_average', filter_params={'window_size': 27})
        df[inlet_flow_name] = inlet_flow_data_filtered
        # Filter the outlet flow data with a savgol filter to remove the noise
        # outlet_flow_name = [col for col in df.columns if 'outlet_flow' in col][0]
        # outlet_flow_data = df[outlet_flow_name].to_numpy()
        # outlet_flow_data_filtered = utils.filter_data(outlet_flow_data, filter_type='savgol', filter_params={'window_length': 7, 'polyorder': 3})
        # df[outlet_flow_name] = outlet_flow_data_filtered
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