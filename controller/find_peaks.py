import numpy as np
import peakutils
from scipy import signal

from data_classes import Vibrometer, Point
from extract import get_point_object

"""Downsample the data"""
def downsample(x: np.ndarray, factor: int) -> np.ndarray:
    samples = int(x.size/factor)
    return signal.resample(x, samples)

"""Find the peaks usint peakutils and return the indexes of the peaks in the numpy array"""
def find_peak_indexes(y: np.ndarray) -> np.ndarray:
    return peakutils.indexes(y, thres=0.3, min_dist=25)

"""Check if the coherence at the peak index is above a threshold, if so then append to the numpy array"""
def peak_coherence_threshold(peak_indexes: np.ndarray, coh_data: np.ndarray, threshold: float) -> np.ndarray:
    new_peak_indexes = np.array([], dtype=int)
    for i in peak_indexes:
        if coh_data[i] > threshold:
            new_peak_indexes = np.append(new_peak_indexes, i)
    return new_peak_indexes


"""Find the peaks in the PSD and FRF data"""
class FindPeaks:
    def __init__(self, vibrometer: Vibrometer) -> None:
        self.vibrometer = vibrometer
        self.num_freqs = vibrometer.freq.size
        
        # Setup variables
        self.downsample_factor = 4                  # Factor to downsample the dataset by
        self.coherence_threshold = 0.99             # Coherence threshold for the peaks
        
        # Find the peaks per point
        print("Start finding the peaks for each point")
        point_nums = self.vibrometer.measured_points
        updated_points = list()
        for i in point_nums:
            point = get_point_object(self.vibrometer.points, i)
            #print(f"Finding peaks for point {i}/{len(point_nums)}")
            updated_points.append(self.find_peaks(point))
        self.vibrometer.points = updated_points
            
        # Find the peaks for average point
        self.vibrometer.avg_point = self.find_peaks(self.vibrometer.avg_point)
        
    """Find the peaks of the data sets (PSD and FRF)"""
    def find_peaks(self, point: Point) -> Point:
        # Grab the data from the point object
        freq = self.vibrometer.freq
        psd = point.psd.data_db
        frf = point.frf.data_db
        coh = point.coherence.data
        
        # Downsample the frequencies, PSD, and FRF data        
        freq = downsample(freq, self.downsample_factor)
        psd = downsample(psd, self.downsample_factor)
        frf = downsample(frf, self.downsample_factor)
        coh = downsample(coh, self.downsample_factor)

        # Find the peaks indices in the PSD and FRF data
        #print("Find peak indices")
        psd_peak_indexes = find_peak_indexes(psd)
        frf_peak_indexes = find_peak_indexes(frf)
        
        # Validate the peaks indices using the coherence data
        psd_peak_indexes = peak_coherence_threshold(psd_peak_indexes, coh, self.coherence_threshold)
        frf_peak_indexes = peak_coherence_threshold(frf_peak_indexes, coh, self.coherence_threshold)
        
        # Find the value of the peaks that are validated
        psd_peak_values = psd[psd_peak_indexes]
        frf_peak_values = frf[frf_peak_indexes]
        psd_peak_freqs = freq[psd_peak_indexes]
        frf_peak_freqs = freq[frf_peak_indexes]

        
        # Create the Peaks object
        psd_peaks = np.column_stack((psd_peak_freqs, psd_peak_values))
        frf_peaks = np.column_stack((frf_peak_freqs, frf_peak_values))
        point.psd_peaks = psd_peaks
        point.frf_peaks = frf_peaks
        #print(frf_peaks[:, 0])
        #print(frf_peaks[:, 1])
        return point

        