from mariokartAPI import speed, brake, UsbController, steering
import time
from Tkinter import Tk, Frame, BOTH, StringVar, Entry, END, Label, Button, Scale, HORIZONTAL, RIDGE

accel = 0
brakePos = 0
steerAngle = 0

class MainFrame(Frame):
  
    def __init__(self, parent):
        Frame.__init__(self, parent, background="white")   
        self.parent = parent
        self.initContents()

    def initContents(self):
        self.grid()
        
        self.SPI = SPIFrame(self)
        self.SPI.grid(column=0, row=0, columnspan=2, sticky='EW')
        
        self.Brake = BrakeFrame(self)
        self.Brake.grid(column=0, row=1, columnspan=2,sticky='EW')

        self.Steering = SteeringFrame(self)
        self.Steering.grid(column=0, row=2, columnspan=3,sticky='EW')
        
        
        #quitButton = Button(self, text="Quit", command=self.quit)
        #quitButton.grid(column=0, row=0)



#Mainframe        
#------------------------------------------------------------------------------
#SPI
class SPIFrame(Frame):
    def __init__(self, parent):
        Frame.__init__(self, parent, bd=4, relief=RIDGE)   
        self.parent = parent
        self.initialize()
        
    def initialize(self):
        self.grid()
        self.entryVariable = StringVar()
        self.entry = Entry(self, textvariable=self.entryVariable)
        self.entry.grid(column=0,row=0,sticky='EW')
        self.entry.bind("<Return>", self.OnPressEnter)

        button = Button(self,text="SPI send", command=self.OnButtonClick)
        button.grid(column=1,row=0)
        
        #ramp = Button(self,text="RAMP", command=self.setlabvar)
        #ramp.grid(column=1,row=1)        

        self.labelVariable = StringVar()
        label = Label(self,textvariable=self.labelVariable,
                              anchor="w",fg="white",bg="blue")
        label.grid(column=0,row=1,columnspan=1,sticky='EW')
        self.labelVariable.set("Start..")
        
        self.slider = Scale(self, from_=0, to=80, orient=HORIZONTAL, 
                            command=self.setlabvar)
        self.slider.grid(column=0, row=2, columnspan=3, sticky='EW')
        
        self.PID = PIDTune(self)
        self.PID.grid(column=0, row=3, columnspan=3, sticky='EW')
        
        self.grid_columnconfigure(0,weight=1)
        self.update()
        #self.geometry(self.geometry()) # caused busy wait?
        self.entry.focus_set()
        #self.entry.selection_range(0, Tkinter.END) # caused busy wait?
        

        
        
    def setlabvar(self, val):
        val = self.slider.get()
        speed.SetAcceleration(val)
        accel = val;
        self.labelVariable.set("Duty set: " + str(val) )       
        

    def OnButtonClick(self):
        accel = int(self.entryVariable.get())
        self.slider.set(accel)
        speed.SetAcceleration(accel)
        self.labelVariable.set("Duty set: " + str(accel) )
        self.entry.focus_set()
        self.entry.selection_range(0, END)
        
    def OnPressEnter(self,event):
        accel = int(self.entryVariable.get())
        self.slider.set(accel)
        speed.SetAcceleration(accel)
        self.labelVariable.set("Duty set: " + str(accel) )
        self.entry.focus_set()
        self.entry.selection_range(0, END)
        
#SPI
#------------------------------------------------------------------------------
#PID     

class PIDTune(Frame):
    def __init__(self, parent):
        Frame.__init__(self, parent, bd=4, relief=RIDGE)   
        self.parent = parent
        self.initialize()
        
    def initialize(self):
        self.grid()
        
        ##PID TUNING
        self.KpVar = StringVar()
        self.KiVar = StringVar()
        self.KdVar = StringVar()
        self.KscaleVar = StringVar()
        
        self.Kp = Entry(self, textvariable=self.KpVar)
        self.Ki = Entry(self, textvariable=self.KiVar)
        self.Kd = Entry(self, textvariable=self.KdVar)
        self.Kscale = Entry(self, textvariable=self.KscaleVar)
        self.Kp.grid(column=0,row=2,sticky='EW')
        self.Ki.grid(column=0,row=3,sticky='EW')
        self.Kd.grid(column=0,row=4,sticky='EW')
        self.Kscale.grid(column=0,row=5,sticky='EW')
        self.Kp.bind("<Return>", self.OnKp)
        self.Ki.bind("<Return>", self.OnKi)
        self.Kd.bind("<Return>", self.OnKd)
        self.Kscale.bind("<Return>", self.OnKscale)
        
        self.KpLabVar = StringVar()
        self.KiLabVar = StringVar()
        self.KdLabVar = StringVar()
        self.KscaleLabVar = StringVar()
        kpLabel = Label(self,textvariable=self.KpLabVar,
                              anchor="w",fg="white",bg="blue")
        kpLabel.grid(column=1, row=2,columnspan=1,sticky='EW')
        self.KpLabVar.set("Kp")
        kiLabel = Label(self,textvariable=self.KiLabVar,
                              anchor="w",fg="white",bg="blue")
        kiLabel.grid(column=1, row=3,columnspan=1,sticky='EW')
        self.KiLabVar.set("Ki")
        kdLabel = Label(self,textvariable=self.KdLabVar,
                              anchor="w",fg="white",bg="blue")
        kdLabel.grid(column=1, row=4,columnspan=1,sticky='EW')
        self.KdLabVar.set("Kd") 
        kscaleLabel= Label(self,textvariable=self.KscaleLabVar,
                              anchor="w",fg="white",bg="blue")
        kscaleLabel.grid(column=1, row=5,columnspan=1,sticky='EW')
        self.KscaleLabVar.set("Kscale")                
    
    
    def OnKp(self, event):
        kp = int(self.KpVar.get())
        if (speed.SetKp(kp)):
            self.KpLabVar.set("Kp: " + str(kp))
        self.Kp.focus_set()
        self.Kp.selection_range(0, END) 
    
    def OnKi(self, event):
            ki = int(self.KiVar.get())
            if (speed.SetKi(ki)):
                self.KiLabVar.set("Ki: " + str(ki))
            self.Ki.focus_set()
            self.Ki.selection_range(0, END)   

    def OnKd(self, event):
            kd = int(self.KdVar.get())
            if (speed.SetKd(kd)):
                self.KdLabVar.set("Kd: " + str(kd))
            self.Kd.focus_set()
            self.Kd.selection_range(0, END) 
            
    def OnKscale(self, event):
            ks = int(self.KscaleVar.get())
            if (speed.SetKscale(ks)):
                self.KscaleLabVar.set("Ks: " + str(ks))
            self.Kscale.focus_set()
            self.Kscale.selection_range(0, END) 
        
        
        
#PID
#------------------------------------------------------------------------------
#Brake
class BrakeFrame(Frame):
    def __init__(self, parent):
        Frame.__init__(self, parent, bd=4, relief=RIDGE)   
        self.parent = parent
        self.initialize()

    def initialize(self):
        self.grid()
        self.entryVariable = StringVar()
        self.entry = Entry(self, textvariable=self.entryVariable)
        self.entry.grid(column=0,row=0,sticky='EW')
        self.entry.bind("<Return>", self.OnPressEnter)

        button = Button(self,text="Set Brake", command=self.OnButtonClick)
        button.grid(column=1,row=0)

        self.labelVariable = StringVar()
        label = Label(self,textvariable=self.labelVariable,
                              anchor="w",fg="white",bg="red")
        label.grid(column=0,row=1,columnspan=1,sticky='EW')
        self.labelVariable.set("Start..")

        onButton = Button(self,text="Brake On", command=self.BrakeOn)
        onButton.grid(column=0,row=3)

        offButton = Button(self,text="Brake Off", command=self.BrakeOff)
        offButton.grid(column=1,row=3)

        self.brakeVariable = StringVar()
        brakeState = Label(self,textvariable=self.brakeVariable,
                           anchor="w",fg="white",bg="red")
        brakeState.grid(column=0,row=2,columnspan=1,sticky='EW')
        self.brakeVariable.set("Start..")
         
        self.grid_columnconfigure(0,weight=1)
        self.update()
        #self.geometry(self.geometry()) # caused busy wait?
        self.entry.focus_set()
        #self.entry.selection_range(0, Tkinter.END)

    def OnButtonClick(self):
        brakePos = int(self.entryVariable.get())
        brake.SetPositionMM(brakePos)
        self.labelVariable.set("Brake set: " + str(brakePos) )
        self.entry.focus_set()
        self.entry.selection_range(0, END)
        
    def OnPressEnter(self,event):
        brakePos = int(self.entryVariable.get())
        brake.SetPositionMM(brakePos)
        self.labelVariable.set("Brake set: " + str(brakePos) )
        self.entry.focus_set()
        self.entry.selection_range(0, END)

    def BrakeOn(self):
        self.brakeVariable.set("Brake: ON")
        brake.SetFullOn()

    def BrakeOff(self):
        self.brakeVariable.set("Brake: OFF")
        brake.SetOff()

#Brake
#------------------------------------------------------------------------------
#Steering
class SteeringFrame(Frame):
    def __init__(self, parent):
        Frame.__init__(self, parent, bd=4, relief=RIDGE)   
        self.parent = parent
        self.initialize()
        
    def initialize(self):
        self.grid()
        self.entryVariable = StringVar()
        self.entry = Entry(self, textvariable=self.entryVariable)
        self.entry.grid(column=0,row=0,sticky='EW')
        self.entry.bind("<Return>", self.OnPressEnter)

        button = Button(self,text="Set Steering", command=self.OnButtonClick)
        button.grid(column=0,row=2)

        self.labelVariable = StringVar()
        label = Label(self,textvariable=self.labelVariable,
                              anchor="w",fg="white",bg="green")
        label.grid(column=0,row=1,columnspan=1,sticky='EW')
        self.labelVariable.set("Start..")

        left = Button(self,text="Left", command=self.Left)
        left.grid(column=1,row=1,columnspan=1,sticky='EW')

        right = Button(self,text="Right", command=self.Right)
        right.grid(column=1,row=2,columnspan=1,sticky='EW')

        centre = Button(self,text="Centre", command=self.Centre)
        centre.grid(column=1,row=0,columnspan=1,sticky='EW')

        self.steerVariable = StringVar()
        steerState = Label(self,textvariable=self.steerVariable,
                           anchor="w",fg="white",bg="green")
        steerState.grid(column=0,row=3,columnspan=1,sticky='EW')
        self.steerVariable.set("Start..")
        
        self.turn_angle = steering.GetTurnAngle();
        
        self.slider = Scale(self, from_=-self.turn_angle, to=self.turn_angle, 
                            orient=HORIZONTAL, command=self.setSteerState)
        self.slider.grid(column=0, row=4, columnspan=2, sticky='EW')        

        self.grid_columnconfigure(0,weight=1)
        self.update()
        #self.geometry(self.geometry()) # caused busy wait?
        self.entry.focus_set()
        #self.entry.selection_range(0, Tkinter.END) # caused busy wait?

    def OnButtonClick(self):
        steerAngle = int(self.entryVariable.get())
        steering.SetAngle(steerAngle)
        self.slider.set(steerAngle)
        self.labelVariable.set("Steering set: " + str(steerAngle) )
        self.entry.focus_set()
        self.entry.selection_range(0, END)
        
    def OnPressEnter(self,event):
        steerAngle = int(self.entryVariable.get())
        steering.SetAngle(steerAngle)
        self.slider.set(steerAngle)
        self.labelVariable.set("Steering set: " + str(steerAngle) )
        self.entry.focus_set()
        self.entry.selection_range(0, END)


    def Left(self):
        self.steerVariable.set("LEFT")
        steering.TurnLeft()
        self.slider.set(-self.turn_angle)

    def Right(self):
        self.steerVariable.set("RIGHT")
        steering.TurnRight()
        self.slider.set(self.turn_angle)

    def Centre(self):
        self.steerVariable.set("CENTRE")
        steering.TurnStraight()
        self.slider.set(0)
        
    def setSteerState(self, val):
        val = self.slider.get()
        steering.SetAngle(val)
        steerAngle = val;
        self.steerVariable.set("Steering: " + str(val) )           
        
        
#Steering  
#------------------------------------------------------------------------------
#main

def centerWindow(self):
    w = 280
    h = 450
    sw = self.winfo_screenwidth()
    sh = self.winfo_screenheight()
    x = (sw - w)/2
    y = (sh - h)/2
    self.geometry('%dx%d+%d+%d' % (w, h, x, y))
    


if __name__ == '__main__':
    #UsbController.Open()
    #UsbController.WaitUntilRunning()
    root = Tk()
    app = MainFrame(root)
    centerWindow(root)
    root.title('Go Kart Gui')
    root.resizable(False,False)
    
    root.mainloop()
    
    #UsbController.Close()
