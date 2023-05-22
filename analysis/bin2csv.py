import os
from pathlib import Path
import numpy as np

""" Numpy dtypes for C/Arduino types

numpy type                  C type              Arduino type
numpy.bool_                 bool                bool
numpy.byte                  signed char         byte
numpy.ubyte                 unsigned char       
numpy.short                 short               int8_t
numpy.ushort                unsigned short      uint8_t
numpy.intc                  int                 int16_t
numpy.uintc                 unsigned int        uint16_t
numpy.int_                  long                int32_t
numpy.uint                  unsigned long       uint32_t
numpy.longlong              long long           int64_t
numpy.ulonglong             unsigned long long  uint64_t
numpy.half/numpy.float16    
numpy.single                float               float  
numpy.double                double              double
numpy.longdouble            long double         
numpy.csingle               float complex       
numpy.cdouble               double complex      
numpy.clongdouble           long double complex  

"""

# def bin2csv(in_file_path: Path, out_file_path: Path, dtype_map: dict, header_bytes: int=0) -> None:
#     """ Convert binary file to CSV file

#     Args:
#         in_file_path (Path): Path to input binary file
#         out_file_path (Path): Path to output CSV file
#         dtype_map (dict): Map of column names and data types
#     """

#     # Create a numpy dtype object from the map
#     dt = np.dtype([(key, dtype_map[key]) for key in dtype_map])

#     # Load binary file and convert to numpy array
#     print("Loading binary file...")
#     data = np.fromfile(in_file_path, dtype=dt, offset=header_bytes, count=-1, sep='')

#     # Convert numpy array to CSV file
#     print("Saving CSV file...")
#     header_string= ','.join(data.dtype.names)
#     np.savetxt(out_file_path, data, delimiter=',', header=header_string, fmt='%s', comments='')
#     print("Done!")


def bin2csv(in_file_path: Path, out_file_path: Path, dtype_map: dict, fmt_map: dict, header_bytes: int=0) -> None:
    """ Convert binary file to CSV file """
    # Create a numpy dtype object from the map, assuming little endian byte order
    dt = np.dtype([(key, np.dtype(dtype_map[key]).newbyteorder('<')) for key in dtype_map])

    # Load binary file and convert to numpy array
    print("Loading binary file...")
    data = np.fromfile(in_file_path, dtype=dt, offset=header_bytes, count=-1, sep='')

    # Convert numpy array to CSV file
    print("Saving CSV file...")
    header_string = ','.join(data.dtype.names)
    fmt_string = [fmt_map[name] for name in data.dtype.names]
    np.savetxt(out_file_path, data, delimiter=',', header=header_string, fmt=fmt_string, comments='')
    print("Done!")


# Input and output dirs
in_dir = Path(os.getcwd(), "bin_data")
out_dir = Path(os.getcwd(), "csv_data")

# Input file name and path
in_file_name = "LOG_1546314397.bin"
in_file_path = Path(in_dir, in_file_name)

# Output file name and path
out_file_name = in_file_name.replace(".bin", ".csv")
out_file_path = Path(out_dir, out_file_name)


# Create a map of the data types per column
dtype_map = {
    "microtime": np.uint32,       # Unsigned long (uint32_t)
    "millitime": np.uint32,       # Unsigned long (uint32_t)
    "numrecords": np.uint32,      # Unsigned long (uint32_t)
    "cptr": np.uint32,            # Unsigned pointer (uint32_t)
}

# Create a map of the output formats per column
fmt_map = {
    "microtime": '%u',    # Output as unsigned integer
    "millitime": '%u',    # Output as unsigned integer
    "numrecords": '%u',   # Output as unsigned integer
    "cptr": '0x%08X',     # Output as hexadecimal
}

# Convert binary file to CSV file
bin2csv(in_file_path, out_file_path, dtype_map, fmt_map, header_bytes=0)

