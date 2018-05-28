# psychotron
A pedalboard built on Pure Data and controlled by an Arduino powered MIDI controller

This project comprises :
- MIDI controller : the Arduino code for an Arduino Mega2560 and a MuxShield II from Mayhew Labs
- Effects engine : The Pure Data code for Pure Data v0.48-1
- (TODO) A rc.local example file to launch the program on start (tested on a Raspberry Pi 3 Model B)


Improvements to male in the v2 :
- MIDI controller : better handling of the callbacks (use a loop based on the function available() to avoid overflowing the serial buffer
- Pure Data : normalize the inputs, outputs, GUI of the different effects
- Pure Data : abstract the engines of the effects a little better in order to avoid code duplication
