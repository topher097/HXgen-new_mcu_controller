from PySide6.QtWidgets import QWidget, QSlider, QTextEdit
from PySide6.QtCore import Signal, Slot 
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg
from matplotlib.figure import Figure
import matplotlib as mpl

from logging import Handler

class QLoggerStream(Handler):
    """Class to create a logging handler for the QPlainTextEdit widget"""
    def __init__(self, widget: QTextEdit):
        super().__init__()
        self.widget = widget

    def emit(self, record):
        msg = self.format(record)
        self.widget.append(msg)


class CentralWidget(QWidget):
    """Derived class of QWidget to create a seperate central widget"""
    def __init__(self, *args):
        super(CentralWidget, self).__init__(*args)



class MplCanvas(FigureCanvasQTAgg):
    """Class to create a Matplotlib figure widget for plotting 2d plots in Qt6"""
    def __init__(self, parent=None, width=5, height=4, dpi=100):
        mpl.rcParams['toolbar'] = 'None'
        fig = Figure(figsize=(width, height), dpi=dpi)
        self.axes = fig.add_subplot(111)
        super(MplCanvas, self).__init__(fig)
    

class ListSlider(QSlider):
    """Creates a list of values for a QSlider, so that the slider can be used to select a value from the list of default index values"""
    elementChanged = Signal(int)

    def __init__(self, values: list[int], *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.setMinimum(0)
        self._values = []       # list of index range of values
        self.valueChanged.connect(self._on_value_changed)
        self.values = values

    """Return the slider values when calling slider.values()"""
    @property
    def values(self):
        return self._values

    """When calling slider.point_number, return the value from the list of slider values"""
    @property
    def point_number(self):
        return self.values[self.sliderPosition()]

    """Set the values of the slider"""
    @values.setter
    def values(self, values: list[int]):
        self._values = values
        # minimum = 1
        maximum = max(0, len(self._values))
        #print(f"max: {maximum}")
        self.setMaximum(maximum)
        self.setValue(0)
    
    @Slot(int)
    def _on_value_changed(self, index):
        slider_value = self.values[index]
        #print(f"point slider value: {slider_value}, index: {index}")
        self.elementChanged.emit(slider_value)