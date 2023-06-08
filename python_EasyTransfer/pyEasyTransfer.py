import asyncio
import logging
from math import e
import time
import numpy as np
from struct import Struct
from typing import Optional, Any
import serial_asyncio
from ETData import ETData, ETDataArrays
import pickle
import os
from datetime import datetime

BYTE_FORMATS = {
    'little-endian': '<',
    'big-endian': '>',
    'network': '!',
    'native': '@',
    'little-endian-no-alignment': '=',
}


def numpy_dtype_to_struct_format(dtype: np.dtype) -> str:
    """Given a numpy dtype, return the corresponding struct format character
    
    Args:
        dtype: np.dtype
            The numpy dtype to convert
            
    Returns:
        struct_format_character: str
    """
    if dtype == np.uint8:
        return 'B'
    elif dtype == np.uint16:
        return 'H'
    elif dtype == np.uint32:
        return 'I'
    elif dtype == np.int8:
        return 'b'
    elif dtype == np.int16:
        return 'h'
    elif dtype == np.int32:
        return 'i'
    elif dtype == np.float32:
        return 'f'
    elif dtype == np.bool_:
        return '?'
    else:
        raise ValueError(f"Unsupported dtype: {dtype}")

def create_struct_format(byte_format: str, struct_def: dict[str, np.dtype]) -> str:
    """Given a byte order and a struct definition, return the corresponding struct format string 
    
    Args:
        byte_format: str
            The byte order to use. Must be one of 'little-endian', 'big-endian', 'network', 'native', or 'little-endian-no-alignment'
        struct_def: dict[str, np.dtype]
            The struct definition to use. Must be a dictionary of {field_name: dtype} pairs.
    
    Returns:
        struct_format: str
    """
    byte_order = BYTE_FORMATS[byte_format]      # Get the byte order character
    struct_format = byte_order                  # Init with the byte order character
    for _, dtype in struct_def.items():
        struct_format += numpy_dtype_to_struct_format(dtype)
    return struct_format

def data_to_bytes(data: Any, format_string: str) -> bytes:
    """Converts structured data to bytes according to the format string.
    
    Args:
        data: Any
            The data to convert to bytes. Can be any type that can be converted to a list.
        format_string: str
            The format string to use when converting the data to bytes, must be properly formatted for the data.
    
    Returns:
        bytes
    """
    return Struct(format_string).pack(*data)

def calculate_checksum(byte_data: bytes) -> tuple[np.uint8, np.uint8]:
    """Calculates the checksum from byte data.
    
    Args:
        byte_data: bytes
            The data to calculate the checksum for.
    
    Returns:
        [checksum: np.uint8, data_size: np.uint8]
    """
    size = np.uint8(len(byte_data))
    checksum = size
    for b in byte_data:
        int_b = int(b)
        checksum ^= int_b
    checksum = np.uint8(checksum)   # Convert the checksum to a uint8
    return checksum, size      

        

class PyEasyTransfer:
    def __init__(self, com_port: str, baud_rate: int,
                 input_struct_def: Optional[dict[str, np.dtype]] = None,
                 output_struct_def: Optional[dict[str, np.dtype]] = None,
                 byte_format: str = 'little-endian', 
                 mode: str = 'both',
                 save_read_data: Optional[ETDataArrays]=None,
                 log: logging.Logger=None,
                 name: str='PyEasyTransfer object'):

        if mode not in ['input', 'output', 'both']:
            raise ValueError("Invalid mode. Mode must be one of 'input', 'output', 'both'")

        self.data_received_event = asyncio.Event()           # Event to indicate when data has been received
        self.buffer = bytearray()                            # Buffer to store the data received over the serial connection
        self.log = log                                       # Logger object for logging events, can init as None and be set later                         
        self.com_port = com_port                             # COM port to connect to
        self.baud_rate = baud_rate                           # Baud rate to connect at
        self.mode = mode                                     # The mode for this PyEasyTransfer instance
        self.byte_format = byte_format                       # The byte format to use when packing and unpacking data
        self.byte_order = BYTE_FORMATS[byte_format]          # Byte order character to use when packing and unpacking data
        self.name = name
        
        # Create the header, size, and checksum formats
        self.header_bytes = bytearray([0x06, 0x85])          # Header bytes to look for when receiving data
        self.header_size = len(self.header_bytes)            # Size of the header in bytes
        self.header_format = 'BB'                            # Format string for the header bytes (uint8_t x 2)
        
        self.size_dtype = np.dtype(np.uint8).newbyteorder(self.byte_order)      # Data type for the size byte
        self.size_size = self.size_dtype.itemsize                               # Size of the size byte in bytes
        self.size_format = numpy_dtype_to_struct_format(self.size_dtype)        # Format string for the size byte (uint8_t)
        
        self.footer_dtype = np.dtype(np.uint8).newbyteorder(self.byte_order)    # Data type for the footer checksum byte
        self.footer_size = self.footer_dtype.itemsize                           # Size of the footer checksum byte in bytes
        self.footer_format = numpy_dtype_to_struct_format(self.footer_dtype)    # Format string for the footer checksum byte (uint8_t)

        # Depending on the mode, initialize the appropriate data classes
        if self.mode in ['input', 'both']:
            if not input_struct_def:
                raise ValueError("An input_struct_def must be provided when mode is 'input' or 'both'.")
            self.read_data = ETData(input_struct_def, name=name)           # Read from this to get the data from the Arduino
            self.buffer_size = self.read_data.struct_bytes      # Size of the buffer in bytes

        if self.mode in ['output', 'both']:
            if not output_struct_def:
                raise ValueError("An output_struct_def must be provided when mode is 'output' or 'both'.")
            self.write_data = ETData(output_struct_def, name=name)         # Write to this to send data to the Arduino

        self.save_read_data = save_read_data    # This is the ETDataArrays object which is used to store the data. If this is not initialized then the data will not be stored
        self.start_saving_data = False          # This is used to indicate when to start saving data

        if self.mode in ['input', 'both']:
            self.struct_format_read = create_struct_format(self.byte_format, self.read_data.struct_def)
        if self.mode in ['output', 'both']:
            self.struct_format_write = create_struct_format(self.byte_format, self.write_data.struct_def)

    def set_log(self, log: logging.Logger):
        """Set the log object for this PyEasyTransfer instance."""
        self.log = log
        
    def start_saving(self, start_saving_data: bool = True):
        """Start saving data to the given ETDataArrays object."""
        self.start_saving_data = start_saving_data
        
    def stop_saving(self, stop_saving_data: bool = True):
        """Stop saving data to the given ETDataArrays object."""
        self.start_saving_data = not stop_saving_data

    async def open(self):
        """Open the serial connection"""
        loop = asyncio.get_running_loop()
        if self.mode in ['input', 'both']:
            self._transport, self.reader = await serial_asyncio.create_serial_connection(loop, EasyTransferReceiver, self.com_port, baudrate=self.baud_rate)
            self.reader.pyeasytransfer = self
        if self.mode in ['output', 'both']:
            self.writer = self._transport

    async def close(self):
        """Close the serial connection"""
        if self._transport:
            self._transport.close()
            await self._transport._closing
        
    async def send_data(self):
        """Send the data to the serial connection"""
        if self.mode in ['both', 'output']:
            # Get the data from the write_data object
            data = [getattr(self.write_data, key) for key in self.write_data.struct_def.keys()]

            # Check the data has the write dtype
            for i, (key, dtype) in enumerate(self.write_data.struct_def.items()):
                if type(data[i]) != dtype:
                    if type(data[i]) == bool:
                        # If bool then set to np.bool_
                        setattr(self.write_data, key, np.bool_(data[i]))
                    else:
                        raise TypeError(f"'{self.name}': Data type for {key} is {type(data[i])}, but should be {dtype}")

            # Get the packed data and calculate the checksum
            packed_data = data_to_bytes(data, self.struct_format_write)
            checksum, packed_data_size = calculate_checksum(packed_data)
                
            # Create the format string for the header and footer
            full_format = self.byte_order + self.header_format + self.size_format + self.struct_format_write.replace(self.byte_order, "") + self.footer_format

            # Pack the data into a byte array and send it
            byte_data_with_checksum = Struct(full_format).pack(self.header_bytes[0], self.header_bytes[1], packed_data_size, *data, checksum)
            self.writer.write(byte_data_with_checksum)
            await asyncio.sleep(0)          # yield control to the event loop
            self.log.debug(f"Sent data: {data}. Checksum sent: {checksum}")
        else:
            raise ValueError("The PyEasyTransfer object is not in output mode. Data sending is not allowed.")

    async def listen(self):
        """Continuously listen to the COM port"""
        if self.mode in ['both', 'input']:
            while True:
                await self.wait_for_data()
        else:
            raise ValueError("The PyEasyTransfer object is not in input mode. Listening is not allowed.")

    async def wait_for_data(self):
        """Wait for data via listening to the COM port"""
        if self.mode in ['both', 'input']:
            await self.data_received_event.wait()
        else:
            raise ValueError("The PyEasyTransfer object is not in input mode. Waiting for data is not allowed.")

    def save_data_recevied(self):
        """Save the current read_data ETData object to the ETDataArrays object."""
        io_count = self.save_read_data.io_count
        if self.save_read_data.max_elements > 0 and io_count <= self.save_read_data.max_elements:
            #keys = list(self.read_data.struct_def.keys())
            for key in self.read_data.struct_def.keys():
                #key = keys[i]
                value = getattr(self.read_data, key)
                getattr(self.save_read_data, key)[io_count] = value
            self.save_read_data.io_count += 1
            if io_count % 100 == 0:
                self.log.debug(f"Saved data to the ETDataArrays object for '{self.name}'. IO count: {io_count}.")
        else:
            self.start_saving_data = False      # Flip flag due to max elements reached
            self.log.error(f"Could not save data to the ETDataArrays object because the max element count has been reached. Stopping data collection.")
    
    @staticmethod
    def format_unpacked_data_for_printing(unpacked_data: tuple[Any]) -> str:
        """Format the unpacked data for printing."""
        string = ""
        for ele in unpacked_data:
            if isinstance(ele, float):
                string += f"{np.format_float_positional(ele, 3)}, "
            else:
                string += f"{ele}, "
        return string[:-2]      # Remove the last comma and space  
    
    def data_received(self, packet: bytes):
        """Data received from the COM port, unpack it and store it in the read_data object."""
        if self.mode in ['both', 'input']:
            # Unpack the data into the read_data object
            unpacked_data = Struct(self.struct_format_read).unpack(bytes(packet))
            # Set the data in the read_data object
            for key, value in zip(self.read_data.struct_def.keys(), unpacked_data):
                setattr(self.read_data, key, value)
            self.read_data.io_count += 1    # Increment the io count
            if self.read_data.io_count % 100 == 0:
                self.log.debug(f"'{self.name}': Received data: [{PyEasyTransfer.format_unpacked_data_for_printing(unpacked_data)}], io_count: {self.read_data.io_count}")
            #self.log.debug(f"Received data: {unpacked_data}. Checksum received: {unpacked_data[-1]}, io_count: {self.read_data.io_count}")
            # If there is a save data object, then save the data
            if self.save_read_data and self.start_saving_data:
                self.save_data_recevied()
        else:
            raise ValueError("The PyEasyTransfer object is not in input mode. Data receiving is not allowed.")



class EasyTransferReceiver(asyncio.Protocol):
    def __init__(self):
        self.pyeasytransfer: PyEasyTransfer = None
        self.buffer = bytearray()

    def connection_made(self, transport: asyncio.BaseTransport) -> None:
        """Method called when the serial connection is made. This method is called by the asyncio loop and should not be called directly."""
        self.transport: asyncio.BaseTransport = transport

    def data_received(self, data: bytes) -> None:
        """Protocol for receiving data from the serial port. Once data is received, it is added to the buffer and then the buffer is checked for a complete packet.
        Once a complete packet is found, the packet is sent PyEasyTransfer object for unpacking and processing.
        """
        self.buffer.extend(data)

        # Look for the header in the buffer
        index = self.buffer.find(self.pyeasytransfer.header_bytes)
        while index != -1:
            # Remove all bytes before the header
            self.buffer = self.buffer[index:]

            # Check that we have enough bytes for the header and the size byte
            if len(self.buffer) < self.pyeasytransfer.header_size + self.pyeasytransfer.size_size:
                break

            # Extract the size of the packet (excluding the header and the size byte itself)
            packet_size = np.frombuffer(self.buffer[self.pyeasytransfer.header_size:self.pyeasytransfer.header_size+self.pyeasytransfer.size_size], dtype=self.pyeasytransfer.size_dtype)[0]
            
            # Check that we have enough bytes for the complete packet, including the checksum byte
            full_expected_size = self.pyeasytransfer.header_size + self.pyeasytransfer.size_size + packet_size + self.pyeasytransfer.footer_size
            if len(self.buffer) < full_expected_size:
                self.pyeasytransfer.log.warning(f"Not enough bytes for the complete packet, expected {full_expected_size}, got {len(self.buffer)}")
                break

            # Extract the packet of data (the ETData data)
            packet_start_index = self.pyeasytransfer.header_size + self.pyeasytransfer.size_size        
            packet_end_index = packet_start_index + packet_size
            packet = self.buffer[packet_start_index:packet_end_index]
            
            # Extract the checksum byte
            received_checksum = self.buffer[packet_end_index]

            # Remove the extracted bytes from the buffer
            end_index = packet_end_index + self.pyeasytransfer.footer_size
            self.buffer = self.buffer[end_index:]

            # Compute the expected checksum, note, each item is a byte in the packet, packet_size is the number of bytes in the packet as a byte
            # expected_checksum = packet_size
            # for item in packet:
            #     item = int(item)
            #     expected_checksum ^= item
            expected_checksum, _ = calculate_checksum(packet)

            # If the checksums match, pass the packet to PyEasyTransfer
            if expected_checksum == received_checksum:
                self.pyeasytransfer.data_received(packet)
            else:
                self.pyeasytransfer.log.error(f"Checksum validation failed. Received: {received_checksum}, expected: {expected_checksum}")

            # Look for the next header in the remaining buffer
            index = self.buffer.find(self.pyeasytransfer.header_bytes)


def get_data_struct_size_from_struct_def(struct_def: dict[str, np.dtype]) -> int:
    """Given a struct definition, return the size of the struct in bytes."""
    struct_format = ''
    for _, value in struct_def.items():
        struct_format += numpy_dtype_to_struct_format(value)
    return Struct(struct_format).size



async def test_both_in_out(com_port: str, baud_rate: int):
    # Input data from the serial connection struct definition, exactly as defined in the Arduino code
    input_struct_def = {"time_ms": np.uint32,
                        "sensor": np.float32,
                        "pc_time_ms_received": np.uint32,
                        "hello_received": np.bool_,
                        "checksum_received": np.uint8,
                        "checksum_expected": np.uint8}

    # Output data to the serial connection struct definition, exactly as defined in the Arduino code
    output_struct_def = {"pc_time_ms": np.uint32,
                         "hello_flag": np.bool_}

    #print(f"Input struct size: {get_data_struct_size(input_struct_def)}")
    #print(f"Output struct size: {get_data_struct_size(output_struct_def)}")
    

    # Create instance of PyEasyTransfer
    ET = PyEasyTransfer(
        log=logging.getLogger(__name__),
        com_port=com_port,
        baud_rate=baud_rate,
        input_struct_def=input_struct_def,
        output_struct_def=output_struct_def,
        byte_order='little-endian',
        mode='both',
    )
    
    # Open connection
    await ET.open()
    start_time = time.time()
    # Create a task to send a True hello flag every 2 seconds
    async def send_hello():
        while True:
            ET.write_data.hello_flag = np.bool_(True)
            ET.write_data.pc_time_ms = np.uint32((time.time()-start_time)*1000)
            await ET.send_data()
            await asyncio.sleep(2)  # pause for 2 seconds

    # Create a task to continuously read data
    async def read_data():
        while True:
            await ET.wait_for_data()

    # Schedule the tasks
    send_task = asyncio.create_task(send_hello())
    read_task = asyncio.create_task(read_data())

    # Run the tasks for a period of time (e.g., 20 seconds), then cancel
    await asyncio.sleep(20)
    send_task.cancel()
    read_task.cancel()
    
    # Close connection
    await ET.close()


if __name__ == "__main__":
    com_port = "COM14"
    baud_rate = 115200
    
    logging.basicConfig(level=logging.INFO)
    asyncio.run(test_both_in_out(com_port, baud_rate))
