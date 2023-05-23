import logging
from logging import Logger
from logging.config import dictConfig
import time
import os


class HXLogger:
    """Class to handle logging configuration and creation of loggers"""
    def __init__(self, log_dir: str) -> None:
        """Initialize the logging configuration
        
        Arguments:
            log_dir {str} -- Path to directory where log files are saved
        """
        self.log_dir = log_dir
        
        # Create the logger object given in the config file
        self.log = self._create_loggers()
        

    def _create_loggers(self) -> Logger:
        """Create the logging configuration and instantiate the logger objects for use"""
        logging_config = dict(
            version=1,
            formatters={
                'verbose': {
                    'format': ("[%(asctime)s] %(levelname)s "
                            "[%(filename)s:%(funcName)s:%(lineno)d] "
                            "%(message)s"),
                    'datefmt': "%d/%b/%Y %H:%M:%S",
                },
                'verbose-console': {
                    'format': '[%(asctime)s] %(levelname)-8s - %(filename)-20s - %(funcName)-35s - %(lineno)-4d - %(message)s',
                    'datefmt': "%d/%b/%Y %H:%M:%S",
                },
                'simple-console': {
                    'format': '%(levelname)-8s %(message)s',
                },
            },
            handlers={
                'gui': {'class': 'logging.handlers.RotatingFileHandler',
                                'formatter': 'verbose',
                                'level': 'DEBUG',
                                'filename': f'{self.log_dir}/{int(time.time())}.log',
                                'mode': 'w',
                                'maxBytes': 52428800,
                                'backupCount': 7},
                'console': {
                    'class': 'logging.StreamHandler',
                    'level': 'DEBUG',
                    'formatter': 'verbose-console'}
            },
            loggers={
                'gui': {
                    'handlers': ['gui', 'console'],
                    'level': 'DEBUG',
                },
            }
        )

        # Set the logging configuration (defined above)
        dictConfig(logging_config)
        
        # Set the log to the handler specified in the config file

        return logging.getLogger('gui')
    