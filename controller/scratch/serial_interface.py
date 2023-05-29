"""
pyEasyTransfer.py

This is a Python implementation of the EasyTransfer Arduino library. It is used to send and receive data between an Arduino and a Python program over a serial connection. 
The data is sent in a packet format which is defined in the EasyTransfer library. The packet format is as follows:

start_byte_1 (0x06) | start_byte_2 (0x85) | packet_size (1 byte) | packet_type (1 byte) | data (packet_size - 2 bytes) | checksum (1 byte)

The packet_size is the number of bytes in the structured data packet. 



"""



import asyncio
import qasync
import serial_asyncio
import logging
import numpy as np
from struct import pack
from typing import Optional
from IOData import IOData, IODataArrays
from dataclasses import dataclass
    
# These are from the EasyTransfer library
START_BYTE_1 = 0x06
START_BYTE_2  = 0x85

MAX_PACKET_SIZE = 0xFE

BYTE_FORMATS = {'native':          '@',
                'native_standard': '=',
                'little-endian':   '<',
                'big-endian':      '>',
                'network':         '!'}

class SerialInterface(asyncio.Protocol):
    """Creates an asyncio serial connection to a COM port at a baud rate

    Args:
        log (logging.Logger): Logger object for logging events
        com_port (str): COM port to connect to
        baud_rate (int): Baud rate to connect at
        struct_def (dict[str, np.dtype]): Dictionary of the data types to be sent and received. The key is the name of the data type and the value is the numpy dtype object
        byte_order (str): Byte order to use when packing and unpacking data. Must be one of the following: 'native', 'native_standard', 'little-endian', 'big-endian', 'network'
    """
    
    def __init__(self, log: logging.Logger, com_port: str, baud_rate: int, struct_def: dict[str, np.dtype], byte_order: str, output_data: Optional(IODataArrays)=None):
        self.data_received_event = asyncio.Event()          # Event to indicate when data has been received
        self.buffer = bytearray()                           # Buffer to store the data received over the serial connection
        self.log = log                                      # Logger object for logging events                            
        self.com_port = com_port                            # COM port to connect to
        self.baud_rate = baud_rate                          # Baud rate to connect at
        self.input_data = IOData(struct_def)                # Read from this from the buffer_to_data function to get the data from the Arduino
        self.read_data = IOData(struct_def)                 # Write to this via the MainWindow callbacks for sending data to the Arduino
        self.output_data = output_data                      # This is the IODataArrays object which is used to store the data. If this is not initialized then the data will not be stored
        self.buffer_size = self.input_data.struct_bytes + 1 # Size of the buffer in bytes
        self.start_buffering = False                        # Flag to indicate when to start buffering data
        self.byte_order = BYTE_FORMATS[byte_order]          # Byte order to use when packing and unpacking data

    def connection_made(self, transport: serial_asyncio.SerialTransport):
        """Take a serial connection and start a task to read from it"""
        self.transport = transport
        self.log.info(f'Port opened: {transport}')
        transport.serial.rts = False
        self.read_task = self.loop.create_task(self.read())

    def data_received(self, data) -> None:
        """When data is received over the Serial connection, add it to the buffer. Once the buffer is full then send it to the buffer_to_data function to convert it to a numpy array"""
        if self.start_buffering:
            self.buffer.extend(data)
            self.data_received_event.set()
            if len(self.buffer) >= self.buffer_size:
                self.loop.create_task(self.buffer_to_data())
        elif b'\xaa\xbb' in data:  # replace '\xaa\xbb' with your specific bytes
            self.start_buffering = True
            self.buffer.extend(data[data.index(b'\xaa\xbb')+2:])  # start buffering from after the start sequence

    def connection_lost(self, exc) -> None:
        """If a connection is lost, cancel the read task and stop the event loop"""
        self.log.warning('Connection lost, port closed')
        self.transport.loop.stop()
        self.read_task.cancel()

    def close_connection(self) -> None:
        """Close the serial connection"""
        self.log.info('Closing serial connection')
        self.transport.close()

    async def create_connection(self, loop: qasync.QEventLoop) -> serial_asyncio.SerialTransport:
        """Create the connection and return the transport and protocol objects"""
        self.loop = loop
        return await serial_asyncio.create_serial_connection(loop, lambda: self, self.com_port, baudrate=self.baud_rate)
   
    async def write(self, data) -> None:
        """Write data to the serial connection"""
        if self.transport:
            self.transport.write(data)
            self.log.info('Data written: %s', data)

    async def read(self) -> None:
        """Read data from the serial connection and add it to the buffer. Once the buffer is full then send it to the buffer_to_data function to convert it to a numpy array"""
        while True:
            await self.data_received_event.wait()
            self.data_received_event.clear()
            if len(self.buffer) >= self.input_data.buffer_size:
                await self.buffer_to_data()
    
    async def buffer_to_data(self) -> None:
        """Take the buffer and convert it and save it to the output_data object (IOData object)"""
        # Assuming that the data is received in the same order as declared in IOData 
        fields = list(self.input_data.dtype_map.keys())
        # Create a numpy dtype object from the map, assuming little endian byte order
        dt = np.dtype([(key, np.dtype(self.input_data.dtype_map[key]).newbyteorder(self.byte_order)) for key in self.input_data.dtype_map])
        idx = 0
        for field in fields:
            size = np.dtype(dt[field]).itemsize
            data = np.frombuffer(self.buffer[idx:idx+size], dtype=dt[field])[0]
            if field in dt.names:
                attr = getattr(self.input_data, field)
                print(f"Type of '{field}': {type(attr)}: {data}")
                attr[self.input_data.io_count] = data
            else:
                print(f"Warning: Trying to assign data to non-array field '{field}' in IOData.")
            idx += size
        #self.buffer = self.buffer[idx:]
        #print(f"received {idx} btyes")
        self.buffer = bytearray()
        self.input_data.io_count += 1
        self.log.debug(f"Received data: {self.input_data}")
    
    def send(self, data: IOData):
        # First, pack the data to bytes, then send it over the serial connection
        packed_data = self.pack_data(data)
        self.write(packed_data)

    @staticmethod
    def pack_data(data: IOData, index_to_pack: int = 0):
        packed_data = b'\xaa\xbb'  # Start sequence
        
        # Iterate over all attributes and pack them to bytes
        for attr, value in data.__dict__.items():
            value = value[index_to_pack]  # Since it's a numpy array with one element, take the first
            if isinstance(value, bool):
                packed_data += pack('?', value)
            elif isinstance(value, np.uint8):
                packed_data += pack('B', value)
            elif isinstance(value, np.uint16):
                packed_data += pack('H', value)
            elif isinstance(value, np.uint32):
                packed_data += pack('I', value)
            elif isinstance(value, float):
                packed_data += pack('f', value)
            else:
                raise TypeError(f"Unsupported data type for attribute {attr}: {type(value)}")
        
        return packed_data
    