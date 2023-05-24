# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

"""PySide6 port of the qt3d/simple-cpp example from Qt v5.x"""
from logging import Logger
from math import pi
from turtle import left
import matplotlib
matplotlib.use('Qt5Agg')

import sys
from enum import IntEnum, auto
from dataclasses import dataclass
from PySide6.QtCore import (Property, QObject, QPropertyAnimation, Signal, QEasingCurve, QSize, Qt, QTimer, Slot, QRect, QSize)
from PySide6.QtGui import (QMatrix4x4, QQuaternion, QVector3D, QWindow, QFont)
from PySide6.QtWidgets import (QMainWindow, QFrame, QHBoxLayout, QLabel, QPushButton, QSizePolicy, QSlider, QVBoxLayout, QWidget, QLayout, QLayoutItem, QTextBrowser, QWidgetItem)
from PySide6.Qt3DCore import (Qt3DCore)
from PySide6.Qt3DExtras import (Qt3DExtras)


# Need to import these after PySide6 for some reason
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg, NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure
from matplotlib.colors import ListedColormap
from matplotlib.offsetbox import AnchoredText
import matplotlib as mpl
import matplotlib.pyplot as plt

from borderlayout import *
import numpy as np
from time import time
import open3d as o3d
import win32gui
from scipy import ndimage
from datetime import datetime
import os
from multiprocessing import Process
from threading import Thread

# from utils import get_nearest_freq_index, get_point_object

THEMES = ["Qt", "Primary Colors", "Digia", "Stone Moss", "Army Blue", "Retro",
          "Ebony", "Isabelle"]

viridis = mpl.colormaps['viridis'].resampled(8)



""" These are the limits to sliders and other variables """
# -------------------- Piezo attributes --------------------
# Frequency
piezo_min_freq = 10
piezo_max_freq = 5000
piezo_freq_step = 5
piezo_default_freq = 500
# Amplitude
piezo_min_amp = 0.0
piezo_max_amp = 50.0
piezo_amp_step = 0.5
piezo_default_amp = 5.0
# Phase
piezo_min_phase = 0.0
piezo_max_phase = 360.0
piezo_phase_step = 45/2
piezo_default_phase = 0.0
# Enable
piezo_1_enable = False
piezo_2_enable = False

# -------------------- Rope Heater attributes --------------------
# Temperature
rope_min_temp = 20
rope_max_temp = 60
rope_temp_step = 0.5
rope_default_temp = 50
rope_heat_enable = False

# -------------------- Heater Block attributes --------------------
# Temperature
Heater_block_max_temp = 160     # If this is ever reached, the heater block should be turned off
# Heat flux
heat_flux_min = 0
heat_flux_max = 30
heat_flux_step = 0.1
heat_block_enable = False

# -------------------- GUI attributes --------------------
min_width_slider = 30

"""Class to encapsulate slider and label"""
class CentralWidget(QWidget):
    def __init__(self, *args):
        super(CentralWidget, self).__init__(*args)


"""Class to create a Matplotlib figure widget for plotting 2d plots in Qt6"""
class MplCanvas(FigureCanvasQTAgg):
    def __init__(self, parent=None, width=5, height=4, dpi=100):
        mpl.rcParams['toolbar'] = 'None'
        fig = Figure(figsize=(width, height), dpi=dpi)
        self.axes = fig.add_subplot(111)
        super(MplCanvas, self).__init__(fig)
    
"""List of values for slider class"""
class ListSlider(QSlider):
    elementChanged = Signal(int)

    def __init__(self, values: list[int], *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.setMinimum(0)
        self._values = []       # list of index range of values
        self.valueChanged.connect(self._on_value_changed)
        self.values = values

    """Return the slider values when calling slider.values()"""
    @property
    def values(self):
        return self._values

    """When calling slider.point_number, return the value from the list of slider values"""
    @property
    def point_number(self):
        return self.values[self.sliderPosition()]

    """Set the values of the slider"""
    @values.setter
    def values(self, values: list[int]):
        self._values = values
        # minimum = 1
        maximum = max(0, len(self._values))
        #print(f"max: {maximum}")
        self.setMaximum(maximum)
        self.setValue(0)
    
    @Slot(int)
    def _on_value_changed(self, index):
        slider_value = self.values[index]
        #print(f"point slider value: {slider_value}, index: {index}")
        self.elementChanged.emit(slider_value)
        
    
    
"""Create the main window for the application"""
class MainWindow(QMainWindow):
    def __init__(self, window_title: str, log: Logger, *args, **kwargs):
        print("Initializing application window")
        super(MainWindow, self).__init__(*args, **kwargs)
        self.log = log
        
        self.log.info("Initializing application window")
        self.setWindowTitle(window_title)
        self.window_title = window_title
        self.setMinimumWidth(2000)
        self.setMinimumHeight(900)
        label_font = QFont()
        label_font.setBold(True)
        label_font.setPointSize(16)
        
        # Button size policy
        button_size_policy = QSizePolicy(QSizePolicy.Preferred, QSizePolicy.Preferred)
        button_size_policy.setVerticalStretch(1)
        button_size_policy.setHeightForWidth(True)
        
        # Button stype
        button_style =   """QPushButton { 
                                font-size: 18px;
                                font-weight: bold;
                                color: black;
                                background-color: light grey; 
                                border-width: 2px; 
                                border-radius: 10px; 
                                border-color: dark grey; 
                                border-style: outset;
                                padding: 20px; 
                            }
                            QPushButton:checked { 
                                background-color: green; 
                                border-style: inset; 
                            }""" 
                                
        # Widget border style
        widget_border_style = """QLayoutItem {
                                    background-color: light grey;
                                    border-width: 2px;
                                    border-radius: 5px;
                                    border-color: black;
                                    border-style: outset;
                                    padding: 10px; 
                                }"""
  
        # Slider style                   
        slider_style =   """QSlider::handle:horizontal {
                                background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #b4b4b4, stop:1 #8f8f8f);
                                border: 1px solid #5c5c5c;
                                width: 18px;
                                margin: -2px 0; /* handle is placed by default on the contents rect of the groove. Expand outside the groove */
                                border-radius: 3px;
                            }"""
                                    
        # style_file ="style.css"
        # self.styles = None
        # with open(style_file, "r") as fh:
        #     self.styles = fh.read()
        #     print("reading css")
        #     print(self.styles)
        # print(self.styles[0])
                                    
                            
        
        # -------------------------------- PIEZO 1 ELEMENTS --------------------------------
        self.log.debug("Creating piezo 1 elements")
        # Create a slider for the frequency of the piezo 1
        self.piezo_1_freq_slider = QSlider(Qt.Horizontal, self)
        self.piezo_1_freq_slider.setMinimum(piezo_min_freq)
        self.piezo_1_freq_slider.setMaximum(piezo_max_freq)
        self.piezo_1_freq_slider.setTickInterval(piezo_freq_step)
        self.piezo_1_freq_slider.setValue(piezo_default_freq)
        self.piezo_1_freq_slider.setEnabled(True)
        self.piezo_1_freq_slider_label = QLabel(f"Frequency: {self.piezo_1_freq_slider.value} (Hz)", self)
        piezo_1_freq_layout = QVBoxLayout()
        piezo_1_freq_layout.addWidget(self.piezo_1_freq_slider_label)
        piezo_1_freq_layout.addWidget(self.piezo_1_freq_slider)
        piezo_1_freq_layout.setAlignment(Qt.AlignCenter | Qt.AlignVCenter)
        piezo_1_freq_layout.setStretch(0, 0)
        self.piezo_1_freq_widget = CentralWidget()
        self.piezo_1_freq_widget.setLayout(piezo_1_freq_layout)
        
        
        # Create a slider for the amplitude of the piezo 1
        self.piezo_1_amp_slider = QSlider(Qt.Horizontal, self)
        self.piezo_1_amp_slider.setMinimum(piezo_min_amp)
        self.piezo_1_amp_slider.setMaximum(piezo_max_amp)
        self.piezo_1_amp_slider.setTickInterval(piezo_amp_step)
        self.piezo_1_amp_slider.setValue(piezo_default_amp)
        self.piezo_1_amp_slider.setEnabled(True)
        self.piezo_1_amp_slider_label = QLabel(f"Amplitude: {self.piezo_1_amp_slider.value} (V)", self)
        piezo_1_amp_layout = QVBoxLayout()
        piezo_1_amp_layout.addWidget(self.piezo_1_amp_slider_label)
        piezo_1_amp_layout.addWidget(self.piezo_1_amp_slider)
        piezo_1_amp_layout.setAlignment(Qt.AlignCenter | Qt.AlignVCenter)
        piezo_1_amp_layout.setStretch(0, 0)
        self.piezo_1_amp_widget = CentralWidget()
        self.piezo_1_amp_widget.setLayout(piezo_1_amp_layout)
                
        # Create a slider for the phase of the piezo 1
        self.piezo_1_phase_slider = QSlider(Qt.Horizontal, self)
        self.piezo_1_phase_slider.setMinimum(piezo_min_phase)
        self.piezo_1_phase_slider.setMaximum(piezo_max_phase)
        self.piezo_1_phase_slider.setTickInterval(piezo_phase_step)
        self.piezo_1_phase_slider.setValue(piezo_default_phase)
        self.piezo_1_phase_slider.setEnabled(True)
        self.piezo_1_phase_slider_label = QLabel(f"Phase: {self.piezo_1_phase_slider.value} (deg)", self)
        piezo_1_phase_layout = QVBoxLayout()
        piezo_1_phase_layout.addWidget(self.piezo_1_phase_slider_label)
        piezo_1_phase_layout.addWidget(self.piezo_1_phase_slider)
        piezo_1_phase_layout.setAlignment(Qt.AlignCenter | Qt.AlignVCenter)
        piezo_1_phase_layout.setStretch(0, 0)
        self.piezo_1_phase_widget = CentralWidget()
        self.piezo_1_phase_widget.setLayout(piezo_1_phase_layout)
        
        # Create a button for enabling the piezo 1
        self.piezo_1_enable_button = QPushButton("Enable", self)
        self.piezo_1_enable_button.setCheckable(True)
        self.piezo_1_enable_button.setChecked(piezo_1_enable)
        self.piezo_1_enable_button.setEnabled(True)
        self.piezo_1_enable_button.setSizePolicy(button_size_policy)
        self.piezo_1_enable_button.setStyleSheet(button_style)
        
        
        # -------------------------------- PIEZO 2 ELEMENTS --------------------------------
        self.log.debug("Creating piezo 2 elements")
        # Create a slider for the frequency of the piezo 2
        self.piezo_2_freq_slider = QSlider(Qt.Horizontal, self)
        self.piezo_2_freq_slider.setMinimum(piezo_min_freq)
        self.piezo_2_freq_slider.setMaximum(piezo_max_freq)
        self.piezo_2_freq_slider.setTickInterval(piezo_freq_step)
        self.piezo_2_freq_slider.setValue(piezo_default_freq)
        self.piezo_2_freq_slider.setEnabled(True)
        self.piezo_2_freq_slider_label = QLabel(f"Frequency: {self.piezo_2_freq_slider.value} (Hz)", self)
        piezo_2_freq_layout = QVBoxLayout()
        piezo_2_freq_layout.addWidget(self.piezo_2_freq_slider_label)
        piezo_2_freq_layout.addWidget(self.piezo_2_freq_slider)
        piezo_2_freq_layout.setAlignment(Qt.AlignCenter | Qt.AlignVCenter)
        piezo_2_freq_layout.setStretch(0, 0)
        self.piezo_2_freq_widget = CentralWidget()
        self.piezo_2_freq_widget.setLayout(piezo_2_freq_layout)        
        
        # Create a slider for the amplitude of the piezo 2
        self.piezo_2_amp_slider = QSlider(Qt.Horizontal, self)
        self.piezo_2_amp_slider.setMinimum(piezo_min_amp)
        self.piezo_2_amp_slider.setMaximum(piezo_max_amp)
        self.piezo_2_amp_slider.setTickInterval(piezo_amp_step)
        self.piezo_2_amp_slider.setValue(piezo_default_amp)
        self.piezo_2_amp_slider.setEnabled(True)
        self.piezo_2_amp_slider_label = QLabel(f"Amplitude: {self.piezo_2_amp_slider.value} (V)", self)
        piezo_2_amp_layout = QVBoxLayout()
        piezo_2_amp_layout.addWidget(self.piezo_2_amp_slider_label)
        piezo_2_amp_layout.addWidget(self.piezo_2_amp_slider)
        piezo_2_amp_layout.setAlignment(Qt.AlignCenter | Qt.AlignVCenter)
        piezo_2_amp_layout.setStretch(0, 0)
        self.piezo_2_amp_widget = CentralWidget()
        self.piezo_2_amp_widget.setLayout(piezo_2_amp_layout)
        
        # Create a slider for the phase of the piezo 2
        self.piezo_2_phase_slider = QSlider(Qt.Horizontal, self)
        self.piezo_2_phase_slider.setMinimum(piezo_min_phase)
        self.piezo_2_phase_slider.setMaximum(piezo_max_phase)
        self.piezo_2_phase_slider.setTickInterval(piezo_phase_step)
        self.piezo_2_phase_slider.setValue(piezo_default_phase)
        self.piezo_2_phase_slider.setEnabled(True)
        self.piezo_2_phase_slider_label = QLabel(f"Phase: {self.piezo_2_phase_slider.value} (deg)", self)
        piezo_2_phase_layout = QVBoxLayout()
        piezo_2_phase_layout.addWidget(self.piezo_2_phase_slider_label)
        piezo_2_phase_layout.addWidget(self.piezo_2_phase_slider)
        piezo_2_phase_layout.setAlignment(Qt.AlignCenter | Qt.AlignVCenter)
        piezo_2_phase_layout.setStretch(0, 0)
        self.piezo_2_phase_widget = CentralWidget()
        self.piezo_2_phase_widget.setLayout(piezo_2_phase_layout)
        
        # Create a button for enabling the piezo 2
        self.piezo_2_enable_button = QPushButton("Enable Rope Heater", self)
        self.piezo_2_enable_button.setCheckable(True)
        self.piezo_2_enable_button.setChecked(piezo_2_enable)
        self.piezo_2_enable_button.setEnabled(True)
        self.piezo_2_enable_button.setSizePolicy(button_size_policy)
        self.piezo_2_enable_button.setStyleSheet(button_style)
        
        # -------------------------------- ROPE HEATER ELEMENTS --------------------------------
        self.log.debug("Creating rope heater elements")
        # Create a slider for the temperature of the inlet fluid
        self.rope_temp_slider = QSlider(Qt.Horizontal, self)
        self.rope_temp_slider.setMinimum(rope_min_temp)
        self.rope_temp_slider.setMaximum(rope_max_temp)
        self.rope_temp_slider.setTickInterval(rope_temp_step)
        self.rope_temp_slider.setValue(rope_default_temp)
        self.rope_temp_slider.setEnabled(True)
        self.rope_temp_slider_label = QLabel(f"Temperature: {self.rope_temp_slider.value} (°C)", self)
        rope_temp_layout = QVBoxLayout()
        rope_temp_layout.addWidget(self.rope_temp_slider_label)
        rope_temp_layout.addWidget(self.rope_temp_slider)
        rope_temp_layout.setAlignment(Qt.AlignCenter | Qt.AlignVCenter)
        rope_temp_layout.setStretch(0, 0)
        self.rope_temp_widget = CentralWidget()
        self.rope_temp_widget.setLayout(rope_temp_layout)
        
        # Create a button for enabling the rope heater
        self.rope_enable_button = QPushButton("Enable Rope Heater", self)
        self.rope_enable_button.setCheckable(True)
        self.rope_enable_button.setChecked(rope_heat_enable)
        self.rope_enable_button.setEnabled(True)
        self.rope_enable_button.setSizePolicy(button_size_policy)
        self.rope_enable_button.setStyleSheet(button_style)
        
        # -------------------------------- HEATER BLOCK ELEMENTS --------------------------------
        self.log.debug("Creating heater block elements")
        # Create a slider for the heat flux of the heater block
        self.heat_flux_slider = QSlider(Qt.Horizontal, self)
        self.heat_flux_slider.setMinimum(heat_flux_min)
        self.heat_flux_slider.setMaximum(heat_flux_max)
        self.heat_flux_slider.setTickInterval(heat_flux_step)
        self.heat_flux_slider.setValue(0)
        self.heat_flux_slider.setEnabled(True)
        self.heat_flux_slider_label = QLabel(f"Heat Flux: {self.heat_flux_slider.value} (W/m²)", self)
        heat_flux_layout = QVBoxLayout()
        heat_flux_layout.addWidget(self.heat_flux_slider_label)
        heat_flux_layout.addWidget(self.heat_flux_slider)
        heat_flux_layout.setAlignment(Qt.AlignCenter | Qt.AlignVCenter)
        heat_flux_layout.setStretch(0, 0)
        self.heat_flux_widget = CentralWidget()
        self.heat_flux_widget.setLayout(heat_flux_layout)
        
        # Create a button for enabling the heater block
        self.heater_block_enable_button = QPushButton("Enable Heater Block", self)
        self.heater_block_enable_button.setCheckable(True)
        self.heater_block_enable_button.setChecked(heat_block_enable)
        self.heater_block_enable_button.setEnabled(True)
        self.heater_block_enable_button.setSizePolicy(button_size_policy)
        self.heater_block_enable_button.setStyleSheet(button_style)
        
        # -------------------------------- PLOT ANIMATION ELEMENTS --------------------------------
        self.log.debug("Creating plot animation elements")
        # Create MplCanvas for the thermistor temperature data
        self.thermistor_plot_canvas = MplCanvas(self, width=8, height=4, dpi=80)
        self.thermistor_plot_toolbar = NavigationToolbar(self.thermistor_plot_canvas, self)
        self.thermistor_plot_widget = QWidget()
        self.thermistor_plot_layout = QVBoxLayout()
        self.thermistor_plot_layout.addWidget(self.thermistor_plot_toolbar)
        self.thermistor_plot_layout.addWidget(self.thermistor_plot_canvas)
        self.thermistor_plot_widget.setLayout(self.thermistor_plot_layout)
        
        # Create MplCanvas for the flow sensor data
        self.flow_sensor_plot_canvas = MplCanvas(self, width=8, height=4, dpi=80)
        self.flow_sensor_plot_toolbar = NavigationToolbar(self.flow_sensor_plot_canvas, self)
        self.flow_sensor_plot_widget = QWidget()
        self.flow_sensor_plot_layout = QVBoxLayout()
        self.flow_sensor_plot_layout.addWidget(self.flow_sensor_plot_toolbar)
        self.flow_sensor_plot_layout.addWidget(self.flow_sensor_plot_canvas)
        self.flow_sensor_plot_widget.setLayout(self.flow_sensor_plot_layout)
        
        # -------------------------------- OTHER ELEMENTS --------------------------------
        self.log.debug("Creating other elements")
        # Creata a button to send the data to the Teensy
        self.send_data_button = QPushButton("Send Data", self)
        self.send_data_button.setEnabled(False)
        self.send_data_button.setChecked(False)
        self.send_data_button.setCheckable(True)
        self.send_data_button.setSizePolicy(button_size_policy)
        self.send_data_button.setStyleSheet(button_style)

        # Create a guage for the heater block temperature
        self.heater_block_temp_guage = QSlider(Qt.Horizontal, self)
        self.heater_block_temp_guage.setMinimum(20)
        self.heater_block_temp_guage.setMaximum(Heater_block_max_temp)
        self.heater_block_temp_guage.setTickInterval(1)
        self.heater_block_temp_guage.setValue(20)
        self.heater_block_temp_guage.setEnabled(False)
        self.heater_block_temp_guage_label = QLabel(f"Heat Block Temp: {self.heater_block_temp_guage.value} (°C)", self)
        heater_block_temp_layout = QVBoxLayout()
        heater_block_temp_layout.addWidget(self.heater_block_temp_guage_label)
        heater_block_temp_layout.addWidget(self.heater_block_temp_guage)
        heater_block_temp_layout.setAlignment(Qt.AlignCenter | Qt.AlignVCenter)
        heater_block_temp_layout.setStretch(0, 0)
        self.heater_block_temp_widget = CentralWidget()
        self.heater_block_temp_widget.setLayout(heater_block_temp_layout)

        
        # Craete an emergency stop button for the heaters and stop the recording on teensy
        self.emergency_stop_button = QPushButton("Emergency Stop", self)
        self.emergency_stop_button.setEnabled(False)
        self.emergency_stop_button.setCheckable(True)    
        self.emergency_stop_button.setChecked(False)   
        self.emergency_stop_button.setSizePolicy(button_size_policy) 
        self.emergency_stop_button.setStyleSheet(button_style)
    
        # -------------------------------- LAYOUTS --------------------------------
        # Have 3 columns, 
        # Left column is the sliders for piezos, heat flux, and inlet temp
        # Middle column is where the 3 plots will be, stacked 2 high
        # Right column is the buttons for enabling the piezos, rope heater, and heater block, and the send data and emergency stop buttons
        self.log.debug("Creating layouts")
        
        # Create layouts for the widgets and containers
        main_layout = QHBoxLayout()                             # Horizontal layout for columns 
        left_layout = QVBoxLayout()                             # Vertical layout for the left side of the window input sliders
        middle_layout = QVBoxLayout()                           # Vertical layout for the midle plots
        right_layout = QVBoxLayout()                            # Vertical layout for the right side of the window (buttons/monitors)

        # Add widgets to left layout
        self.log.debug("Adding widgets to left layout")
        # left_layout.addWidget(self.piezo_1_freq_slider_label)       # Add label to the piezo 1 frequency slider
        # left_layout.addWidget(self.piezo_1_freq_slider)             # Add the piezo 1 frequency slider to the left layout
        # left_layout.addWidget(self.piezo_1_freq_widget)
        # left_layout.addWidget(self.piezo_1_amp_slider_label)        # Add label to the piezo 1 amplitude slider
        # left_layout.addWidget(self.piezo_1_amp_slider)              # Add the piezo 1 amplitude slider to the left layout
        # left_layout.addWidget(self.piezo_1_phase_slider_label)      # Add label to the piezo 1 phase slider
        # left_layout.addWidget(self.piezo_1_phase_slider)            # Add the piezo 1 phase slider to the left layout
        # left_layout.addWidget(self.piezo_1_enable_button)           # Add the piezo 1 enable button to the left layout
        left_layout.addWidget(self.piezo_1_freq_widget)
        left_layout.addWidget(self.piezo_1_amp_widget)
        left_layout.addWidget(self.piezo_1_phase_widget)
        left_layout.addWidget(self.piezo_1_enable_button)
        left_layout.addWidget(self.piezo_2_freq_widget)
        left_layout.addWidget(self.piezo_2_amp_widget)
        left_layout.addWidget(self.piezo_2_phase_widget)
        left_layout.addWidget(self.piezo_2_enable_button)
        left_layout.addWidget(self.rope_temp_widget)
        left_layout.addWidget(self.heater_block_temp_widget)
        
        # left_layout.addWidget(self.piezo_2_freq_slider_label)       # Add label to the piezo 2 frequency slider
        # left_layout.addWidget(self.piezo_2_freq_slider)             # Add the piezo 2 frequency slider to the left layout
        # left_layout.addWidget(self.piezo_2_amp_slider_label)        # Add label to the piezo 2 amplitude slider
        # left_layout.addWidget(self.piezo_2_amp_slider)              # Add the piezo 2 amplitude slider to the left layout
        # left_layout.addWidget(self.piezo_2_phase_slider_label)      # Add label to the piezo 2 phase slider
        # left_layout.addWidget(self.piezo_2_phase_slider)            # Add the piezo 2 phase slider to the left layout
        # left_layout.addWidget(self.piezo_2_enable_button)           # Add the piezo 2 enable button to the left layout
        
        # left_layout.addWidget(self.rope_temp_slider_label)          # Add label to the rope heater temperature slider
        # left_layout.addWidget(self.rope_temp_slider)                # Add the rope heater temperature slider to the left layout
        
        # left_layout.addWidget(self.heat_flux_slider_label)          # Add label to the heater block heat flux slider
        # left_layout.addWidget(self.heat_flux_slider)                # Add the heater block heat flux slider to the left layout
        
        # Add widgets to middle layout
        self.log.debug("Adding widgets to middle layout")
        middle_layout.addWidget(self.thermistor_plot_widget, 2)     # Add the thermistor plot widget to the right layout
        middle_layout.addWidget(self.flow_sensor_plot_widget, 2)    # Add the flow sensor plot widget to the right layout
        
        # Add widgets to right layout
        self.log.debug("Adding widgets to right layout")
        right_layout.addWidget(self.send_data_button)               # Add the send data button to the right layout
        right_layout.addWidget(self.heater_block_temp_guage_label)  # Add label to the heater block temperature guage
        right_layout.addWidget(self.heater_block_temp_guage)        # Add the heater block temperature guage to the right layout
        right_layout.addWidget(self.heater_block_enable_button)     # Add the heater block enable button to the right layout
        right_layout.addWidget(self.rope_enable_button)             # Add the rope heater enable button to the right layout
        right_layout.addWidget(self.emergency_stop_button)          # Add the emergency stop button to the right layout
        
        # Layout alignments        
        side_margin = 15
        top_btm_margin = 10
        self.log.debug("Aligning layouts")
        left_layout.setAlignment(Qt.AlignVCenter)     # Align the left layout be evenly distributed in layout center
        left_layout.setContentsMargins(side_margin, top_btm_margin, side_margin, top_btm_margin)
        for i in range(left_layout.count()):
            item = left_layout.itemAt(i)
            # Format the CentralWidget()s in the left layout
            if isinstance(item.widget(), CentralWidget):
                for j in range(item.widget().layout().count()):
                    subitem = item.widget().layout().itemAt(j)
                    # Format the QLabel
                    if isinstance(subitem.widget(), QLabel):
                        print("Found a QLabel inside a CentralWidget()")
                        subitem.widget().setFont(label_font)
                        subitem.setAlignment(Qt.AlignCenter | Qt.AlignVCenter)
                        subitem.widget().setMinimumWidth(min_width_slider)
                    # Format the QSliders
                    elif isinstance(subitem.widget(), QSlider):
                        print("Found a QSlider inside a CentralWidget()")
                        subitem.widget().setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
                        subitem.widget().setStyleSheet(slider_style) 
                item.widget().setStyleSheet(widget_border_style) 
            # Format the QPushButtons in the left layout
            if isinstance(left_layout.itemAt(i).widget(), QPushButton):
                left_layout.itemAt(i).widget().setStyleSheet(button_style)
            left_layout.setStretch(i, 1)                               # Set the stretch of each slider in the left layout to 1
        middle_layout.setAlignment(Qt.AlignCenter)                     # Align the middle layout to the top of the window
        middle_layout.setStretch(0, 1)                                 # Set the stretch of the thermistor plot to 1
        middle_layout.setStretch(1, 1)                                 # Set the stretch of the flow sensor plot to 1
        right_layout.setAlignment(Qt.AlignCenter | Qt.AlignVCenter)    # Align the right layout be evenly distributed in layout center
        right_layout.setContentsMargins(side_margin, top_btm_margin, side_margin, top_btm_margin)
        for i in range(right_layout.count()):
            if isinstance(right_layout.itemAt(i).widget(), QSlider):
                right_layout.setStretch(i, 1)                              # Set the stretch of each widget in the right layout to 1

            right_layout.itemAt(i).widget().setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding) # Set the size policy of each widget in the left layout to expanding
        
        # Creat widgets for the vertical layouts and set the left and right to have a fixed width
        side_layout_width = 300
        self.log.debug("Creating widgets for vertical layouts")
        left_widget = QWidget()
        left_widget.setLayout(left_layout)
        left_widget.setFixedWidth(side_layout_width)
        middle_widget = QWidget()
        middle_widget.setLayout(middle_layout)
        right_widget = QWidget()
        right_widget.setLayout(right_layout)
        right_widget.setFixedWidth(side_layout_width)
        
        # Create main widget to insert into the main layout
        self.log.debug("Creating main widget (application window)")      
        main_layout.addWidget(left_widget)
        main_layout.addWidget(middle_widget)
        main_layout.addWidget(right_widget)               
        main_widget = QWidget()
        main_widget.setLayout(main_layout)
        self.setCentralWidget(main_widget)
        
    
        # -------------------------------- CALLBACK FOR ELEMENTS --------------------------------
        self.log.debug("Connecting signals to callback functions")
        # Connect the signals for the piezo 1 sliders
        self.piezo_1_freq_slider.valueChanged.connect(self.update_piezo_1_freq_slider_label)
        self.piezo_1_amp_slider.valueChanged.connect(self.update_piezo_1_amp_slider_label)
        self.piezo_1_phase_slider.valueChanged.connect(self.update_piezo_1_phase_slider_label)
        
        # Connect the signals for the piezo 2 sliders
        self.piezo_2_freq_slider.valueChanged.connect(self.update_piezo_2_freq_slider_label)
        self.piezo_2_amp_slider.valueChanged.connect(self.update_piezo_2_amp_slider_label)
        self.piezo_2_phase_slider.valueChanged.connect(self.update_piezo_2_phase_slider_label)
        
        # Connect the signals for the rope heater slider
        self.rope_temp_slider.valueChanged.connect(self.update_rope_temp_slider_label)
        
        # Connect the signals for the heat flux slider
        self.heat_flux_slider.valueChanged.connect(self.update_heat_flux_slider_label)
        
        # Connect the signals for the piezo 1 enable button
        self.piezo_1_enable_button.clicked.connect(self.enable_piezo_1)

        # Connect the signals for the piezo 2 enable button
        self.piezo_2_enable_button.clicked.connect(self.enable_piezo_2)
        
        # Connect the signals for the rope heater enable button
        self.rope_enable_button.clicked.connect(self.enable_rope_heater)

        # Connect the signals for the heater block enable button
        self.heater_block_enable_button.clicked.connect(self.enable_heater_block)
        
        # Connect the signals for the send data button
        self.send_data_button.clicked.connect(self.send_data)
        
        # Connect the signals for the emergency stop button
        self.emergency_stop_button.clicked.connect(self.emergency_stop)
        
        # # Create timer for the animation widget
        # self.mesh_animation_timer = QTimer(self)
        # self.current_frame_number = 0       # Start the count at 1
        # self.start_of_animation = True
        # self.mesh_animation_timer.timeout.connect(self.update_mesh_animation)
        # self.mesh_timer_interval = self.mesh_frame_generator.animation_length_ms/self.mesh_frame_generator.animation_steps
        # self.mesh_animation_timer.start(self.mesh_timer_interval)
           
        # # Populate the matplotlib plots
        # self.update_point_plot()
        # self.update_avg_point_plot()
        # self.update_freq_slider_label(self.freq_slider.value())
        # self.update_point_slider_label(self.point_slider.point_number)
        # self.stop_timer()
        # self.regenerate_mesh_and_point_frames()
        # self.update_color_bar()
        # self.start_timer()
        # self.ready_to_record = True
        
        self.init_update_all_slider_elements()
        
        
        
    def init_update_all_slider_elements(self):
        """ Update all slider elements to initial values for label updates """
        self.log.debug("Updating all slider elements to initial values for label updates")
        self.update_piezo_1_freq_slider_label(self.piezo_1_freq_slider.value())
        self.update_piezo_1_amp_slider_label(self.piezo_1_amp_slider.value())
        self.update_piezo_1_phase_slider_label(self.piezo_1_phase_slider.value())
        
        self.update_piezo_2_freq_slider_label(self.piezo_2_freq_slider.value())
        self.update_piezo_2_amp_slider_label(self.piezo_2_amp_slider.value())
        self.update_piezo_2_phase_slider_label(self.piezo_2_phase_slider.value())
        
        self.update_rope_temp_slider_label(self.rope_temp_slider.value())
        self.update_heat_flux_slider_label(self.heat_flux_slider.value())
        
        self.update_heater_block_temp_guage_label(self.heater_block_temp_guage.value())
    
        
    """ Update the element labels """
    def update_piezo_1_freq_slider_label(self, value):
        self.piezo_1_freq_slider_label.setText(f"Frequency: {value} (Hz)")
        #self.log.info(f"Piezo 1 frequency label update: {value} (Hz)")
    
    def update_piezo_1_amp_slider_label(self, value):
        self.piezo_1_amp_slider_label.setText(f"Amplitude: {value} (V)")
        #self.log.info(f"Piezo 1 amplitude label update: {value} (V)")
        
    def update_piezo_1_phase_slider_label(self, value):
        self.piezo_1_phase_slider_label.setText(f"Phase: {value} (deg)")
        #self.log.info(f"Piezo 1 phase label update: {value} (deg)")
        
    def update_piezo_2_freq_slider_label(self, value):
        self.piezo_2_freq_slider_label.setText(f"Frequency: {value} (Hz)")
        #self.log.info(f"Piezo 2 frequency label update: {value} (Hz)")
        
    def update_piezo_2_amp_slider_label(self, value):
        self.piezo_2_amp_slider_label.setText(f"Amplitude: {value} (V)")
        #self.log.info(f"Piezo 2 amplitude label update: {value} (V)")
        
    def update_piezo_2_phase_slider_label(self, value):
        self.piezo_2_phase_slider_label.setText(f"Phase: {value} (deg)")
        #self.log.info(f"Piezo 2 phase label update: {value} (deg)")
    
    def update_rope_temp_slider_label(self, value):
        self.rope_temp_slider_label.setText(f"Temperature: {value} (°C)")
        #self.log.info(f"Rope heater temperature update: {value} (°C)")
    
    def update_heat_flux_slider_label(self, value):
        self.heat_flux_slider_label.setText(f"Heat Flux: {value} (W/m²)")
        #self.log.info(f"Heat flux label update: {value} (W/m²)")
    
    def update_heater_block_temp_guage_label(self, value):
        self.heater_block_temp_guage_label.setText(f"Heat Block Temp: {value} (°C)")
        #self.log.info(f"Heat block temperature label update: {value} (°C)")
    
    """ Update the element values """
    def update_piezo_1_freq_slider_value(self, value):
        self.piezo_1_freq_slider.setValue(value)
        #self.log.info(f"Piezo 1 slider update, frequency: {value} (Hz)")
    
    def update_piezo_1_amp_slider_value(self, value):
        self.piezo_1_amp_slider.setValue(value)
        #self.log.info(f"Piezo 1 slider update, amplitude: {value} (V)")
    
    def update_piezo_1_phase_slider_value(self, value):
        self.piezo_1_phase_slider.setValue(value)
        #self.log.info(f"Piezo 1 slider update, phase: {value} (deg)")
        
    def update_piezo_2_freq_slider_value(self, value):
        self.piezo_2_freq_slider.setValue(value)
        #self.log.info(f"Piezo 2 slider update, frequency: {value} (Hz)")
        
    def update_piezo_2_amp_slider_value(self, value):
        self.piezo_2_amp_slider.setValue(value)
        #self.log.info(f"Piezo 2 slider update, amplitude: {value} (V)")
        
    def update_piezo_2_phase_slider_value(self, value):
        self.piezo_2_phase_slider.setValue(value)
        #self.log.info(f"Piezo 2 slider update, phase: {value} (deg)")
        
    def update_rope_temp_slider_value(self, value):
        self.rope_temp_slider.setValue(value)
        #self.log.info(f"Rope temp slider update, temperature: {value} (°C)")
        
    def update_heat_flux_slider_value(self, value):
        self.heat_flux_slider.setValue(value)   
        #self.log.info(f"Heat flux slider update, heat flux: {value} (W/m²)")
    
    def update_heater_block_temp_guage(self, value):
        self.heater_block_temp_guage.setValue(value)
        #self.log.info(f"Heater block temp guage update, temperature: {value} (°C)")
        
    """ Button callbacks """
    def send_data(self):
        self.send_data_button.setEnabled(False)
        self.send_data_button.setText("Sending Data...")
        self.log.info("Sending data to the teensy")
        self.log.debug("PUT SERIAL STRING HERE")
        ################# SEND THE DATA TO SERIAL CONNECTION #################
        
        self.send_data_button.setText("Send Data")
        self.send_data_button.setEnabled(True)
        
    def emergency_stop(self):
        self.emergency_stop_button.setEnabled(False)
        self.emergency_stop_button.setText("Stopping...")
        self.log.error("Emergency stop")
        ################# SEND THE DATA TO SERIAL CONNECTION #################
        
        self.emergency_stop_button.setText("Emergency Stop")
        self.emergency_stop_button.setEnabled(True)
        
    def enable_piezo_1(self):
        if self.piezo_1_enable_button.isChecked():
            self.log.info("Piezo 1 enabled")
        else:
            self.log.info("Piezo 1 disabled")
            
    def enable_piezo_2(self):
        if self.piezo_2_enable_button.isChecked():
            self.log.info("Piezo 2 enabled")
        else:
            self.log.info("Piezo 2 disabled")
        
    def enable_rope_heater(self):
        if self.rope_enable_button.isChecked():
            self.log.info("Rope heater enabled")
        else:
            self.log.info("Rope heater disabled")
            
    def enable_heater_block(self):
        if self.heater_block_enable_button.isChecked():
            self.log.info("Heater block enabled")
        else:
            self.log.info("Heater block disabled")

  
    """ Timer methods """      
    def stop_timer(self):
        pass
        #self.camera_view = self.mesh_animation_visualizer.get_view_control().convert_to_pinhole_camera_parameters()
        #self.mesh_animation_timer.stop()
        
    def start_timer(self):
        pass
        #self.mesh_animation_timer.start(self.mesh_timer_interval)
        #self.mesh_animation_visualizer.get_view_control().convert_from_pinhole_camera_parameters(self.camera_view)
        
   
    # """Update the point plot canvas when the frequency slider or point slider is modified"""
    # def update_point_plot(self):
    #     self.point_plot_canvas.axes.clear()       # Clear the canvas of the old data
    #     self.point_coherence_axis.axes.clear()
    #     self.current_point_number = self.point_slider.point_number
    #     #print(f"current point number: {self.current_point_number}")
    #     #self.current_point = get_point_object(self.vibrometer.points, self.current_point_number)            # Get the current point object given slider value
    #     self.point_plot_canvas.axes.grid(True)
    #     self.point_plot_canvas.axes.set_title("Point " + str(self.current_point_number))
    #     self.point_plot_canvas.axes.set_xlabel("Frequency (Hz)")
    #     self.point_plot_canvas.axes.set_ylabel("FRF Amplitude (dB)")
    #     #self.point_plot_canvas.axes.set_yscale('symlog')
    #     self.point_coherence_axis.axes.plot(self.vibrometer.freq, self.vibrometer.avg_point.coherence.data, color='blue', alpha=0.5, linewidth=0.5)
    #     self.point_coherence_axis.axes.set_ylabel(f"Coherence")
    #     self.point_plot_canvas.axes.plot(self.vibrometer.freq, self.current_point.frf.data_db, color='red', linewidth=0.5)              # Plot the point FRF data in db scale
    #     self.point_plot_canvas.axes.axvline(self.freq_slider.value(), color='black', linestyle='--', linewidth=1.0)        # Plot vertical line of the current frequency
    #     peaks_x = self.current_point.frf_peaks[:, 0]
    #     peaks_y = self.current_point.frf_peaks[:, 1]
    #     self.point_plot_canvas.axes.scatter(peaks_x, peaks_y, edgecolors='black', s=20.0, facecolors=None)
    #     curr_x = self.freq_slider.value()
    #     #index = get_nearest_freq_index(self.vibrometer.freq, curr_x)
    #     #curr_y = self.current_point.frf.data_db[index]
    #     #curr_y_coh = np.real(self.current_point.coherence.data[index])
    #     textbox_str = f"Freq: {np.round(curr_x, 2)} Hz\nAmplitude: {np.round(curr_y, 2)} dB\nCoherence: {np.round(curr_y_coh, 2)}"
    #     textbox = AnchoredText(textbox_str, loc=4, prop=dict(size=10))
    #     self.point_plot_canvas.axes.add_artist(textbox)
    #     self.point_plot_canvas.draw()
        
    # """Update the average point plot canvas when frequency slider is modified"""
    # def update_avg_point_plot(self):
    #     self.avg_point_plot_canvas.axes.clear()    # Clear the canvas of the old data
    #     self.avg_point_coherence_axis.axes.clear()
    #     self.current_point_number = self.point_slider.point_number
    #     self.avg_point_plot_canvas.axes.grid(True)
    #     self.avg_point_plot_canvas.axes.set_title("Average of Points")
    #     self.avg_point_plot_canvas.axes.set_xlabel(f"Frequency (Hz)")
    #     self.avg_point_plot_canvas.axes.set_ylabel(f"FRF (dB)")
    #     #self.avg_point_plot_canvas.axes.set_yscale('symlog')
    #     self.avg_point_coherence_axis.axes.plot(self.vibrometer.freq, self.vibrometer.avg_point.coherence.data, color='blue', alpha=0.5, linewidth=0.5)
    #     self.avg_point_coherence_axis.axes.set_ylabel(f"Coherence")
    #     self.avg_point_plot_canvas.axes.plot(self.vibrometer.freq, self.vibrometer.avg_point.frf.data_db, color='red', linewidth=0.5)    # Plot the average FRF data in db scale
    #     self.avg_point_plot_canvas.axes.axvline(self.freq_slider.value(), color='black', linestyle='--', linewidth=1.0)     # Plot vertical line of the current frequency
    #     peaks_x = self.vibrometer.avg_point.frf_peaks[:, 0]
    #     peaks_y = self.vibrometer.avg_point.frf_peaks[:, 1]
    #     self.avg_point_plot_canvas.axes.scatter(peaks_x, peaks_y, edgecolors='black', s=20.0, facecolors=None)
    #     curr_x = self.freq_slider.value()
    #     index = get_nearest_freq_index(self.vibrometer.freq, curr_x)
    #     curr_y = self.vibrometer.avg_point.frf.data_db[index]
    #     curr_y_coh = np.real(self.vibrometer.avg_point.coherence.data[index])
    #     textbox_str = f"Freq: {np.round(curr_x, 2)} Hz\nAmplitude: {np.round(curr_y, 2)} dB\nCoherence: {np.round(curr_y_coh, 2)}"
    #     textbox = AnchoredText(textbox_str, loc=4, prop=dict(size=10))
    #     self.avg_point_plot_canvas.axes.add_artist(textbox)
    #     self.avg_point_plot_canvas.draw()
    
    # """Regenerate the frames if the freq slider changes"""
    # def regenerate_mesh_and_point_frames(self):
    #     # Generate the new frames
    #     self.mesh_frame_generator.generate_frames(float(self.freq_slider.value()), self.point_slider.point_number, True, True)
    #     self.regen_frames = True
    #     # Clear the window of all meshes
    #     self.mesh_animation_visualizer.clear_geometries()    
    #     # Update the renderer
    #     self.mesh_animation_visualizer.poll_events()
    #     self.mesh_animation_visualizer.update_renderer()

    # """Regenerate the frames if the point slider changes"""
    # def regenerate_point_frames(self):
    #     # Generate the new frames (points only)
    #     self.mesh_frame_generator.generate_frames(float(self.freq_slider.value()), self.point_slider.point_number, False, True)
    #     self.regen_frames = True
    #     # Clear the window of all meshes
    #     self.mesh_animation_visualizer.clear_geometries()    
    #     # Update the renderer
    #     self.mesh_animation_visualizer.poll_events()
    #     self.mesh_animation_visualizer.update_renderer()
        
    # """Update the mesh animation when the frequency slider is modified"""
    # def update_mesh_animation(self):
    #     #print(f"updating mesh {self.current_frame_number}")

    #     # Save the camera view
    #     if not self.regen_frames:
    #         self.camera_view = self.mesh_animation_visualizer.get_view_control().convert_to_pinhole_camera_parameters()
    #     else:
    #         self.regen_frames = False

    #     # Set mesh and update the geometry
    #     # for mesh in self.mesh_frame_generator.animation_mesh_frames:
    #     #     self.mesh_animation_visualizer.remove_geometry(mesh)
    #     self.mesh_animation_visualizer.clear_geometries()
    #     self.mesh_animation_visualizer.add_geometry(self.mesh_frame_generator.animation_mesh_frames[self.current_frame_number])
    #     self.mesh_animation_visualizer.add_geometry(self.mesh_frame_generator.animation_point_frames[self.current_frame_number])
    #     for mesh in self.mesh_frame_generator.animation_mesh_frames:
    #         self.mesh_animation_visualizer.update_geometry(mesh)
    #     for pcd in self.mesh_frame_generator.animation_point_frames:
    #         self.mesh_animation_visualizer.update_geometry(pcd)
        
    #     # Set camera view and update renderer
    #     if not self.start_of_animation:
    #         self.mesh_animation_visualizer.get_view_control().convert_from_pinhole_camera_parameters(self.camera_view)
    #     else:
    #         self.start_of_animation = False
        
    #     #self.mesh_animation_visualizer.get_view_control().convert_from_pinhole_camera_parameters(camera_view)
    #     self.mesh_animation_visualizer.get_render_option().mesh_show_back_face = True
    #     self.mesh_animation_visualizer.get_render_option().mesh_show_wireframe = True
    #     self.mesh_animation_visualizer.get_render_option().light_on = True
    #     self.mesh_animation_visualizer.get_render_option().background_color = np.asarray([0.1, 0.1, 0.1])
    #     self.mesh_animation_visualizer.get_render_option().mesh_shade_option = o3d.visualization.MeshShadeOption.Color
    #     self.mesh_animation_visualizer.get_render_option().show_coordinate_frame = True
    #     #self.mesh_animation_visualizer.get_render_option().ground_plane_visibility = True
    #     self.mesh_animation_visualizer.poll_events()
    #     self.mesh_animation_visualizer.update_renderer()
        
    #     # Set current frame num, if over the number of frames, loop back to 0
    #     # self.current_frame_number += 1
    #     # if self.current_frame_number == self.mesh_frame_generator.animation_steps:
    #     #     self.current_frame_number = 0
    #     self.current_frame_number = (self.current_frame_number + 1) % self.mesh_frame_generator.animation_steps
    #     self.current_frame_number = 0           # Basically pause animation at max amplitude
    #     #print(self.current_frame_number)
        

if __name__ == '__main__':
    # app = QApplication(sys.argv)
    # view = 
    # view.show()
    # app.exec()
    pass