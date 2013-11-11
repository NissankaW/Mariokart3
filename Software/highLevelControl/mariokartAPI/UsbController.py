'''
Mariokart project

Copyright 2012 University of Canterbury

Author: Matthew Wigley

This module contains functions to open a connection with the communications board
and to send and receive data.

In general this class should only be used to open a connection, close a connection,
or wait for the boards to enter run state.  Higher level Get and Set functions should be used instead
(e.g. look at speed.py, steering.py, brake.py).

Before using any ohter functions involving the boards
UsbController.Open() must be called.
It is recommended that WaitUntilRunning() is called after to ensure that the system is in the run state.
'''

from sys import stderr, exit, argv
from serial import Serial, SerialException
from definitions import *
import struct
import error





#connection
serial = None

CONNECTION_TIMEOUT = 5 #5 seconds timeout - need to decrease!


'''-------------------
    PUBLIC FUNCTIONS
   --------------------'''

#open a connection to the boards. By default uses ttyACM0
#but can also take the device path from the first command line option
def Open():
    global serial
    if len(argv) > 1 and argv[1] in ('-h', '--help'):
        print 'Usage: %s [serial device]\n\tDefault device is /dev/ttyACM0\n' % (argv[0])
        exit(0)


    serial = Serial()
    serial.port = argv[1] if len(argv) > 1 else '/dev/ttyACM0'
    serial.timeout = CONNECTION_TIMEOUT 

    #try to connect
    try:
        serial.open()
    except SerialException, e:
        print('Error, could not connect to the boards.')
        stderr.write('%s\n' % (e))
        serial = None
        exit(1)

    print('Connected.')


def BoardsRunning():
    if GetValue(VAR_BRK_POS) == None or GetValue(VAR_BRK_POS) == False:
        return False
    else:
        return True

#do a blocking wait until the system is started
def WaitUntilRunning():
    while BoardsRunning() == False:
        pass


#close the connection
def Close():
    global serial
    if serial:
        serial.close()
        serial = None

    print('Disconnected.')


'''--------------------
    DEVELOPER USE ONLY
   --------------------'''

#send a message telling the boards to enter error state
#send a set value comand over usb to the comms board which will then send it on the canbus
#returns True on success, False on failure
def EnterErrorState():
    global serial
    if serial:
        
        message = ADDR_COMMS_USB + ADDR_COMMS + CMD_ERROR + '\x00' + '\x00' + intToData(0) + '\xFF'

        serial.write(message)
        return True
    else:
        print('Error setting error state, not connected')
        return False

#send a set value comand over usb to the comms board which will then send it on the canbus
#returns True on success, False on failure
def SetValue(address, variableName, dataValue):
    global serial
    if serial:
        dataStr = intToData(dataValue);
        
        message = ADDR_COMMS_USB + address + CMD_SET + DATA_LENGTH_SET + variableName + dataStr  + '\xFF'

        serial.write(message)
        #TODO: confirmation of write - return false if the communications board does not reply with success
        return True
    else:
        print('Error setting value, not connected')
        return False


#send a request for the value of a variable to the comms board, which will then return the value back
#returns the value in the variable as an integer
#or None on failure
MESSAGE_SIZE = 9
def GetValue(variableName):
    global serial
    if serial:
        message = ADDR_COMMS_USB + ADDR_COMMS + DATA_LENGTH_GET + CMD_GET + variableName + intToData(0) + '\xFF'
        serial.write(message)

        #get the value
        result = serial.read(MESSAGE_SIZE)

        if result:
            #check if it is actually a reply, throw an exception if the boards are in Error state
            commandType = result[2]
            if commandType != CMD_REPLY:
                if commandType == CMD_ERROR:
                    raise error.BoardError("system has entered error state")
                print "system has entered error state"
                return False

            #make sure the right variable was returned
            variable = result[4]
            if variable != variableName:
                print "returned variable not correct\n"
                return False


            return dataToInt(result)
        else:
            print "Reception timed out!!!!\n"
            return None
    else:
        print "Error setting value, not connected\n"
        return None



#gets the integer stored in .data of a message and return it
def dataToInt(message):
    return struct.unpack('<i', message[5:9])[0] #little endian integer


#takes an integer and converts it into a string representation of a .data integer
def intToData(integer):
    dataStruct = struct.pack('<i', integer) #little endian integer
    #print(repr(dataStruct))
    #print(str(dataStruct))
    return str(dataStruct)

