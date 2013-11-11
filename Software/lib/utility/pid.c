#include "pid.h"

#include <utility/trace.h>

/*Do a single step of a PID controller.
This function can only be used to calculate PID gains for one peripheral*/
int pid_calculate_gain(int desired, int current, double P, double I, double D) {
    static int err = 0;
    static int err_old = 0;
    static int I_err = 0;

    int P_err;
    int D_err;

    err_old = err;
    err = desired - current;

    P_err = err;
    I_err += err;

    D_err = err - err_old;

    int drive_percent = P * P_err + I * I_err + D*D_err;


    return drive_percent;
}


/*
 * Lookup table for kart duty
 */
int LOOK_UP_DUTY[N_LOOK_UP_PTS][2] = { {0,0}, {175,10}, {295,12}, {465,15},
{730,20}, {940,24}, {1058,26}, {1140,28}, {1230,30}, {1385,33}, {1445,35},
{1585,38}, {1720,40}, {1810,42}, {2000,46}, {2125,48}, {2205,50} };


/*
 * Look up the approximate SPI-duty corresponding to a target speed
 */
int pid_look_up_duty(int target){
	int i=0;
	while ((target > LOOK_UP_DUTY[i][0]) && (i<N_LOOK_UP_PTS)){
		i++;
	}
	return LOOK_UP_DUTY[i][1];
}
	

//2013 new PID
/*Do a single step of a PID controller.
This function can only be used to calculate PID gains for one peripheral*/
int pid_calculate_PID(int setpoint, int measured, int Kp, int Ki, int Kd, float dt) 
{
	static float prev_error = 0;
	static float integral = 0;
	
	float error;
	float derivative;
	float output;
	
	error = setpoint - measured;
	integral = integral + error*dt;
	derivative = (error - prev_error)/dt;
	prev_error = error;

	//TRACE_WARNING("E= %d I= %d D= %d", (int)error, (int)integral, (int)derivative);
	//TRACE_WARNING("prev= %d\n", (int)prev_error);
	
	output = (int)(Kp*error + Ki*integral + Kd*derivative);

	return output;
}

/*do a single step of a proportional controller*/
int pid_calculate_P_gain(int desired, int current, int P) {
    int error = desired - current;
    return P*error;
}
