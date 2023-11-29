# arduinowatercooler

Group Name: Masterson-Chau
Names: Jacob Masterson, Elena Chau
Link to video demonstration:  https://youtu.be/En1O1CxAWRo 

The design is a swamp cooler in which there are four states: disable, idle, running, error. 
The system uses interrupts in order to switch between disable and the other 3 states, and to reset after an error.
There is a real-time clock module for event reporting in which it outputs the date and time the state or vent position changes.
All changes to state are reported in the Serial Monitor.
In the disable state, the start/stop button has not been pressed yet, and all sensors are turned off to save power.
After clicking on the start button, the sensors turn on, and the system is enabled. It checks for the water levels.
If the water levels are too low, the error state occurs.
If the temperature is high, the running state occurs in which the fan is turned on. The fan is attached to a kit motor with a power supply of input voltage of 6.5-9v (DC) via 5.5mm x 2.1mm plug and an output voltage of 3.3V/5v
The DHT11 checks for the temperature and humidity. When there is no error and the temperature is low, then the idle state occurs. 
The stepper motor can adjust the vent directon accordingly with a potentionmeter. Changes to vent position are reported in the Serial Monitor.
The LCD displays the temperature and humidity when in idle or enabled state. When disabled, it says the machine is off, and when there is an error, it says water level is too low.0b

Constraints:
The DHT11 humidity sensor can read from 0 to 50 degrees Celsius and can read from 20 to 80% humidity. It operates from 3-5 volts.
The Water Sensor functions when the water temperature is between 10 and 30 degrees celsius. It functions at around 5 volts, and outputs an analog signal that ranges from 0 to 4.2 volts.
The LEDS are powered by around 2 volts.
The LCD is powered by ~5 volts.
The stepper motor is controlled by a potentiometer analog reading and takes 5 volts as input. 
The DC motor attached to the fan takes 6 volts and must be attached to a separate power supply so that it does not damage the Arduino.
The DS1307 Real Time Clock takes 5 volts as input
The push buttons operate at a maximum voltage of 24 volts. They have been built in pull down resistor configuration.
