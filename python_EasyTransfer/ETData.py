from dataclasses import dataclass
import numpy as np

""" Numpy dtypes for C/Arduino types

    numpy type                  C type              Arduino type
    ------------------------------------------------------------
    numpy.bool_                 bool                bool
    numpy.byte                  signed char         byte
    numpy.ubyte                 unsigned char       char
    numpy.short                 short               int8_t
    numpy.ushort                unsigned short      uint8_t
    numpy.intc                  int                 int16_t
    numpy.uintc                 unsigned int        uint16_t
    numpy.int_                  long                int32_t
    numpy.uint                  unsigned long       uint32_t
    numpy.longlong              long long           int64_t
    numpy.ulonglong             unsigned long long  uint64_t
    numpy.half/numpy.float16    
    numpy.float32               float               float  
    numpy.double                double              double
    numpy.longdouble            long double         
    numpy.csingle               float complex       
    numpy.cdouble               double complex      
    numpy.clongdouble           long double complex  

"""

"""
Note: The struct_def MUST be a dictionary with the following format:
    struct_def = {
        'variable_name': np.dtype,
        }
        
    For example:
    struct_def = {
        'bool_var': np.bool_,
        'uint8_var': np.uint8,
        'uint16_var': np.uint16,
        'uint32_var': np.uint32,
        'float_var': np.float32,
        'double_var': np.double,
        }
        
    The struct_def is used to create the dataclass attributes and the numpy arrays.
    
    The struct_def MUST be in the same order as the data is received from the serial connection, as well as correct types.
"""

"""IO Data Class for single element of input data. Can be used to send, receive, and store data for a single element"""
@dataclass
class ETData:
    struct_def: dict[str, np.dtype]         # This is the dictionary which defines the data structure of the input/output data
    name: str                               # This is the name of the class, used in saving the data to a file as default
    io_count: int = 0                       # This is the number of elements in the numpy arrays which have been filled, keep at zero when initializing the class
    
    def __post_init__(self):
        # Set the attributes of the class to match the struct_def and init the numpy arrays
        for variable, _dtype in self.struct_def.items():
            setattr(self, variable, _dtype)
        # Calculate the number of bytes in the struct_def
        self.struct_bytes = np.sum([np.dtype(dtype).itemsize for dtype in self.struct_def.values()]) 
    
    def populate_dummy_data(self):
        """Populate dummy data for the input/output data"""
        for variable, _dtype in self.struct_def.items():
            if _dtype != np.bool_:
                setattr(self, variable, (np.random.rand(1)*100).astype(_dtype)[0])
            else:
                setattr(self, variable, np.random.choice([True, False]))

"""IO Data Class for input/output data. Can be used to send, receive, and store data for multiple elements"""
@dataclass
class ETDataArrays:
    struct_def: dict[str, np.dtype]         # This is the dictionary which defines the data structure
    max_elements: int                       # This is the default size of the numpy arrays, set when initializing the class    
    name: str                               # This is the name of the class, used in saving the data to a file as default
    io_count: int = 0                       # This is the number of elements in the numpy arrays which have been filled, keep at zero when initializing the class
    
    def __post_init__(self):
        # Set the attributes of the class to match the struct_def and init the numpy arrays
        for variable, _dtype in self.struct_def.items():
            setattr(self, variable, np.full(shape=int(self.max_elements), dtype=_dtype, fill_value=np.nan))     # Fill with np.nan for easy identification of empty elements
        # Calculate the number of bytes in the struct_def
        self.struct_bytes = np.sum([np.dtype(dtype).itemsize for dtype in self.struct_def.values()]) 

    def get_last_element(self, array):
        """Return the last element in the array"""
        return array[self.io_count-1]
    
    def get_values(self, array, start_index: int, last_index: int):
        """Return a numpy array of the values from the array between the start and last index"""
        return array[start_index:last_index]
    
    def add_dummy_data(self, num: int=1):
        """Add dummy data to the numpy arrays for elements io_count to io_count + num"""
        # First check to see that the number of elements to add to the io_count is less than the max_elements
        if self.io_count + num > self.max_elements:
            raise ValueError(f'Cannot add {num} elements to the numpy arrays because the io_count will be greater than the max_elements')
        
        # Iterate over all the variables in the struct_def and add random data to the numpy arrays
        for _ in range(num):
            for variable, _dtype in self.struct_def.items():
                if _dtype != np.bool_:
                    getattr(self, variable)[self.io_count] = (np.random.rand(1)*100).astype(_dtype)[0]
                else:
                    getattr(self, variable)[self.io_count] = np.random.choice([True, False])
            self.io_count += 1
    

def test_io_data_arrays() -> None:
    """ Test the creation of the ETDataArrays class, addition of dummy data, grabbing values between indices, and getting the last element """
    
    print("\n\nTesting ETDataArrays class")
    
    # Test the creation of dataclass from dictionary
    struct_def = {'time': np.float32, 
                  'pressure': np.float32, 
                  'temperature': np.uint16,
                  'flag': np.bool_}
    input_data = ETDataArrays(struct_def=struct_def, max_elements=5)
    
    # Pretty print the attributes of the class, and their values
    print("ETDataArays Attributes:")
    for key, value in input_data.__dict__.items():
        if type(value) is np.ndarray:
            print(f'{key}: {value.shape}, {value.dtype}')
        else:
            print(f'{key}: {value}')
        
    # Add 3 dummy data points (elements 0 thru 2 bc io_count is currently 0)
    input_data.add_dummy_data(3) 
    
    # Print the elements 0 thru 4 for each field (full size of the array)
    print("\nElements 0 thru 4 for each field (full size of the array):")
    for key in input_data.struct_def:
        print(f"{key}: {input_data.get_values(getattr(input_data, key), 0, 5)}")        # last element is non-inclusive to keep consistent with python slicing
        
    # Print the last added element for each field
    print(f"\nLast added element ({input_data.io_count-1}) for each field:")
    for key in input_data.struct_def:
        print(f"{key}: {input_data.get_last_element(getattr(input_data, key))}")


def test_io_data() -> None:
    """ Test the creation of the ETData class, and the population of dummy data """
    
    print("\n\nTesting ETData class")
    
    # Test the creation of dataclass from dictionary
    struct_def = {'time': np.float32, 
                  'pressure': np.float32, 
                  'temperature': np.uint16,
                  'flag': np.bool_}
    input_data = ETData(struct_def=struct_def)
    
    # Pretty print the attributes of the class, and their values
    print("ETData Attributes:")
    for key, value in input_data.__dict__.items():
        print(f'{key}: {value}')
    
    # Populate the dummy data
    input_data.populate_dummy_data()
    
    # Print the elements 0 thru 4 for each field (full size of the array)
    print("\nDummy data for each field:")
    for key in input_data.struct_def:
        print(f"{key}: {getattr(input_data, key)}")        # last element is non-inclusive to keep consistent with python slicing

if __name__ == "__main__":
    test_io_data_arrays()
    test_io_data()
    
    
