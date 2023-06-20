from pathlib import Path
from matplotlib import legend
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as tck
import pandas as pd
import seaborn as sns
import shutil
import os

from analysis_logger import CustomLogger
from analysis_main import Analysis
import analysis_utils as utils


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
        thermistor_plot_data = self._plot_thermistor_data()
        
        # Next we will plot the flow data, such as the inlet flow rate over time, and the differential flow rate of the outlet flow sensor over time
        flow_plot_data = self._plot_flow_data()
        
        # We will then plot the thermistor and flow data for each dataset individually on the same figure
        self._plot_timestamp_data(thermistor_plot_data, flow_plot_data)
        
    def _plot_timestamp_data(self, thermistor_plot_data: dict[str, pd.DataFrame], flow_plot_data: dict[str, pd.DataFrame]) -> None:
        """Plot the thermistor and flow data for each dataset individually on the same figure"""
        sns.set_style("ticks", {'grid.color': '.9', 'axes.grid': True})
        plot_data = {}
        
        
        # Create plots for each timestamp
        for dataset_name, dataset in self.datasets.items():
            for timestamp, _ in dataset.items():
                timestamp: str
                t_df = thermistor_plot_data[timestamp]#.reset_index(drop=False).rename(columns={'Seconds': 't Seconds'})
                f_df = flow_plot_data[timestamp]#.reset_index(drop=False).rename(columns={'Seconds': 'f Seconds'})
                df_plot = pd.concat([t_df, f_df], axis=1)
                #print(df_plot)
                plot_data[timestamp] = df_plot
                #df_plot: pd.DataFrame = plot_data[timestamp]
                
                # Create the figure
                n_rows = 3
                n_cols = 1
                ratios = [1/n_rows] * n_rows        # Equal height subplots
                grid = dict(hspace=0.0, height_ratios=ratios)
                fig, axs = plt.subplots(nrows=n_rows, ncols=n_cols, figsize=(12, 12), sharex=True, gridspec_kw=grid)
                fig.suptitle(f'{dataset_name} - {timestamp}', fontsize=16)
                
                # Plot the thermistor data
                # Find the limits of the y axis using min and max of the data              
                y_min = df_plot['Total Avg'].min() * 0.98
                y_max = df_plot['Total Avg'].max() * 1.02
                
                # Set the x lims to be data between the first and last points
                x_min = df_plot.index[0]
                x_max = df_plot.index[-1]
                
                legend_font_size = 8
                ax: plt.Axes = axs[0]
                
                # Plot the data with error bands for the top and bottom thermistors
                top_line,   = ax.plot(df_plot.index, df_plot['Top Avg'], color='b', linestyle='-')
                top_fill    = ax.fill_between(df_plot.index, df_plot['Top Avg'] - df_plot['Top Std'], df_plot['Top Avg'] + df_plot['Top Std'], color='b', alpha=0.2)
                btm_line,   = ax.plot(df_plot.index, df_plot['Btm Avg'], color='r', linestyle='-')
                btm_fill    = ax.fill_between(df_plot.index, df_plot['Btm Avg'] - df_plot['Btm Std'], df_plot['Btm Avg'] + df_plot['Btm Std'], color='r', alpha=0.2)
                total_avg,  = ax.plot(df_plot.index, df_plot['Total Avg'], color='g', linestyle='-')
                piezo_on    = ax.axvline(x=self.piezo_on_time_sec, color='k', linestyle='--')
                # Set limits, axes labels, title, and the legend for the plot
                ax.set_xlim(x_min, x_max)
                #ax.set_ylim(y_min, y_max)
                ax.legend([(top_line, top_fill), (btm_line, btm_fill), (total_avg), (piezo_on)], ['Top Thermistors', 'Btm Thermistors', 'Avg Thermistors', 'Piezo Turn On'], loc='upper right', prop={'size': legend_font_size})
                ax.set_ylabel('Temperature (C)')
                #ax.set_title(f"Thermistor and Flow Data at {timestamp.replace('filtered', '').replace('_', '')}")
                
                # Plot the flow data
                # Find the limits of the y axis using min and max of the data              
                y_min_inlet = df_plot['Inlet Raw'].min() * 0.98
                y_max_inlet = df_plot['Inlet Raw'].max() * 1.02
                y_min_outlet = df_plot['Outlet Cumm'].min() * 0.98
                y_max_outlet = df_plot['Outlet Cumm'].max() * 1.02
                self.log.debug(f"y max outlet: {y_max_outlet}, y min outlet: {y_min_outlet}")
                
                # Set the x lims to be data between the first and last points
                x_min = df_plot.index[0]
                x_max = df_plot.index[-1]
                self.log.debug(f"x min: {x_min}, x max: {x_max}")
                
                legend_font_size = 8
                ax_inlet: plt.Axes = axs[1]
                ax_outlet: plt.Axes = axs[2]
                
                # Plot the data for inlet flow rate
                ax_inlet.plot(df_plot.index, df_plot['Inlet Raw'], color='b', linestyle='-', label='Inlet Flow')
                ax_inlet.axhline(0, color='k', alpha=0.7, linestyle='--', linewidth=0.6)
                ax_inlet.axvline(x=self.piezo_on_time_sec, color='k', linestyle='--', label='Piezo On')
                ax_inlet.set_xlim(x_min, x_max)
                #ax_inlet.set_ylim(y_min_inlet, y_max_inlet)
                ax_inlet.legend(loc='upper right', prop={'size': legend_font_size}) 
                ax_inlet.set_ylabel('Flow Rate (units)')
                
                # Plot the data for outlet flow rate
                ax_outlet.plot(df_plot.index, df_plot['Outlet Cumm'], color='r', linestyle='-', label='Cummulative Differential\nOutlet Flow')
                ax_outlet.axhline(0, color='k', alpha=0.7, linestyle='--', linewidth=0.6)
                #ax_outlet.plot(df_plot.index, df_plot['Outlet Raw'], color='g', linestyle='-', label='Raw Outlet Flow')
                ax_outlet.axvline(x=self.piezo_on_time_sec, color='k', linestyle='--', label='Piezo On')
                ax_outlet.set_xlim(x_min, x_max)
                #ax_outlet.set_ylim(y_min_outlet, y_max_outlet)
                ax_outlet.legend(loc='upper right', prop={'size': legend_font_size}) 
                ax_outlet.set_xlabel('Time (seconds)')
                ax_outlet.set_ylabel('Cummulative Flow\nRate (units)')
                    
                # Add minor ticks to the x axis
                for axis in axs.reshape(-1):
                    axis.xaxis.set_minor_locator(tck.AutoMinorLocator())  
                    
                fig.tight_layout()
                plt.savefig(Path(self.save_plot_dir, f"{timestamp}_data.png"), dpi=600)
                plt.draw()
            
                
        
        
    def _plot_flow_data(self) -> dict[str, pd.DataFrame]:
        """Plot relevant data from the flow sensors, such as the inlet flow rate over time, and the differential flow rate of the outlet flow sensor over time
        Each plot will have a vertical line at the point of when the piezos were turned on, and each dataset will have an individual plot for each timestamp, and there will be a combined plot for all datasets
        datasets:
            dataset_name (str): dataset (dict[str, pd.DataFrame])
        
        dataset:
            timestamp (str): df (pd.DataFrame)       
        """
        sns.set_style("ticks", {'grid.color': '.9', 'axes.grid': True})
        for dataset_name, dataset in self.datasets.items():
            # Get the number of subplots we will need and set the rows and columns accordingly
            num_subplots = len(dataset)
            max_num_columns = 3
            max_num_rows = 5
            # Find the optimal grid configuration for the number of subplots and the set max columns and rows
            rows, cols = self._optimal_subplot_grid(num_subplots, max_num_rows, max_num_columns)
            #print(rows, cols)
            # Create the subplot height grid
            h_empty = 0.2                # Height of the empty subplot
            h_inlet = (1-h_empty)/2      # Height of the inlet flow subplot
            h_outlet = (1-h_empty)/2     # Height of the outlet flow subplot
            n_rows = rows * 3            # Number of rows in the grid
            h_ratios = [h_inlet/n_rows, h_outlet/n_rows, h_empty/n_rows] * rows
            grid = dict(hspace=0.0, height_ratios=h_ratios)
            
            # Create figure with subplots for each timestamp in the dataset
            fig, axs = plt.subplots(nrows=n_rows, ncols=cols, figsize=(12, 12), gridspec_kw=grid)     # we want the inlet flow on the top and the outlet flow on the bottom per timestamp, and an invisible plot hence multiply rows by 3
            sp_count = 0    # Subplot count
            fig.suptitle(f"Flow Data for '{dataset_name.replace('_', ' ')}' Dataset", fontsize=16)
            
            plot_data = {}      # Dictionary to store the data to be plotted
            
            for timestamp, df in dataset.items():
                timestamp: str
                df: pd.DataFrame
                
                # Calculate the current row and column of the subplot and the axis
                row = (sp_count // (cols)) * 3
                col = sp_count % (cols)
                
                ax_inlet: plt.Axes
                ax_outlet: plt.Axes
                ax_empty: plt.Axes
                
                if cols == 1: 
                    ax_inlet = axs[row]
                    ax_outlet = axs[row+1]
                    ax_empty = axs[row+2]
                else: 
                    ax_inlet = axs[row, col]
                    ax_outlet = axs[row+1, col]
                    ax_empty = axs[row+2, col]
                    
                # Share the x axis between the inlet and outlet plots, remove tick labels from the inlet plot
                ax_inlet.get_shared_x_axes().join(ax_inlet, ax_outlet)
                ax_inlet.set_xticklabels([])
                
                # Skip the non-filtered data
                if 'filtered' not in timestamp:
                    continue
                
                # Get all of the flow data we want to plot over time
                flow_sensor_names: list[str] = [col for col in df.columns if 'flow' in col]     
                inlet_flow_name = [name for name in flow_sensor_names if 'inlet' in name][0]   
                outlet_flow_name = [name for name in flow_sensor_names if 'outlet' in name][0]
                #df.set_index('monitor_time_ms', inplace=True)
                
                # Find the index where the piezo was turned on
                piezo_1_enable_data = df['driver_piezo_1_enable'].to_numpy()
                piezo_on_index = np.where(piezo_1_enable_data == 1)[0][0]
                self.piezo_on_time_sec = df['monitor_time_ms'][piezo_on_index] / 1000
                
                # Get the x and y data, trim the data first and last 5 points
                x = df['monitor_time_ms'] / 1000     # ms to s
                y = df[flow_sensor_names]
                x = x[:]
                y = y[:]
                
                # # Get the average of the inlet flow rate over time
                # y_inlet_avg = y[inlet_flow_name].mean(axis=1)
                # y_inlet_std = y[inlet_flow_name].std(axis=1)
                
                # Calculate the differential flow rate of the outlet flow sensor over time
                y_outlet_avg = y[outlet_flow_name].mean(axis=0)
                y[outlet_flow_name] = y[outlet_flow_name].sub(y_outlet_avg, axis=0)
                y_outlet_cummulative = y[outlet_flow_name].cumsum()
                y_outlet_cummulative_filtered = utils.filter_data(y_outlet_cummulative, 'moving_average', filter_params={'window_size': 27})
                
                # Create a dataframe of the data with with the top, bottom, and total averages of temperature
                df_plot = pd.DataFrame({'Seconds': x, 'Inlet Raw': y[inlet_flow_name], 'Outlet Cumm': y_outlet_cummulative_filtered, 'Outlet Raw': y[outlet_flow_name]})
                num_bins = len(df_plot) // 5
                df_plot: pd.DataFrame = df_plot.groupby(pd.qcut(df_plot.index, num_bins)).mean()       # Decimate to 1/5th of the data
                df_plot.set_index('Seconds', inplace=True)
                #df_plot = df_plot.iloc[1:-1]    # Remove the first and last 1 bins
                
                # Find the limits of the y axis using min and max of the data              
                y_min_inlet = df_plot['Inlet Raw'].min() * 0.98
                y_max_inlet = df_plot['Inlet Raw'].max() * 1.02
                y_min_outlet = y_outlet_cummulative_filtered.min() * 0.98
                y_max_outlet = y_outlet_cummulative_filtered.max() * 1.02
                self.log.debug(f"y max outlet: {y_max_outlet}, y min outlet: {y_min_outlet}")
                
                # Set the x lims to be data between the first and last points
                x_min = df_plot.index[0]
                x_max = df_plot.index[-1]
                self.log.debug(f"x min: {x_min}, x max: {x_max}")
                
                legend_font_size = 8
                
                # Plot the data for inlet flow rate
                ax_inlet.plot(df_plot.index, df_plot['Inlet Raw'], color='b', linestyle='-', label='Inlet Flow')
                ax_inlet.axvline(x=self.piezo_on_time_sec, color='k', linestyle='--', label='Piezo On')
                ax_inlet.set_xlim(x_min, x_max)
                ax_inlet.set_ylim(y_min_inlet, y_max_inlet)
                ax_inlet.legend(loc='upper right', prop={'size': legend_font_size}) 
                ax_inlet.set_ylabel('Flow Rate (units)')
                ax_inlet.set_title(f"Flow Data for {dataset_name.replace('_', ' ')} at {timestamp.replace('filtered', '').replace('_', '')}")
                
                # Plot the data for outlet flow rate
                ax_outlet.plot(df_plot.index, df_plot['Outlet Cumm'], color='r', linestyle='-', label='Cummulative Differential Outlet Flow')
                #ax_outlet.plot(df_plot.index, df_plot['Outlet Raw'], color='g', linestyle='-', label='Raw Outlet Flow')
                ax_outlet.axvline(x=self.piezo_on_time_sec, color='k', linestyle='--', label='Piezo On')
                ax_outlet.set_xlim(x_min, x_max)
                ax_outlet.set_ylim(y_min_outlet, y_max_outlet)
                ax_outlet.legend(loc='upper right', prop={'size': legend_font_size}) 
                ax_outlet.set_xlabel('Time (seconds)')
                ax_outlet.set_ylabel('Cummulative Flow\nRate (units)')
              
                # Hide the empty plot
                ax_empty.set_visible(False)
                
                # Save the plot data
                plot_data[timestamp] = df_plot
            
                # Increment the subplot count
                sp_count += 1
                                
            # Add minor ticks to the x axis
            for axis in axs.reshape(-1):
                axis.xaxis.set_minor_locator(tck.AutoMinorLocator())    
            
            # Save and draw the plot with tight layout
            fig.tight_layout()
            plt.savefig(Path(self.save_plot_dir, f"flow_data_{dataset_name}.png"), dpi=600)
            plt.draw()
            return plot_data
        

    def _plot_thermistor_data(self) -> dict[str, pd.DataFrame]:
        """Plot relevant data from the thermistors, such as the average temperature over time, and the temperature difference between the bottom and the top thermistors
        Each plot will have a vertical line at the point of when the piezos were turned on, and each dataset will have an individual plot for each timestamp, and there will be a combined plot for all datasets
        datasets:
            dataset_name (str): dataset (dict[str, pd.DataFrame])
            
        dataset: 
            timestamp (str): df (pd.DataFrame)
        """ 
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
            fig.suptitle(f"Thermistor Data for '{dataset_name.replace('_', ' ')}' Dataset", fontsize=16)
            
            plot_data = {}
            
            for timestamp, df in dataset.items():
                timestamp: str = timestamp      # Set the type
                df: pd.DataFrame = df           # Set the type

                # Calculate the current row and column of the subplot and the axis
                row = sp_count // cols
                col = sp_count % cols
                ax: plt.Axes
                if rows == 1: ax = axs[col]
                elif cols == 1: ax = axs[row]
                else: ax = axs[row, col]
                
                # Skip the non-filtered data
                if 'filtered' not in timestamp:
                    continue
                
                # Get all of the thermistor data we want to plot over time
                thermistor_names: list[str] = [col for col in df.columns if 'thermistor' in col and '13' not in col and '14' not in col]       # remove the 13 and 14 from the list
                top_thermistor_names = [name for name in thermistor_names if int(name.split("_")[2]) % 2 == 0]   
                btm_thermistor_names = [name for name in thermistor_names if int(name.split("_")[2]) % 2 == 1]
                #df.set_index('monitor_time_ms', inplace=True)
                
                # Find the index where the piezo was turned on
                piezo_1_enable_data = df['driver_piezo_1_enable'].to_numpy()
                piezo_on_index = np.where(piezo_1_enable_data == 1)[0][0]
                self.piezo_on_time_sec = df['monitor_time_ms'][piezo_on_index] / 1000
                
                # Get the x and y data, trim the data first and last 5 points
                x = df['monitor_time_ms'] / 1000     # ms to s
                y = df[thermistor_names]
                x = x[:]
                y = y[:]
                
                # Get the average of the top, bottom, and total thermistors
                y_total_avg = y.mean(axis=1)
                y_top_avg = y[top_thermistor_names].mean(axis=1)
                y_btm_avg = y[btm_thermistor_names].mean(axis=1)
                y_total_std = y.std(axis=1)
                y_top_stdev = y[top_thermistor_names].std(axis=1)
                y_btm_stdev = y[btm_thermistor_names].std(axis=1)
                
                # Create a dataframe of the data with with the top, bottom, and total averages of temperature
                df_plot = pd.DataFrame({'Seconds': x, 'Total Avg': y_total_avg, 'Total Std': y_total_std, 'Top Avg': y_top_avg, 'Top Std': y_top_stdev, 'Btm Avg': y_btm_avg, 'Btm Std': y_btm_stdev})
                num_bins = len(df_plot) // 5
                df_plot: pd.DataFrame = df_plot.groupby(pd.qcut(df_plot.index, num_bins)).mean()       # Decimate to 1/5th of the data
                df_plot.set_index('Seconds', inplace=True)
                #df_plot = df_plot.iloc[1:-1]    # Remove the first and last 2 bins
                
                # Find the limits of the y axis using min and max of the data              
                y_min = df_plot['Total Avg'].min() * 0.98
                y_max = df_plot['Total Avg'].max() * 1.02
                
                # Set the x lims to be data between the first and last points
                x_min = df_plot.index[0]
                x_max = df_plot.index[-1]
                
                legend_font_size = 8
                
                # Plot the data with error bands for the top and bottom thermistors
                top_line,   = ax.plot(df_plot.index, df_plot['Top Avg'], color='b', linestyle='-')
                top_fill    = ax.fill_between(df_plot.index, df_plot['Top Avg'] - df_plot['Top Std'], df_plot['Top Avg'] + df_plot['Top Std'], color='b', alpha=0.2)
                btm_line,   = ax.plot(df_plot.index, df_plot['Btm Avg'], color='r', linestyle='-')
                btm_fill    = ax.fill_between(df_plot.index, df_plot['Btm Avg'] - df_plot['Btm Std'], df_plot['Btm Avg'] + df_plot['Btm Std'], color='r', alpha=0.2)
                total_avg,  = ax.plot(df_plot.index, df_plot['Total Avg'], color='g', linestyle='-')
                piezo_on    = ax.axvline(x=self.piezo_on_time_sec, color='k', linestyle='--')
                # Set limits, axes labels, title, and the legend for the plot
                ax.set_xlim(x_min, x_max)
                ax.set_ylim(y_min, y_max)
                ax.legend([(top_line, top_fill), (btm_line, btm_fill), (total_avg), (piezo_on)], ['Top Thermistors', 'Btm Thermistors', 'Avg Thermistors', 'Piezo Turn On'], loc='upper right', prop={'size': legend_font_size})
                ax.set_xlabel('Time (seconds)')
                ax.set_ylabel('Temperature (C)')
                ax.set_title(f"Thermistor Data for {dataset_name.replace('_', ' ')} at {timestamp.replace('filtered', '').replace('_', '')}")
                
                # Save the plot data
                plot_data[timestamp] = df_plot
                
                sp_count += 1
                                
            # Add minor ticks to the x axis
            for axis in axs.reshape(-1):
                axis.xaxis.set_minor_locator(tck.AutoMinorLocator())    
            
            # Save and draw the plot with tight layout
            fig.tight_layout()
            plt.savefig(Path(self.save_plot_dir, f"thermistor_data_{dataset_name}.png"), dpi=600)
            plt.draw()
            return plot_data
