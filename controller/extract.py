import numpy as np
import os
import mat73
import re
import pickle

from data_classes import Vibrometer, Point, Values

from utils import linear_to_db_scale, get_point_object, get_mesh_numpy_ij, save_vibrometer_object, load_vibrometer_object

class VibrometerDataExtractor:
    """Init the converter"""
    def __init__(self, data_file_path: str, mesh_size: list=[1,1], use_db_scale: bool = True) -> None:
        self.use_db_scale = use_db_scale
        self.mesh_size = mesh_size
        
        # Check to see if the file even exists
        if not os.path.exists(data_file_path):
            raise Exception(f"Data file was not found at path: {data_file_path}")
        
        # Init the Dataclass for the vibrometer
        self.vibrometer = Vibrometer()
        
        # Open the .mat file and save data to different dataclasses
        self.mat_data = self.extract_from_mat_file(data_file_path)
        self.get_measured_points_list()
        self.load_geometry()
        self.load_data()
        self.calculate_average_of_measured_points()
        
    """Extract the data from the raw data file"""
    def extract_from_mat_file(self, mat_file_path: str) -> dict:
        # Load the mat file and return a dict
        return mat73.loadmat(filename=mat_file_path, use_attrdict=True)
    
    """Determines which point numbers are measured and returns a list of numbers"""
    def get_measured_points_list(self) -> list[int]:
        self.measured_points = list()
        for key in self.mat_data.Auto_spectrum:
            if key.startswith("Point"):
                point_num = int(re.findall(r'\d+', key)[0])
                self.vibrometer.measured_points.append(point_num)
        self.vibrometer.measured_points = sorted(self.vibrometer.measured_points)
    
    """Load the geometry into it's Dataclass"""
    def load_geometry(self) -> None:
        geom = self.mat_data["Geometry"]
        num_points = len(self.vibrometer.measured_points)
        
        # Construct the geometry numpy arrays
        pointXYZ = np.zeros(shape=(num_points, 3), dtype=float)
        i = 0
        for i in range(num_points):
            # Points in the mat file have the format: [x, y, z]
            point_num = self.vibrometer.measured_points[i]
            key = f"Point_{point_num}"
            xyz = geom[key]
            pointXYZ[i] = [xyz[0], xyz[1], xyz[2]]
        self.vibrometer.geometry.units = "mm"
        self.vibrometer.geometry.num_points = num_points
        self.vibrometer.geometry.pointXYZ = pointXYZ
        self.vibrometer.geometry.mesh_size = self.mesh_size
        
        # Get the min and max of points
        self.vibrometer.geometry.min_x = np.min(pointXYZ[:, 0])
        self.vibrometer.geometry.min_y = np.min(pointXYZ[:, 1])
        self.vibrometer.geometry.min_z = np.min(pointXYZ[:, 2])
        self.vibrometer.geometry.max_x = np.max(pointXYZ[:, 0])
        self.vibrometer.geometry.max_y = np.max(pointXYZ[:, 1])
        self.vibrometer.geometry.max_z = np.max(pointXYZ[:, 2])
        
    """Load the PSD CSD, Coherence, FRF, and FFT into it's Dataclass"""
    def load_data(self) -> None:
        # Load the frequency data into the Database
        self.vibrometer.freq = self.mat_data.Auto_spectrum.x
        num_freq_points = len(self.vibrometer.freq)
        
        # Go through each point in the .mat file and create and fill the data in the point dataclass
        points = list()
        for point_num in self.vibrometer.measured_points:
            point_name = f"Point_{point_num}"
            
            # Load the PSD data into the Dataclass
            #psd_units = re.sub(r"()", "", self.mat_data.Auto_spectrum.Units.Meas_In)
            #psd_ref_units = re.sub(r"()", "", self.mat_data.Auto_spectrum.Units.Ref_1)
            psd_data = self.read_data_point(self.mat_data.Auto_spectrum, point_name, num_freq_points)
            #print(f"{point_name}: {temp_point.psd.data[::5]}")
            
            # Load the CSD data into the Dataclass
            csd_ref_data = self.read_data_point(self.mat_data.Cross_spectrum, point_name, num_freq_points, channel='Ref_1')
            
            # Load the Coherence data into the Dataclass
            coherence_data = self.read_data_point(self.mat_data.Coherence, point_name, num_freq_points, channel='Ref_1')
            
            # Load the FFT data into the Dataclass
            #fft_units = re.sub(r"()", "", self.mat_data.FFT.Units.Meas_In)
            #fft_ref_units = re.sub(r"()", "", self.mat_data.FFT.Units.Ref_1)
            fft_data = self.read_data_point(self.mat_data.FFT.Average, point_name, num_freq_points)     # Using the average of the x amount of runs per point
            fft_ref_data = self.read_data_point(self.mat_data.FFT.Average, point_name, num_freq_points, channel='Ref_1')
            
            # Load the FRF data into the Dataclass
            frf_data = self.read_data_point(self.mat_data.FRF, point_name, num_freq_points, channel='Ref_1')

            # Convert to dB
            psd_data_db = linear_to_db_scale(psd_data)
            csd_ref_data_db = linear_to_db_scale(csd_ref_data)
            coherence_data_db = linear_to_db_scale(coherence_data)
            fft_data_db = linear_to_db_scale(fft_data)
            fft_ref_data_db = linear_to_db_scale(fft_ref_data)
            frf_data_db = linear_to_db_scale(frf_data)
            
            # Create the Point object
            psd_units = "m/s"
            psd_ref_units = "V"
            fft_units = "m/s/Hz"
            fft_ref_units = "V"
            row, col = get_mesh_numpy_ij(self.mesh_size[0], self.mesh_size[1], point_num)
            temp_point = Point(number=point_num,
                               row=row,
                               column=col,
                               psd=Values(units=psd_units, ref_units=psd_ref_units, data=psd_data, data_db=psd_data_db, ref_data=None, ref_data_db=None),
                               csd=Values(units=None, ref_units=None, data=None, data_db=None, ref_data=csd_ref_data, ref_data_db=csd_ref_data_db),
                               coherence=Values(units=None, ref_units=None, data=coherence_data, data_db=coherence_data_db, ref_data=None, ref_data_db=None),
                               fft=Values(units=fft_units, ref_units=fft_ref_units, data=fft_data, data_db=fft_data_db, ref_data=fft_ref_data, ref_data_db=fft_ref_data_db),
                               frf=Values(units=None, ref_units=None, data=frf_data, data_db=frf_data_db, ref_data=None, ref_data_db=None)
                               )               
             
            # Save the data into the point dataclass
            points.append(temp_point)

        # Save the points into the vibrometer dataclass
        self.vibrometer.points = points
        # for point_num in range(num_points):
        #     point = get_point_object(points, point_num+1)
        #     print(f"Point_{point_num}: {point.psd.data[::5]}")

        
    """Read the point data return a numpy array"""
    def read_data_point(self, data: dict, point_name: str, num_freq_points: int, channel: str='Meas_In') -> np.ndarray:
        return np.reshape(data[point_name][channel], newshape=(num_freq_points, ))

    """Calculate the average data for all of the measured points"""
    def calculate_average_of_measured_points(self):
        # Create empty arrays to hold the data
        psd_avg = np.zeros(len(self.vibrometer.freq), dtype=complex)
        csd_avg = np.zeros(len(self.vibrometer.freq), dtype=complex)
        coherence_avg = np.zeros(len(self.vibrometer.freq), dtype=complex)
        fft_avg = np.zeros(len(self.vibrometer.freq), dtype=complex)
        fft_ref_avg = np.zeros(len(self.vibrometer.freq), dtype=complex)
        frf_avg = np.zeros(len(self.vibrometer.freq), dtype=complex)
        
        # Iterate the points and add the data to the arrays
        for point_num in self.vibrometer.measured_points:
            point = get_point_object(self.vibrometer.points, point_num)
            psd_avg += point.psd.data
            csd_avg += point.csd.ref_data
            coherence_avg += point.coherence.data
            fft_avg += point.fft.data
            fft_ref_avg += point.fft.ref_data
            frf_avg += point.frf.data
            
        # Average the arrays
        psd_avg /= len(self.vibrometer.measured_points)
        csd_avg /= len(self.vibrometer.measured_points)
        coherence_avg /= len(self.vibrometer.measured_points)
        fft_avg /= len(self.vibrometer.measured_points)
        fft_ref_avg /= len(self.vibrometer.measured_points)
        frf_avg /= len(self.vibrometer.measured_points)
        
        # Save the Point obect
        avg_point = Point(number=-1, row=-1, column=-1,
                            psd=Values(units="m/s", ref_units="V", data=psd_avg, data_db=linear_to_db_scale(psd_avg), ref_data=None, ref_data_db=None),
                            csd=Values(units=None, ref_units=None, data=None, data_db=None, ref_data=csd_avg, ref_data_db=linear_to_db_scale(csd_avg)),
                            coherence=Values(units=None, ref_units=None, data=coherence_avg, data_db=linear_to_db_scale(coherence_avg), ref_data=None, ref_data_db=None),
                            fft=Values(units="m/s/Hz", ref_units="V", data=fft_avg, data_db=linear_to_db_scale(fft_avg), ref_data=fft_ref_avg, ref_data_db=linear_to_db_scale(fft_ref_avg)),
                            frf=Values(units=None, ref_units=None, data=frf_avg, data_db=linear_to_db_scale(frf_avg), ref_data=None, ref_data_db=None)
                        )
        self.vibrometer.avg_point = avg_point

if __name__ == "__main__":
    data_file_path = "./data/channel_top_11112022.mat"
    vibe = VibrometerDataExtractor(data_file_path)
    #print(len(vibe.vibrometer.measured_points))
    #print(vibe.vibrometer.measured_points[62])