from PySide6 import QtGui, QtCore, QtWidgets
import pylab as plt
import numpy as np

N_SAMPLES = 100000

def test_plot():
    time = np.arange(N_SAMPLES)*0.001
    sample = np.random.randn(N_SAMPLES)
    plt.plot(time, sample, label="Gaussian noise")
    plt.legend(fancybox=True)
    plt.title("Use the slider to scroll and the spin-box to set the width")
    q = ScrollingToolQT(plt.gcf())
    return q   # WARNING: it's important to return this object otherwise
               # python will delete the reference and the GUI will not respond!


class ScrollingToolQT(object):
    def __init__(self, fig):
        # Setup data range variables for scrolling
        self.fig = fig
        self.xmin, self.xmax = fig.axes[0].get_xlim()
        self.step = 1 # axis units

        self.scale = 1e3 # conversion betweeen scrolling units and axis units

        # Retrive the QMainWindow used by current figure and add a toolbar
        # to host the new widgets
        QMainWin = fig.canvas.parent()
        toolbar = QtWidgets.QToolBar(QMainWin)
        QMainWin.addToolBar(QtCore.Qt.BottomToolBarArea, toolbar)

        # Create the slider and spinbox for x-axis scrolling in toolbar
        self.set_slider(toolbar)
        self.set_spinbox(toolbar)

        # Set the initial xlimits coherently with values in slider and spinbox
        self.set_xlim = self.fig.axes[0].set_xlim
        self.draw_idle = self.fig.canvas.draw_idle
        self.ax = self.fig.axes[0]
        self.set_xlim(0, self.step)
        self.fig.canvas.draw()

    def set_slider(self, parent):
        # Slider only support integer ranges so use ms as base unit
        smin, smax = self.xmin*self.scale, self.xmax*self.scale

        self.slider = QtWidgets.QSlider(QtCore.Qt.Horizontal, parent=parent)
        self.slider.setTickPosition(QtWidgets.QSlider.TicksAbove)
        self.slider.setTickInterval((smax-smin)/10.)
        self.slider.setMinimum(smin)
        self.slider.setMaximum(smax-self.step*self.scale)
        self.slider.setSingleStep(self.step*self.scale/5.)
        self.slider.setPageStep(self.step*self.scale)
        self.slider.setValue(0)  # set the initial position
        self.slider.valueChanged.connect(self.xpos_changed)
        parent.addWidget(self.slider)

    def set_spinbox(self, parent):
        self.spinb = QtWidgets.QDoubleSpinBox(parent=parent)
        self.spinb.setDecimals(3)
        self.spinb.setRange(0.001, 3600.)
        self.spinb.setSuffix(" s")
        self.spinb.setValue(self.step)   # set the initial width
        self.spinb.valueChanged.connect(self.xwidth_changed)
        parent.addWidget(self.spinb)

    def xpos_changed(self, pos):
        #pprint("Position (in scroll units) %f\n" %pos)
        #        self.pos = pos/self.scale
        pos /= self.scale
        self.set_xlim(pos, pos + self.step)
        self.draw_idle()

    def xwidth_changed(self, xwidth):
        #pprint("Width (axis units) %f\n" % step)
        if xwidth <= 0: return
        self.step = xwidth
        self.slider.setSingleStep(self.step*self.scale/5.)
        self.slider.setPageStep(self.step*self.scale)
        old_xlim = self.ax.get_xlim()
        self.xpos_changed(old_xlim[0] * self.scale)
#        self.set_xlim(self.pos,self.pos+self.step)
 #       self.fig.canvas.draw()

if __name__ == "__main__":
    q = test_plot()
    plt.show()