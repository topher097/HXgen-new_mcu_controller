
import os
import sys

import asyncio
from PySide6.QtWidgets import QApplication
from qasync import QEventLoop
import serial

from serial_interface import SerialInterface
from data_transfer import IOData
from application import MainWindow
import logger


class HXController:
    """Main controller class for the HX2.5 PC Controller application
    
    Arguments:
        log_dir {str} -- Path to directory where log files are saved
        com_port {str} -- COM port to connect to
        baud_rate {int} -- Baud rate to connect at
    """
    def __init__(self, log_dir: str, com_port: str, baud_rate: int) -> None:
        # Init the logging
        self.log = logger.HXLogger(log_dir).log
        self.log.info("Starting HX2.5 PC Controller")
        
        # Init the app and event loop
        self.app = QApplication(sys.argv)
        self.loop = QEventLoop(self.app)
        asyncio.set_event_loop(self.loop)
        
        # Init the serial connection
        self.serial_interface = SerialInterface(log=self.log)
        self.loop.run_until_complete(self.setup_serial(com_port, baud_rate))  
        self.log.info(f"Serial connection established on port {com_port} at {baud_rate} baud")  
        
        # Initialize MainWindow after setting up serial connection
        window_title = "HX2.5 PC Controller"
        self.view = MainWindow(window_title=window_title, log=self.log, serial_interface=self.serial_interface)
        self.log.info("MainWindow initialized")
        
    async def setup_serial(self, com_port, baud_rate):
        """Setup the serial connection via the SerialInterface class"""
        self.log.debug(f"Setting up serial connection on port {com_port} at {baud_rate} baud")
        await self.serial_interface.create_connection(self.loop, com_port, baud_rate)
        
    def stop(self):
        """Stop the application"""
        self.log.info("Stopping application and closing serial connection")
        self.serial_interface.close_connection()
        self.log.debug("Serial connection closed")
        self.loop.stop()
        self.log.debug("Event loop stopped")
        self.app.quit()
        self.log.debug("Application quit")
        
    def start(self):
        """Start the application"""
        self.log.info("Starting application and displaying MainWindow")
        self.view.show()
        self.loop.run_forever()


if __name__ == "__main__":
    # Define the log output path
    lod_dir = os.path.join(os.getcwd(), 'logs')
    com_port = 'COM9'
    baud_rate = 9600
    
    controller = HXController(lod_dir, com_port, baud_rate)
    controller.start()