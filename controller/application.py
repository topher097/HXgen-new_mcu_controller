# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

"""PySide6 port of the qt3d/simple-cpp example from Qt v5.x"""
import logging

import pyEasyTransfer
from pyEasyTransfer import PyEasyTransfer

import sys
from enum import IntEnum, auto
from dataclasses import dataclass
from PySide6.QtCore import (QMutex, Qt, QTimer, Slot, QRect, QSize)
from PySide6.QtGui import (QMatrix4x4, QQuaternion, QVector3D, QWindow, QFont)
from PySide6.QtWidgets import (QMainWindow, QFrame, QHBoxLayout, QLabel, QPushButton, QSizePolicy, QSlider, QVBoxLayout, QWidget, QTextEdit, QLayout, QLayoutItem, QTextBrowser, QWidgetItem)
import pyqtgraph as pg
import asyncio


# Import custom Qt elements
from application_elements import CentralWidget, MplCanvas, ListSlider, QLoggerStream


# Other imports
import numpy as np
from time import time
import qasync

""" These are the limits to sliders and other variables """
# -------------------- Piezo attributes --------------------
# Frequency
piezo_min_freq = 150
piezo_max_freq = 15000
piezo_freq_step = 5
piezo_default_freq = 2000
# Amplitude
piezo_min_vpp = 0.0
piezo_max_vpp = 150.0
piezo_vpp_step = 1
piezo_default_vpp = 100.0
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
heat_flux_default = 8
heat_block_enable = False

# -------------------- GUI attributes --------------------
min_width_slider = 30


        
    
    
"""Create the main window for the application"""
class MainWindow(QMainWindow):
  
    
    def __init__(self, window_title: str, 
                 loop: qasync.QEventLoop, 
                 log: logging.Logger, 
                 monitor_serial_interface: PyEasyTransfer,
                 driver_serial_interface: PyEasyTransfer,
                 input_data_rate: int, 
                 test_time: int, 
                 *args, **kwargs):
        super(MainWindow, self).__init__(*args, **kwargs)
        self.log = log      # Get the logger from the main application
        self.loop = loop
        self.test_time = test_time
        self.input_data_rate = input_data_rate
        self.mutex = QMutex()
        
        # Setup the ET monitor
        self.ET_monitor = monitor_serial_interface
        self.monitor_input_data = self.ET_monitor.read_data
        self.monitor_output_data = self.ET_monitor.write_data
        self.last_io_count_monitor = 0
        
        # Setup the ET driver
        self.ET_driver = driver_serial_interface
        self.driver_input_data = self.ET_driver.read_data
        self.driver_output_data = self.ET_driver.write_data
        self.last_io_count_driver = 0
        
        self.emergency_stop = False
 
        # Initialize the main window
        self.log.info("Initializing application window")
        self.setWindowTitle(window_title)
        self.window_title = window_title
        self.setMinimumWidth(1800)
        self.setMinimumHeight(800)
        label_font = QFont()
        label_font.setBold(True)
        label_font.setPointSize(16)
        
        # Button size policy
        button_size_policy = QSizePolicy(QSizePolicy.Preferred, QSizePolicy.Preferred)
        button_size_policy.setVerticalStretch(1)
        button_size_policy.setHeightForWidth(True)
        
        # Set asyncio task to continiously read data
        self.log.info("Starting asyncio task to read data")
        self.monitor_read_data_task = self.loop.create_task(self.ET_monitor.listen())
        self.driver_read_data_task = self.loop.create_task(self.ET_driver.listen())
        
        # Button stype
        button_style =  """
                            QPushButton { 
                                font-size: 18px;
                                font-weight: bold;
                                color: black;
                                background-color: red; 
                                border-width: 2px; 
                                border-radius: 10px; 
                                border-color: dark grey; 
                                border-style: outset;
                                padding: 20px; 
                            }
                            QPushButton:pressed {
                                background-color: red;
                            }
                            QPushButton:clicked {
                                background-color: red;
                            }
                            QPushButton:checked { 
                                background-color: green; 
                                border-style: inset; 
                            }
                        """ 
                                
        # Widget border style
        widget_border_style =   """
                                QLayoutItem {
                                    background-color: light grey;
                                    border-width: 2px;
                                    border-radius: 5px;
                                    border-color: black;
                                    border-style: outset;
                                    padding: 10px; 
                                }
                                """
  
        # Slider style                   
        slider_style =      """
                            QSlider::handle:horizontal {
                                background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #b4b4b4, stop:1 #8f8f8f);
                                border: 1px solid #5c5c5c;
                                width: 18px;
                                margin: -2px 0; /* handle is placed by default on the contents rect of the groove. Expand outside the groove */
                                border-radius: 3px;
                            }
                            """
        
        # -------------------------------- LOGGER ELEMENT --------------------------------
        self.log_widget = QTextEdit()
        self.log_widget.setReadOnly(True)
        handler = QLoggerStream(self.log_widget)
        handler.setFormatter(self.log.handlers[1].formatter)        # Use the same formatter as the console handler
        self.log.addHandler(handler)   
        self.log_widget.setStyleSheet("""background-color: light grey;; 
                                      font-size: 12px; 
                                      border-width: 2px;
                                      border-radius: 5px;
                                      border-color: black;
                                      font-family: Courier;""")    
                            
        
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
        piezo_1_freq_layout.setAlignment(Qt.AlignmentFlag.AlignCenter | Qt.AlignmentFlag.AlignVCenter)
        #piezo_1_freq_layout.setStretch(0, 0)
        self.piezo_1_freq_widget = CentralWidget()
        self.piezo_1_freq_widget.setLayout(piezo_1_freq_layout)
        
        
        # Create a slider for the amplitude of the piezo 1
        self.piezo_1_amp_slider = QSlider(Qt.Horizontal, self)
        self.piezo_1_amp_slider.setMinimum(piezo_min_vpp)
        self.piezo_1_amp_slider.setMaximum(piezo_max_vpp)
        self.piezo_1_amp_slider.setTickInterval(piezo_vpp_step)
        self.piezo_1_amp_slider.setValue(piezo_default_vpp)
        self.piezo_1_amp_slider.setEnabled(True)
        self.piezo_1_amp_slider_label = QLabel(f"Amplitude: {self.piezo_1_amp_slider.value} (V p-p)", self)
        piezo_1_amp_layout = QVBoxLayout()
        piezo_1_amp_layout.addWidget(self.piezo_1_amp_slider_label)
        piezo_1_amp_layout.addWidget(self.piezo_1_amp_slider)
        piezo_1_amp_layout.setAlignment(Qt.AlignmentFlag.AlignCenter | Qt.AlignmentFlag.AlignVCenter)
        #piezo_1_amp_layout.setStretch(0, 0)
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
        piezo_1_phase_layout.setAlignment(Qt.AlignmentFlag.AlignCenter | Qt.AlignmentFlag.AlignVCenter)
        #piezo_1_phase_layout.setStretch(0, 0)
        self.piezo_1_phase_widget = CentralWidget()
        self.piezo_1_phase_widget.setLayout(piezo_1_phase_layout)
        
        # Create a button for enabling the piezo 1
        self.piezo_1_enable_button = QPushButton("Enable Piezo 1", self)
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
        piezo_2_freq_layout.setAlignment(Qt.AlignmentFlag.AlignCenter | Qt.AlignmentFlag.AlignVCenter)
        piezo_2_freq_layout.setStretch(0, 0)
        self.piezo_2_freq_widget = CentralWidget()
        self.piezo_2_freq_widget.setLayout(piezo_2_freq_layout)        
        
        # Create a slider for the amplitude of the piezo 2
        self.piezo_2_amp_slider = QSlider(Qt.Horizontal, self)
        self.piezo_2_amp_slider.setMinimum(piezo_min_vpp)
        self.piezo_2_amp_slider.setMaximum(piezo_max_vpp)
        self.piezo_2_amp_slider.setTickInterval(piezo_vpp_step)
        self.piezo_2_amp_slider.setValue(piezo_default_vpp)
        self.piezo_2_amp_slider.setEnabled(True)
        self.piezo_2_amp_slider_label = QLabel(f"Amplitude: {self.piezo_2_amp_slider.value} (V p-p)", self)
        piezo_2_amp_layout = QVBoxLayout()
        piezo_2_amp_layout.addWidget(self.piezo_2_amp_slider_label)
        piezo_2_amp_layout.addWidget(self.piezo_2_amp_slider)
        piezo_2_amp_layout.setAlignment(Qt.AlignmentFlag.AlignCenter | Qt.AlignmentFlag.AlignVCenter)
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
        piezo_2_phase_layout.setAlignment(Qt.AlignmentFlag.AlignCenter | Qt.AlignmentFlag.AlignVCenter)
        piezo_2_phase_layout.setStretch(0, 0)
        self.piezo_2_phase_widget = CentralWidget()
        self.piezo_2_phase_widget.setLayout(piezo_2_phase_layout)
        
        # Create a button for enabling the piezo 2
        self.piezo_2_enable_button = QPushButton("Enable Piezo 2", self)
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
        rope_temp_layout.setAlignment(Qt.AlignmentFlag.AlignCenter | Qt.AlignmentFlag.AlignVCenter)
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
        self.heat_flux_slider.setValue(heat_flux_default)
        self.heat_flux_slider.setEnabled(True)
        self.heat_flux_slider_label = QLabel(f"Heat Flux: {self.heat_flux_slider.value} (W/m²)", self)
        heat_flux_layout = QVBoxLayout()
        heat_flux_layout.addWidget(self.heat_flux_slider_label)
        heat_flux_layout.addWidget(self.heat_flux_slider)
        heat_flux_layout.setAlignment(Qt.AlignmentFlag.AlignCenter | Qt.AlignmentFlag.AlignVCenter)
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
        self.plot_x_time_limit = 60     # Seconds to show on the plots
        # Create a layout for the two plots
        self.plot_layout = pg.GraphicsLayoutWidget(show=True)
        
        self.create_plot_items()
        
        # Add these PlotItems to a QWidget
        self.plot_widget = QWidget()
        self.plot_layout_widget = QVBoxLayout()
        self.plot_layout_widget.addWidget(self.plot_layout)
        self.plot_widget.setLayout(self.plot_layout_widget)
        
        # -------------------------------- OTHER ELEMENTS --------------------------------
        self.log.debug("Creating other elements")
        # Creata a button to send the data to the Teensy
        self.start_test_button = QPushButton("Start Test", self)
        self.start_test_button.setEnabled(True)
        self.start_test_button.setChecked(False)
        self.start_test_button.setCheckable(True)
        self.start_test_button.setSizePolicy(button_size_policy)
        self.start_test_button.setStyleSheet(button_style)
        
        # Create a preheat button to enable the heaters and wait for them to reach the desired temperature
        self.preheat_button = QPushButton("Preheat", self)
        self.preheat_button.setEnabled(True)
        self.preheat_button.setChecked(False)
        self.preheat_button.setCheckable(True)
        self.preheat_button.setSizePolicy(button_size_policy)
        self.preheat_button.setStyleSheet(button_style)

        # Create a guage for the heater block temperature
        self.heater_block_temp_gauge = QSlider(Qt.Horizontal, self)
        self.heater_block_temp_gauge.setMinimum(20)
        self.heater_block_temp_gauge.setMaximum(Heater_block_max_temp)
        self.heater_block_temp_gauge.setTickInterval(1)
        self.heater_block_temp_gauge.setValue(20)
        self.heater_block_temp_gauge.setEnabled(True)
        self.heater_block_temp_gauge_label = QLabel(f"Heat Block Temp: {self.heater_block_temp_gauge.value} (°C)", self)
        heater_block_temp_layout = QVBoxLayout()
        heater_block_temp_layout.addWidget(self.heater_block_temp_gauge_label)
        heater_block_temp_layout.addWidget(self.heater_block_temp_gauge)
        heater_block_temp_layout.setAlignment(Qt.AlignmentFlag.AlignCenter | Qt.AlignmentFlag.AlignVCenter)
        heater_block_temp_layout.setStretch(0, 0)
        self.heater_block_temp_widget = CentralWidget()
        self.heater_block_temp_widget.setLayout(heater_block_temp_layout)
        
        # Create a gauge for the inlet temperature
        self.inlet_temp_gauge = QSlider(Qt.Horizontal, self)
        self.inlet_temp_gauge.setMinimum(20)
        self.inlet_temp_gauge.setMaximum(70)
        self.inlet_temp_gauge.setTickInterval(1)
        self.inlet_temp_gauge.setValue(20)
        self.inlet_temp_gauge.setEnabled(True)
        self.inlet_temp_gauge_label = QLabel(f"Inlet Temp: {self.inlet_temp_gauge.value} (°C)", self)
        inlet_temp_layout = QVBoxLayout()
        inlet_temp_layout.addWidget(self.inlet_temp_gauge_label)
        inlet_temp_layout.addWidget(self.inlet_temp_gauge)
        inlet_temp_layout.setAlignment(Qt.AlignmentFlag.AlignCenter | Qt.AlignmentFlag.AlignVCenter)
        inlet_temp_layout.setStretch(0, 0)
        self.inlet_temp_widget = CentralWidget()
        self.inlet_temp_widget.setLayout(inlet_temp_layout)        

        
        # Craete an emergency stop button for the heaters and stop the recording on teensy
        self.emergency_stop_button = QPushButton("Emergency Stop", self)
        self.emergency_stop_button.setEnabled(True)
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
        left_layout.addWidget(self.piezo_1_freq_widget)
        left_layout.addWidget(self.piezo_1_amp_widget)
        left_layout.addWidget(self.piezo_1_phase_widget)
        left_layout.addWidget(self.piezo_1_enable_button)
        left_layout.addWidget(self.piezo_2_freq_widget)
        left_layout.addWidget(self.piezo_2_amp_widget)
        left_layout.addWidget(self.piezo_2_phase_widget)
        left_layout.addWidget(self.piezo_2_enable_button)
        left_layout.addWidget(self.rope_temp_widget)
        
        # Add widgets to middle layout
        middle_layout.addWidget(self.plot_widget)
      
        # Add widgets to right layout
        self.log.debug("Adding widgets to right layout")
        right_layout.addWidget(self.start_test_button)               # Add the start test button to the right layout
        right_layout.addWidget(self.preheat_button)                  # Add the preheat button to the right layout
        right_layout.addWidget(self.heat_flux_widget)               # Add the heat flux widget to the right layout
        right_layout.addWidget(self.heater_block_temp_widget)       # Add the heater block temperature widget to the right layout
        right_layout.addWidget(self.inlet_temp_widget)              # Add the inlet temperature widget to the right layout
        right_layout.addWidget(self.heater_block_enable_button)     # Add the heater block enable button to the right layout
        right_layout.addWidget(self.rope_enable_button)             # Add the rope heater enable button to the right layout
        right_layout.addWidget(self.emergency_stop_button)          # Add the emergency stop button to the right layout
        
        # Layout alignments        
        side_margin = 15
        top_btm_margin = 10
        self.log.debug("Aligning layouts")
        left_layout.setAlignment(Qt.AlignmentFlag.AlignVCenter)     # Align the left layout be evenly distributed in layout center
        left_layout.setContentsMargins(side_margin, top_btm_margin, side_margin, top_btm_margin)
        for i in range(left_layout.count()):
            item = left_layout.itemAt(i)
            # Format the CentralWidget()s in the left layout
            if isinstance(item.widget(), CentralWidget):
                for j in range(item.widget().layout().count()):
                    subitem = item.widget().layout().itemAt(j)
                    # Format the QLabel
                    if isinstance(subitem.widget(), QLabel):
                        #print("Found a QLabel inside a CentralWidget()")
                        subitem.widget().setFont(label_font)
                        subitem.setAlignment(Qt.AlignmentFlag.AlignCenter | Qt.AlignmentFlag.AlignVCenter)
                        subitem.widget().setMinimumWidth(min_width_slider)
                    # Format the QSliders
                    elif isinstance(subitem.widget(), QSlider):
                        #print("Found a QSlider inside a CentralWidget()")
                        subitem.widget().setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
                        subitem.widget().setStyleSheet(slider_style) 
                #item.widget().setStyleSheet(widget_border_style) 
            # Format the QPushButtons in the left layout
            elif isinstance(left_layout.itemAt(i).widget(), QPushButton):
                left_layout.itemAt(i).widget().setStyleSheet(button_style)
            left_layout.setStretch(i, 2)                               # Set the stretch of each slider in the left layout to 1
        #middle_layout.setAlignment(Qt.AlignmentFlag.AlignCenter)                     # Align the middle layout to the top of the window
        #middle_layout.setStretch(0, 1)                                 # Set the stretch of the thermistor plot to 1
        #middle_layout.setStretch(1, 1)                                 # Set the stretch of the flow sensor plot to 1
        right_layout.setAlignment(Qt.AlignmentFlag.AlignCenter | Qt.AlignmentFlag.AlignVCenter)    # Align the right layout be evenly distributed in layout center
        right_layout.setContentsMargins(side_margin, top_btm_margin, side_margin, top_btm_margin)
        for i in range(right_layout.count()):
            item = right_layout.itemAt(i)
            # Format the CentralWidget()s in the left layout
            if isinstance(item.widget(), CentralWidget):
                for j in range(item.widget().layout().count()):
                    subitem = item.widget().layout().itemAt(j)
                    # Format the QLabel
                    if isinstance(subitem.widget(), QLabel):
                        #print("Found a QLabel inside a CentralWidget()")
                        subitem.widget().setFont(label_font)
                        subitem.setAlignment(Qt.AlignmentFlag.AlignCenter | Qt.AlignmentFlag.AlignVCenter)
                        subitem.widget().setMinimumWidth(min_width_slider)
                    # Format the QSliders
                    elif isinstance(subitem.widget(), QSlider):
                        #print("Found a QSlider inside a CentralWidget()")
                        subitem.widget().setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
                        subitem.widget().setStyleSheet(slider_style) 
                #item.widget().setStyleSheet(widget_border_style) 
            # Format the QPushButtons in the left layout
            elif isinstance(right_layout.itemAt(i).widget(), QPushButton):
                right_layout.itemAt(i).widget().setStyleSheet(button_style)
            right_layout.setStretch(i, 2)   
            
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
        
        # Create main window layout
        window_layout = QVBoxLayout()
        main_widget.setSizePolicy(QSizePolicy.Preferred, QSizePolicy.Expanding)
        window_layout.addWidget(main_widget)
        window_layout.setStretchFactor(main_widget, 8)  # 80% of the window veritcal space is the main widget
        self.log_widget.setSizePolicy(QSizePolicy.Preferred, QSizePolicy.Expanding)
        window_layout.addWidget(self.log_widget)
        window_layout.setStretchFactor(self.log_widget, 2)  # 10% of the window veritcal space is the log widget
        self.window_widget = QWidget()
        self.window_widget.setLayout(window_layout)
        
        self.setCentralWidget(self.window_widget)
        
    
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
        
        # Connect the signals for the temperature of heater block 
        self.heater_block_temp_gauge.valueChanged.connect(self.update_heater_block_temp_gauge_label)
        self.inlet_temp_gauge.valueChanged.connect(self.update_inlet_temp_gauge_label)
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
        
        # Connect the signals for the start test button
        self.start_test_button.clicked.connect(self.start_test_button_click)
        
        # Connect the signals for the preheat button
        self.preheat_button.clicked.connect(self.preheat_button_click)
        
        # Connect the signals for the emergency stop button
        self.emergency_stop_button.clicked.connect(self.emergency_stop_button_click)
        
        # Initialize all slider elements to initial values for label updates
        self.init_update_all_slider_elements()
        
        # Create timer for the plotting and run it
        self.plot_timer = QTimer(self)
        self.plot_timer.timeout.connect(self.update_plots_from_teensy_input_data)
        self.plot_timer.start(200)      # Update the plot every 200 ms
        
        # Create a timer to update the heater block temp gauge
        self.heater_block_temp_gauge_timer = QTimer(self)
        self.heater_block_temp_gauge_timer.timeout.connect(self.update_heater_block_temp_gauge)
        self.heater_block_temp_gauge_timer.start(500)      # Update the gauge every 500 ms
        
        # Create a timer to update the inlet temperature gauge
        self.inlet_temp_gauge_timer = QTimer(self)
        self.inlet_temp_gauge_timer.timeout.connect(self.update_inlet_temp_gauge)
        self.inlet_temp_gauge_timer.start(500)      # Update the gauge every 500 ms
        
        # Create a timer to update the test time
        self.test_time_timer = QTimer(self)
        self.test_time_timer.timeout.connect(self.update_test_time)
        
        
        
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
        
        self.update_heater_block_temp_gauge_label(self.heater_block_temp_gauge.value())
        self.update_inlet_temp_gauge_label(self.inlet_temp_gauge.value())
    
        
    def update_elements_from_teensy_input_data(self):
        """Updates the GUI elements from the latest Teensy input data, not necessarily when Teensy data is received and stored"""
        self.log.debug("Updating GUI elements from latest Teensy input data")
        
        self.update_piezo_1_freq_slider_value(self.driver_input_data.piezo_1_freq_hz)
        self.update_piezo_1_amp_slider_value(self.driver_input_data.piezo_1_vpp)
        self.update_piezo_1_phase_slider_value(self.driver_input_data.piezo_1_phase_deg)
        
        self.update_piezo_2_freq_slider_value(self.driver_input_data.piezo_2_freq_hz)
        self.update_piezo_2_amp_slider_value(self.driver_input_data.piezo_2_vpp)
        self.update_piezo_2_phase_slider_value(self.driver_input_data.piezo_2_phase_deg)
        
        self.update_rope_temp_slider_value(self.monitor_input_data.inlet_fluid_temp_c)
        self.update_heat_flux_slider_value(self.monitor_input_data.heat_flux)
        
        self.update_heater_block_temp_gauge()
        
        self.update_piezo_1_enable_button(self.driver_input_data.piezo_1_enable)
        self.update_piezo_2_enable_button(self.driver_input_data.piezo_2_enable)
        self.update_rope_enable_button(self.monitor_input_data.rope_heater_enable)
        self.update_heater_block_enable_button(self.monitor_input_data.heater_block_enable)
        
        self.update_start_test_button(False)
        self.update_preheat_button(False)
        self.update_emergency_stop_button(False)    
        
    def update_heater_block_temp_gauge(self):
        """Updates the heater block temp gauge via timer"""
        heat_block_temp = np.average((self.monitor_input_data.thermistor_13_temp_c, self.monitor_input_data.thermistor_14_temp_c))
        self.update_heater_block_temp_gauge_value(heat_block_temp)
        
    def update_inlet_temp_gauge(self):
        """Updates the inlet temp gauge via timer"""
        inlet_temp = self.monitor_input_data.inlet_fluid_temp_c
        self.update_inlet_temp_gauge_value(inlet_temp)
     
    @qasync.asyncSlot()   
    async def update_test_time(self):
        """Updates the test time on the start button label via timer"""
        # Check to see if the current_test_time is not zero, if so, decrement it and update the label
        #self.emergency_stop_button.setEnabled(True)
        #self.emergency_stop_button.setCheckable(True)
        if self.current_test_time > 0:
            self.current_test_time -= 1
            self.start_test_button.setText(f"TESTING... {self.current_test_time}")
            # If the current test time is half of the test time then start the piezos given their current settings
            if self.current_test_time == self.test_time/2:
                self.log.debug("Starting piezos")
                # We use the data that was written at the start of the test as the piezo settings, so we don't update them here
                self.driver_output_data.reset_time = np.bool_(False)
                self.driver_output_data.piezo_1_enable = np.bool_(self.piezo_1_enable_button.isChecked())
                self.driver_output_data.piezo_2_enable = np.bool_(self.piezo_2_enable_button.isChecked())
                await self.ET_driver.send_data()        # Send the data to the Driver
        else:
            # Reset the button, enable all elements, and stop the timer
            self.start_test_button.setText("Start Test")
            self.start_test_button.setChecked(False)
            self.start_test_button.setEnabled(True)
            self.start_test_button.setCheckable(True)
            self.ET_monitor.stop_saving()
            self.ET_driver.stop_saving()
            # Stop the piezos
            self.log.debug("Stopping piezos")
            self.driver_output_data.reset_time = np.bool_(False)
            self.driver_output_data.piezo_1_enable = np.bool_(False)
            self.driver_output_data.piezo_2_enable = np.bool_(False)
            await self.ET_driver.send_data()        # Send the data to the Driver
            # Save the data to a file
            pyEasyTransfer.save_data_to_pickle_file(self.ET_monitor.save_read_data, dir='data')
            pyEasyTransfer.save_data_to_pickle_file(self.ET_driver.save_read_data, dir='data')
            self.enable_all_elements()
            self.test_time_timer.stop()

    def create_plot_items(self):
        # Add the two plots
        self.thermistor_plot = self.plot_layout.addPlot(row=0, col=0)
        self.flow_sensor_plot = self.plot_layout.addPlot(row=1, col=0)
        
        # Create the data numpy arrays
        self.num_elements_plot = self.input_data_rate * self.plot_x_time_limit
        self.num_thermistors = 12
        self.monitor_time_data = np.empty(shape=(self.num_elements_plot))                          # Each time the thermistor is read, the data shifted using np.roll and is added to the end of this array
        self.driver_time_data = np.empty(shape=(self.num_elements_plot))                           # Each time the thermistor is read, the data shifted using np.roll and is added to the end of this array
        self.thermistor_data = np.empty(shape=(self.num_thermistors, self.num_elements_plot))      # Each time the thermistor is read, the data shifted using np.roll and is added to the end of this array
        self.flow_sensor_data = np.empty(shape=(2, self.num_elements_plot))                   # Each time the flow sensor is read, the data shifted using np.roll and is added to the end of this array
        
        # Set background color of plots
        vb1 = self.thermistor_plot.getViewBox()
        vb1.setBackgroundColor('w')
        vb2 = self.flow_sensor_plot.getViewBox()
        vb2.setBackgroundColor('w')
        
        # Set the plot attributes
        self.thermistor_plot.showGrid(x=True, y=True)  # Show grid
        self.thermistor_plot.setLabel('left', 'Temperature', units='°C')  # Set y-axis label
        self.thermistor_plot.setLabel('bottom', 'Time', units='s')  # Set x-axis label
        self.thermistor_plot.setTitle('Thermistor Temperature vs. Time')  # Set plot title
        self.thermistor_plot.setLimits(yMin=20, yMax=150)  # Set y-axis limits
        self.thermistor_plot.addLegend(offset=(-1, 1))  # Set legend offset
        
        self.flow_sensor_plot.showGrid(x=True, y=True)  # Show grid
        self.flow_sensor_plot.setLabel('left', 'Flow Rate', units='mL/min')  # Set y-axis label
        self.flow_sensor_plot.setLabel('bottom', 'Time', units='s')  # Set x-axis label
        self.flow_sensor_plot.setTitle('Flow Sensor Flow Rate vs. Time')  # Set plot title
        self.flow_sensor_plot.setLimits(yMin=0, yMax=1000)  # Set y-axis limits
        self.flow_sensor_plot.addLegend(offset=(-1, 1))  # Set legend offset
        
        # Create the curve objects
        colors = [
            (255, 100, 50),     # Orange
            (255, 100, 200),    # Pink
            (255, 0, 0),        # Red
            (255, 255, 0),      # Yellow
            (0, 255, 0),        # Green
            (0, 255, 255),      # Cyan
            (0, 0, 255),        # Blue
            (255, 0, 255),      # Magenta
            (0, 0, 0),          # Black
            (200, 200, 255),    # Sky Blue
            (255, 200, 200),    # Salmon
            (200, 255, 200),    # Spring green           
        ]
        line_width = 2
        self.thermistor_curves = [self.thermistor_plot.plot(pen=pg.mkPen(color=colors[i], width=line_width), name=f"Thermistor {i+1}") for i in range(self.num_thermistors)]
        fs_inlet = self.flow_sensor_plot.plot(pen=pg.mkPen(color=colors[0], width=line_width), name="Inlet Flow Sensor")
        fs_outlet = self.flow_sensor_plot.plot(pen=pg.mkPen(color=colors[1], width=line_width), name="Outlet Flow Sensor")
        self.flow_sensor_curves = [fs_inlet, fs_outlet]
        
    def update_plots_from_teensy_input_data(self):
        """Updates the plots from the latest Teensy input data, not necessarily when Teensy data is received and stored
        This function is asyncronous and should be called from a separate thread"""
        # Need to use the self.monitor_input_data onject to grab the most recent data
        self.mutex.lock()
        try:
            # First check and see if the time has been reset, if so, reset the plots
            monitor_time_reset = self.monitor_input_data.time_ms < self.monitor_time_data[-1]
            driver_time_reset = self.driver_input_data.time_ms < self.driver_time_data[-1]
            
            if monitor_time_reset or driver_time_reset:
                self.plot_layout.removeItem(self.thermistor_plot)
                self.plot_layout.removeItem(self.flow_sensor_plot)
                self.create_plot_items()        # Reset the plots
            
            # if monitor_time_reset or driver_time_reset:
            # if self.monitor_input_data.time_ms < self.monitor_time_data[-1]: 
            #     self.thermistor_plot.clear()
            #     for curve in self.thermistor_curves:
            #         self.thermistor_plot.removeItem(curve)
            #     # Reset the time and data arrays
            #     self.monitor_time_data[:] = 0
            #     self.thermistor_data[:, :] = 0
            #     vb1 = self.thermistor_plot.getViewBox()
            #     #vb1.clear()
            # if self.driver_input_data.time_ms < self.driver_time_data[-1]:
            #     self.flow_sensor_plot.clear()
            #     for curve in self.flow_sensor_curves:
            #         self.flow_sensor_plot.removeItem(curve)
            #     # Reset the time and data arrays
            #     self.driver_time_data[:] = 0        
            #     self.flow_sensor_data[:, :] = 0
            #     vb2 = self.flow_sensor_plot.getViewBox()
            #     #vb2.clear()
            # if self.valve_input_data.time_ms < self.valve_time_data[-1]:
            #     self.flow_sensor_plot.clear()
            #     for curve in self.flow_sensor_curves:
            #         self.flow_sensor_plot.removeItem(curve)
            #     # Reset the time and data arrays
            #     self.valve_time_data[:] = 0        
            #     self.flow_sensor_data[:, :] = 0
            #     vb2 = self.flow_sensor_plot.getViewBox()
            #     #vb2.clear()
            
            # Update the thermistor plot
            if not (self.last_io_count_monitor == self.monitor_input_data.io_count):
                #self.log.debug("Updating plots from latest Teensy input data")
                
                # Get the time of the latest data
                self.monitor_time_data[:-1] = self.monitor_time_data[1:]                            # shift the time data in the array one sample left
                self.monitor_time_data[-1] = np.round(self.monitor_input_data.time_ms/1000, 3)      # add the latest time data to the end of the array, this is in seconds
                
                # Get the min and max value of the time for updating the X limits of the plots
                if self.monitor_time_data[-1]-self.monitor_time_data[0] > self.plot_x_time_limit:
                    min_time = self.monitor_time_data[-1]-self.plot_x_time_limit
                    max_time = self.monitor_time_data[-1]
                else:
                    min_time = self.monitor_time_data[0]
                    max_time = min_time + self.plot_x_time_limit
                max_time += 2    # Add some seconds to the max time to give some padding at the end of the plot
                    
                # Update the x limits of the plots
                self.thermistor_plot.setXRange(min_time, max_time, padding=0.0) 

                # For each curve in self.thermistor_curves, update the data in self.thermistor_data given the latest Teensy input data
                thermistor_temps = [self.monitor_input_data.thermistor_1_temp_c,
                                    self.monitor_input_data.thermistor_2_temp_c,
                                    self.monitor_input_data.thermistor_3_temp_c,
                                    self.monitor_input_data.thermistor_4_temp_c,
                                    self.monitor_input_data.thermistor_5_temp_c,
                                    self.monitor_input_data.thermistor_6_temp_c,
                                    self.monitor_input_data.thermistor_7_temp_c,
                                    self.monitor_input_data.thermistor_8_temp_c,
                                    self.monitor_input_data.thermistor_9_temp_c,
                                    self.monitor_input_data.thermistor_10_temp_c,
                                    self.monitor_input_data.thermistor_11_temp_c,
                                    self.monitor_input_data.thermistor_12_temp_c]
                for curve_num in range(len(self.thermistor_curves)):
                    self.thermistor_data[curve_num][:-1] = self.thermistor_data[curve_num][1:]      # Shift data in the array one sample left
                    self.thermistor_data[curve_num][-1] = thermistor_temps[curve_num]                          # Add latest data to the end of the array
                    curve = self.thermistor_curves[curve_num]
                    #self.log.debug(f"Shape of time data: {self.time_data.shape}")
                    #self.log.debug(f"Shape of thermistor data: {self.thermistor_data[:][curve_num].shape}")
                    curve.setData(self.monitor_time_data, self.thermistor_data[curve_num][:])
                    
                # Update the y limits of ALL of the thermistor curves on the plot
                min_y = np.min(self.thermistor_data)
                max_y = np.max(self.thermistor_data)
                padding = (min_y+max_y)/2 * 0.1     # Add 10% padding to the y limits
                self.thermistor_plot.setYRange(min_y, max_y, padding=padding)
                #self.log.debug("Updated thermistor plot y limits to: " + str(min_y) + " to " + str(max_y))

                # Update the last IO count
                self.last_io_count_monitor = self.monitor_input_data.io_count
            
            # Update the flow rate plot
            if not (self.last_io_count_driver == self.driver_input_data.io_count):   
                # Get the time of the latest data
                self.driver_time_data[:-1] = self.driver_time_data[1:]                            # shift the time data in the array one sample left
                self.driver_time_data[-1] = np.round(self.driver_input_data.time_ms/1000, 3)      # add the latest time data to the end of the array, this is in seconds
                
                # Get the min and max value of the time for updating the X limits of the plots
                if self.driver_time_data[-1]-self.driver_time_data[0] > self.plot_x_time_limit:
                    min_time = self.driver_time_data[-1]-self.plot_x_time_limit
                    max_time = self.driver_time_data[-1]
                else: 
                    min_time = self.driver_time_data[0]
                    max_time = min_time + self.plot_x_time_limit    
                max_time += 2    # Add some seconds to the max time to give some padding at the end of the plot
                
                # Update the x limits of the plots
                self.flow_sensor_plot.setXRange(min_time, max_time, padding=0.0)            
                
                # For each curve in self.flow_sensor_curves, update the data in self.flow_sensor_data given the latest Teensy input data
                flow_rates   = [self.driver_input_data.inlet_flow_sensor_ml_min,
                                self.driver_input_data.outlet_flow_sensor_ml_min]
                
                for curve_num in range(len(self.flow_sensor_curves)):
                    self.flow_sensor_data[curve_num][:-1] = self.flow_sensor_data[curve_num][1:]
                    self.flow_sensor_data[curve_num][-1] = flow_rates[curve_num]
                    curve = self.flow_sensor_curves[curve_num]
                    curve.setData(self.driver_time_data, self.flow_sensor_data[curve_num][:])
                    
                # Update the y limits of the flow sensor plot
                min_y = np.min(self.flow_sensor_data)
                max_y = np.max(self.flow_sensor_data)
                padding = (min_y+max_y)/2 * 0.1     # Add 10% padding to the y limits
                self.flow_sensor_plot.setYRange(min_y, max_y, padding=padding)
                #self.flow_sensor_plot.setLimits(min_y, max_y, padding=padding)
                #self.log.debug("Updated flow sensor plot y limits to: " + str(min_y) + " to " + str(max_y))
                    
                # Update the last IO count
                self.last_io_count_driver = self.driver_input_data.io_count
        finally:
            self.mutex.unlock()
            
    """ Update the element labels """
    def update_piezo_1_freq_slider_label(self, value):
        self.piezo_1_freq_slider_label.setText(f"Frequency: {value} (Hz)")
        #self.log.info(f"Piezo 1 frequency label update: {value} (Hz)")
    
    def update_piezo_1_amp_slider_label(self, value):
        self.piezo_1_amp_slider_label.setText(f"Amplitude: {value} (V p-p)")
        #self.log.info(f"Piezo 1 amplitude label update: {value} (V)")
        
    def update_piezo_1_phase_slider_label(self, value):
        self.piezo_1_phase_slider_label.setText(f"Phase: {value} (deg)")
        #self.log.info(f"Piezo 1 phase label update: {value} (deg)")
        
    def update_piezo_2_freq_slider_label(self, value):
        self.piezo_2_freq_slider_label.setText(f"Frequency: {value} (Hz)")
        #self.log.info(f"Piezo 2 frequency label update: {value} (Hz)")
        
    def update_piezo_2_amp_slider_label(self, value):
        self.piezo_2_amp_slider_label.setText(f"Amplitude: {value} (V p-p)")
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
    
    def update_heater_block_temp_gauge_label(self, value):
        self.heater_block_temp_gauge_label.setText(f"Heat Block Temp: {value} (°C)")
        #self.log.info(f"Heat block temperature label update: {value} (°C)")
        
    def update_inlet_temp_gauge_label(self, value):
        self.inlet_temp_gauge_label.setText(f"Inlet Temp: {value} (°C)")
        #self.log.info(f"Inlet temperature label update: {value} (°C)")
    
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
    
    def update_heater_block_temp_gauge_value(self, value):
        self.heater_block_temp_gauge.setValue(value)
        #self.log.info(f"Heater block temp guage update, temperature: {value} (°C)")
        
    def update_inlet_temp_gauge_value(self, value):
        self.inlet_temp_gauge.setValue(value)
        #self.log.info(f"Inlet temp guage update, temperature: {value} (°C)")
        
    """ Update button states """
    def update_start_test_button(self, state):
        self.start_test_button.setChecked(state)
        #self.log.info(f"Send data button update, state: {state}")
    
    def update_preheat_button(self, state):
        self.preheat_button.setChecked(state)
        #self.log.info(f"Preheat button update, state: {state}")
    
    def update_emergency_stop_button(self, state):
        self.emergency_stop_button.setChecked(state)
        #self.log.info(f"Emergency stop button update, state: {state}")

    def update_piezo_1_enable_button(self, state):
        self.piezo_1_enable_button.setChecked(state)
        #self.log.info(f"Piezo 1 enable button update, state: {state}")
        
    def update_piezo_2_enable_button(self, state):
        self.piezo_2_enable_button.setChecked(state)
        #self.log.info(f"Piezo 2 enable button update, state: {state}")
        
    def update_rope_enable_button(self, state):
        self.rope_enable_button.setChecked(state)
        #self.log.info(f"Rope temp enable button update, state: {state}")
        
    def update_heater_block_enable_button(self, state):
        self.heater_block_enable_button.setChecked(state)
        #self.log.info(f"Heat flux enable button update, state: {state}")
        
    """ Button callbacks """
    @qasync.asyncSlot()
    async def start_test_button_click(self):
        self.start_test_button.setEnabled(False)
        self.log.info("Sending data to the teensy, starting test...")
        ################# SEND THE DATA TO SERIAL CONNECTION #################
        # Go through each variable in the output data struct and set the value given the value in the GUI
        self.update_output_ETdata(emergency=False)   
        # Disable the piezos temporarily, we turn them on half way through the test
        self.driver_output_data.piezo_1_enable = False
        self.driver_output_data.piezo_2_enable = False
        # Send the data to the teensys
        await self.ET_driver.send_data()
        await self.ET_monitor.send_data()
        #self.start_test_button.setText("TESTING...")
        #self.disable_all_elements()
        self.emergency_stop_button.setEnabled(True)     # Enable the emergency stop button
        # Reset the io count (This overwrites the data in the save_read_data object)
        self.ET_monitor.save_read_data.io_count = 0
        self.ET_driver.save_read_data.io_count = 0        
        # Start saving on the objects
        self.ET_driver.start_saving()
        self.ET_monitor.start_saving()
        self.current_test_time = self.test_time     # Set the current test time to the total test time
        self.test_time_timer.start(1000)            # Start the test time timer for updating the test time label wihtout blocking the emergency stop button
    
    @qasync.asyncSlot()
    async def emergency_stop_button_click(self):
        self.emergency_stop_button.setChecked(True)
        #Turn off all boolean buttons
        self.piezo_1_enable_button.setChecked(False)
        self.piezo_2_enable_button.setChecked(False)
        self.rope_enable_button.setChecked(False)
        self.heater_block_enable_button.setChecked(False)
        self.start_test_button.setChecked(False)
        self.preheat_button.setChecked(False)
        
        self.current_test_time = 0      # This will stop any current testing
        
        # Disable all elements
        #self.disable_all_elements()
        #self.emergency_stop_button.setEnabled(False)
        self.emergency_stop_button.setText("Stopping...")
        self.log.error("Emergency stop")
        ################# SEND THE DATA TO SERIAL CONNECTION #################
        # Set all values to zero or False in the output for driver
        self.update_output_ETdata(emergency=False)      # Init values if needed
        self.update_output_ETdata(emergency=True)
        await self.ET_driver.send_data()
        await self.ET_monitor.send_data()       
        await asyncio.sleep(1)
        self.emergency_stop_button.setChecked(False)
        self.emergency_stop_button.setText("Emergency Stop")
        #self.emergency_stop_button.setEnabled(True)
        #self.enable_all_elements()
        
        
    @qasync.asyncSlot()
    async def preheat_button_click(self):
        """Enable rope heater and the heater block at given temperature/heatflux, send data to monitor"""
        state = self.preheat_button.isChecked()
        
        # Check the element buttons
        self.heater_block_enable_button.setChecked(state)
        self.rope_enable_button.setChecked(state)
        
        # Set the data struct
        self.monitor_output_data.rope_heater_enable             = np.bool_(self.rope_enable_button.isChecked())
        self.monitor_output_data.heater_block_enable            = np.bool_(self.heater_block_enable_button.isChecked())
        self.monitor_output_data.inlet_fluid_temp_setpoint_c    = np.float32(self.rope_temp_slider.value())
        self.monitor_output_data.heat_flux                      = np.float32(self.heat_flux_slider.value())
        self.monitor_output_data.reset_time                     = np.bool_(False)
        await self.ET_monitor.send_data()
        
  
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

    def disable_all_elements(self):
        """Disable all elements in the window, except the emergency stop button and the plots"""
        for widget in self.window_widget.children():
            if not isinstance(widget, pg.GraphicsLayoutWidget):
                try:
                    widget.setEnabled(False)
                except:
                    pass
        # Re-enable the emergency stop button
        self.emergency_stop_button.setEnabled(True)
        self.emergency_stop_button.setChecked(False)
        self.emergency_stop_button.setCheckable(True)
        
    def enable_all_elements(self):
        """Enable all elements that are active components, don't enable the heater block temp guage"""
        for widget in self.window_widget.children():
            try:
                widget.setEnabled(True)
            except:
                pass
        # Disable the heater block temp gauge
        self.heater_block_temp_gauge.setEnabled(False)
        
            
    def update_output_ETdata(self, emergency=False, reset_time=True):
        """ Write the output iodata object to the output queue before sending to the serial connection """
        if emergency:
            # Turn off all heaters and piezos
            self.monitor_output_data.rope_heater_enable = np.bool_(False)
            self.monitor_output_data.heater_block_enable = np.bool_(False)
            self.driver_output_data.piezo_1_enable = np.bool_(False)
            self.driver_output_data.piezo_2_enable = np.bool_(False)
            self.driver_output_data.reset_time = np.bool_(False)
            self.monitor_output_data.reset_time = np.bool_(False)
            
            # Set all values to zero or False in the output for monitor
            # for out_data in [self.driver_output_data, self.monitor_output_data]:
            #     for variable, _dtype in out_data.struct_def.items():
            #         if _dtype == np.bool_:
            #             setattr(self, variable, False)
            #         elif _dtype == np.float32:
            #             setattr(self, variable, 0.0)
            #         elif _dtype == np.int32 or _dtype == np.uint32:
            #             setattr(self, variable, 0)
            #         else:
            #             setattr(self, variable, 0)    
        else:
           
            # Update the driver elements
            self.driver_output_data.reset_time          = np.bool_(reset_time)
            self.driver_output_data.signal_type_piezo_1 = np.uint8(0)     # TODO: Add signal type selection
            self.driver_output_data.signal_type_piezo_2 = np.uint8(0)     # TODO: Add signal type selection
            self.driver_output_data.piezo_1_freq_hz     = np.float32(self.piezo_1_freq_slider.value())
            self.driver_output_data.piezo_1_vpp         = np.float32(self.piezo_1_amp_slider.value())
            self.driver_output_data.piezo_1_phase_deg   = np.float32(self.piezo_1_phase_slider.value())
            self.driver_output_data.piezo_2_freq_hz     = np.float32(self.piezo_2_freq_slider.value())
            self.driver_output_data.piezo_2_vpp         = np.float32(self.piezo_2_amp_slider.value())
            self.driver_output_data.piezo_2_phase_deg   = np.float32(self.piezo_2_phase_slider.value())
            self.driver_output_data.piezo_1_enable      = np.bool_(self.piezo_1_enable_button.isChecked())
            self.driver_output_data.piezo_2_enable      = np.bool_(self.piezo_2_enable_button.isChecked())
            
            # Update the monitor elements
            self.monitor_output_data.reset_time                     = np.bool_(reset_time)
            self.monitor_output_data.heater_block_enable            = np.bool_(self.heater_block_enable_button.isChecked())
            self.monitor_output_data.rope_heater_enable             = np.bool_(self.rope_enable_button.isChecked())
            self.monitor_output_data.heat_flux                      = np.float32(self.heat_flux_slider.value())
            self.monitor_output_data.inlet_fluid_temp_setpoint_c    = np.float32(45)      # TODO: Get this from the GUI
    
   
   
        

if __name__ == '__main__':
    # app = QApplication(sys.argv)
    # view = 
    # view.show()
    # app.exec()
    pass