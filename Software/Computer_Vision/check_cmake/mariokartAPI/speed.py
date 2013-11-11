'''
Mariokart project

Copyright 2012 University of Canterbury


Author: Matthew Wigley

This module contains high level functions involving speed control in the mariokart project

This module contains functions for getting and setting the speed of the kart

Note that the UsbController module must be initialized before using this module.

'''

import UsbController
import definitions

'''-------------------------
       PRIVATE CONSTANTS
   -------------------------'''
#conversion factor to convert the speed sent over USB to a m/s value
SPEED_MULTIPLIER = 1000.0

#goForward() speed in meters per second
GO_FORWARD_SPEED = 1

#maximum speed of the kart in meters per second
MAX_SPEED = 3

#PID gain and scaling limits
MAX_GAIN = 500
MIN_GAIN = 1
MAX_SCALE = 500
MIN_SCALE = 1

#for setting the acceleration directly, should not be used except during testing
#max 80% inverted PWM allowed-> max PWM = 20%
MAX_ACCELERATION = (100)

'''-------------------------
       PUBLIC FUNCTIONS
   -------------------------'''

#get the speed of the kart in meters per second
#returns None on failure
def GetSpeed():
    return UsbController.GetValue(definitions.VAR_SPEED)/SPEED_MULTIPLIER

#set the intended speed of the kart in meters per second
#PID control will be used to accelerate the kart to the intended value
#returns True/False on success/failure
def SetSpeed(speed):
    if speed >= 0 and speed <= MAX_SPEED:
        return UsbController.SetValue(definitions.ADDR_MOTOR, definitions.VAR_SPEED, int(SPEED_MULTIPLIER*speed))
    else:
        return False

#start travelling forwards at a slow constant speed
#returns True/False on success/failure
def GoForwards():
    return setSpeed(GO_FORWARD_SPEED)

#slow the kart to a stop
#returns True/False on success/failure
def StopKart():
    return setSpeed(0)


#this is for open loop accelerator control.
#it should NOT be used. SetSpeed should be used instead
#the acceleration value is in pulse width units for the student board
#a sufficiently low acceleration will instead do regenerative braking
#basically, use this at your own risk
def SetAcceleration(acceleration):
    if acceleration >= 0 and acceleration <= MAX_ACCELERATION:
        return UsbController.SetValue(definitions.ADDR_MOTOR, definitions.VAR_ACCELERATION, acceleration)
    else:
        return False;
    
#Set PID gains and scale factor
def SetKp(Kp):
    if Kp >= MIN_GAIN and Kp <= MAX_GAIN:
        return UsbController.SetValue(definitions.ADDR_MOTOR, definitions.VAR_KP, int(Kp))
    else:
        return False
    
def SetKi(Ki):
    if Ki >= MIN_GAIN and Ki <= MAX_GAIN:
        return UsbController.SetValue(definitions.ADDR_MOTOR, 
                                      definitions.VAR_KI, int(Ki))
    else:
        return False
    
def SetKd(Kd):
    if Kd >= MIN_GAIN and Kd <= MAX_GAIN:
        return UsbController.SetValue(definitions.ADDR_MOTOR, 
                                      definitions.VAR_KD, int(Kd))
    else:
        return False
    
def SetKscale(Kscale):
    if Kscale >= MIN_SCALE and Kscale <= MAX_SCALE:
        return UsbController.SetValue(definitions.ADDR_MOTOR, 
                                      definitions.VAR_KSCALE, int(Kscale))
    else:
        return False