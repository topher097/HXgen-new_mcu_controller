import asyncio
import qasync
import serial_asyncio
import logging
import numpy as np
from struct import pack

from data_transfer import IOData

class SerialInterface(asyncio.Protocol):
    """Creates an asyncio serial connection to a COM port at a baud rate

    Args:
        log (logging.Logger): Logger object for logging events
    """
    def __init__(self, log: logging.Logger) -> None:
        self.transport = None
        self.data_received_event = asyncio.Event()
        self.buffer = bytearray()
        self.log = log
        self.input_data = IOData(5000)          # Read from this via the MainWindow callbacks for displaying to the user
        self.output_data = IOData(1)            # Write to this via the MainWindow callbacks for sending to the MCU
        self.buffer_size = self.input_data.buffer_size
        self.start_buffering = False

    def connection_made(self, transport: serial_asyncio.SerialTransport):
        self.transport = transport
        self.log.info(f'Port opened: {transport}')
        transport.serial.rts = False
        self.read_task = self.loop.create_task(self.read())

    def data_received(self, data):
        if self.start_buffering:
            self.buffer.extend(data)
            self.data_received_event.set()
            if len(self.buffer) >= self.input_data.buffer_size:
                self.loop.create_task(self.buffer_to_data())
        elif b'\xaa\xbb' in data:  # replace '\xaa\xbb' with your specific bytes
            self.start_buffering = True
            self.buffer.extend(data[data.index(b'\xaa\xbb')+2:])  # start buffering from after the start sequence


    def connection_lost(self, exc):
        self.log.warning('Connection lost, port closed')
        self.transport.loop.stop()
        self.read_task.cancel()

    def close_connection(self):
        self.log.info('Closing serial connection')
        self.transport.close()

    async def create_connection(self, loop: qasync.QEventLoop, port, baudrate):
        self.loop = loop
        return await serial_asyncio.create_serial_connection(loop, lambda: self, port, baudrate=baudrate)
   
    async def write(self, data):
        if self.transport:
            self.transport.write(data)
            self.log.info('Data written: %s', data)

    async def read(self):
        while True:
            await self.data_received_event.wait()
            self.data_received_event.clear()
            if len(self.buffer) >= self.input_data.buffer_size:
                await self.buffer_to_data()
    
    async def buffer_to_data(self):
        # Assuming that the data is received in the same order as declared in IOData
        fields = list(self.input_data.dtype_map.keys())
        # Create a numpy dtype object from the map, assuming little endian byte order
        dt = np.dtype([(key, np.dtype(self.input_data.dtype_map[key]).newbyteorder('<')) for key in self.input_data.dtype_map])
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
        print(f"received {idx} btyes")
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
    