# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

"""PySide6 port of the qt3d/simple-cpp example from Qt v5.x"""
import matplotlib
matplotlib.use('Qt5Agg')

import sys
from PySide6.QtCore import (Property, QObject, QPropertyAnimation, Signal, QEasingCurve, QSize, Qt, QTimer)
from PySide6.QtGui import (QMatrix4x4, QQuaternion, QVector3D, QWindow, QFont)
from PySide6.QtWidgets import (QApplication, QMainWindow, QComboBox, QGroupBox, QHBoxLayout, QLabel, QMessageBox, QPushButton, QRadioButton, QSizePolicy, QSlider, QVBoxLayout, QWidget)
from PySide6.Qt3DCore import (Qt3DCore)
from PySide6.Qt3DExtras import (Qt3DExtras)


# Need to import these after PySide6 for some reason
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg, NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure
from matplotlib.colors import ListedColormap
import matplotlib as mpl
import matplotlib.pyplot as plt

import numpy as np
from time import time
import open3d as o3d
import win32gui
from scipy import ndimage

from data_classes import Vibrometer, Point
from extract import load_vibrometer_object
from generate_point_cloud_mesh import AnimationGenerator
from utils import get_nearest_freq_index, get_point_object, get_mesh_numpy_ij

THEMES = ["Qt", "Primary Colors", "Digia", "Stone Moss", "Army Blue", "Retro",
          "Ebony", "Isabelle"]

viridis = mpl.colormaps['viridis'].resampled(8)


class SphereMeshTransformController(QObject):
    """Class to control the mesh data for the animation"""
    def __init__(self, parent=None):
        super().__init__(parent)
        self._target = None
        self._size = 3      # Sphere mesh scale
        
    def setTarget(self, t):
        self._target = t

    def getTarget(self):
        return self._target

    def setMeshSize(self, scale):
        if self._size != scale:
            self._size = scale
            self.updateMeshSize()
            self.sizeChanged.emit()
        
    def getMeshSize(self):
        return self._size
    
    def updateMeshSize(self):
        if self._target is not None:
            self._target.setRadius(self._size)
    
    sizeChanged = Signal()
    mesh_size = Property(float, getMeshSize, setMeshSize, notify=sizeChanged)


class OrbitTransformController(QObject):
    def __init__(self, parent):
        super().__init__(parent)
        self._target = None
        self._matrix = QMatrix4x4()
        self._radius = 1    # Orbit radius
        self._angle = 0     # Orbit angle
        self.last_time = 0
        self.cur_time = time()

    def calcFPS(self):
        self.cur_time = time()
        fps = 1 / (self.cur_time - self.last_time)
        self.last_time = self.cur_time
        return 1/fps

    def setTarget(self, t):
        self._target = t

    def getTarget(self):
        return self._target

    def setRadius(self, radius):
        if self._radius != radius:
            self._radius = radius
            self.updateOrbitMatrix()
            self.radiusChanged.emit()

    def getRadius(self):
        return self._radius

    def setAngle(self, angle):
        #print(f"fps: {self.calcFPS()}")
        if self._angle != angle:
            self._angle = angle
            self.updateOrbitMatrix()
            self.angleChanged.emit()

    def getAngle(self):
        return self._angle

    def updateOrbitMatrix(self):
        self._matrix.setToIdentity()
        self._matrix.rotate(self._angle, QVector3D(0, 1, 0))
        self._matrix.translate(self._radius, 0, 0)
        if self._target is not None:
            self._target.setMatrix(self._matrix)

    angleChanged = Signal()
    radiusChanged = Signal()
    angle = Property(float, getAngle, setAngle, notify=angleChanged)
    radius = Property(float, getRadius, setRadius, notify=radiusChanged)
    
"""Class to create a Matplotlib figure widget for plotting 2d plots in Qt6"""
class MplCanvas(FigureCanvasQTAgg):
    def __init__(self, parent=None, width=5, height=4, dpi=100):
        fig = Figure(figsize=(width, height), dpi=dpi)
        self.axes = fig.add_subplot(111)
        super(MplCanvas, self).__init__(fig)
    
    
"""Create the main window for the application"""
class MainWindow(QMainWindow):
    def __init__(self, vibrometer: Vibrometer, *args, **kwargs):
        print("Initializing application window")
        super(MainWindow, self).__init__(*args, **kwargs)
        self.vibrometer = vibrometer
        self.setMinimumWidth(2000)
        self.setMinimumHeight(900)
        
        # Create a slider for the frequency of the FRF data
        self.freq_slider = QSlider(Qt.Horizontal, self)
        self.freq_slider.setMinimum(1)
        self.freq_slider.setMaximum(self.vibrometer.freq.max())
        self.freq_slider.setTickInterval(self.vibrometer.freq[1]-self.vibrometer.freq[0])
        self.freq_slider.setValue(1000)
        self.freq_slider.setEnabled(True)
        
        # Create a slider for the points in the mesh
        self.point_slider = QSlider(Qt.Horizontal, self)
        self.point_slider.setMinimum(self.vibrometer.measured_points[0])
        self.point_slider.setMaximum(self.vibrometer.measured_points[-1])
        self.point_slider.setTickInterval(1)
        self.point_slider.setEnabled(True)
        
        # Create the Qt3DWindow which will hold the 3D content
        print("Creating the mesh animation")
        self.mesh_frame_generator = AnimationGenerator(self.vibrometer)         # Create the mesh animation generator object
        print(f"target freq: {self.freq_slider.value()}, type: {type(self.freq_slider.value())}")
        self.mesh_frame_generator.generate_frames(float(self.freq_slider.value()))     # Create the frames for the mesh animation
        self.mesh_animation_visualizer = o3d.visualization.Visualizer()         # Create the open3d visualizer object
        self.mesh_animation_visualizer.create_window()                          # Create a window for the visualizer
        #self.mesh_animation_visualizer.add_geometry(self.mesh_frame_generator.animation_frames[0])  # Add the first frame from the mesh animation frame list                         
        hwnd = win32gui.FindWindow(None, "Open3D")                              # Get the window handle for the open3d window
        self.mesh_animation_window = QWindow.fromWinId(hwnd)                         # Create a Qt window from the open3d window handle
        print("Created mesh animation")
        
        # Define color map for colorbar
        gradient = self.mesh_frame_generator.color_gradient
        alphas = np.ones(shape=(len(gradient), 1))
        colors = np.concatenate((gradient, alphas), axis=1)
        self.color_bar_map = ListedColormap(colors, name="color_bar_map")
        
        # Create the container for the Q3DWindow and the UI controls
        print("Creating container for mesh animation")
        self.mesh_animation_widget = QWidget()
        self.mesh_animation_container = QWidget.createWindowContainer(self.mesh_animation_window, self.mesh_animation_widget)
        animation_screen_size = self.mesh_animation_window.size()
        mesh_width_min = animation_screen_size.width() / 2
        mesh_height_min = animation_screen_size.height() / 1.6
        print(animation_screen_size)
        self.mesh_animation_container.setMinimumSize(QSize(w=mesh_width_min, h=mesh_height_min))
        self.mesh_animation_container.setMaximumSize(animation_screen_size)
        self.mesh_animation_container.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.mesh_animation_container.setFocusPolicy(Qt.StrongFocus)
        #self.mesh_animation_container.show()
        print("Created container for mesh animation")
        
        # Create MplCanvas for the point and average points FRF data
        print("Creating widgets for the 2d plots")
        self.point_plot_canvas = MplCanvas(self, width=5, height=4, dpi=80)
        self.point_coherence_axis = self.point_plot_canvas.axes.twinx()
        self.point_plot_toolbar = NavigationToolbar(self.point_plot_canvas, self)
        self.point_plot_widget = QWidget()
        self.point_plot_layout = QVBoxLayout()
        self.point_plot_layout.addWidget(self.point_plot_toolbar)
        self.point_plot_layout.addWidget(self.point_plot_canvas)
        self.point_plot_widget.setLayout(self.point_plot_layout)

        self.avg_point_plot_canvas = MplCanvas(self, width=5, height=4, dpi=80)
        self.avg_point_coherence_axis = self.avg_point_plot_canvas.axes.twinx()
        self.avg_point_plot_toolbar = NavigationToolbar(self.avg_point_plot_canvas, self)
        self.avg_point_plot_widget = QWidget()
        self.avg_point_plot_layout = QVBoxLayout()
        self.avg_point_plot_layout.addWidget(self.avg_point_plot_toolbar)
        self.avg_point_plot_layout.addWidget(self.avg_point_plot_canvas)
        self.avg_point_plot_widget.setLayout(self.avg_point_plot_layout)        
        print("Created widgets for the 2d plots")
        
        # Create color bar for the mesh animation
        print("Creating color bar for mesh animation")
        self.mesh_color_bar_canvas = MplCanvas(self, width=2, height=8, dpi=100)
        self.color_bar_widget = QWidget()
        self.color_bar_layout = QVBoxLayout()
        self.color_bar_layout.addWidget(self.mesh_color_bar_canvas)
        self.color_bar_widget.setLayout(self.color_bar_layout)
        print("Created color bar for mesh animation")
        
        # Create layouts for the widgets and containers
        main_layout = QHBoxLayout()                             # Horizontal layout 
        left_layout = QVBoxLayout(self)                             # Vertical layout for the left side of the window (animation, plots, and sliders)
        left_layout.setAlignment(Qt.AlignTop)                       # Align the left layout to the top of the window

        # Create labels for the sliders
        label_font = QFont()
        label_font.setBold(True)
        label_font.setPointSize(16)
        self.freq_slider_label = QLabel(f"Frequency: {self.freq_slider.value()} (Hz)", self)
        self.freq_slider_label.setFont(label_font)
        self.freq_slider_label.setAlignment(Qt.AlignCenter | Qt.AlignVCenter)
        self.freq_slider_label.setMinimumWidth(80)
        self.point_slider_label = QLabel(f"Point: {self.point_slider.value()}", self)
        self.point_slider_label.setFont(label_font)
        self.point_slider_label.setAlignment(Qt.AlignCenter | Qt.AlignVCenter)
        self.point_slider_label.setMinimumWidth(80)

        # Add widgets to left layout
        left_layout.addWidget(self.point_plot_widget)            # Add the point plot canvas to the left layout
        left_layout.addWidget(self.avg_point_plot_widget)        # Add the average point plot canvas to the left layout
        left_layout.addWidget(self.freq_slider_label)            # Add label to the frequency slider
        left_layout.addWidget(self.freq_slider)                  # Add the frequency slider to the left layout
        left_layout.addWidget(self.point_slider_label)           # Add label to the point slider
        left_layout.addWidget(self.point_slider)                 # Add the point slider to the left layout
        
        # Create main widget
        main_layout.addWidget(self.color_bar_widget)
        main_layout.addWidget(self.mesh_animation_container)
        main_layout.addLayout(left_layout)                       # Add the left layout to the main layout
        main_widget = QWidget()
        main_widget.setLayout(main_layout)
        self.setCentralWidget(main_widget)
        
        # Connect plot functions to the frequency slider
        self.freq_slider.valueChanged.connect(self.update_point_plot)
        self.freq_slider.valueChanged.connect(self.update_avg_point_plot)
        self.freq_slider.valueChanged.connect(self.update_freq_slider_label)
        self.point_slider.valueChanged.connect(self.update_point_plot)
        self.point_slider.valueChanged.connect(self.update_point_slider_label)
        
        # Connect animation disconnect and connect functions to the slider
        self.freq_slider.sliderPressed.connect(self.stop_timer)
        self.freq_slider.sliderReleased.connect(self.regenerate_frames)
        self.freq_slider.sliderReleased.connect(self.update_color_bar)   
        self.freq_slider.sliderReleased.connect(self.start_timer) 
        
        # Create timer for the animation widget
        self.mesh_animation_timer = QTimer(self)
        self.current_frame_number = 0       # Start the count at 1
        self.start_of_animation = True
        self.mesh_animation_timer.timeout.connect(self.update_mesh_animation)
        self.mesh_timer_interval = self.mesh_frame_generator.animation_length_ms/self.mesh_frame_generator.animation_steps
        self.mesh_animation_timer.start(self.mesh_timer_interval)
           
        # Populate the matplotlib plots
        self.update_point_plot()
        self.update_avg_point_plot()
        self.update_freq_slider_label(self.freq_slider.value())
        self.update_point_slider_label(self.point_slider.value())
        self.stop_timer()
        self.regenerate_frames()
        self.update_color_bar()
        self.start_timer()
        
    """Update the label on the freq slider"""
    def update_freq_slider_label(self, value):
        self.freq_slider_label.setText(f"Frequency: {value} (Hz)")
    
    """Update the label on the point slider"""
    def update_point_slider_label(self, value):
        self.point_slider_label.setText(f"Point: {value}")
        
    """Stop the timer"""
    def stop_timer(self):
        self.camera_view = self.mesh_animation_visualizer.get_view_control().convert_to_pinhole_camera_parameters()
        self.mesh_animation_timer.stop()
        
    """Start the timer"""
    def start_timer(self):
        self.mesh_animation_timer.start(self.mesh_timer_interval)
        self.mesh_animation_visualizer.get_view_control().convert_from_pinhole_camera_parameters(self.camera_view)
        
    """Update the color bar"""
    def update_color_bar(self):
        self.mesh_color_bar_canvas.axes.clear()
        x, y = np.abs(self.mesh_frame_generator.z_min), np.abs(self.mesh_frame_generator.z_max)
        a = np.array([[x, y],[x*1000, y*1000]])
        #a = np.rot90(a, 2)
        img = self.mesh_color_bar_canvas.axes.imshow(a, interpolation='nearest', cmap=self.color_bar_map)
        #self.mesh_color_bar_canvas.axes.set_visible(False)
        cax = self.mesh_color_bar_canvas.axes
        cbar = plt.colorbar(mappable=img, cax=cax, label='(m/s)/(V) x10e-3', aspect=10)
        cbar.set_ticks(np.linspace(x*1000, y*1000, 11, endpoint=True))
        cbar.draw_all()
        self.mesh_color_bar_canvas.draw()
        
    """Update the point plot canvas when the frequency slider or point slider is modified"""
    def update_point_plot(self):
        self.point_plot_canvas.axes.clear()       # Clear the canvas of the old data
        self.point_coherence_axis.axes.clear()
        self.current_point_number = self.point_slider.value()
        self.current_point = get_point_object(self.vibrometer.points, self.current_point_number)            # Get the current point object given slider value
        self.point_plot_canvas.axes.grid(True)
        self.point_plot_canvas.axes.set_title("Point " + str(self.current_point_number))
        self.point_plot_canvas.axes.set_xlabel("Frequency (Hz)")
        self.point_plot_canvas.axes.set_ylabel("FRF Amplitude (dB)")
        #self.point_plot_canvas.axes.set_yscale('symlog')
        
        
        self.point_coherence_axis.axes.plot(self.vibrometer.freq, self.vibrometer.avg_point.coherence.data, color='blue', alpha=0.5, linewidth=0.5)
        self.point_coherence_axis.axes.set_ylabel(f"Coherence")
        self.point_plot_canvas.axes.plot(self.vibrometer.freq, self.current_point.frf.data_db, color='red', linewidth=0.5)              # Plot the point FRF data in db scale
        self.point_plot_canvas.axes.axvline(self.freq_slider.value(), color='black', linestyle='--', linewidth=1.0)        # Plot vertical line of the current frequency
        self.point_plot_canvas.draw()
        
    """Update the average point plot canvas when frequency slider is modified"""
    def update_avg_point_plot(self):
        self.avg_point_plot_canvas.axes.clear()    # Clear the canvas of the old data
        self.avg_point_coherence_axis.axes.clear()
        self.current_point_number = self.point_slider.value()
        self.avg_point_plot_canvas.axes.grid(True)
        self.avg_point_plot_canvas.axes.set_title("Average of Points")
        self.avg_point_plot_canvas.axes.set_xlabel(f"Frequency (Hz)")
        self.avg_point_plot_canvas.axes.set_ylabel(f"FRF (dB)")
        #self.avg_point_plot_canvas.axes.set_yscale('symlog')
        self.avg_point_coherence_axis.axes.plot(self.vibrometer.freq, self.vibrometer.avg_point.coherence.data, color='blue', alpha=0.5, linewidth=0.5)
        self.avg_point_coherence_axis.axes.set_ylabel(f"Coherence")
        self.avg_point_plot_canvas.axes.plot(self.vibrometer.freq, self.vibrometer.avg_point.frf.data_db, color='red', linewidth=0.5)    # Plot the average FRF data in db scale
        self.avg_point_plot_canvas.axes.axvline(self.freq_slider.value(), color='black', linestyle='--', linewidth=1.0)     # Plot vertical line of the current frequency
        self.avg_point_plot_canvas.draw()
    
    """Regenerate the frames if the freq slider changes"""
    def regenerate_frames(self):
        # Clear the window of all meshes
        self.mesh_animation_visualizer.clear_geometries()    
        # Update the renderer
        self.mesh_animation_visualizer.poll_events()
        self.mesh_animation_visualizer.update_renderer()
        #self.start_of_animation = True
        # Generate the new frames
        self.mesh_frame_generator.generate_frames(float(self.freq_slider.value()))
        self.regen_frames = True

        
    """Update the mesh animation when the frequency slider is modified"""
    def update_mesh_animation(self):
        #print(f"updating mesh {self.current_frame_number}")

        # Save the camera view
        if not self.regen_frames:
            self.camera_view = self.mesh_animation_visualizer.get_view_control().convert_to_pinhole_camera_parameters()
        else:
            self.regen_frames = False

        # Set mesh and update the geometry
        # for mesh in self.mesh_frame_generator.animation_frames:
        #     self.mesh_animation_visualizer.remove_geometry(mesh)
        self.mesh_animation_visualizer.clear_geometries()
        self.mesh_animation_visualizer.add_geometry(self.mesh_frame_generator.animation_frames[self.current_frame_number])
        for mesh in self.mesh_frame_generator.animation_frames:
            self.mesh_animation_visualizer.update_geometry(mesh)
        
        # Set camera view and update renderer
        if not self.start_of_animation:
            self.mesh_animation_visualizer.get_view_control().convert_from_pinhole_camera_parameters(self.camera_view)
        else:
            self.start_of_animation = False
        
        #self.mesh_animation_visualizer.get_view_control().convert_from_pinhole_camera_parameters(camera_view)
        self.mesh_animation_visualizer.get_render_option().mesh_show_back_face = True
        self.mesh_animation_visualizer.get_render_option().mesh_show_wireframe = True
        self.mesh_animation_visualizer.get_render_option().light_on = True
        self.mesh_animation_visualizer.get_render_option().background_color = np.asarray([0.1, 0.1, 0.1])
        self.mesh_animation_visualizer.get_render_option().mesh_shade_option = o3d.visualization.MeshShadeOption.Color
        self.mesh_animation_visualizer.get_render_option().show_coordinate_frame = True
        #self.mesh_animation_visualizer.get_render_option().ground_plane_visibility = True
        self.mesh_animation_visualizer.poll_events()
        self.mesh_animation_visualizer.update_renderer()
        
        # Set current frame num, if over the number of frames, loop back to 0
        # self.current_frame_number += 1
        # if self.current_frame_number == self.mesh_frame_generator.animation_steps:
        #     self.current_frame_number = 0
        self.current_frame_number = (self.current_frame_number + 1) % self.mesh_frame_generator.animation_steps
        #print(self.current_frame_number)
        

# """Controller for the 3D mesh surface"""
# class SurfaceController(QObject):
#     def __init__(self, parent=None):
#         super().__init__(parent)
#         self._target = None
#         self.step_number = 0
        
#     def set_target(self, t):
#         self._target = t

#     def get_target(self):
#         return self._target

#     def set_step_number(self, step):
#         if self._step != step:
#             self._step = step
#             self.update_surface_mesh()
#             self.step_number_changed.emit()
        
#     def get_step_number(self):
#         return self._step
    
#     def update_surface_mesh(self):
#         if self._target is not None:
#             self._target.setRadius(self._size)
    
#     step_number_changed = Signal()
#     mesh_at_step = Property(int, set_step_number, get_step_number, notify=step_number_changed)

# """Create a window to display the 3D mesh animation"""
# class MeshAnimationWindow(Qt3DExtras.Qt3DWindow):
#     def __init__(self, mesh: o3d.geometry.TriangleMesh, parent=None):
#         super(MeshAnimationWindow, self).__init__()
#         self.vibrometer = vibrometer
#         self.main_window = parent
#         self.setWidth(2560)
#         self.setHeight(1440)

#         # Camera
#         self.camera().lens().setPerspectiveProjection(45, 16 / 9, 0.1, 1000)
#         self.camera().setPosition(QVector3D(0, 0, 40))
#         self.camera().setViewCenter(QVector3D(0, 0, 0))

#         # For camera controls
#         self.create_3D_scene()
#         self.camController = Qt3DExtras.QOrbitCameraController(self.rootEntity)
#         self.camController.setLinearSpeed(50)
#         self.camController.setLookSpeed(180)
#         self.camController.setCamera(self.camera())
#         self.setRootEntity(self.rootEntity)
        
#         # Create the animation frames list
#         self.animation_steps = 30
#         self.animation_length_ms = 5000.0
#         frame_generator = AnimationGenerator(vibrometer=self.vibrometer, 
#                                              target_frequency=self.)
#         self.animation_frames = frame_generator.animation_frames

#     def create_3D_scene(self):
#         # Root entity
#         self.rootEntity = Qt3DCore.QEntity()

#         # Material
#         self.material = Qt3DExtras.QPhongMaterial(self.rootEntity)

#         # Mesh
#         self.surface_entity = Qt3DCore.QEntity(self.rootEntity)
#         self.surface_mesh = Qt3DExtras.QCuboidMesh()
#         #self.surface_mesh.
        
        
#         # Mesh transform
#         self.surface_transform = Qt3DCore.QTransform()
#         self.surface_transform
        
        
#         self.surface_entity.addComponent(self.surface_mesh)
#         self.surface_entity.addComponent(self.surface_transform)
#         self.surface_entity.addComponent(self.material)
        
#         # Mesh controller
#         self.surface_controller = SurfaceController(self.surface_transform)
#         self.surface_controller.set_target(self.surface_mesh)

#         # Torus
#         # self.torusEntity = Qt3DCore.QEntity(self.rootEntity)
#         # self.torusMesh = Qt3DExtras.QTorusMesh()
#         # self.torusMesh.setRadius(5)
#         # self.torusMesh.setMinorRadius(1)
#         # self.torusMesh.setRings(100)
#         # self.torusMesh.setSlices(20)

#         # self.torusTransform = Qt3DCore.QTransform()
#         # self.torusTransform.setScale3D(QVector3D(1.5, 1, 0.5))
#         # self.torusTransform.setRotation(QQuaternion.fromAxisAndAngle(QVector3D(1, 0, 0), 45))

#         # self.torusEntity.addComponent(self.torusMesh)
#         # self.torusEntity.addComponent(self.torusTransform)
#         # self.torusEntity.addComponent(self.material)

#         # Sphere Mesh and controller
#         self.sphereEntity = Qt3DCore.QEntity(self.rootEntity)
#         self.sphereMesh = Qt3DExtras.QSphereMesh()
#         self.sphereMesh.setRadius(3)
#         self.sphereMeshTransform = Qt3DCore.QTransform()
#         self.sphereMesh_controller = SphereMeshTransformController(self.sphereMeshTransform)
#         self.sphereMesh_controller.setTarget(self.sphereMesh)
#         self.sphereMesh_controller.setMeshSize(1)

#         # Orbit controller
#         self.sphereTransform = Qt3DCore.QTransform()
#         self.controller = OrbitTransformController(self.sphereTransform)
#         self.controller.setTarget(self.sphereTransform)
#         self.controller.setRadius(20)

#         # Orbit animation
#         self.sphereRotateTransformAnimation = QPropertyAnimation(self.sphereTransform)
#         self.sphereRotateTransformAnimation.setTargetObject(self.controller)
#         self.sphereRotateTransformAnimation.setPropertyName(b"angle")
#         self.sphereRotateTransformAnimation.setStartValue(0)
#         self.sphereRotateTransformAnimation.setEndValue(360)
#         self.sphereRotateTransformAnimation.setDuration(10000)
#         self.sphereRotateTransformAnimation.setLoopCount(-1)
#         self.sphereRotateTransformAnimation.start()
        
#         # Sphere mesh size animation
#         self.sphereSizeTransformAnimation = QPropertyAnimation(self.sphereMeshTransform)
#         self.sphereSizeTransformAnimation.setTargetObject(self.sphereMesh_controller)
#         self.sphereSizeTransformAnimation.setPropertyName(b"mesh_size")
#         self.sphereSizeTransformAnimation.setEasingCurve(QEasingCurve.Linear)
#         self.sphereSizeTransformAnimation.setStartValue(1)
#         self.sphereSizeTransformAnimation.setEndValue(10)
#         self.sphereSizeTransformAnimation.setDuration(10000)
#         self.sphereSizeTransformAnimation.setLoopCount(-1)
#         self.sphereSizeTransformAnimation.start()
#         #self.sphereSizeTransformAnimation.emit()


#         # Combine the components of the sphere entity
#         self.sphereEntity.addComponent(self.sphereMesh)
#         self.sphereEntity.addComponent(self.sphereMeshTransform)
#         self.sphereEntity.addComponent(self.sphereTransform)
#         self.sphereEntity.addComponent(self.material)
        
#     def create_mesh_key_values(self, total_duration_ms: int=1000, num_steps: int=100, zmax_amplitude: float=1.0):
        
#         pass
        
        

# """Function to create the time series amplitude for a particular point and for a particular frequency"""
# def velocity_animation_function(total_duration_ms: int, num_steps: int, zmax_amplitude: float) -> np.ndarray:
#     # Create the z(t) array for given point
#         t = np.linspace(0, total_duration_ms, num_steps)
#         z_real = zmax_amplitude.real
#         z_imag = zmax_amplitude.imag
#         z_shift = 0
#         period = total_duration_ms
#         z_t = z_real*np.cos(period*(t + z_imag)) + z_shift
#         return z_t

if __name__ == '__main__':
    vibrometer = load_vibrometer_object()
    app = QApplication(sys.argv)
    view = MainWindow(vibrometer)
    view.show()
    app.exec()