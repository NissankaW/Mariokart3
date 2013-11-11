from mariokartAPI import speed, brake, UsbController, steering
import time
from Tkinter import Tk, Frame, BOTH, StringVar, Entry, END, Label, Button, Scale, HORIZONTAL
import os, sys, tempfile, time
from subprocess import Popen, PIPE

MAX_ANGLE = steering.MAX_STEERING_ANGLE
HORI_RANGE = 50

MAX_ACCEL = 20
Z_RANGE = 52


def relativeX(xVal):
    if xVal > HORI_RANGE/2:
        xVal = -(xVal - HORI_RANGE/2)
    if xVal > HORI_RANGE:
        xVal = HORI_RANGE
    return xVal


def steeringTrack(x):
    steer_angle = (relativeX(x)*MAX_ANGLE)/HORI_RANGE
    print "steering", steer_angle
    steering.SetAngle(steer_angle)  
    
def distanceTrack(z):
    #ADJUST baseDist in test2.cpp
    if z < 5:
        print "braking"
        brake.SetFullOn()
        speed.SetAcceleration(0)
    else:
        print "moving", z
        brake.SetOff()
        z = (z*MAX_ACCEL)/Z_RANGE
        if z > MAX_ACCEL:
            z = MAX_ACCEL
        speed.SetAcceleration(z)
    
    
    

def parentTask(r,w):
    os.close(w)
    r = os.fdopen(r, 'r')
    
    tvec = [0,0,0]
    
    UsbController.Open()
    brake.SetOff()
    
    while(True):
        txt = r.readline()
        if not txt: break
        parts = txt.split(" ")
        for i in range(len(parts)):
            if parts[i].startswith("X"):
                tvec[0] = int(parts[i][2:len(parts[i])])
            elif parts[i].startswith("Y"):
                tvec[1] = int(parts[i][2:len(parts[i])])
            elif parts[i].startswith("Z"):
                tvec[2] = int(parts[i][2:len(parts[i])]) 
        print"translation", tvec
        steeringTrack(tvec[0])
        distanceTrack(tvec[2])
        
              
    r.close()
    brake.SetFullOn()
    print "**parent closed**" 
    speed.SetAcceleration(0)
    UsbController.Close()
    
    
    

def childTask(r,w):
    os.close(r)
    w = os.fdopen(w, 'w')
    print "**configuring pipe**"
    proc = Popen(['/home/nissanka/Documents/prog/check_cmake/Checker'], 
                 stdout=w) 
    w.close()
    print "**child closed**"
    sys.exit(0)    
    
    
if __name__ == '__main__':
    r, w = os.pipe()
    print "**starting C++ pipe interface**"
    
    pid = os.fork()

    ##parent
    if pid:
        print "**running parent thread**"
        parentTask(r,w)
        
        
    ##child  
    else:
        print "running child thread"
        childTask(r,w)
        
    
