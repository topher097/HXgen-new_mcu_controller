from datetime import datetime
import os
import pickle
from ETData import ETDataArrays, ETData

def save_ETDataArrays(data: ETDataArrays, dir: str, filename: str=None) -> None:
    """Save the current ETDataArrays object to a pickle file.

    Args:
        data: ETDataArrays
            The ETDataArrays object to save to a pickle file
        dir: str
            The directory to save the pickle file to
        filename: str
            The filename to save the pickle file as. If None, the current time will be used with the name of the ETDataArrays object
    """
    if filename is None:
        datetime_str = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
        filename = f"{data.name}_{datetime_str}.pkl"
    file_path = os.path.join(dir, filename)
    with open(file_path, 'wb') as f:
        pickle.dump(data, f)
        
        
def load_ETDataArrays(dir: str, filename: str) -> ETDataArrays:
    """Load the current ETDataArrays object from a pickle file.

    Args:
        file_path: str
            The path to load the pickle file from

    Returns:
        data: ETDataArrays
            The ETDataArrays object loaded from the pickle file
    """
    file_path = os.path.join(dir, filename)
    
    # Confirm the file exists and is a pickle file
    if not os.path.isfile(file_path):
        raise FileNotFoundError(f"File '{file_path}' does not exist")
    if not file_path.endswith('.pkl'):
        raise FileNotFoundError(f"File '{file_path}' is not a pickle file")
    
    # Open the pickle file and load the data    
    with open(file_path, 'rb') as f:
        data = pickle.load(f)
        
    # Confirm it is a valid ETDataArrays object
    if not isinstance(data, ETDataArrays):
        raise TypeError(f"Loaded data is not of type ETDataArrays, it is of type {type(data)}")
    return data