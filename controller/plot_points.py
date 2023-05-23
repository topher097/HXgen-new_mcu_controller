import matplotlib.pyplot as plt
import mpl_toolkits.mplot3d.axes3d as p3
import matplotlib.animation as animation
from matplotlib import cm
import numpy as np

from data_classes import Vibrometer, Geometry, Point, Values, Peaks
from find_peaks import downsample
from extract import get_point_object

"""
    Take each Point object in the Vibrometer object and plot the FRF data on one figure 
"""

from utils import meter_to_micrometer, find_nearest_freq_index


class Plot3DData:
    def __init__(self, vibrometer: Vibrometer, mesh_size: tuple[int, int]) -> None:
        self.vibrometer = vibrometer
        self.num_freqs = vibrometer.freq.size
        self.freq = vibrometer.freq
        self.num_points = vibrometer.geometry.num_points
        self.mesh_size = mesh_size
        
        
    def create_static_figure(self, title: str, xlabel: str, ylabel: str, zlabel: str, data_type: str, ref_data: bool) -> None:
        fig = plt.figure()
        ax = fig.add_subplot(projection='3d')
        ax.set_title(title)
        ax.set_xlabel(xlabel)
        ax.set_ylabel(zlabel)   # swapped to have amplitude in the zdir and the point num in the y dir
        ax.set_zlabel(ylabel)
        
        # Go through each point and plot the data on the plots
        for point_num in range(self.num_points):
            point_name = f"Point_{point_num+1}"
            point = get_point_object(self.vibrometer.points, point_num+1)
            point_data_type = getattr(point, data_type)
            if ref_data:
                y = point_data_type.ref_data_db
            else:
                y = point_data_type.data_db
                
            # Simplify the data by downsampling
            scale = 1
            x = downsample(self.freq, scale)
            y = downsample(y, scale)            
                
            ax.plot(x, y, zs=point_num, zdir='y', label=point_name)
        ax.set_xlim(0,5000)
        plt.draw()
  