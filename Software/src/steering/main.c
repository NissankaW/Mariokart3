/* ----------------------------------------------------------------------------
 *          Mariokart project
 * ----------------------------------------------------------------------------
 *  Copyright (c) 2011, University of Canterbury
 */

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------
#include <actuator_driver.h>
#include <components/char_display.h>
#include <components/debug.h>
#include <components/switches.h>
#include <tc/tc.h>
#include <encoder.h>
#include <protocol/protocol.h>
#include <pio/pio.h>
#include <pio/pio_it.h>
#include <utility/pid.h>

//------------------------------------------------------------------------------
//         Local defines
//------------------------------------------------------------------------------
#define SOFTWARE_NAME "Steering"

//angle in degrees that the wheel may come within the end of its travel
//due to some round processes in converting from pulses to degrees this 
//must not be set to 0
#define SAFETY_MARGIN (10)
//proportional gain for driving the steering motor
#define STEERING_PROPORTIONAL_GAIN (2)

//Apparently not meant to be driven at 100%
#define MAX_STEERING_DUTY_PERCENT (80)

#define CALIBRATE_DUTY (50)


//------------------------------------------------------------------------------
//         Local variables
//------------------------------------------------------------------------------

int center_angle;

//minimum angle wheel can be turned to in degrees
int steering_min_angle;

//maximum angle wheel can be turned to in degrees
int steering_max_angle;

//location of wheel measured in pulses from encoder
int steering_loc_in_pulses = 0;

//sets pins of limit switches
const Pin pin_lim_left = LIM_SW_DOWN;	//changed from pin_lim_up to pin_lim_left and swapped macros - were wrong
const Pin pin_lim_right = LIM_SW_UP;	//changed from pin_lim_down to pin_lim_right and swapped macros - were wrong 

typedef enum {
    UNCALIBRATED,
    FINDING_ANTICLOCK,
    FINDING_CLOCK,
    CENTERING,
    SETTING_SWITCHES,
    CALIBRATED
} cal_state;

//------------------------------------------------------------------------------
//         Interrupt Handlers
//------------------------------------------------------------------------------
//triggers error on limit switches (ISR must not be set till after calibration)
//THIS IS CURRENTLY IGNORED AFTER CALIBRATION!!
void LIMIT_ISR(const Pin *pin) {
    if (!PIO_Get(&pin_lim_left) || !PIO_Get(&pin_lim_right)) {
        //something wrong with steering so stops motor
        act_driver_drive(0);

        TRACE_WARNING("LIMIT SWITCH ACTIVATED ENTERING ERROR STATE\r\n");
        //proto_state_error();
        //char_display_number(11);
        //CURRRENTLY IGNORED AFTER CALIBRATION - false triggering - needs to be fixed
        // theoretically the steering should never hit the limits anyway after calibration
        // so it works ok - should be fixed as a safety fallback
    }
}

void timer_callback() {
    char_display_tick();
}

//------------------------------------------------------------------------------
//         Local Functions
//------------------------------------------------------------------------------



/**
 * P controller with STEERING_PROPORTIONAL_GAIN
 * @param desired    The location the controller is driving to
 * @param current    The location the actuator is at
 * @return           The pid output value
 */
int steering_P_control(int desired, int current) {
    int drive_percent = pid_calculate_P_gain(desired, current, STEERING_PROPORTIONAL_GAIN);

    //lets limit it even more for now!
    //can't drive at full duty cycle
    if (drive_percent < -MAX_STEERING_DUTY_PERCENT) {
        drive_percent = -MAX_STEERING_DUTY_PERCENT;
    }
    if (drive_percent > MAX_STEERING_DUTY_PERCENT) {
        drive_percent = MAX_STEERING_DUTY_PERCENT;
    }

    return drive_percent;
}

//convert an encoder value to degrees
int encoder_value_to_degrees(int encoder_value) {
    return (encoder_value * 360.0) / ENCODER_PULSES_PER_REV;
}

//convert degrees to an encoder value
int degrees_to_encoder_value(int degrees) {
    return (degrees * ENCODER_PULSES_PER_REV) / 360;
} 

//sets steering to an angle (in degrees) between min_angle and max_angle
void set_steering(int angle) {

    angle += center_angle;

    if (angle > steering_max_angle) {
        angle = steering_max_angle;
    } 
    else if (angle < steering_min_angle) {
        angle = steering_min_angle;
    }

    //convert angle to pulses and account for any offset
    steering_loc_in_pulses = degrees_to_encoder_value(angle);
}

//gets the steering wheels current position in degrees
int get_steering_pos(void) {
    return encoder_value_to_degrees(encoder_position_output) - center_angle;
}

//gets where the steering wheel is moving to in degrees
int get_steering_desired_pos(void) {
    return encoder_value_to_degrees(steering_loc_in_pulses) - center_angle;
}

//sets up the limit switch interrupts
void steering_limit_sw_init(void) {

    // Initialize interrupts
    PIO_ConfigureIt(&pin_lim_left, LIMIT_ISR);
    PIO_ConfigureIt(&pin_lim_right, LIMIT_ISR);
    
    PIO_EnableIt(&pin_lim_left);
    PIO_EnableIt(&pin_lim_right);

    TRACE_INFO("Limit switch ISR activated\n\r");

}

//calibrates steering position by finding location of limit switches and
//assuming straight ahead is halfway between them
void cal_steering(cal_state* cal) {

    switch (*cal) {
        case UNCALIBRATED:

            TRACE_INFO("Steering calibration started\n\r");
            *cal = FINDING_ANTICLOCK;
            break;

        case FINDING_ANTICLOCK:
            //drive steering anticlockwise until limit hit

            act_driver_drive(-CALIBRATE_DUTY);


            if (PIO_Get(&pin_lim_right)) {
                //stop motor
                act_driver_drive(0);

                //record value in degrees
                steering_min_angle = encoder_value_to_degrees(encoder_position_output);
                TRACE_INFO("Anticlockwise limit found at %i degrees\n\r", steering_min_angle);
                *cal = FINDING_CLOCK;
            }
            break;

        case FINDING_CLOCK:
            //drive steering clockwise until limit hit
            act_driver_drive(CALIBRATE_DUTY);
            if (PIO_Get(&pin_lim_left)) {
                //record value in degrees
                steering_max_angle = encoder_value_to_degrees(encoder_position_output);
                TRACE_INFO("Clockwise limit found at %i degrees\n\r", steering_max_angle);

                //check motor direction and enter error state if required
                if (steering_max_angle < steering_min_angle) {
                    TRACE_INFO("ERROR MOTOR SPINNING BACKWARDS\n\r");

                    //stop the motor
                    act_driver_drive(0);
                    //enter error state
                    proto_state_error();
                    char_display_number(12);
                }
                    

                center_angle = ((steering_max_angle - steering_min_angle) / 2) + steering_min_angle;
                steering_min_angle += SAFETY_MARGIN;
                steering_max_angle -= SAFETY_MARGIN;
                TRACE_INFO("Center offset found to be %i degrees\n\r", center_angle);

                
                //set starting position to center position
                set_steering(0);
                act_driver_drive(-CALIBRATE_DUTY); //dont return until motor driven back to center

                *cal = CENTERING;
            }
            break;

        case CENTERING:
            if (get_steering_pos() <= get_steering_desired_pos()) {
                //stop motor
                act_driver_drive(0);
                TRACE_INFO("Steering centered calibration finished\n\r");
                *cal = SETTING_SWITCHES;
                set_steering(0);
            }
            break;

        case SETTING_SWITCHES:
            TRACE_INFO("Activating limit switch ISR \n\r");
            steering_limit_sw_init();
            *cal = CALIBRATED;
            break;
        default:
            proto_state_error();
            char_display_number(13);
            break;
    }

}

void send_ack(message_t orig_msg) {
    message_t msg = {
        .from     = ADDR_STEERING,
        .to       = orig_msg.from,
        .command  = CMD_ACK_SET,
        .data[0]  = orig_msg.data[0],
    };
    proto_write(msg);
}

void send_current_steering_angle(address_t send_to) {
    message_t msg = {
        .from     = ADDR_STEERING,
        .to       = send_to,
        .command  = CMD_REPLY,
        .data  = { VAR_STEERING_ANGLE, 0, 0, 0, 0}

    };
    int_to_data(get_steering_pos(), msg.data);	
    proto_write(msg);
}


//#define DISABLE_CALIBRATION

#ifdef DISABLE_CALIBRATION
cal_state cal = CALIBRATED; 
#else		
cal_state cal = UNCALIBRATED; 
#endif	
message_t msg;
void do_protocol_step(void) {
    int speed;



    switch (proto_state()) {
        case STARTUP:
            break;
        case CALIBRATING:

            if (cal == CALIBRATED) {
                proto_calibration_complete();
            } else {
                cal_steering(&cal);
            }
            break;
        case RUNNING:

            msg = proto_read();
            switch (msg.command) {
                case CMD_GET:
                    if (msg.data[0] == VAR_STEERING_ANGLE ) {
                        send_current_steering_angle(msg.from);			
                    }
                    proto_refresh();
                    break;
                case CMD_SET:
					send_ack(msg);
                    //char_display_number(data_to_int(msg.data));
                    set_steering(data_to_int(msg.data));								
                    proto_refresh();
                    break;
                case CMD_KEEP_ALIVE:
                    proto_refresh();
                case CMD_NONE:
                    break;
                default:
                    TRACE_WARNING("Invalid command %i received in running state", msg.command);
                    proto_state_error();
                    char_display_number(14);
                    break;
            }
            speed = steering_P_control(steering_loc_in_pulses, encoder_position_output);			
            act_driver_drive(speed);																

            break;
        default: // ERROR
            act_driver_drive(0);									
            break;
    }
}

//------------------------------------------------------------------------------
//         Main Function
//------------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    debug_init(SOFTWARE_NAME);

    TRACE_INFO("Start steering board initialization\n\r");
    //enables interrupts (note resets all configured interrupts)
    PIO_InitializeInterrupts(AT91C_AIC_PRIOR_LOWEST);

    //Main initialisations
    char_display_init();
    //switches_init();
    encoder_init();

    
    act_use_alternate_pwm_channel();
    act_driver_init();


    // Starts a 100Hz display update timer
    TC_PeriodicCallback(100, timer_callback);

    //sets pins as inputs
    PIO_Configure(&pin_lim_left, 1);
    PIO_Configure(&pin_lim_right, 1);
    PIO_Clear(&pin_lim_left);
    PIO_Clear(&pin_lim_right);

    proto_init(ADDR_STEERING);

    TRACE_INFO("Steering board initialization completed\n\r");

    while (1) {
        

        do_protocol_step();

    }
    return 0;
}

