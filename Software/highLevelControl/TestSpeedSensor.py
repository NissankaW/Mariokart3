from mariokartAPI import UsbController
from mariokartAPI import speed
import time


def testSpeedSensor():
    speeds = 0
    for i in range(1, 100):
        for j in range(1, 2):
            speeds += speed.GetSpeed()
            time.sleep(0.5)
        print(speeds/2)
        speeds =0

def testSpeedSensorSingle():
    for i in range(1, 500):
        print(speed.GetSpeed())
        time.sleep(0.3)
        

if __name__ == "__main__":
    UsbController.Open()
    #testSpeedSensor()
    testSpeedSensorSingle()

    UsbController.Close()
