from dataclasses import dataclass
import numpy as np


"""Dataclass for all input and output data"""
@dataclass
class IOData:
    max_elements: int             # This is the default size of the numpy arrays, set when initializing the class
    io_count: int = 0
    

    def __post_init__(self):
        # Initialize the numpy arrays
        self.time_us                        = np.zeros(shape=self.max_elements, dtype=np.uint32)
        self.time_ms                        = np.zeros(shape=self.max_elements, dtype=np.uint32)
        self.inlet_flow_ml_per_min          = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.outlet_flow_ml_per_min         = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.inlet_fluid_temp_c             = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.inlet_fluid_temp_setpoint_c    = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.piezo_1_freq_hz                = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.piezo_2_freq_hz                = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.piezo_1_amp_v                  = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.piezo_2_amp_v                  = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.piezo_1_phase_deg              = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.piezo_2_phase_deg              = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.heater_block_enable            = np.zeros(shape=self.max_elements, dtype=bool)
        self.rope_heater_enable             = np.zeros(shape=self.max_elements, dtype=bool)
        self.piezo_1_enable                 = np.zeros(shape=self.max_elements, dtype=bool)
        self.piezo_2_enable                 = np.zeros(shape=self.max_elements, dtype=bool)
        self.heat_flux_w_per_m2             = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.thermistor_1_temp_c            = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.thermistor_2_temp_c            = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.thermistor_3_temp_c            = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.thermistor_4_temp_c            = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.thermistor_5_temp_c            = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.thermistor_6_temp_c            = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.thermistor_7_temp_c            = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.thermistor_8_temp_c            = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.thermistor_9_temp_c            = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.thermistor_10_temp_c           = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.thermistor_11_temp_c           = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.thermistor_12_temp_c           = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.thermistor_13_temp_c           = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.thermistor_14_temp_c           = np.zeros(shape=self.max_elements, dtype=np.float32)
        self.number_of_sd_records           = np.zeros(shape=self.max_elements, dtype=np.uint32)
        self.cpp_pointer                    = np.zeros(shape=self.max_elements, dtype=np.uint32)    # Pointer to the current element in the circular buffer, uin8_t in code but sent as uint32_t
    
        
    @property
    def dtype_map(self) -> dict:
        """ Create a dictionary of numpy data types for each field in the IOData class"""
        return {
            'time_us': np.uint32,
            'time_ms': np.uint32,
            'inlet_flow_ml_per_min': np.float32,
            'outlet_flow_ml_per_min': np.float32,
            'inlet_fluid_temp_c': np.float32,
            'inlet_fluid_temp_setpoint_c': np.float32,
            'piezo_1_freq_hz': np.float32,
            'piezo_2_freq_hz': np.float32,
            'piezo_1_amp_v': np.float32,
            'piezo_2_amp_v': np.float32,
            'piezo_1_phase_deg': np.float32,
            'piezo_2_phase_deg': np.float32,
            'heater_block_enable': bool,
            'rope_heater_enable': bool,
            'piezo_1_enable': bool,
            'piezo_2_enable': bool,
            'heat_flux_w_per_m2': np.float32,
            'thermistor_1_temp_c': np.float32,
            'thermistor_2_temp_c': np.float32,
            'thermistor_3_temp_c': np.float32,
            'thermistor_4_temp_c': np.float32,
            'thermistor_5_temp_c': np.float32,
            'thermistor_6_temp_c': np.float32,
            'thermistor_7_temp_c': np.float32,
            'thermistor_8_temp_c': np.float32,
            'thermistor_9_temp_c': np.float32,
            'thermistor_10_temp_c': np.float32,
            'thermistor_11_temp_c': np.float32,
            'thermistor_12_temp_c': np.float32,
            'thermistor_13_temp_c': np.float32,
            'thermistor_14_temp_c': np.float32,
            'number_of_sd_records': np.uint32,
            'cpp_pointer': np.uint32
        }
    
        
    @property
    def buffer_size(self) -> int:
        """ Return the size of the serial buffer using the dtype_map """
        return sum([np.dtype(self.dtype_map[key]).itemsize for key in self.dtype_map])