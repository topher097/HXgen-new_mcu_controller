import matplotlib.pyplot as plt
import mpl_toolkits.mplot3d.axes3d as p3
import matplotlib.animation as animation
from matplotlib.widgets import Slider
from matplotlib import cm
import numpy as np
from typing import Optional

from PySide6.QtCore import Property, Signal, QObject

from pyqtgraph.Qt import QtCore, QtGui, QtWidgets
import pyqtgraph.opengl as gl
import pyqtgraph as pg
import sys

from data_classes import Vibrometer
from extract import get_point_object

"""
    Take each Point object in the Vibrometer object and plot the FRF data on one figure 
"""

"""Function to create the time series amplitude for a particular point and for a particular frequency"""
def velocity_animation_function(num_frames: int, freq: float, zmax_amplitude: float) -> np.ndarray:
    t = np.linspace(0, num_frames/freq, num_frames)
    z_real = zmax_amplitude.real
    z_imag = zmax_amplitude.imag
    z_shift = 0
    period = 1/freq
    z_t = z_real*np.cos(period*(t + z_imag)) + z_shift
    return z_t

"""Given a number in meters, return the value as micrometers"""
def meter_to_micrometer(value: float) -> float:
    return value * 1e6

"""Find the index of closest frequency input value"""
def find_nearest(array: np.ndarray, value: float) -> int:
    array = np.asarray(array)
    idx = (np.abs(array - value)).argmin()
    return idx

class Animation3DMesh(QObject):
    def __init__(self, vibrometer: Vibrometer, mesh_size: tuple[int, int]) -> None:
        super().__init__()
        self.vibrometer = vibrometer
        self.num_freqs = vibrometer.freq.size
        self.freq = vibrometer.freq
        self.num_points = vibrometer.geometry.num_points
        self.mesh_size = mesh_size  
        self.current_animation_point_number = self.vibrometer.measured_points[0] 
        self.current_point = get_point_object(self.vibrometer.points, self.current_animation_point_number)
        self.num_frames = 60                             # Number of frames to animate
        self.fps = 10                                    # Number of frames per second
        self.current_frame_number = 0
    
    """Calculate the X, Y, and Z arrays for the trisurface"""
    def create_data_arrays(self) -> None:
        # Create mesh points using the geometry
        x = self.vibrometer.geometry.pointXYZ[:,0].flatten()
        y = self.vibrometer.geometry.pointXYZ[:,1].flatten()
        num_points = len(self.vibrometer.measured_points)
        
        # Find the z_amplitude (amplitude at each point at given frequency)
        freq_index = find_nearest(self.freq, self.current_plot_freq)
        zmax_amplitude = np.zeros(num_points, dtype=complex)
        i = 0
        for point_num in self.vibrometer.measured_points:
            point = get_point_object(self.vibrometer.points, point_num)
            zmax_amplitude[i] = point.psd.data[freq_index]
            i+=1
        zmax_amplitude = zmax_amplitude*1e6         # Convert to micrometers
        
        # Function for animating surface over time
        t = np.linspace(0, 2*np.pi, self.num_frames, endpoint=True)
        z_real = zmax_amplitude.real
        z_imag = zmax_amplitude.imag
        f = lambda t_frame : z_real*np.sin(t_frame + z_imag)
        
        # Iterate through all frames and calculate the z value for each point
        zarray = np.zeros((self.num_points, self.num_frames))
        for frame_num in range(self.num_frames):
            zt = f(t[frame_num])
            zt = zt.flatten()
            zarray[:, frame_num] = zt
            
        # Set arrays and min/max
        self.x = x
        self.y = y
        self.zarray = zarray
        self.z_max = np.max(self.zarray)*0.9
        self.z_min = np.min(self.zarray)*1.1
        mesh_width = np.max(self.x) - np.min(self.x)
        mesh_height = np.max(self.y) - np.min(self.y)
        mesh_center_width = np.mean(self.x)
        mesh_center_height = np.mean(self.y)
        mesh_max_side = np.max([mesh_width, mesh_height])
        self.x_max = mesh_center_width + mesh_max_side/2
        self.x_min = mesh_center_width - mesh_max_side/2
        self.y_max = mesh_center_height + mesh_max_side/2
        self.y_min = mesh_center_height - mesh_max_side/2   

    
    def create_pyqt_mesh_animation(self, initial_desired_freq: float) -> None:
        self.current_plot_freq = initial_desired_freq
        
        # Initialize the X Y and Z data for current point at desired freq
        self.create_data_arrays()
        
        # Create app
        background_color = pg.mkColor("#F6F6F5")
        self.app = QtWidgets.QApplication([])
        self.window = pg.GraphicsView(background=background_color)
        self.layout = QtWidgets.QGridLayout()
        self.window.setLayout(self.layout)
        self.window.show()
        
        # Create 3D widget
        self.view3d = gl.GLViewWidget()
        self.view3d.opts['distance'] = 40
        self.view3d.setWindowTitle('Vibrometer Mesh Visualization')
        self.view3d.setGeometry(0, 110, 1920, 1080)
        self.view3d.show()
        
        # Create x and y grids for 3D widget
        gx = gl.GLGridItem()
        gx.rotate(90, 0, 1, 0)
        gx.translate(-10, 0, 0)
        self.view3d.addItem(gx)
        gy = gl.GLGridItem()
        gy.rotate(90, 1, 0, 0)
        gy.translate(0, -10, 0)
        self.view3d.addItem(gy)
        gz = gl.GLGridItem()
        gz.translate(0, 0, -10)
        self.view3d.addItem(gz)
        
        # Create 3d plot widget object
        self.mesh3d = gl.GLMeshItem()
        self.mesh3d.set

        
        # Pens
        frf_color = pg.mkColor(100, 80, 245, 255)       # blue
        coh_color = pg.mkColor(250, 200, 50, 200)       # orangish
        self.left_border_pen = pg.mkPen(frf_color, width=3)
        self.right_border_pen = pg.mkPen(coh_color, width=3)
        self.bottom_border_pen = pg.mkPen("k", width=3)
        self.coherence_pen = pg.mkPen(coh_color, width=0.2)
        self.frf_pen = pg.mkPen(frf_color, width=1)
        
        # Create 2d plot for current point FRF and coherence
        self.view2d_point = pg.PlotWidget(background=background_color)
        self.view2d_point.getAxis('left').setLogMode(False, True)                                                           # Set frf y axis to log scale
        self.view2d_point.setXRange(np.min(self.freq), np.max(self.freq))                                                   # Set x axis range
        self.view2d_point.setYRange(np.min(self.current_point.frf.data_db), np.max(self.current_point.frf.data_db))         # Set y range for left y axis
        self.view2d_point.setWindowTitle(f"FRF and Coherence of signal at Point {self.current_animation_point_number}")     # Window title
        self.view2d_point.setLabel('bottom', 'Frequency', units='Hz', **{'font-size':'14pt'})                               # Set x axis label
        self.view2d_point.setLabel('left', 'FRF (dB)', **{'font-size':'14pt'})                                              # Set left y axis label
        self.view2d_point.setLabel('right', 'Coherence', **{'font-size':'14pt'})                                            # Set right y axis label
        self.view2d_point.getAxis('bottom').setPen(self.bottom_border_pen)                                                  # Set x axis border pen
        self.view2d_point.getAxis('left').setPen(self.left_border_pen)                                                      # Set left y axis border pen
        self.view2d_point.getAxis('right').setPen(self.right_border_pen)                                                    # Set right y axis border pen
        self.view2d_point.showAxis('right')                                                                                 # Show the right y axis
        
        # Create coherence curve object and frf plot object
        self.view2d_point_coherence_curve = pg.PlotCurveItem(pen=self.coherence_pen)     # Curve object for coherence data
        self.view2d_point_frf_curve = pg.PlotCurveItem(pen=self.frf_pen)                 # Curve object for frf data
        
        # Link the right and left axes on the point plot
        self.view2d_point_coherence_axis = pg.ViewBox()
        self.view2d_point.scene().addItem(self.view2d_point_coherence_axis)                 # Add view box to scene
        self.view2d_point.addItem(self.view2d_point_frf_curve)          # Add the frf curve to the plot widget
        self.view2d_point.getAxis('right').linkToView(self.view2d_point_coherence_axis)     # Link the right y axis to the view box object
        self.view2d_point_coherence_axis.addItem(self.view2d_point_coherence_curve)         # Add the coherence curve to the view box
        self.view2d_point_coherence_axis.setXLink(self.view2d_point)                        # Link the viewbox x axis to the plot widget x axis
        self.view2d_point_coherence_axis.setYRange(0, 1)  
        
        # Create 2d plot for average of points FRF and coherence
        self.view2d_average = pg.PlotWidget(background=background_color)
        self.view2d_average.getAxis('left').setLogMode(False, True)                                                           # Set frf y axis to log scale
        self.view2d_average.setXRange(np.min(self.freq), np.max(self.freq))                                                   # Set x axis range
        self.view2d_average.setYRange(np.min(self.current_point.frf.data_db), np.max(self.current_point.frf.data_db))         # Set y range for left y axis
        self.view2d_average.setWindowTitle("Average FRF and Coherence")                                                       # Window title
        self.view2d_average.setLabel('bottom', 'Frequency', units='Hz', **{'font-size':'14pt'})                               # Set x axis label
        self.view2d_average.setLabel('left', 'FRF (dB)', **{'font-size':'14pt'})                                              # Set left y axis label
        self.view2d_average.setLabel('right', 'Coherence', **{'font-size':'14pt'})  
        self.view2d_average.getAxis('bottom').setPen(self.bottom_border_pen)                                                     # Set x axis border pen
        self.view2d_average.getAxis('left').setPen(self.left_border_pen)                                                      # Set left y axis border pen
        self.view2d_average.getAxis('right').setPen(self.right_border_pen)                                                    # Set right y axis border pen
        self.view2d_average.showAxis('right')                                                                                 # Show the right y axis
        
        # Create coherence curve object and frf plot object
        self.view2d_average_coherence_curve = pg.PlotCurveItem(pen=self.coherence_pen)     # Curve object for coherence data
        self.view2d_average_frf_curve = pg.PlotCurveItem(pen=self.frf_pen)                 # Curve object for frf data
        
        # Link the right and left axes on the point plot
        self.view2d_average_coherence_axis = pg.ViewBox()
        self.view2d_average.scene().addItem(self.view2d_average_coherence_axis)                 # Add view box to scene
        self.view2d_average.addItem(self.view2d_average_frf_curve)          # Add the frf curve to the plot widget
        self.view2d_average.getAxis('right').linkToView(self.view2d_average_coherence_axis)     # Link the right y axis to the view box object
        self.view2d_average_coherence_axis.addItem(self.view2d_average_coherence_curve)         # Add the coherence curve to the view box
        self.view2d_average_coherence_axis.setXLink(self.view2d_average)                        # Link the viewbox x axis to the plot widget x axis
        self.view2d_average_coherence_axis.setYRange(0, 1)  
        
        # Add widgets to the layout
        #self.layout.addWidget(self.view3d, 1, 0)                        # Add 3D widget to layout
        self.layout.addWidget(self.view2d_point, 0, 0) 
        self.layout.addWidget(self.view2d_average, 1, 0)

        # Set the update of viewbox to be called when the plots are resized
        self.update_pyqt_2d_plots_size()
        self.view2d_point.getViewBox().sigResized.connect(self.update_pyqt_2d_plots_size)
        self.view2d_average.getViewBox().sigResized.connect(self.update_pyqt_2d_plots_size)
        
        # Set the data for both 2d plots
        self.update_pyqt_2d_plot_data()
        
        #self.create_mesh_animation(initial_desired_freq)
        
        # Start the PyQt window and the animation
        self.start_pyqt_app()

    """Update the data of the plot widgets"""
    def update_pyqt_2d_plot_data(self) -> None:
        # Plot the frf and coherence data on 2d plot point
        self.view2d_point_frf_curve.setData(self.freq, self.current_point.frf.data_db)
        self.view2d_point_coherence_curve.setData(self.freq, self.current_point.coherence.data)
        
        # Plot the frf and coherence data on 2d average plot
        self.view2d_average_frf_curve.setData(self.freq, np.real(self.vibrometer.avg_point.frf.data_db))
        self.view2d_average_coherence_curve.setData(self.freq, np.real(self.vibrometer.avg_point.coherence.data))
    
    """Update the size of the plots when the PyQt window is resized"""
    def update_pyqt_2d_plots_size(self) -> None:
        # Point data
            self.view2d_point_coherence_axis.setGeometry(self.view2d_point.getViewBox().sceneBoundingRect())
            self.view2d_point_coherence_axis.linkedViewChanged(self.view2d_point.getViewBox(), self.view2d_point_coherence_axis.XAxis)
            # Avg data
            self.view2d_average_coherence_axis.setGeometry(self.view2d_average.getViewBox().sceneBoundingRect())
            self.view2d_average_coherence_axis.linkedViewChanged(self.view2d_average.getViewBox(), self.view2d_average_coherence_axis.XAxis)
        
    """Start the PyQt application"""
    def start_pyqt_app(self) -> None:
        self.app.instance().exec_()
        
    """Define the PyQt animation timer"""
    def define_pyqt_animation(self):
        timer = QtCore.QTimer()
        timer.timeout.connect(self.update_pyqt_animation)
        timer.start(1000/self.fps)
        self.start_pyqt_animation()
        
    def update_pyqt_animation(self):
        frame_z = self.zarray[:, self.current_frame_number].flatten()
        
        
        
    def create_mesh_animation(self, initial_desired_freq: float) -> None:
        self.current_plot_freq = initial_desired_freq
                
        # Create the figure
        self.fig = plt.figure(figsize=(10, 10))
        #spec = self.fig.add_gridspec(ncols=1, nrows=5, height_ratios=[7, 0.5, 0.5, 3, 3])
        spec = self.fig.add_gridspec(ncols=1, nrows=4, height_ratios=[0.5, 0.5, 3, 3])
        self.ax_freq_slider = self.fig.add_subplot(spec[0])
        self.ax_point_slider = self.fig.add_subplot(spec[1])
        self.ax_plot = self.fig.add_subplot(spec[2])
        self.ax_plot.set_yscale('symlog')
        self.ax_plot_coh = self.ax_plot.twinx()
        self.ax_plot_avg = self.fig.add_subplot(spec[3])
        self.ax_plot_avg.set_yscale('symlog')
        self.ax_plot_avg_coh = self.ax_plot_avg.twinx()
        self.plot_args = {'linewidth': 0.1, 'antialiased': True}
        
        # Create sliders
        self.freq_slider = Slider(self.ax_freq_slider, 'Frequency', np.min(self.freq), np.max(self.freq), valinit=initial_desired_freq, valstep=1.0)
        self.freq_slider.on_changed(self.update)
        self.point_slider = Slider(self.ax_point_slider, 'Point', np.min(self.vibrometer.measured_points), np.max(self.vibrometer.measured_points), valinit=self.vibrometer.measured_points[0], valstep=self.current_animation_point_number)
        self.point_slider.on_changed(self.update)
                
        # Update the plot     
        self.update()
        plt.tight_layout()
        
        # Start the animation
        plt.show()
        
        
    """Update the animation when the slider is changed"""
    def update(self, w: float=None) -> None:
        # Get current frequency from slider
        self.current_plot_freq = self.freq_slider.val
        self.current_animation_point_number = self.point_slider.val
        self.current_point = get_point_object(self.vibrometer.points, self.current_animation_point_number)
        print(self.current_plot_freq)
            
        # Calculate the X, Y, and Z arrays for the trisurface
        self.create_data_arrays()
        
        
        # Plot the FRF of current point vs frequency, as well as the coherence
        self.ax_plot.clear()
        self.ax_plot.plot(self.freq, self.current_point.frf.data_db, color='blue', label='FRF', linewidth=0.25)
        self.ax_plot_coh.plot(self.freq, self.current_point.coherence.data, color='red', label='Coherence', linewidth=0.25)
        self.ax_plot.axvline(x=self.current_plot_freq, color='black', linestyle='--', linewidth=1)     
        self.ax_plot.set_ylim(np.min(self.current_point.frf.data_db), np.max(self.current_point.frf.data_db))
        self.ax_plot_coh.set_ylim(np.min(self.current_point.coherence.data), np.max(self.current_point.coherence.data)) 
        self.ax_plot.legend(loc='upper right')
        
        # Plot the average FRF of all points vs Frequency, as well as coherence
        self.ax_plot_avg.clear()
        self.ax_plot_avg.plot(self.freq, self.vibrometer.avg_point.frf.data_db, color='blue', label='FRF', linewidth=0.25)
        self.ax_plot_avg_coh.plot(self.freq, self.vibrometer.avg_point.coherence.data, color='red', label='Coherence', linewidth=0.25)
        self.ax_plot_avg.axvline(x=self.current_plot_freq, color='black', linestyle='--', linewidth=1)
        self.ax_plot_avg.set_ylim(np.min(self.vibrometer.avg_point.frf.data_db), np.max(self.vibrometer.avg_point.frf.data_db))
        self.ax_plot_avg_coh.set_ylim(np.min(self.vibrometer.avg_point.coherence.data), np.max(self.vibrometer.avg_point.coherence.data))
        self.ax_plot_avg.legend(loc='upper right')
        self.ax_plot_avg_coh.legend(loc='upper right')
        
        
        
    # """Refresh the animation with the next frame"""
    # def draw_frame(self, frame_num) -> None:
    #     #print(f"Frame: {frame_num} for freq: {self.current_plot_freq}")
    #     self.ax.clear()
    #     self.ax.set_xlim(self.x_min, self.x_max)
    #     self.ax.set_ylim(self.y_min, self.y_max)
    #     self.ax.set_zlim(self.z_min, self.z_max)
    #     frame_z = self.zarray[:, frame_num].flatten()
    #     # Reverse the color map at the halfway point
    #     if frame_num/self.num_frames >= 0.5:
    #         cmap = cm.hot
    #     else:
    #         cmap = cm.hot_r
    #     self.ax.plot_trisurf(self.x, self.y, frame_z, cmap=cmap, linewidth=0.1, antialiased=True)
        
    #     # Plot scatter point at current selected point on the animation
    #     x = self.vibrometer.geometry.pointXYZ[self.current_animation_point_number, 0]
    #     y = self.vibrometer.geometry.pointXYZ[self.current_animation_point_number, 1]
    #     z = self.zarray[self.current_animation_point_number, frame_num]
    #     self.ax.scatter(x, y, z, color='blue', s=20)
        
        

            