# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import sys

import matplotlib
matplotlib.use('QtAgg')
from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg
from matplotlib.figure import Figure

from PySide6.Qt3DCore import Qt3DCore   
from PySide6.Qt3DExtras import Qt3DExtras
from PySide6.QtCore import QSize, Qt, QObject, Property, QPropertyAnimation, Signal
from PySide6.QtDataVisualization import Q3DSurface
from PySide6.QtGui import QBrush, QIcon, QLinearGradient, QPainter, QPixmap
from PySide6.QtWidgets import QApplication, QComboBox, QGroupBox, QHBoxLayout, QLabel, QMessageBox, QPushButton, QRadioButton, QSizePolicy, QSlider, QVBoxLayout, QWidget

from utils import get_nearest_freq_index, get_point_object, get_mesh_numpy_ij
from data_classes import Vibrometer
from make_surfacegraph import SurfaceGraph

THEMES = ["Qt", "Primary Colors", "Digia", "Stone Moss", "Army Blue", "Retro",
          "Ebony", "Isabelle"]


"""Class to create a Matplotlib figure widget for plotting 2d plots in Qt6"""
class MplCanvas(FigureCanvasQTAgg):
    def __init__(self, parent=None, width=5, height=4, dpi=100):
        fig = Figure(figsize=(width, height), dpi=dpi)
        self.axes = fig.add_subplot(111)
        super(MplCanvas, self).__init__(fig)
        

"""Class to create a surface mesh"""
class SurfaceMesh():
    pass
        

"""Class to control the mesh data for the animation"""
class SurfaceAnimationController(QObject):
    point_changed = Signal()
    
    def __init__(self, vibrometer: Vibrometer, parent=None):
        super().__init__(parent)
        self.vibrometer = vibrometer
        self.set_point_number(self.vibrometer.measured_points[0])

    """Set the point number and the point object for the surface animation data"""
    def set_point_number(self, value):
        self.current_point_number = value
        self.current_point = get_point_object(self.vibrometer.points, self.current_point_number)
        self.point_changed.emit()
        
    """Calculate the """
    def calculate_mesh_data(self):
        self.mesh_data = self.current_point.get_mesh_data()
        self.mesh_data_changed.emit()

class Window(QWidget, Qt3DExtras.Qt3DWindow):
    def __init__(self, vibrometer: Vibrometer, parent=None):
        super().__init__(parent)
        # Set surface and vibrometer objects
        self.vibrometer = vibrometer
        self.surface_controller = SurfaceAnimationController()
        self.surface_mesh = 1
        
        # Create container for the 3d mesh 
        self.mesh_container = QWidget.createWindowContainer(self.graph, self, Qt.Widget)
        screen_size = self.graph.screen().size()
        mesh_width_min = screen_size.width() / 2
        mesh_width_max = screen_size.height() / 1.6
                            
        self.mesh_container.setMinimumSize(QSize(mesh_width_min, mesh_width_max))
        self.mesh_container.setMaximumSize(screen_size)
        self.mesh_container.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.mesh_container.setFocusPolicy(Qt.StrongFocus)
        
        # Create containers for the point and average points FRF data
        self.point_plot_canvas = MplCanvas(self, width=5, height=4, dpi=100)
        self.avg_point_plot_canvas = MplCanvas(self, width=5, height=4, dpi=100)
        
        # Create a slider for the frequency of the FRF data
        self.freq_slider = QSlider(Qt.Horizontal, self)
        self.freq_slider.setMinimum(self.vibrometer.freq.min())
        self.freq_slider.setMaximum(self.vibrometer.freq.max())
        self.freq_slider.setTickInterval(self.vibrometer.freq[1]-self.vibrometer.freq[0])
        self.freq_slider.setEnabled(True)
        
        # Create a slider for the points in the mesh
        self.point_slider = QSlider(Qt.Horizontal, self)
        self.point_slider.setMinimum(self.vibrometer.measured_points[0])
        self.point_slider.setMaximum(self.vibrometer.measured_points[-1])
        self.point_slider.setTickInterval(1)
        self.point_slider.setEnabled(True)
        

        # Create layouts for the widgets
        main_layout = QHBoxLayout(self)                        # Horizontal layout 
        left_layout = QVBoxLayout(self)                        # Vertical layout on left
        right_layout = QVBoxLayout(self)                       # Vertical layout on right
        left_layout.addWidget(self.mesh_container, 1)          # Add the 3d mesh container to the left vertical layout
        left_layout.addWidget(self.point_plot_canvas, 2)       # Add the point FRF data plot to the left vertical layout 
        left_layout.addWidget(self.avg_point_plot_canvas, 3)   # Add the average point FRF data plot to the left vertical layout
        left_layout.addWidget(self.freq_slider, 4)             # Add the frequency slider to the right vertical layout
        left_layout.addWidget(self.point_slider, 5)            # Add the point slider to the right vertical layout
        main_layout.addLayout(left_layout, 1)                  # Add the left vertical layout to the horizontal layout
        main_layout.addLayout(right_layout, 2)                 # Add the right vertical layout to the horizontal layout
        right_layout.setAlignment(Qt.AlignTop)                 # Align the vertical layout widgets to the top
        left_layout.setAlignment(Qt.AlignTop)                  # Align the vertical layout widgets to the top
        
        
        
        # Create a container for the point FRF plot
        self.point_frf_container = QWidget(self)
        
        # Add sliders and the frf plots to the vertical layout
        

        # Add a theme list to the left vertical layout
        theme_list = QComboBox(self)
        theme_list.addItems(THEMES)
        theme_list.setCurrentIndex(2)

        # Add the widgets to the vertical layout
        right_layout.addWidget(QLabel("Frequency"))
        right_layout.addWidget(self.freq_slider)
        right_layout.addWidget(QLabel("Point Selection"))
        right_layout.addWidget(self.point_slider)
        right_layout.addWidget(QLabel("Theme"))
        right_layout.addWidget(theme_list)

        # Create modifier for 3d mesh and link application values to the modifications
        self.mesh_modifier = SurfaceGraph(self.graph)
        theme_list.currentIndexChanged[int].connect(self.mesh_modifier.change_theme)
        
        # When the frequency slider is moved, update the mesh and FRF plots
        self.freq_slider.valueChanged.connect(self.update_point_plot)
        self.freq_slider.valueChanged.connect(self.update_avg_point_plot)
        self.freq_slider.valueChanged.connect(self.mesh_modifier.update_mesh)
        
        # Used the current index of the slider to set the initial data to plot on figures
        self.update_point_plot()
        self.update_avg_point_plot()

        

    """Update the point plot canvas when the frequency slider or point slider is modified"""
    def update_point_plot(self):
        self.point_plot_canvas.axes.cla()       # Clear the canvas of the old data
        self.current_point_number = self.point_slider.value()
        self.current_point = get_point_object(self.vibrometer.points, self.current_point_number)            # Get the current point object given slider value
        self.point_plot_canvas.axes.plot(self.vibrometer.freq, self.current_point.frf.data_db)              # Plot the point FRF data in db scale
        self.point_plot_canvas.axes.axvline(self.freq_slider.value(), color='black', linestyle='--')        # Plot vertical line of the current frequency

        
    """Update the average point plot canvas when frequency slider is modified"""
    def update_avg_point_plot(self):
        self.avg_point_plot_canvas.axes.cla()    # Clear the canvas of the old data
        self.current_point_number = self.point_slider.value()
        self.avg_point_plot_canvas.axes.plot(self.vibrometer.freq, self.vibrometer.avg_point.frf.data_db)    # Plot the average FRF data in db scale
        self.avg_point_plot_canvas.axes.axvline(self.freq_slider.value(), color='black', linestyle='--')     # Plot vertical line of the current frequency
        




if __name__ == "__main__":
    app = QApplication(sys.argv)
    graph = Q3DSurface()
    if not graph.hasContext():
        msg_box = QMessageBox()
        msg_box.setText("Couldn't initialize the OpenGL context.")
        msg_box.exec()
        sys.exit(-1)

    window = Window(graph)
    window.setWindowTitle("Surface example")
    window.show()

    sys.exit(app.exec())