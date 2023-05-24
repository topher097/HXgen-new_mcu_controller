from PySide6.QtWidgets import QApplication
import os
import sys

from application import MainWindow
import logger


class HXController:
    """Main controller class for the HX2.5 PC Controller application
    
    Arguments:
        log_dir {str} -- Path to directory where log files are saved
    """
    def __init__(self, log_dir: str) -> None:
        # Init the logging
        self.log = logger.HXLogger(log_dir).log
        self.log.info("Starting HX2.5 PC Controller")
        
        # Init the application
        self.app = QApplication(sys.argv)
        window_title = "HX2.5 PC Controller"
        self.view = MainWindow(window_title=window_title, log=self.log)


if __name__ == "__main__":
    # Define the log output path
    lod_dir = os.path.join(os.getcwd(), 'logs')
    controller = HXController(lod_dir)
    controller.view.show()
    controller.app.exec()