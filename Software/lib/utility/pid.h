#ifndef PID_H
#define PID_H

/**
PID gain calculator.
Note that this uses static variables for the I and D components
so pid_calculate_gain can only be used once on a board.

The proportional control calculation is stateless and so can be used as many times as desired
*/

#define N_LOOK_UP_PTS 17

/*Do a single step of a PID controller.
This function can only be used to calculate PID gains for one peripheral*/
int pid_calculate_gain(int desired, int current, double P, double I, double D);

//2013 new PID
/*Do a single step of a PID controller.
This function can only be used to calculate PID gains for one peripheral*/
int pid_calculate_PID(int setpoint, int measured, int Kp, int Ki, int Kd, float dt) ;

/*
 * Look up the approximate SPI-duty corresponding to a target speed
 */
int pid_look_up_duty(int target);


/*do a single step of a proportional controller*/
int pid_calculate_P_gain(int desired, int current, int P);


#endif
