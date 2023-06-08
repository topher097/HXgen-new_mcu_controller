from scipy.signal import lfilter, savgol_filter
from pykalman import KalmanFilter
import numpy as np
from numpy.typing import ArrayLike
from typing import Optional


def filter_data(data: ArrayLike, filter_type: str, filter_params: Optional[dict]=dict()) -> np.ndarray:
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
        weights = np.repeat(1.0, window_size) / window_size
        filtered_data = np.convolve(data, weights, 'same')
    elif filter_type == 'least_squares':
        N = filter_params.get('N', 3)       # Order of the filter
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