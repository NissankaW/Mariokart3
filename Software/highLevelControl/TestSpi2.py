from mariokartAPI import UsbController
from mariokartAPI import speed
import time

def testSpiMany():
    print("running SPI acceleration test")
    #print("speed: ")
    #print(speed.GetSpeed())
    print(speed.SetSpeed(0))
    #print(speed.GetSpeed())
    print("done")

def testRamp():
    print("Ramping")
    speed.RampPWM(15,4,-1)

def accel():
    print(speed.SetAcceleration(10))

if __name__ == "__main__":
    UsbController.Open()
        
    #testSpiMany()
    #testRamp()
    accel()

    UsbController.Close()
