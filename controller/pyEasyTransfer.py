import asyncio
import logging
from math import e
import struct
import numpy as np
from struct import Struct
from typing import Optional
import serial_asyncio
from IOData import IOData, IODataArrays


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
    struct_format = ""
    for name, dtype in struct_def.items():
        struct_format += byte_order + numpy_dtype_to_struct_format(dtype)
    return struct_format


class PyEasyTransfer:
    def __init__(self, log: logging.Logger, com_port: str, baud_rate: int,
                 input_struct_def: Optional[dict[str, np.dtype]] = None,
                 output_struct_def: Optional[dict[str, np.dtype]] = None,
                 byte_order: str = 'little-endian', mode: str = 'both',
                 save_read_data: Optional[IODataArrays]=None):

        if mode not in ['input', 'output', 'both']:
            raise ValueError("Invalid mode. Mode must be one of 'input', 'output', 'both'")

        self.data_received_event = asyncio.Event()           # Event to indicate when data has been received
        self.buffer = bytearray()                            # Buffer to store the data received over the serial connection
        self.log = log                                       # Logger object for logging events                            
        self.com_port = com_port                             # COM port to connect to
        self.baud_rate = baud_rate                           # Baud rate to connect at
        self.mode = mode                                     # The mode for this PyEasyTransfer instance
        self.byte_order = BYTE_FORMATS[byte_order]           # Byte order to use when packing and unpacking data

        # Depending on the mode, initialize the appropriate data classes
        if self.mode in ['input', 'both']:
            if not input_struct_def:
                raise ValueError("An input_struct_def must be provided when mode is 'input' or 'both'.")
            self.read_data = IOData(input_struct_def)           # Read from this to get the data from the Arduino
            self.buffer_size = self.read_data.struct_bytes      # Size of the buffer in bytes

        if self.mode in ['output', 'both']:
            if not output_struct_def:
                raise ValueError("An output_struct_def must be provided when mode is 'output' or 'both'.")
            self.write_data = IOData(output_struct_def)         # Write to this to send data to the Arduino

        self.save_read_data = save_read_data    # This is the IODataArrays object which is used to store the data. If this is not initialized then the data will not be stored
        self.start_buffering = False            # Flag to indicate when to start buffering data

        if self.mode in ['input', 'both']:
            self.struct_format_read = create_struct_format(self.byte_order, self.read_data.struct_def)
        if self.mode in ['output', 'both']:
            self.struct_format_write = create_struct_format(self.byte_order, self.write_data.struct_def)

        print(f"Struct format read: {self.struct_format_read}")
        print(f"Struct format write: {self.struct_format_write}")

    async def open(self):
        """Open the serial connection, if the """
        loop = asyncio.get_running_loop()
        if self.mode in ['input', 'both']:
            self._transport, self.reader = await serial_asyncio.create_serial_connection(loop, EasyTransferReceiver, self.com_port, baudrate=self.baud_rate)
            self.reader.pyeasytransfer = self
            self.reader.buffer_size = self.buffer_size
        if self.mode in ['output', 'both']:
            self.writer = self._transport

    async def close(self):
        if self._transport:
            self._transport.close()
            await self._transport._closing

    async def send_data(self):
        if self.mode in ['both', 'output']:
            data = [getattr(self.write_data, key) for key in self.write_data.struct_def.keys()]
            size = len(data)
            checksum = size
            for byte in data:
                checksum ^= byte  # XOR with each data byte
            #data.insert(0, checksum)                        # Insert the checksum at the end of the data
            

            # Create the format string for the header and footer
            header_format = (self.byte_order + 'B') * 3        # Three header bytes
            footer_format = self.byte_order + 'B'              # One footer byte
            full_format = header_format + self.struct_format_write + footer_format
            
            # Pack the data into a byte array
            byte_data_with_checksum = Struct(full_format).pack(0x06, 0x85, size, *data, checksum)
            self.writer.write(byte_data_with_checksum)
            await self.writer.drain()
            self.log.info(f"Sent data: {data}.")
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
            self.data_received_event.clear()
        else:
            raise ValueError("The PyEasyTransfer object is not in input mode. Waiting for data is not allowed.")

    def data_received(self, packet: bytes):
        if self.mode in ['both', 'input']:
            # Receive the data packet without the header/footer bytes

            # Create the format string for the header and footer
            #header_format = (self.byte_order + 'B') * 3        # Three header bytes
            #footer_format = self.byte_order + 'B'              # One footer byte
            #full_format = header_format + self.struct_format_read + footer_format
            print(f"Packet: {packet}")
            unpacked_data = Struct(self.struct_format_read).unpack(bytes(packet))
            print(unpacked_data)
            for key, value in zip(self.read_data.struct_def.keys(), unpacked_data):
                setattr(self.read_data, key, value)
            self.log.info(f"Received data: {unpacked_data}.")
        else:
            raise ValueError("The PyEasyTransfer object is not in input mode. Data receiving is not allowed.")



class EasyTransferReceiver(asyncio.Protocol):
    def __init__(self, byte_order='>'):
        self.pyeasytransfer: PyEasyTransfer = None
        self.buffer = bytearray()
        self.header = bytearray([0x06, 0x85])
        self.header_size = len(self.header)
        self.header_format = (byte_order + 'B') * self.header_size
        self.size_type = np.dtype(np.uint8).newbyteorder(byte_order)
        self.size_size = self.size_type.itemsize
        self.size_format = byte_order + numpy_dtype_to_struct_format(self.size_type)
        self.checksum_type = np.dtype(np.uint8).newbyteorder(byte_order)
        self.checksum_size = self.checksum_type.itemsize
        self.checksum_format = byte_order + numpy_dtype_to_struct_format(self.checksum_type)
        self.packet_format = "<I<f<?"
        #self.packet_dtypes = [np.uint32, np.float32, np.bool_]
        #print(f"header format: {self.header_format}, size: {self.header_size}")
        #print(f"size format: {self.size_format}, size: {self.size_size}")
        #print(f"checksum format: {self.checksum_format}, size: {self.checksum_size}")

    def connection_made(self, transport):
        self.transport: asyncio.BaseTransport = transport

    def data_received(self, data):
        """Protocol for receiving data from the serial port. Once data is received, it is added to the buffer and then the buffer is checked for a complete packet.
        Once a complete packet is found, the packet is sent PyEasyTransfer object for unpacking and processing.
        """
        self.buffer.extend(data)

        # Look for the header in the buffer
        index = self.buffer.find(self.header)
        while index != -1:
            # Remove all bytes before the header
            self.buffer = self.buffer[index:]

            # Check that we have enough bytes for the header and the size byte
            if len(self.buffer) < self.header_size + self.size_size:
                break

            # Extract the size of the packet (excluding the header and the size byte itself)
            packet_size = np.frombuffer(self.buffer[self.header_size:self.header_size+self.size_size], dtype=self.size_type)[0]
            print(f"Read packet size {packet_size}")
            
            # Check that we have enough bytes for the complete packet, including the checksum byte
            full_expected_size = self.header_size + self.size_size + packet_size + self.checksum_size
            if len(self.buffer) < full_expected_size:
                print(f"Not enough bytes for the complete packet, expected {full_expected_size}, got {len(self.buffer)}")
                break
            print(f"Expected packet size {full_expected_size}, got {len(self.buffer)}")

            # Extract the packet of data (the IOData data)
            packet_start_index = self.header_size + self.size_size
            packet_end_index = packet_start_index + packet_size
            packet = self.buffer[packet_start_index:packet_end_index]
            
            # Extract the checksum byte
            received_checksum = self.buffer[packet_end_index]

            # Remove the extracted bytes from the buffer
            end_index = packet_end_index + self.checksum_size
            self.buffer = self.buffer[end_index:]

            # Compute the expected checksum
            expected_checksum = packet_size
            for item in data:
                expected_checksum ^= item

            # If the checksums match, pass the packet to PyEasyTransfer
            if expected_checksum == received_checksum:
                print(f"Checksum validation passed. Received: {received_checksum}, expected: {expected_checksum}")
                self.pyeasytransfer.data_received(packet)
            else:
                print(f"Checksum validation failed. Received: {received_checksum}, expected: {expected_checksum}")

            # Look for the next header in the remaining buffer
            index = self.buffer.find(self.header)


def get_data_struct_size(struct_def: dict):
    struct_format = ''
    for _, value in struct_def.items():
        #print(value)
        struct_format += numpy_dtype_to_struct_format(value)
    return Struct(struct_format).size


async def test_both_in_out():
    # Input data from the serial connection struct definition, exactly as defined in the Arduino code
    input_struct_def = {"time_ms": np.uint32,
                        "sensor": np.float32,
                        "hello_received": np.bool_}

    # Output data to the serial connection struct definition, exactly as defined in the Arduino code
    output_struct_def = {"hello_flag": np.bool_}

    print(f"Input struct size: {get_data_struct_size(input_struct_def)}")
    print(f"Output struct size: {get_data_struct_size(output_struct_def)}")
    
    # Arduino serial connection parameters
    com_port = "COM14"
    baud_rate = 115200

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

    # Create a task to send a True hello flag every 2 seconds
    async def send_hello():
        while True:
            ET.write_data.hello_flag = True
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
    logging.basicConfig(level=logging.INFO)
    asyncio.run(test_both_in_out())
