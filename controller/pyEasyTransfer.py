import asyncio
import logging
from math import e
import struct
import numpy as np
from struct import Struct
from typing import Optional
import serial_asyncio
from ETData import ETData, ETDataArrays


BYTE_FORMATS = {
    'little-endian': '<',
    'big-endian': '>',
    'network': '!',
    'native': '@',
    'little-endian-no-alignment': '=',
}


def numpy_dtype_to_struct_format(dtype):
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


def create_struct_format(byte_order, struct_def):
    struct_format = byte_order
    for name, dtype in struct_def.items():
        struct_format += numpy_dtype_to_struct_format(dtype)
    return struct_format

def calculate_checksum(*byte_data: bytes, byte_order: str, footer_dtype: np.dtype) -> int:  
    """Calculates the checksum of the given bytes, according to the given byte order and footer data type."""
    checksum = 0
    for byte in byte_data:
        checksum += int.from_bytes(byte, byteorder=byte_order)
    checksum = checksum % (2 ** (8 * footer_dtype.itemsize))
    return checksum

class PyEasyTransfer:
    def __init__(self, log: logging.Logger, com_port: str, baud_rate: int,
                 input_struct_def: Optional[dict[str, np.dtype]] = None,
                 output_struct_def: Optional[dict[str, np.dtype]] = None,
                 byte_order: str = 'little-endian', mode: str = 'both',
                 save_read_data: Optional[ETDataArrays]=None):

        if mode not in ['input', 'output', 'both']:
            raise ValueError("Invalid mode. Mode must be one of 'input', 'output', 'both'")

        self.data_received_event = asyncio.Event()           # Event to indicate when data has been received
        self.buffer = bytearray()                            # Buffer to store the data received over the serial connection
        self.log = log                                       # Logger object for logging events                            
        self.com_port = com_port                             # COM port to connect to
        self.baud_rate = baud_rate                           # Baud rate to connect at
        self.mode = mode                                     # The mode for this PyEasyTransfer instance
        self.byte_order = BYTE_FORMATS[byte_order]           # Byte order to use when packing and unpacking data
        
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
            self.read_data = ETData(input_struct_def)           # Read from this to get the data from the Arduino
            self.buffer_size = self.read_data.struct_bytes      # Size of the buffer in bytes

        if self.mode in ['output', 'both']:
            if not output_struct_def:
                raise ValueError("An output_struct_def must be provided when mode is 'output' or 'both'.")
            self.write_data = ETData(output_struct_def)         # Write to this to send data to the Arduino

        self.save_read_data = save_read_data    # This is the ETDataArrays object which is used to store the data. If this is not initialized then the data will not be stored
        self.start_buffering = False            # Flag to indicate when to start buffering data

        if self.mode in ['input', 'both']:
            self.struct_format_read = create_struct_format(self.byte_order, self.read_data.struct_def)
        if self.mode in ['output', 'both']:
            self.struct_format_write = create_struct_format(self.byte_order, self.write_data.struct_def)

        #print(f"Struct format read: {self.struct_format_read}")
        #print(f"Struct format write: {self.struct_format_write}")

    async def open(self):
        """Open the serial connection, if the """
        loop = asyncio.get_running_loop()
        if self.mode in ['input', 'both']:
            self._transport, self.reader = await serial_asyncio.create_serial_connection(loop, EasyTransferReceiver, self.com_port, baudrate=self.baud_rate)
            self.reader.pyeasytransfer = self
        if self.mode in ['output', 'both']:
            self.writer = self._transport

    async def close(self):
        if self._transport:
            self._transport.close()
            await self._transport._closing
            
    @staticmethod
    def data_to_bytes(data, format_string: str) -> bytes:
        """Converts structured data to bytes according to the format string."""
        return Struct(format_string).pack(*data)

    @staticmethod
    def calculate_checksum(byte_data: bytes, byte_order: str, footer_dtype: np.dtype) -> np.uint8:
        """Calculates the checksum from byte data."""
        checksum = len(byte_data)
        for b in byte_data:
            int_b = int(b)
            checksum ^= int_b
        return np.uint8(checksum)      
        

    async def send_data(self):
        if self.mode in ['both', 'output']:
            # Get the data from the write_data object
            data = [getattr(self.write_data, key) for key in self.write_data.struct_def.keys()]

            # Get the size of the data
            packed_data = PyEasyTransfer.data_to_bytes(data, self.struct_format_write)
            packed_data_size = np.uint8(len(packed_data))
 
            # Calculate the checksum
            checksum = packed_data_size
            for byte in packed_data: 
                byte = int(byte)        # Convert the byte to an integer
                checksum ^= byte        # XOR the byte with the checksum
                
            # Create the format string for the header and footer
            full_format = self.byte_order + self.header_format + self.size_format + self.struct_format_write.replace(self.byte_order, "") + self.footer_format

            # Pack the data into a byte array and send it
            byte_data_with_checksum = Struct(full_format).pack(self.header_bytes[0], self.header_bytes[1], packed_data_size, *data, checksum)
            self.writer.write(byte_data_with_checksum)
            await asyncio.sleep(0)  # yield control to the event loop
            self.log.info(f"Sent data: {data}. Checksum sent: {checksum}")
        else:
            raise ValueError("The PyEasyTransfer object is not in output mode. Data sending is not allowed.")

    async def listen(self):
        if self.mode in ['both', 'input']:
            while True:
                await self.wait_for_data()
        else:
            raise ValueError("The PyEasyTransfer object is not in input mode. Listening is not allowed.")

    async def wait_for_data(self):
        if self.mode in ['both', 'input']:
            await self.data_received_event.wait()
        else:
            raise ValueError("The PyEasyTransfer object is not in input mode. Waiting for data is not allowed.")

    def save_data_recevied(self):
        """Save the current read_data ETData object to the ETDataArrays object."""
        ioc = self.save_read_data.io_count
        if self.save_read_data.max_elements > 0 and ioc <= self.save_read_data.max_elements:
            keys = list(self.read_data.struct_def.keys())
            for i in range(len(keys)):
                key = keys[i]
                value = getattr(self.read_data, key)
                getattr(self.save_read_data, key)[ioc] = value
            self.save_read_data.io_count += 1
            self.log.debug(f"Saved data to the ETDataArrays object. IO count: {ioc}.")
        else:
            self.log.error(f"Could not save data to the ETDataArrays object. IO count: {ioc}, Max element count: {self.save_read_data.max_elements}.")
            

    def data_received(self, packet: bytes):
        if self.mode in ['both', 'input']:
            # Unpack the data into the read_data object
            unpacked_data = Struct(self.struct_format_read).unpack(bytes(packet))
            #print(unpacked_data)
            for key, value in zip(self.read_data.struct_def.keys(), unpacked_data):
                setattr(self.read_data, key, value)
            self.read_data.io_count += 1
            self.log.info(f"Received data: {unpacked_data}.")
            # If there is a save data object, then save the data
            if self.save_read_data:
                self.save_data_recevied()
        else:
            raise ValueError("The PyEasyTransfer object is not in input mode. Data receiving is not allowed.")



class EasyTransferReceiver(asyncio.Protocol):
    def __init__(self):
        self.pyeasytransfer: PyEasyTransfer = None
        self.buffer = bytearray()

    def connection_made(self, transport):
        """Method called when the serial connection is made. This method is called by the asyncio loop and should not be called directly."""
        self.transport: asyncio.BaseTransport = transport

    def data_received(self, data):
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
            expected_checksum = packet_size
            for item in packet:
                item = int(item)
                expected_checksum ^= item

            # If the checksums match, pass the packet to PyEasyTransfer
            if expected_checksum == received_checksum:
                self.pyeasytransfer.data_received(packet)
            else:
                self.pyeasytransfer.log.error(f"Checksum validation failed. Received: {received_checksum}, expected: {expected_checksum}")

            # Look for the next header in the remaining buffer
            index = self.buffer.find(self.pyeasytransfer.header_bytes)


def get_data_struct_size(struct_def: dict):
    struct_format = ''
    for _, value in struct_def.items():
        #print(value)
        struct_format += numpy_dtype_to_struct_format(value)
    return Struct(struct_format).size


import time
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
