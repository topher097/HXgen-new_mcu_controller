from dataclasses import dataclass, field
import numpy as np

"""Dataclass for the individual """
@dataclass 
class Values:
    units: str
    ref_units: str
    data: np.ndarray
    data_db: np.ndarray
    ref_data: np.ndarray
    ref_data_db: np.ndarray
    
"""Dataclass for the geometry of the points"""
@dataclass 
class Geometry:
    units: str = ""
    ref_units: str = ""
    num_points: int = 0
    mesh_size: list = field(default_factory=list)
    pointXYZ: np.ndarray = np.ndarray(0)   # array of form: [[point1_x, point1_y, point1_z], [point2_x, point2_y, point2_z], ...]
    min_x: float = -1.0
    min_y: float = -1.0
    min_z: float = -1.0
    max_x: float = -1.0
    max_y: float = -1.0
    max_z: float = -1.0
    
"""Dataclass of the peaks in both the FRF and the PSD (auto spectrum)"""
@dataclass 
class Peaks:
    psd_peaks: np.ndarray = np.ndarray(0, dtype='complex_')
    frf_peaks: np.ndarray = np.ndarray(0, dtype='complex_')
    
@dataclass
class Point:
    number: int
    row: int    
    column: int 
    psd: Values
    csd: Values
    coherence: Values
    fft: Values
    frf: Values
    psd_peaks: np.ndarray = np.ndarray(0, dtype='complex_')    # Both arrays are forms of [[freq_of_freq1, value1], [freq_of_peak2, value2], ...]
    frf_peaks: np.ndarray = np.ndarray(0, dtype='complex_')
    
"""Dataclass for the data from the .mat file from experiment"""
@dataclass 
class Vibrometer:
    geometry: Geometry = Geometry()
    points: list = field(default_factory=list)      # Form: [Point(), Point(), ...]
    freq: np.ndarray = np.ndarray(0)
    measured_points: list = field(default_factory=list)
    avg_point: Point = None
    
    