from data_classes import Vibrometer
import os
import csv
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap, LinearSegmentedColormap
import matplotlib.colors as colors
import matplotlib.patches as mpatches

from find_peaks import downsample
from utils import save_vibrometer_object, load_vibrometer_object, get_nearest_freq_index, get_point_object


vibrometer_objects_dir = os.path.join(os.getcwd(), "vibrometer_objects")
report_output_dir = os.path.join(os.getcwd(), "peak_report_output")

MESH_INTEREST_POINT_NUM = 30
MFC_MESH_INTEREST_POINT_NUM = 26
CHANNEL_TOP_INTEREST_POINT_NUM = 19
MFC_CHANNEL_TOP_INTEREST_POINT_NUM = 15

class PeakReportGenerator:
    def __init__(self, output_dir: str=report_output_dir, vibrometer_dir: str=vibrometer_objects_dir):
        # Set input vibrometer dir
        self.vibrometer_dir = vibrometer_dir
        
        # Set plot cmap
        self.cmap = plt.get_cmap('hot', 100)
        
        # Get the vibrometer objects from the vibrometer object directrory
        self.vibrometer_objects_paths = [f for f in os.listdir(vibrometer_objects_dir) if os.path.isfile(os.path.join(vibrometer_objects_dir, f))]
        self.num_vibrometers = len(self.vibrometer_objects_paths)
        self.vibrometers = list()
        self.output_report_file_paths = list()
        self.output_plot_file_paths = list()
        self.prefixes = list()
        for path in self.vibrometer_objects_paths:
            prefix = os.path.basename(path).split('.')[0].replace("_vibrometer", "")
            self.vibrometers.append(load_vibrometer_object(prefix))
            output_file_name = prefix + "_peak_report"
            self.output_report_file_paths.append(os.path.join(output_dir, output_file_name+'.csv'))
            self.output_plot_file_paths.append(os.path.join(output_dir, output_file_name+'.png'))
            self.prefixes.append(prefix)
            
        
    def run(self):
        # Generate the full gap plot and csv file
        #self.generate_full_gap_plot()
        self.generate_full_csv_file()
        
        # Generate just mesh plot and just top plot for gap
        #self.generate_mesh_and_top_gap_plot(keyword='Mesh')
        #self.generate_mesh_and_top_gap_plot(keyword='Top')
        
        # Generate just mesh plot and just top plot for frf
        self.generate_mesh_and_top_frf_plot(keyword='Mesh')
        self.generate_mesh_and_top_frf_plot(keyword='Top')
        
        # Generate the full frf plot
        self.generate_full_frf_plot()
        
    """Generate a plot using the average frf point peaks with a line of 3 Hz bandgap with the color of the band getting redder as the value increases"""
    def generate_full_gap_plot(self):
        # Create the figure
        fig = plt.figure(figsize=(10, 8))
        
        # Axes limits
        xmin = 0
        xmax = 10000
        ymin = 0
        ymax = 0.5
        
        # Add n subplots for number of vibrometer object and plot the data
        for i in range(self.num_vibrometers):  
            vibrometer: Vibrometer = self.vibrometers[i] 
            #output_path = self.output_plot_file_paths[i]
            ax  = fig.add_subplot(self.num_vibrometers, 1, i+1)
            x = vibrometer.avg_point.frf_peaks[:, 0]        # hz
            y = vibrometer.avg_point.frf_peaks[:, 1]        # db
            min_y = np.min(y)
            max_y = np.max(y)
            ax.set_ylim(ymin, ymax)
            ax.set_xlim(xmin, xmax)
            ax.set_title(f"{self.prefixes[i]}")
            if i == self.num_vibrometers-1:
                ax.set_xlabel("Frequency (Hz)")
            
            # Create image data
            image = np.ones(shape=(1, len(vibrometer.freq), 4))
            
            # Go through each peak and color the vertical line based on the value bewteen the min and max
            for j in range(len(x)):
                index = get_nearest_freq_index(vibrometer.freq, x[j])
                y_val = y[j]
                color = np.asarray(self.cmap((y_val-min_y)/(max_y-min_y)))
                for k in range(40):
                    ind = index-20+k
                    image[:, ind, :] = color
            # Show the image and colorbar on plot
            img = ax.imshow(image, interpolation='nearest', aspect='auto', vmin=min_y, vmax=max_y, cmap=self.cmap)
            ax.get_yaxis().set_visible(False)
            #ax.sharex(ax)
            cbar = plt.colorbar(mappable=img, ax=ax, label="FRF Peak Value (dB)")
            cbar.set_ticks(np.linspace(min_y, max_y, 9, endpoint=True))
            plt.draw()
        plt.tight_layout()
        plt.savefig(os.path.join(report_output_dir, "full_gap_plot.png"))
            
    """Generate gap plot for mesh (with and without mfc)"""
    def generate_mesh_and_top_gap_plot(self, keyword: str="mesh"):
        # Create the figure
        fig = plt.figure(figsize=(10, 8))
        
        # Axes limits
        xmin = 0
        xmax = 10000
        ymin = 0
        ymax = 0.5
        
        # Get list of vibrometer mesh objects
        vibrometers = list()
        prefixes = list()
        for i in range(self.num_vibrometers):
            name = self.prefixes[i]
            if keyword in name:
                prefixes.append(name)
                vibrometers.append(self.vibrometers[i])
        num_vibrometers = len(vibrometers)
        
        # Add n subplots for number of vibrometer object and plot the data
        for i in range(num_vibrometers):  
            vibrometer: Vibrometer = vibrometers[i] 
            #output_path = self.output_plot_file_paths[i]
            ax  = fig.add_subplot(num_vibrometers, 1, i+1)
            x = vibrometer.avg_point.frf_peaks[:, 0]        # hz
            y = vibrometer.avg_point.frf_peaks[:, 1]        # db
            min_y = np.min(y)
            max_y = np.max(y)
            ax.set_ylim(ymin, ymax)
            ax.set_xlim(xmin, xmax)
            ax.set_title(f"{prefixes[i]}")
            if i == num_vibrometers-1:
                ax.set_xlabel("Frequency (Hz)")
            # Create image data
            image = np.ones(shape=(1, len(vibrometer.freq), 4))
            # Go through each peak and color the vertical line based on the value bewteen the min and max
            for j in range(len(x)):
                index = get_nearest_freq_index(vibrometer.freq, x[j])
                y_val = y[j]
                color = np.asarray(self.cmap((y_val-min_y)/(max_y-min_y)))
                for k in range(40):
                    ind = index-20+k
                    image[:, ind, :] = color
            # Show the image and colorbar on plot
            img = ax.imshow(image, interpolation='nearest', aspect='auto', vmin=min_y, vmax=max_y, cmap=self.cmap)
            ax.get_yaxis().set_visible(False)
            #ax.sharex(ax)
            cbar = plt.colorbar(mappable=img, ax=ax, label="FRF Peak Value (dB)")
            cbar.set_ticks(np.linspace(min_y, max_y, 9, endpoint=True))
            plt.draw()
        plt.tight_layout()
        plt.savefig(os.path.join(report_output_dir, f"{keyword}_gap_plot.png"))
        
    """Generate frf plot for mesh (with and without mfc)"""
    def generate_mesh_and_top_frf_plot(self, keyword: str="mesh"):
        # Create the figure
        fig = plt.figure(figsize=(14, 8))
        ax = fig.add_subplot(111)
        
        # Axes limits
        xmin = 0
        xmax = 10000
        ax.set_xlim(xmin, xmax)
        if keyword == "Mesh":
            ax.set_title(f"Channel Mesh FRF @ Point")
        else:
            ax.set_title(f"Channel Top FRF @ Point")
        ax.set_ylabel("FRF (dB)")
        ax.set_xlabel("Frequency (Hz)")
        
        # Get list of vibrometer mesh objects
        vibrometers = list()
        prefixes = list()
        for i in range(self.num_vibrometers):
            name = self.prefixes[i]
            if keyword in name:
                prefixes.append(name)
                vibrometers.append(self.vibrometers[i])
        num_vibrometers = len(vibrometers)
        
        # Add n subplots for number of vibrometer object and plot the data
        for i in range(num_vibrometers):
            # Get vibrometer object
            vibrometer: Vibrometer = self.vibrometers[i]
            name = prefixes[i]
            # Get the point for the point number of interest
            if "Mesh" in name:
                if "MFC" in name:
                    point_num_of_interest = MFC_MESH_INTEREST_POINT_NUM
                else:
                    point_num_of_interest = MESH_INTEREST_POINT_NUM
            elif "Top" in name:
                if "MFC" in name:
                    point_num_of_interest = MFC_CHANNEL_TOP_INTEREST_POINT_NUM
                else:
                    point_num_of_interest = CHANNEL_TOP_INTEREST_POINT_NUM
            else:
                raise Exception("Invalid keyword")
            point_of_interest = get_point_object(vibrometer.points, point_num_of_interest)
            
            # ax  = fig.add_subplot(num_vibrometers, 1, i+1)
            # ax.set_xlim(xmin, xmax)
            # ax.set_title(f"{prefixes[i]} @ Point # {point_num_of_interest}")
            # ax.set_ylabel("FRF (dB)")
            # if i == num_vibrometers-1:
            #     ax.set_xlabel("Frequency (Hz)")
                
            # Get line data
            freq = vibrometer.freq
            frf = point_of_interest.frf.data_db            
            # Downsameple by 6
            factor = 6
            freq = downsample(freq, factor)
            frf = downsample(frf, factor)
            # Plot lines
            start = 50
            ax.plot(freq[start:], frf[start:], label=f"{prefixes[i]} @ Point # {point_num_of_interest}", linewidth=0.5)
            # Get peak data
            peaks = point_of_interest.frf_peaks
            # Plot peak data
            ax.scatter(peaks[:, 0], peaks[:, 1], edgecolors='black', s=20.0, facecolors=None)
            
            plt.legend(loc='best')
            plt.tight_layout()
            plt.draw()
            plt.savefig(os.path.join(report_output_dir, f"{keyword}_frf_plot.png"))


    """Generate a csv file with the peak data"""
    def generate_full_csv_file(self):
        output_path = os.path.join(report_output_dir, "full_peak_report.csv")       
        with open(output_path, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile, delimiter=',')
            writer.writerow(["Frequency (Hz)", "FRF Peak Value (dB)", "Test Name"])
            for i in range(self.num_vibrometers):
                vibrometer: Vibrometer = self.vibrometers[i]
                test_name: str = self.prefixes[i]
                for j in range(len(vibrometer.avg_point.frf_peaks)):
                    writer.writerow((vibrometer.avg_point.frf_peaks[j, :], test_name))
        
    """Plot all of the vibrometer objects frf data in a single plot, with a legend"""            
    def generate_full_frf_plot(self):
        
        # Create figure
        fig = plt.figure(figsize=(12, 7))
        ax = fig.add_subplot(1, 1, 1)
        ax.set_xlim(0, 10000)
        ax.set_title(f"FRF of All Vibrometer Tests")
        ax.set_xlabel("Frequency (Hz)")
        ax.set_ylabel("FRF (dB)")
        
        for i in range(self.num_vibrometers):
            # Get vibrometer object
            vibrometer: Vibrometer = self.vibrometers[i]
            name = self.prefixes[i]
            # Get the point for the point number of interest
            if "Mesh" in name:
                point_num_of_interest = MESH_INTEREST_POINT_NUM
            elif "Top" in name:
                point_num_of_interest = CHANNEL_TOP_INTEREST_POINT_NUM
            else:
                raise Exception("Invalid keyword")
            point_of_interest = get_point_object(vibrometer.points, point_num_of_interest)
            name = name + f" @ Point #{point_num_of_interest}"
            
            # Get line data
            freq = vibrometer.freq
            frf = point_of_interest.frf.data_db            
            # Downsameple by 6
            factor = 6
            freq = downsample(freq, factor)
            frf = downsample(frf, factor)
            # Plot lines
            start = 50
            ax.plot(freq[start:], frf[start:], label=name, linewidth=1)
            # Get peak data
            peaks = point_of_interest.frf_peaks
            # Plot peak data
            ax.scatter(peaks[:, 0], peaks[:, 1], edgecolors='black', s=20.0, facecolors=None)
            
        plt.legend(loc='best')
        plt.tight_layout()
        plt.draw()
        plt.savefig(os.path.join(report_output_dir, "full_frf_plot.png"))
            


if __name__ == "__main__":
    generator = PeakReportGenerator()
    generator.run()
    plt.show()