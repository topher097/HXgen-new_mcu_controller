from logging import Logger
from logging import handlers, Formatter
import time
import os
import sys
import colorlog
from colorlog import ColoredFormatter

class HXLogger(Logger):
    """Class to handle logging configuration and creation of loggers""" 

    def __init__(self, log_dir: str) -> None:
        """Initialize the logging configuration
        
        Arguments:
            log_dir {str} -- Path to directory where log files are saved
        """
        self.log_dir = log_dir
        self.log_filename = f'{int(time.time())}.log'
        self.log_filepath = os.path.join(self.log_dir, self.log_filename)
        
        # Create the logger object given in the config file
        self.log: Logger = self._create_logger()
        
    def _create_logger(self) -> Logger:
        """Returns the logger object given in the config file"""
        file_handler = self._get_handler('controller')
        console_handler = self._get_handler('console')
        logger = colorlog.getLogger('controller')
        
        logger.addHandler(file_handler)
        logger.addHandler(console_handler)
        logger.setLevel('DEBUG')
        return logger
        
    
    def _get_handler(self, handler_name: str) -> handlers:
        """Create the logger handlers"""
        if handler_name == 'console':
            # Console handler (streams to the console)
            handler = colorlog.StreamHandler()
            handler.setFormatter(self._get_formatter('verbose-console'))
            handler.setLevel('DEBUG')
        elif handler_name == 'controller':
            # Controller handler (streams to the controller log file)
            handler = handlers.RotatingFileHandler(
                filename=self.log_filepath,
                mode='w',
                maxBytes=52428800,
                backupCount=7
                )
            handler.setFormatter(self._get_formatter('verbose'))
            handler.setLevel('DEBUG')
        else:
            raise ValueError(f"Invalid handler '{handler_name}' set in the config file, must be 'console' or 'controller'")
        return handler
    
    
    def _get_formatter(self, formatter_name: str) -> Formatter:
        """Create the formatter for the loggers"""
        if formatter_name == 'verbose-console':
            formatter = ColoredFormatter(
                stream=sys.stdout,
                fmt="%(log_color)s[%(asctime)s] %(levelname)-8s - %(filename)-20s - %(funcName)-35s - %(lineno)-4d - %(message)s",
                datefmt="%d/%b/%Y %H:%M:%S",
                reset=True,
                log_colors={
                    'DEBUG':    'cyan',
                    'INFO':     'green',
                    'WARNING':  'yellow',
                    'ERROR':    'red',
                    'CRITICAL': 'red,bg_white'},
                secondary_log_colors={},
                style='%'
                )
            # formatter = ColoredFormatter(
            #     fmt="%(log_color)s%(levelname)s:%(name)s:%(message)s",
            #     )
        elif formatter_name == 'simple-console':
            formatter = ColoredFormatter(
                fmt="%(log_color)s%(levelname)-8s %(message)s",
                datefmt="%d/%b/%Y %H:%M:%S",
                reset=True,
                log_colors={
                    'DEBUG':    'cyan',
                    'INFO':     'green',
                    'WARNING':  'yellow',
                    'ERROR':    'red',
                    'CRITICAL': 'red,bg_white'},
                secondary_log_colors={},
                style='%'
                )
        elif formatter_name == 'verbose':
            # Not a colored formatter, just a regular one
            formatter = Formatter(
                fmt="[%(asctime)s] %(levelname)s [%(filename)s:%(funcName)s:%(lineno)d] %(message)s",
                datefmt="%d/%b/%Y %H:%M:%S",
                style='%'
                )
        else:
            raise ValueError(f"Invalid formatter '{formatter_name}' set in the config file, must be 'verbose-console', 'simple-console' or 'verbose'")
        return formatter
     
 
if __name__ == "__main__":
    # Define the log output path
    lod_dir = os.path.join(os.getcwd(), 'logs')
    logger = HXLogger(lod_dir)
    logger.log.info("Test info message")
    logger.log.debug("Test debug message")
    logger.log.warning("Test warning message")
    logger.log.error("Test error message")
    logger.log.critical("Test critical message")