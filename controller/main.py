import numpy as np
import os
import sys

import asyncio
from PySide6.QtWidgets import QApplication
from qasync import QEventLoop
from pyEasyTransfer import PyEasyTransfer, IODataArrays
from application import MainWindow
import logger

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
    def __init__(self, log_dir: str, com_port: str, baud_rate: int, input_struct_def: dict[str, np.dtype], output_struct_def: dict[str, np.dtype], byte_order: str, save_read_data: Optional[IODataArrays]=None) -> None:
        # Init the logging
        self.log = logger.HXLogger(log_dir).log
        self.log.info("Starting HX2.5 PC Controller")
        
        # Init the app and event loop
        self.app = QApplication(sys.argv)
        self.loop = QEventLoop(self.app)
        asyncio.set_event_loop(self.loop)
        
        # Init the serial connections
        self.input_serial_interface = PyEasyTransfer(log=self.log, com_port=com_port, baud_rate=baud_rate, struct_def=input_struct_def, byte_order=byte_order, mode='input', save_read_data=save_read_data)
        self.output_serial_interface = PyEasyTransfer(log=self.log, com_port=com_port, baud_rate=baud_rate, struct_def=output_struct_def, byte_order=byte_order, mode='output')
        self.loop.run_until_complete(self.input_serial_interface.open())
        self.loop.run_until_complete(self.output_serial_interface.open())
        self.log.info(f"Serial connections established on port {com_port} at {baud_rate} baud")  
        
        # Initialize MainWindow after setting up serial connections
        window_title = "HX2.5 PC Controller"
        self.view = MainWindow(window_title=window_title, log=self.log, input_serial_interface=self.input_serial_interface, output_serial_interface=self.output_serial_interface)
        self.log.info("MainWindow initialized")
        
    def stop(self):
        """Stop the application"""
        self.log.info("Stopping application and closing serial connections")
        self.loop.run_until_complete(self.input_serial_interface.close())
        self.loop.run_until_complete(self.output_serial_interface.close())
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


# Input data from the serial connection struct definition, exactly as defined in the Arduino code
input_struct_def = {"time_ms": np.uint32,
                    "temp_C": np.float32,
                    "hello_received": np.bool_}  

# Output data to the serial connection struct definition, exactly as defined in the Arduino code
output_struct_def = {"hello_flag": np.bool_} 

if __name__ == "__main__":
    # Define the log output path
    log_dir = os.path.join(os.getcwd(), 'logs')
    com_port = 'COM14'
    baud_rate = 115200
    byte_order = 'little-endian'
    save_read_data = IODataArrays(input_struct_def, 1000)                   # initialize IODataArrays if necessary
    
    controller = HXController(log_dir, com_port, baud_rate, input_struct_def, output_struct_def, byte_order, save_read_data)
    controller.start()
