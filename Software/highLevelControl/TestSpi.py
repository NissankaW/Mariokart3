from mariokartAPI import UsbController
from mariokartAPI import speed
import time

def testSpiMany():
    print("running SPI acceleration test")
    for i in range(1, 254):
        speed.SetAcceleration(i)
        time.sleep(0.1)
    print("done")


if __name__ == "__main__":
    UsbController.Open()
        
    #testSpiMany()
    accel = 20
    print("setting acceleration %i" % accel)

    speed.SetAcceleration(accel)

    UsbController.Close()
