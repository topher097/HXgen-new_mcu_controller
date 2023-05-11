#include <flow_valve.h>

void FlowValve::measure( void ){
    // Run the functions to measure the flow through the valve
    measure_pressures();
    measure_potentiometer();
    calculate_inlet_flow_rate();
}

// Calculate the inlet flow rate using the two pressure measurements from inlet sensors
void FlowValve::calculate_inlet_flow_rate(){
    // Calculate the Cv value given the potentiometer value
    valve_rotation = map(potentiometer_value, potentiometer_min_rotation, potentiometer_max_rotation, 0, cv_size-1);   // Calculate the valve rotation on scale from 0 to cv_size-1
    upper_rotation = ceil(valve_rotation);                  // index of lower Cv value index
    lower_rotation = floor(valve_rotation);                 // index of upper Cv value index
    upper_cv_value = cv[upper_rotation];                    // lower value of Cv
    lower_cv_value = cv[lower_rotation];                    // upper value of Cv
    // Linearly interpolate the Cv value from the two closest Cv values
    valve_cv_value = lower_cv_value + ((valve_rotation-lower_rotation)*(upper_cv_value - lower_cv_value))/(upper_rotation - lower_rotation);   // linear interpolation of Cv value
    // Calculate the inlet flow rate using the Cv value and the pressure measurements
    inlet_flow_rate = valve_cv_value * sqrt(abs(inlet_pressure_upstream - inlet_pressure_downstream)/sg);                                       // flow rate in gal/min
    // Check if the inlet flow rate is negative or NaN
    if (isnan(inlet_flow_rate) || inlet_flow_rate < 0.0){
        inlet_flow_rate = 0.0;
    }
    // Convert the inlet flow rate from gal/min to mL/min
    inlet_flow_rate = inlet_flow_rate * galpermin_to_mlpermin;            
}


// Measure and calculate the pressures of the pressure transducers across the valve
void FlowValve::measure_pressures(){
    // Measure the pressure transducer voltages
    inlet_pressure_upstream_raw = analog_dc->newAnalogRead(UPSTREAM_PRESSURE_SENSOR_PIN);
    inlet_pressure_downstream_raw = analog_dc->newAnalogRead(DOWNSTREAM_PRESSURE_SENSOR_PIN);
    
    // Convert the pressure transducer voltages to pressures in psi
    inlet_pressure_upstream = map(inlet_pressure_upstream_raw, 0, max_pressure_analog, 0, max_pressure_psi);
    inlet_pressure_downstream = map(inlet_pressure_downstream_raw, 0, max_pressure_analog, 0, max_pressure_psi);
}


// Measure the potentiometer value
void FlowValve::measure_potentiometer(){
    potentiometer_value = analog_dc->newAnalogRead(VALVE_POTENTIOMETER_PIN);  
}