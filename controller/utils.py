import numpy as np
import os
import pickle


"""Given a number in meters, return the value as micrometers"""
def meter_to_micrometer(value: float) -> float:
    return value * 1e6

"""Transform the data from linear to dB power scale"""
def linear_to_db_scale(x: np.array, log_ref: float=1.0) -> np.array:
    x = np.where(x==0, 0.000000001, x)     # All 0 values are set to 0.000000001
    return 20*np.log10(np.abs(x)/np.abs(log_ref))

# """Return the Point object given the list of Point objects and the desired point number"""
# def get_point_object(points: list[Point], point_number: int) -> Point:
#     return next((i for i in points if i.number == point_number), None)

"""Return the i, j location in the mesh given the point number (point number starts at 1, i and j start at 0)"""
def get_mesh_numpy_ij(num_rows, num_cols, point_num: int) -> tuple[int, int]:
    row = (point_num-2) % num_rows
    col = (point_num-2) % num_cols 
    return row, col

"""Find the index of closest frequency input value"""
def get_nearest_freq_index(freq_array: np.ndarray, value: float) -> int:
    freq_array = np.asarray(freq_array)
    idx = (np.abs(freq_array - value)).argmin()
    return idx

# """Save the Vibromter object to a pickle file"""
# def save_vibrometer_object(vibrometer: Vibrometer, prefix: str) -> None:
#     with open(os.path.join(os.getcwd(), f'vibrometer_objects\{prefix}_vibrometer.pkl'), 'wb') as f:
#         pickle.dump(vibrometer, f)
        
# """Open the Vibrometer pickle file and return the Vibrometer object"""  
# def load_vibrometer_object(prefix: str) -> Vibrometer:
#     with open(os.path.join(os.getcwd(), f'vibrometer_objects\{prefix}_vibrometer.pkl'), 'rb') as f:
#         return pickle.load(f)