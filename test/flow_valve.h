#ifndef _FLOW_VALVE_H_
#define _FLOW_VALVE_H_

#pragma once
#include <Arduino.h>
#include <eRCaGuy_NewAnalogRead.h>
#include <config.h>


class FlowValve {
public:
    void measure_pressures( void );
    void measure_potentiometer( void );
    void calculate_inlet_flow_rate( void );
    void measure( void );
    float get_output_value( void );

    FlowValve(eRCaGuy_NewAnalogRead *adc_obj): analog_dc(adc_obj){};
    ~FlowValve(){};

    volatile float inlet_flow_rate;     // Flow rate of the inlet flow, in mL/min, calculated from the pressure measurements and valve rotation

private:
    eRCaGuy_NewAnalogRead *analog_dc;                       // Pointer to the analog_dc object

    // Const vars
    const uint16_t potentiometer_min_rotation = 9;          // Value when valve is fully closed
    const uint16_t potentiometer_max_rotation = 3655;       // Value when valve is fully open
    const float max_pressure_voltage = 0.1;                 // Maximum output voltage of the pressure transducer, in V (0 to 100mV)
    const uint16_t max_pressure_psi = 30;                   // Maximum pressure of the pressure transducer, in psi (0 to 30 psi)
    const uint16_t max_pressure_analog = (max_pressure_voltage/analog_vref) * max_analog;   // Maximum analog signal from the pressure transducer
    const uint8_t cv_size = 21;                             // Number of Cv values in the Cv array
    const float cv[21] = {0.0, 0.0053, 0.0120, 0.0184, 0.0245, 0.0303, 0.0358, 0.0410, 0.0458, 0.0504, 0.0546, 0.0585, 0.0623, 0.0656, 0.0688, 0.0715, 0.0742, 0.0764, 0.0786, 0.0802, 0.0818};       // Cv for valve 2
    const float galpermin_to_mlpermin = 3785.41;            // 1 gal/min is 3785.41 mL/min
    const float sg = 1.54;                                  // HFE7100 fluid specific gravity   
    //const float sg = 1.0;                                   // water specific gravity (used for testing)


    // Other vars
    volatile float potentiometer_value;                     // Rotation of the valve potentiometer as a voltage from ADC
    volatile float valve_rotation;                          // Rotation of the valve
    volatile uint16_t upper_rotation;                       // ceiling of the valve rotation
    volatile uint16_t lower_rotation;                       // floor of the valve rotation
    volatile float upper_cv_value;                          // Cv value of the upper rotation
    volatile float lower_cv_value;                          // Cv value of the lower rotation
    volatile float valve_cv_value;                          // Cv value of the valve (interpolated)
    volatile uint16_t inlet_pressure_upstream_raw;          // Measured analog signal from the upstream pressure transducer
    volatile uint16_t inlet_pressure_downstream_raw;        // Measured analog signal from the downstream pressure transducer
    volatile float valve_percent_open;                      // Percent the valve is open, measured by the potentiometer                     
    volatile float inlet_pressure_upstream;                 // Pressure of the upstream pressure transducer, in psi
    volatile float inlet_pressure_downstream;               // Pressure of the downstream pressure transducer, in psi


};


#endif