from scipy.signal import lfilter, savgol_filter, medfilt
from pykalman import KalmanFilter
import numpy as np
import pandas as pd
from numba import jit, prange
from numpy.typing import ArrayLike
from typing import Optional
from analysis_code.analysis_logger import CustomLogger

def filter_data(data: ArrayLike, filter_type: str, filter_params: Optional[dict]=dict(), log: CustomLogger=None) -> np.ndarray:
    """Filter the data given the filter type and parameters
    
    Args:
        data: ArrayLike
            The data to filter, can be a list, tuple, or numpy array
        filter_type: str
            The type of filter to use. Must be one of: "moving_average", "least_squares", "kalman", "savgol"
        filter_params: dict
            The parameters for the filter. The parameters depend on the filter type. See below for the parameters for each filter type.
    """
    data = np.array(data)   # Convert to numpy array
    
    if filter_type == 'moving_average':
        window_size = filter_params.get('window_size', 3)
        filtered_data = medfilt(data, window_size)
    elif filter_type == 'least_squares':
        B = filter_params.get('B', [1.0])   # Numerator coefficients of the transfer function
        A = filter_params.get('A', [1.0])   # Denominator coefficients of the transfer function
        filtered_data = lfilter(B, A, data)
    elif filter_type == 'kalman':
        kf = KalmanFilter(initial_state_mean=0, n_dim_obs=1)
        if data.shape[0] != 2:
            raise ValueError('Kalman filter requires 2D data')
        data = np.ma.asarray(data)
        data[1] = np.ma.masked
        kf = kf.em(data, n_iter=5)
        if filter_params.get('type') == 'smooth':
            filtered_data, _ = kf.smooth(data)          # Smooth the data
        else:
            filtered_data, _ = kf.filter(data)          # Default to filter
    elif filter_type == 'savgol':
        window_length = filter_params.get('window_length', 5)
        polyorder = filter_params.get('polyorder', 2)
        filtered_data = savgol_filter(data, window_length, polyorder)
    else:
        raise ValueError('Invalid filter type. Must be "moving_average", "least_squares", "kalman", or "savgol"')
    return filtered_data

@jit(nopython=True, parallel=True)
def numba_interpolate_to_fixed_dt_2D(data: np.ndarray, time: np.ndarray, new_data: np.ndarray, new_time: np.ndarray) -> np.ndarray:
    """Interpolate the data to a fixed time step using numba
    
    Args:
        data: np.ndarray
            The data to interpolate, must be 2D
        time: np.ndarray
            The time vector of the data
        new_data: np.ndarray
            The new data array to interpolate the data to
        new_time: np.ndarray
            The new time vector to interpolate the data to
            
    Returns:
        np.ndarray
            The interpolated data
    """
    # For each row of data...
    for i in prange(data.shape[0]):
        # Interpolate the data to the new time vector
        new_data[i, :] = np.interp(x=new_time, xp=time, fp=data[i, :])
        
    return new_data


@jit(nopython=True)
def numba_interpolate_to_fixed_dt_1D(data: np.ndarray, time: np.ndarray, new_time: np.ndarray) -> np.ndarray:
    """Interpolate the data to a fixed time step using numba
    
    Args:
        data: np.ndarray
            The data to interpolate, must be 1D
        time: np.ndarray
            The time vector of the data
        new_time: np.ndarray
            The new time vector to interpolate the data to
            
    Returns:
        np.ndarray
            The interpolated data
    """
    # Create the new data array
    new_data = np.zeros((new_time.shape[0]))

    # Interpolate the data to the new time vector
    new_data[:] = np.interp(new_time, time, data[:])

    return new_data


def interpolate_to_fixed_dt_2D(data: np.ndarray, time: np.ndarray, new_data: np.ndarray, new_time: np.ndarray) -> np.ndarray:
    """Interpolate the data to a fixed time step
    
    Args:
        data: np.ndarray
            The data to interpolate, must be 2D
        time: np.ndarray
            The time vector of the data
        new_data: np.ndarray
            The new data array to interpolate the data to
        new_time: np.ndarray
            The new time vector to interpolate the data to
            
    Returns:
        np.ndarray
            The interpolated data
    """
    
    # For each columns of data...
    for i in range(data.shape[1]):
        # Interpolate the data to the new time vector
        new_data[:, i] = np.interp(x=new_time, xp=time, fp=data[:, i])

    return new_data