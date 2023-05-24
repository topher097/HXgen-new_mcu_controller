import cv2
import numpy as np
import pyautogui
import pygetwindow as gw
import sys
import os
import threading


class ApplicationRecorder:
    def __init__(self, window_title: str=None, fps:int=60, output_dir: str=None, output_file_name: str=None, output_format: str='.mp4') -> None:
        # Set window title, output path, and fps
        self.window_title = window_title
        self.output_format = output_format
        self.output_path = os.path.join(output_dir, output_file_name+output_format)
        self.fps = fps
        
        # Get the window information
        self.window = gw.getWindowsWithTitle(self.window_title)[0]
        self.window.activate()      # Bring the window to front
        self.window_width = self.window.width
        self.window_height = self.window.height
        
        # Initialize the video writer
        fourcc = None
        if self.output_format == ".avi":
            fourcc = cv2.VideoWriter_fourcc(*'XVID')
        elif self.output_format == ".mp4":
            fourcc = cv2.VideoWriter_fourcc(*'mp4v')
        self.video = cv2.VideoWriter(self.output_path, fourcc, self.fps, (self.window_width, self.window_height))
        
    """Record the window for a given duration"""
    def record(self, duration_ms: float) -> None:
        # Record the video
        for _ in range(int(self.fps * duration_ms)):
            img = pyautogui.screenshot(region=(self.window.left, self.window.top, self.window.width, self.window.height))
            img = cv2.cvtColor(np.array(img), cv2.COLOR_RGB2BGR)
            self.video.write(img)
            
        # Release the video writer
        self.video.release()