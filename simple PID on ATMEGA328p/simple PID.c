//PID controlled PWM output from OC1A = PB1


#define OVERSAMPLING 3 //ADC samples
#define I_memory 40 //number of samples kept for integral value

//PID value coefficients, change accordingly
#define P_val 10 
#define I_val 3 
#define D_val 6 


//add libraries in use here
/*
#include <stdlib.h>
#include <stdio.h>

#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
*/


//first order of business is to declare global values that are used in interrupts as volatiles,
volatile int32_t    mean_I_err[I_memory],
					PID=0,					
					err=0,
					err_old=0,
					mean_I_error=0.0,
					I_err=0.0,
					P_err=0,
					D_err=0;
					
					
volatile uint16_t	volt_value=0,
					duty_change_counter=0;

volatile int16_t   duty=0, //initial number
				   fb_voltage=0,	
				   duty_max=312; //39% for safety
				   
volatile double 	right_voltage=400.0, //desired output voltage for SBM-20 tubes
					HV_vol=0.0; 				   

//call in the beginning of main function
void system_setup() {
//add pin configurations and other stuff here	

// for reference only
//DDRB = 0b00000010; //pb1 = pwm(out), all other set as inputs
//PORTB = 0b00000000; //output 0 and input tri-state
//


//ADMUX adlar=0, mux 0111 for feedback using ADC7, which is a dedicated ADC input (pin22) on TQFP32
ADMUX=0b01000111;
//autotriggering disabled, starts conversion right away, since 1st ADC-value is usually bad
ADCSRA=0b11000110; //with 64 clk division from 8MHz is 125kHz, 
//datasheet states that one should have the sample rate between 50-200kHz to get maximum resolution

//16-bit PWM out from OC1A pin = PB1 (pin13) make sure to set it as output pin!
//fast PWM, non-inverting mode set, 16bit timer, 
//using ICR1 as top value and prescaler set to 1 => 8Mhz/(1*(799+1))= 10kHz

ICR1=799; //x+1 due counter starts from zero
OCR1A = 0; //initial duty cycle
TCCR1A = 0b10000010; //ICR1 selected as top value
TCCR1B = 0b00011001; //falling edge capture, clk/1 prescaling selected 
TIMSK1 = 0b00000010; //compare match A interrupts (make sure interrupts are activated)

}




//interrupt vector, this interrupt vector is a bit long and takes time to run through,
//so optimize if needed
ISR(TIMER1_COMPA_vect){	
	
	duty_change_counter++;
	if(duty_change_counter >= 100){ //10kHz/100=100Hz 
		duty_change_counter=0;	
		ADMUX=0b01000111;
		//_delay_us(1);
		volt_value=0;
		fb_voltage=0;
		while(volt_value < OVERSAMPLING){
			ADCSRA |= (1 << ADSC);	 //start an ADC conversion
			while(ADCSRA & _BV(ADSC));    //wait until conversion is complete
			fb_voltage+=ADC;
			volt_value++;
		}
		fb_voltage=round(fb_voltage/OVERSAMPLING); //divide to get the average
		HV_vol=(double)1.02*fb_voltage; //correct voltage feedback resistances here
		//and calculate the value for voltage compared to the ADC value
		
		//measured 5v actual vcc and resistor values, for 5v to 400v SMPS
		//((fb_voltage*5)*((9680000+46.4E3)/46.4E3))/1024=voltage
		//5*209.620689655=0.977 => 1.02
	
		//PID
		err_old=err;
		err=right_voltage-HV_vol;
		P_err=err; //Proportional is directly the difference times P_val (bang bang value)
		
		//save new value to last
		mean_I_err[I_memory-1]=(int32_t)err;
		mean_I_error=0;
		//move all samples left by one in array and sum them up
		for (int i=0;i< I_memory-1;i++){
			mean_I_err[i]=(int32_t)mean_I_err[i+1];
			mean_I_error+=(int32_t)mean_I_err[i];
		}
		//mean_I_error+=(int32_t)err; //take the latest value in account
		I_err=(int32_t)mean_I_error;
		D_err=(int32_t)err-err_old;
	
		PID=(int32_t)P_val*P_err + I_val*I_err + D_val*D_err;
	
		if( PID > duty_max ){		
			PID=duty_max;
		}
		else if(PID < 0){
			PID=0;
		}
		duty=(uint16_t)PID;
		OCR1A=duty; //OCR1A is 16-bit register and now houses the "right" PWM duty cycle
		}
}

//main function could be something like this
int main(void){
	
// somewhere at the beginning of the main function write all the values to zero (on startup)!	
for (int i=0;i< I_memory-1;i++){
	mean_I_err[i]=(int32_t)0.0;
}

system_setup();

//run both of these commands right after other, otherwise the startup PWM might be glitchy
cli();  // disable interrupts
sei(); //enable interrupt

	//main loop
	while(1){
		//if target voltage from PWM output is not static, you can change it here to your liking
		//right_voltage=400;
			
	}

	
}
	
