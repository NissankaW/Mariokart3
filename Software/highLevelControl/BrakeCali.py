from mariokartAPI import UsbController
from mariokartAPI import brake
import time

def testBrake():
    print("manual brake calibration")
     #testing some of brake.py
    #print("initial brake position in mm:")
    #print(brake.GetPositionMM())
    #print("initial brake position in adc:")
    #print(brake.GetActuatorPositionADCUnits())

    #print("\n--------------------------")
    #print("moving brake to 18mm")
    #brake.SetPositionMM(18)
    #time.sleep(6)
    #print("brake position in mm:")
    #print(brake.GetPositionMM())
    #print("brake position in adc:")
    #print(brake.GetActuatorPositionADCUnits())

    print("\n--------------------------")
    print("setting brake to full on")
    brake.SetFullOn()
    time.sleep(5)
    print("brake position in mm:")
    print(brake.GetPositionMM())
    print("brake full on position in mm:")
    print(brake.GetFullOnPositionMM())
    print("brake position in adc:")
    print(brake.GetActuatorPositionADCUnits())
    time.sleep(10)

    print("\n--------------------------")
    print("setting brake to off")
    brake.SetOff()
    time.sleep(5)
    print("brake position in mm:")
    print(brake.GetPositionMM())
    print("brake position in adc:")
    print(brake.GetActuatorPositionADCUnits())

    print("done brake test")



if __name__ == "__main__":
    UsbController.Open()
    testBrake()
    UsbController.Close()
