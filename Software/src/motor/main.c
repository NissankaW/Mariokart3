/* ----------------------------------------------------------------------------
 *          Mariokart project
 * ----------------------------------------------------------------------------
 *  Copyright (c) 2011, University of Canterbury

This board controls the acceleration of the kart.
currently it does this by interfacing with the student board via SPI
(the student board has to be reflashed to use the SPI code)
It allows two options of control:
open loop acceleration control
closed loop speed control
the acceleration control merely sends out any incoming USB SetAcceleration signals via SPI.
The closed loop speed control takes a target speed as input via USB SetSpeed() and uses proportional control
to set the kart speed to this value

The acceleration value is measured as a % of maximum acceleration between 0 and 256.
The maximum acceleration value that can be used is 220.
 */

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------
#include <components/char_display.h>
#include <components/debug.h>
#include <components/switches.h>
#include <board.h>
#include <pio/pio.h>
#include <pio/pio_it.h>
#include <protocol/protocol.h>
#include <spi/spi.h>
#include <tc/tc.h>
#include <utility/trace.h>
#include <utility/pid.h>
#include <utility/math.h>

//------------------------------------------------------------------------------
//         Local defines
//------------------------------------------------------------------------------
#define SOFTWARE_NAME "Motor"

#define MAX_ACCELERATION (255) // not correct - for some reason 0-255 is split into two scales (0-120) and (127-255) therefore the MAX_ALLOWABLE below is used

//corresponds to the maximum allowed 80% duty to the motor
#define MAX_ALLOWBLE_ACCELERATION (30) //adjusted for testing, reset to 100 for 80% max

//Sets the chip select for the student board SPI
#define STUDENT_CS 0x0

//The Student board SPI baud rate
//#define STUDENT_BAUD 128000
#define STUDENT_BAUD 8000


//PID time step
#define SPEED_DT (0.5)

//Frequency for requesting speed value from sensor board
#define SPEED_REQUEST_FREQ 9999

//tollerable error in speed (mm/s)
#define MAX_ALLOWABLE_SPEED_ERROR 50

//speed error greater than which the lookup table is consulted (mm/s)
#define MAX_LOOKUP_ERROR 180

#define STOP_RAMP_DEC 100

//Ramping states
typedef enum {RAMP_START, RAMP_STOP, RAMP_OFF} Ramp_t;

//------------------------------------------------------------------------------
//         Local variables
//------------------------------------------------------------------------------
static const Pin student_spi_pins[] = {PINS_SPI1,PIN_SPI1_NPCS0,PIN_SPI1_NPCS1,PIN_SPI1_NPCS2};
AT91PS_SPI student_spi = AT91C_BASE_SPI1;

/*PID Gains for speed control*/
static int Kp = 10;
static int Ki = 0;
static int Kd = 0;
static int Kscale = 1000;


//type of control to use
typedef enum {
    SPEED_CONTROL,
    ACCELERATION_CONTROL,
} control_type_t;

static int kart_speed = 0; //current kart speed in 1000*m/s
static int target_speed = 0;
static unsigned short target_acceleration = 0;
static control_type_t control_type = SPEED_CONTROL; //type of control to use - acceleration or speed control



//------------------------------------------------------------------------------
//         Local functions
//------------------------------------------------------------------------------

//send a request to the sensor board for its speed value
void request_speed_value() {
    static const message_t msg = {
        .from = ADDR_MOTOR,
        .to   = ADDR_SENSOR,
        .command = CMD_GET,
        .data = { VAR_SPEED, 0, 0, 0, 0 }
    };
    proto_write(msg);
}

void set_motor(unsigned short setpoint) {
    // Make sure the value is valid else error
    if (setpoint > MAX_ALLOWBLE_ACCELERATION) {
        TRACE_WARNING("The accelerator value (%d) sent to the student board is invalid!\n\r", setpoint);
        SPI_Write(student_spi, STUDENT_CS, 0x0000);
        SPI_Write_better(student_spi, STUDENT_CS, 0);
        proto_state_error();
        char_display_number(11);
    }
    else {
        TRACE_DEBUG("Setting the motor board accelerator value to: %d\n\r", setpoint);
        SPI_Write_better(student_spi, STUDENT_CS, setpoint);
        //SPI_Write(student_spi, STUDENT_CS, setpoint);
    }
}


void init_student_spi() {
    TRACE_DEBUG("Configuring SPI for student board\n\r");
    PIO_Configure(student_spi_pins, PIO_LISTSIZE(student_spi_pins));
 
    AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_SPI1;
    student_spi->SPI_CR = AT91C_SPI_SPIDIS;
    // Execute a software reset of the SPI twice
    student_spi->SPI_CR = AT91C_SPI_SWRST;
    student_spi->SPI_CR = AT91C_SPI_SWRST;
    student_spi->SPI_MR = AT91C_SPI_MSTR | AT91C_SPI_PS_FIXED;

    student_spi->SPI_CSR[STUDENT_CS] = AT91C_SPI_NCPHA | AT91C_SPI_BITS_8 | (0x0000FF00);

    /*SPI_Enable(student_spi);*/
    student_spi->SPI_CR = AT91C_SPI_SPIEN;
}


void timer_callback(void) {
    char_display_tick();
}


//acknowledge a set request
void send_ack(message_t orig_msg) {
    message_t msg = {
        .from     = ADDR_MOTOR,
        .to       = orig_msg.from,
        .command  = CMD_ACK_SET,
        .data[0]  = orig_msg.data[0],
    };
    proto_write(msg);
}


//send a keep alive reply back to the comms board so that it knows this board is still connected
void send_keep_alive() {
    message_t msg = {
        .from     = ADDR_MOTOR,
        .to       = ADDR_COMMS,
        .command  = CMD_KEEP_ALIVE,
    };
    proto_write(msg);
}


int initiateStop(void)
{
	if (kart_speed > STOP_RAMP_DEC) {
		return kart_speed - STOP_RAMP_DEC;
	}
	else {
		return 0;
	}
}
	

/*
uses PID control to accelerate the go-kart towards the target speed
*/
void do_speed_control_step(void) {
	static int roughDuty = 0;
	static Ramp_t state = RAMP_STOP;
	int desired_speed = 0;
	int err = 0;
	int spiSend = 0;
	int pid_out = 0;
	
	
	/*set desired speed based on ramping state*/
	switch (state){
		case (RAMP_OFF):	//normal running
			desired_speed = target_speed;
			if (target_speed == 0) {
				desired_speed = initiateStop();
				//state = RAMP_STOP;
			}
			break;
		case (RAMP_START):	//ramping on start-up
			if (target_speed == 0) {
				desired_speed = initiateStop();
				//state = RAMP_STOP;
			}
			else if (kart_speed < 300) { 
				desired_speed = 300;
			}
			else {
				desired_speed = target_speed;
				state = RAMP_OFF;
			}
			break;
		case (RAMP_STOP):	//ramp down target speed to stop smoothly
			if (kart_speed > STOP_RAMP_DEC) {
				desired_speed = kart_speed - STOP_RAMP_DEC;
				desired_speed = 0;
			}
			else {
				desired_speed = 0;
				if((kart_speed == 0) && (target_speed > 0)) {
					state = RAMP_START;
				}
			}
			break;
	}
	//desired_speed = target_speed;
	TRACE_WARNING("D %d", desired_speed);
	err = absv(desired_speed - kart_speed);
	TRACE_WARNING("E %d", err);
	/*Consult look-up table if error is very large*/
	if (err > MAX_LOOKUP_ERROR){
		roughDuty = pid_look_up_duty(desired_speed);
	}
	spiSend = roughDuty;
	TRACE_WARNING("SPI %d\n", spiSend);
	
	/*Add in control if speed not acceptable*/
	/*if (err > MAX_ALLOWABLE_SPEED_ERROR) {
		pid_out = pid_calculate_P_gain(desired_speed, kart_speed, Kp);
		spiSend += pid_out/Kscale;
	}*/
    
	//static int Kpold = 0;
	//if (Kp != Kpold) {
		//TRACE_WARNING("pid= %d", pid_out);
		//TRACE_WARNING("accel= %d\n", acceleration);
		//Kpold = Kp;
	//}
  
    if (spiSend < 0) {
        spiSend = 0;
    }
    else if (spiSend > MAX_ALLOWBLE_ACCELERATION) {
        spiSend = MAX_ALLOWBLE_ACCELERATION;
    }

    set_motor(spiSend);
}


//------------------------------------------------------------------------------
//         Main Function
//------------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    debug_init(SOFTWARE_NAME);

    //enables interrupts (note resets all configured interrupts)
    PIO_InitializeInterrupts(AT91C_AIC_PRIOR_LOWEST);

    //Main initialisations
    char_display_init();
    switches_init();
    init_student_spi();

    //Starts a timer running at 100Hz to maintain the display
    TC_PeriodicCallback(100, timer_callback);

    message_t msg;

    proto_init(ADDR_MOTOR);
    int i;

    while (1) {

        switch (proto_state()) {
            case STARTUP:
                break;
            case CALIBRATING:
                proto_calibration_complete();
                break;
            case RUNNING:

                //periodically get the speed of the kart from the sensor board
                if ((i % SPEED_REQUEST_FREQ) == 0) {
                    request_speed_value();
                }
                i++;
                i %= (SPEED_REQUEST_FREQ +1);	//reset at 10000

                msg = proto_read();
                switch(msg.command) {
                    case CMD_SET:
                        switch (msg.data[0]) {
                            case VAR_SPEED:
                                control_type = SPEED_CONTROL;
                                target_speed = data_to_int(msg.data);
                                TRACE_WARNING("TS %d\n", target_speed);
                                target_acceleration = 0;
                                break;
                                
                            case VAR_ACCELERATION:
                                control_type = ACCELERATION_CONTROL;
                                target_acceleration = data_to_int(msg.data);
                                if (target_acceleration < 0) {
                                    target_acceleration = 0;
                                }
                                if (target_acceleration > MAX_ALLOWBLE_ACCELERATION) {
                                    target_acceleration = MAX_ALLOWBLE_ACCELERATION;
                                }
                                set_motor(target_acceleration);
                                target_speed = 0;
                                break;
                                
							case VAR_KP:
								Kp = data_to_int(msg.data);
								TRACE_WARNING("Kp:%d", Kp);
								break;
								
							case VAR_KI:
								Ki = data_to_int(msg.data);
								TRACE_WARNING("Ki:%d", Ki);
								break;
								
							case VAR_KD:
								Kd = data_to_int(msg.data);
								TRACE_WARNING("Kd:%d", Kd);
								break;
								
							case VAR_KSCALE:
								Kscale = data_to_int(msg.data);
								TRACE_WARNING("Ks:%d", Kscale);
								break;
                             
                            default:
                                proto_state_error();
                                char_display_number(12);
                                break;
                        }
                        send_ack(msg);
                        proto_refresh();
                        break;

                    /**
                    checks if the sensor board has sent a speed response, and if so saves it
                    */
                    case CMD_REPLY:
                        switch (msg.data[0]) {
                            case VAR_SPEED:
                                kart_speed = data_to_int(msg.data);
                                break;
                            default:
                                proto_state_error();
                                char_display_number(13);
                                break;
                        }

                    case CMD_KEEP_ALIVE:
                        send_keep_alive();
                        proto_refresh();
                    case CMD_NONE:
                        break;
                    default:
                        TRACE_WARNING("Invalid command %i received in running state", msg.command);
                        proto_state_error();
                        char_display_number(14);
                        break; 
                }

                if (control_type == SPEED_CONTROL) {
                    do_speed_control_step();
                }

                break;
            default: // ERROR

                    SPI_Write_better(student_spi, STUDENT_CS, 0);

                    //set_motor(0);

                break;
        }
    }
    return 0;
}

