from logging import Logger
from logging import handlers, Formatter, NOTSET
import time
import os
import sys
import colorlog
from colorlog import ColoredFormatter

class CustomLogger(Logger):
    """Class to handle logging configuration and creation of loggers""" 

    def __init__(self, log_dir: str, 
                 console_log_level: str="DEBUG", 
                 file_log_level: str="DEBUG", 
                 console_verbosity: str="verbose-console",
                 file_verbosity: str="verbose",
                 file_handler_name: str="file_handler") -> None:
        """Initialize the logging configuration
        
        Arguments:
            log_dir: str
                Path to directory where log files are saved
            console_log_level: str, default="DEBUG"
                The log level for the console handler
            file_log_level: str, default="DEBUG"
                The log level for the file handler
            console_verbosity: str, default="verbose-console"
                The verbosity for the console handler
            file_verbosity: str, default="verbose"
                The verbosity for the file handler
            file_handler_name: str, default="file_handler"
                The name of the file handler
        """
        # Init the parent class
        super().__init__(self, NOTSET)
        
        # Set the log file path
        self.log_dir = log_dir
        self.log_filename = f'{int(time.time())}.log'
        self.log_filepath = os.path.join(self.log_dir, self.log_filename)
        
        # Check to see if the log dir exists, if it does not, then create it
        created_log_dir = False
        if not os.path.exists(self.log_dir):
            os.makedirs(self.log_dir)
            created_log_dir = True
        
        # Set the handler names, log levels, and the verbosity of the loggers
        self.file_handler_name = file_handler_name
        self.file_log_level = file_log_level
        self.file_verbosity = file_verbosity
        self.console_handler_name = "console"
        self.console_log_level = console_log_level
        self.console_verbosity = console_verbosity
        
        # Create the logger object given in the config file
        self.log: Logger = self._create_logger()
        time.sleep(0.1) # Sleep to allow the logger to be created and log file to be initialized
        
        # Make it so all uncaught exceptions are sent to the logger
        sys.excepthook = self.handle_uncaught_exceptions
        
        # Log the logger attributes
        self.log.debug(f"Log file path: {self.log_filepath}, log level: {self.file_log_level}, verbosity: {self.file_verbosity}")

        # If the log dir was created, then log it
        if created_log_dir:
            self.log.debug(f"Created log directory at: '{self.log_dir}' since it did not exist")
            
            
    def handle_uncaught_exceptions(self, exc_type, exc_value, exc_traceback):
        """Uncaught exceptions will be sent to the logger"""
        if issubclass(exc_type, KeyboardInterrupt):
            sys.__excepthook__(exc_type, exc_value, exc_traceback)
            return
        self.error("Uncaught exception", exc_info=(exc_type, exc_value, exc_traceback))
        
        
    def _create_logger(self) -> Logger:
        """Returns the logger object given in the config file"""
        file_handler = self._get_handler(self.file_handler_name)            # File log handler
        console_handler = self._get_handler(self.console_handler_name)      # Console log handler
        logger = colorlog.getLogger(self.file_handler_name)                 # Logger object
        
        logger.addHandler(file_handler)                     # Add the file handler to the logger
        logger.addHandler(console_handler)                  # Add the console handler to the logger
        logger.setLevel('DEBUG')                            # The handler levels will trump this, but we want to always set the level to debug, otherwise it will be set to warning by default
        return logger
        
    
    def _get_handler(self, handler_name: str) -> handlers:
        """Create the logger handlers"""
        if handler_name == self.console_handler_name:
            # Console handler (streams to the console)
            handler = colorlog.StreamHandler()
            handler.setFormatter(self._get_formatter(self.console_verbosity))
            handler.setLevel(self.console_log_level)
        elif handler_name == self.file_handler_name:
            # Controller handler (streams to the controller log file)
            handler = handlers.RotatingFileHandler(
                filename=self.log_filepath,
                mode='w',
                maxBytes=52428800,
                backupCount=7
                )
            handler.setFormatter(self._get_formatter(self.file_verbosity))
            handler.setLevel(self.file_log_level)
        else:
            raise ValueError(f"Invalid handler '{handler_name}' set in the config file, must be self.console_handler_name or self.file_handler_name")
        return handler
    
    
    def _get_formatter(self, formatter_name: str) -> Formatter:
        """Create the formatter for the loggers. Can add different formatters here for different loggers"""
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

     
def test_logger():
    """Test the logger"""
    # Define the log output path
    lod_dir = os.path.join(os.getcwd(), 'logs')
    logger = CustomLogger(lod_dir)              # Using the default log handler names, verbosity, and log levels
    logger.log.debug("Test debug message")
    logger.log.info("Test info message")
    logger.log.warning("Test warning message")
    logger.log.error("Test error message")
    logger.log.critical("Test critical message")
 
if __name__ == "__main__":
    # Test the logger
    test_logger()