import numpy as np
import os
import sys

import asyncio
from PySide6.QtWidgets import QApplication
from qasync import QEventLoop
from pyEasyTransfer import PyEasyTransfer, ETDataArrays
from application import MainWindow
import logger

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
    def __init__(self, log_dir: str, com_port: str, baud_rate: int, input_data_rate: int, test_time: int,
                 input_struct_def: dict[str, np.dtype], output_struct_def: dict[str, np.dtype], 
                 byte_order: str, save_read_data: Optional[ETDataArrays]=None) -> None:
        # Init the logging
        self.log = logger.HXLogger(log_dir).log
        self.log.info("Starting HX2.5 PC Controller")
        
        # Init the app and event loop
        self.app = QApplication(sys.argv)
        self.loop = QEventLoop(self.app)
        asyncio.set_event_loop(self.loop)
        
        # Init the serial connections
        self.log.info("Initializing serial connection")
        self.serial_interface       = PyEasyTransfer(log=self.log, com_port=com_port, baud_rate=baud_rate, 
                                                     input_struct_def=input_struct_def, output_struct_def=output_struct_def,
                                                     byte_order=byte_order, mode='both', 
                                                     save_read_data=save_read_data)

        self.loop.run_until_complete(self.open_connection())
        self.log.info(f"Serial connections established on port {com_port} at {baud_rate} baud")  
        # Read data from the serial connection at the specified rate forever
        self.loop.create_task(self.read_data())
        
        # Initialize MainWindow after setting up serial connections
        window_title = "HX2.5 PC Controller"
        self.view = MainWindow(window_title=window_title, loop=self.loop, log=self.log, serial_interface=self.serial_interface,
                               input_data_rate=input_data_rate, test_time=test_time)
        self.log.info("MainWindow initialized")
        
    async def read_data(self):
        while True:
            await self.serial_interface.wait_for_data()
        
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
        self.view.show()
        self.loop.run_forever()
        
    async def open_connection(self):
        """Open the serial connection"""
        await self.serial_interface.open()
        
    async def close_connection(self):
        """Close the serial connection"""
        await self.serial_interface.close()


# Input data from the serial connection struct definition, exactly as defined in the Arduino code
input_struct_def = {"time_us": np.uint32,
                    "time_ms": np.uint32,
                    "inlet_flow_sensor_ml_min": np.float32,
                    "outlet_flow_sensor_ml_min": np.float32,
                    "inlet_fluid_temp_c": np.float32,
                    "inlet_fluid_temp_setpoint_c": np.float32,
                    "piezo_1_freq_hz": np.float32,
                    "piezo_2_freq_hz": np.float32,
                    "piezo_1_vpp": np.float32,
                    "piezo_2_vpp": np.float32,
                    "piezo_1_phase_deg": np.float32,
                    "piezo_2_phase_deg": np.float32,
                    "heater_block_enable": np.bool_,
                    "rope_heater_enable": np.bool_,
                    "piezo_1_enable": np.bool_,
                    "piezo_2_enable": np.bool_,
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
                    "thermistor_14_temp_c": np.float32,
                    "num_records_sd_card": np.uint32,
                    "cptr": np.uint32
                    }  

# Output data to the serial connection struct definition, exactly as defined in the Arduino code
output_struct_def = {"heater_block_enable": np.bool_,
                     "rope_heater_enable": np.bool_,
                     "heat_flux": np.float32,
                     "inlet_fluid_temp_setpoint_c": np.float32,
                     "piezo_1_freq_hz": np.float32,
                     "piezo_2_freq_hz": np.float32,
                     "piezo_1_vpp": np.float32,
                     "piezo_2_vpp": np.float32,
                     "piezo_1_phase_deg": np.float32,
                     "piezo_2_phase_deg": np.float32,
                     "piezo_1_enable": np.bool_,
                     "piezo_2_enable": np.bool_
                     } 

if __name__ == "__main__":
    # Define the log output path
    log_dir = os.path.join(os.getcwd(), 'logs')
    com_port = 'COM9'
    baud_rate = 115200
    byte_order = 'little-endian'
    input_data_rate = 50        # Number of data points received per second
    test_time = 60              # Number of seconds to run the test for (get from arduino code)
    save_read_data = ETDataArrays(input_struct_def, max_elements=int(input_data_rate*test_time*1.1))       # initialize for test with some extra space
    
    controller = HXController(log_dir, com_port, baud_rate, input_data_rate, 
                              test_time, input_struct_def, output_struct_def, 
                              byte_order, save_read_data)
    controller.start()
