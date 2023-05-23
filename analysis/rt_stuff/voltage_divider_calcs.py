import pandas as pd
import numpy as np
from sympy import symbols, Eq, solve
from scipy.optimize import curve_fit
import matplotlib.pyplot as plt

# Define the symbolic equation
V, R1, Rt, Vref = symbols('V R1 Rt Vref')
equation = Eq(V, Vref * R1 / (R1 + Rt))

def load_thermistor_data(file_path):
    data = pd.read_csv(file_path)
    return data['temperature'], data['resistance']

def fit_function(x, R1, Vref, analog_res):
    T = np.array(x)
    analog_reading = (Vref * R1 / (R1 + T)) * (analog_res - 1) / Vref
    return analog_reading

def create_thermistor_function(file_path, R1, Vref, analog_res):
    temperature, resistance = load_thermistor_data(file_path)
    popt, _ = curve_fit(fit_function, resistance, temperature, p0=[R1, Vref, analog_res])
    return lambda analog_reading: fit_function(analog_reading, *popt)

def plot_analog_to_temperature(func, file_path):
    temperature, resistance = load_thermistor_data(file_path)
    voltage = func(resistance)
    plt.scatter(temperature, voltage, label='Data')
    T = np.linspace(min(temperature), max(temperature), 500)
    V = func(T)
    plt.plot(T, V, 'r', label='Fitted function')
    plt.xlabel('Temperature (°C)')
    plt.ylabel('Analog Reading')
    plt.legend()
    plt.draw()

def plot_current_draw(file_path, R1, Vref, analog_res, num_thermistors):
    temperature, resistance = load_thermistor_data(file_path)
    popt, _ = curve_fit(fit_function, resistance, temperature, p0=[R1, Vref, analog_res])
    T = np.linspace(min(temperature), max(temperature), 500)
    Rt = fit_function(T, *popt)
    I = Vref / (R1 + Rt)
    total_I = num_thermistors * I
    plt.figure()  # Create a new figure
    plt.plot(T, total_I)
    plt.xlabel('Temperature (°C)')
    plt.ylabel('Total current draw (A)')
    plt.title('Current Draw vs Temperature')
    
def get_teensy_equation_symbolic():
    # Define the symbols
    analog_read, R1_s, Rt_s, Vref_s, analog_res_s = symbols('analog_read R1 Rt Vref analog_res')

    # Equation for the voltage at the analog pin in terms of Rt
    voltage_eq = Eq(Vref_s * R1_s / (R1_s + Rt_s), analog_read * Vref_s / (analog_res_s - 1))

    # Solve for Rt
    Rt_eq = solve(voltage_eq, Rt_s)[0]

    # Here you could insert your conversion from Rt to temperature.
    # If it's a simple equation, you could write something like:
    # temperature_eq = Eq('temperature', Rt_eq * conversion_factor + conversion_offset)
    # If not, just return the equation for Rt:

    return Rt_eq



# Define the parameters
from pathlib import Path
rt_table_path = Path(Path.cwd(), "rt_data", '103JG1F_RT_Table.csv')        # 10kOhm @ 25C
vref = 3.3  # 3.3V
r1 = 5100  # resistance of the fixed resistor
num_thermistors = 12  # number of thermistors in the voltage divider
analog_res = 1024 # resolution of the analog to digital converter 12 bits

# Usage
thermistor_function = create_thermistor_function(rt_table_path, r1, vref, analog_res)
#print(thermistor_function(8191))  # Use the function
plot_analog_to_temperature(thermistor_function, rt_table_path)
plot_current_draw(rt_table_path, r1, vref, analog_res, num_thermistors)
teensy_eq = get_teensy_equation_symbolic()
print(f"float thermistor_temp = {teensy_eq}")

plt.show()
