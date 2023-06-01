import numpy as np
import os
import sys

import asyncio
from PySide6.QtWidgets import QApplication
from qasync import QEventLoop
from pyEasyTransfer import PyEasyTransfer, ETDataArrays
from application import MainWindow
import logger
import signal
from asyncio_button_helper import AsyncHelper

import serial_asyncio
from typing import Optional

class HXController:
    """Main controller class for the HX2.5 PC Controller application
    
    Arguments:
        log_dir {str} -- Path to directory where log files are saved
        com_port {str} -- COM port to connect to
        baud_rate {int} -- Baud rate to connect at
        input_struct_def {dict[str, np.dtype]} -- Dictionary of the input struct definition
        output_struct_def {dict[str, np.dtype]} -- Dictionary of the output struct definition
        byte_order {str} -- Byte order to use when packing and unpacking data
        save_read_data {Optional[IODataArrays]} -- IODataArrays object to store the data received from the Arduino (default: {None})
    """
    def __init__(self, log_dir: str, 
                 input_data_rate: int, test_time: int,
                 monitor_serial_interface: PyEasyTransfer,
                 driver_serial_interface: PyEasyTransfer) -> None:
        # Init the logging
        self.log = logger.HXLogger(log_dir).log
        self.log.info("Starting HX2.5 PC Controller")
        
        # Init the app and event loop
        self.app = QApplication(sys.argv)
        self.loop = QEventLoop(self.app)
        asyncio.set_event_loop(self.loop)       # Set the event loop to the QEventLoop
        
        # Init and set log
        interfaces = [monitor_serial_interface, driver_serial_interface]
        for interface in interfaces:
            # Open the serial connection
            self.loop.run_until_complete(self.open_connection(interface))
            self.log.info(f"Serial connection established on port {interface.com_port} at {interface.baud_rate} baud")
            # Set the log object
            interface.set_log(self.log)
        
        # Initialize MainWindow after setting up serial connections
        window_title = "HX2.5 PC Controller"
        self.view = MainWindow(window_title=window_title, 
                               loop=self.loop, 
                               log=self.log, 
                               monitor_serial_interface=monitor_serial_interface,
                               driver_serial_interface=driver_serial_interface,
                               input_data_rate=input_data_rate, 
                               test_time=test_time)
        #self.async_helper = AsyncHelper(self.view, self.view.send_data, self.loop)
        self.log.info("MainWindow initialized")
        
    def stop(self):
        """Stop the application"""
        self.log.info("Stopping application and closing serial connections")
        self.loop.run_until_complete(self.close_connection())
        self.log.debug("Serial connections closed")
        self.loop.stop()
        self.log.debug("Event loop stopped")
        self.app.quit()
        self.log.debug("Application quit")
        
    def start(self):
        """Start the application"""
        self.log.info("Starting application and displaying MainWindow")
        #signal.signal(signal.SIGINT, signal.SIG_DFL)
        self.view.show()
        self.loop.run_forever()
        
    async def open_connection(self, serial_connection: PyEasyTransfer):
        """Open the serial connection"""
        await serial_connection.open()
        
    async def close_connection(self, serial_connection: PyEasyTransfer):
        """Close the serial connection"""
        await serial_connection.close()


# Input data from the serial connection struct definition, exactly as defined in the Arduino code
monitor_input_struct_def = {"time_ms": np.uint32,
                            "time_us": np.uint32,
                            "inlet_fluid_temp_c": np.float32,
                            "inlet_fluid_temp_setpoint_c": np.float32,
                            "heater_block_enable": np.bool_,
                            "rope_heater_enable": np.bool_,
                            "heat_flux": np.float32,
                            "thermistor_1_temp_c": np.float32,
                            "thermistor_2_temp_c": np.float32,
                            "thermistor_3_temp_c": np.float32,
                            "thermistor_4_temp_c": np.float32,
                            "thermistor_5_temp_c": np.float32,
                            "thermistor_6_temp_c": np.float32,
                            "thermistor_7_temp_c": np.float32,
                            "thermistor_8_temp_c": np.float32,
                            "thermistor_9_temp_c": np.float32,
                            "thermistor_10_temp_c": np.float32,
                            "thermistor_11_temp_c": np.float32,
                            "thermistor_12_temp_c": np.float32,
                            "thermistor_13_temp_c": np.float32,
                            "thermistor_14_temp_c": np.float32
                            }

# Output data to the serial connection struct definition, exactly as defined in the Arduino code
monitor_output_struct_def    = {"reset_time": np.bool_,
                                "heater_block_enable": np.bool_,
                                "rope_heater_enable": np.bool_,
                                "heat_flux": np.float32,
                                "inlet_fluid_temp_setpoint_c": np.float32
                                }

driver_input_struct_def = {"time_ms": np.uint32,
                           "time_us": np.uint32,
                           "signal_type_piezo_1": np.uint8,
                           "signal_type_piezo_2": np.uint8,
                           "piezo_1_enable": np.bool_,
                           "piezo_2_enable": np.bool_,
                           "piezo_1_freq_hz": np.float32,
                           "piezo_2_freq_hz": np.float32,
                           "piezo_1_vpp": np.float32,
                           "piezo_2_vpp": np.float32,
                           "piezo_1_phase_deg": np.float32,
                           "piezo_2_phase_deg": np.float32,
                           "inlet_flow_sensor_ml_min": np.float32,
                           "outlet_flow_sensor_ml_min": np.float32
                           }

driver_output_struct_def = {"reset_time": np.bool_,
                            "signal_type_piezo_1": np.uint8,
                            "signal_type_piezo_2": np.uint8,
                            "piezo_1_enable": np.bool_,
                            "piezo_2_enable": np.bool_,
                            "piezo_1_freq_hz": np.float32,
                            "piezo_2_freq_hz": np.float32,
                            "piezo_1_vpp": np.float32,
                            "piezo_2_vpp": np.float32,
                            "piezo_1_phase_deg": np.float32,
                            "piezo_2_phase_deg": np.float32,
                            }

if __name__ == "__main__":
    # Define the log output path
    log_dir             = os.path.join(os.getcwd(), 'logs')
    baud_rate           = 115200
    byte_format         = 'little-endian'
    input_data_rate     = 20                # Number of data points received per second
    test_time           = 60                # Number of seconds to run the test for (get from arduino code)
    max_ele             = int(input_data_rate*test_time*1.1)
    monitor_save_read_data  = ETDataArrays(monitor_input_struct_def, name='monitor', max_elements=max_ele)     # initialize for test with some extra space
    driver_save_read_data   = ETDataArrays(driver_input_struct_def, name='driver', max_elements=max_ele)       # initialize for test with some extra space
    
    monitor_interface  = PyEasyTransfer(com_port="COM9", 
                                        baud_rate=baud_rate, 
                                        input_struct_def=monitor_input_struct_def, 
                                        output_struct_def=monitor_output_struct_def,
                                        byte_format=byte_format, 
                                        mode='both', 
                                        save_read_data=monitor_save_read_data,
                                        name="monitor")
    
    driver_interface   = PyEasyTransfer(com_port="COM12",
                                        baud_rate=baud_rate,
                                        input_struct_def=driver_input_struct_def,
                                        output_struct_def=driver_output_struct_def,
                                        byte_format=byte_format,
                                        mode='both',
                                        save_read_data=driver_save_read_data,
                                        name="driver")
    
    controller = HXController(log_dir=log_dir, 
                              monitor_serial_interface=monitor_interface, 
                              driver_serial_interface=driver_interface,
                              input_data_rate=input_data_rate, 
                              test_time=test_time)
    controller.start()
