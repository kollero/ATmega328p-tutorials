typedef struct{
	unsigned int bit0:1;
	unsigned int bit1:1;
	unsigned int bit2:1;
	unsigned int bit3:1;
	unsigned int bit4:1;
	unsigned int bit5:1;
	unsigned int bit6:1;
	unsigned int bit7:1;
} _io_reg;
#define REGISTER_BIT(rg,bt) ((volatile _io_reg*)&rg)->bit##bt

//on atmega328p, only pd2 and pd3 have their own dedicated external interrupt vectors.
//You can use them also on other pins using masks but beware if you set both of the
//interrupts on same PCIEx area they will conflict, PCIE2=PCINT[23:16],PCIE1=PCINT[14:8], PCIE0=PCINT[7:0]
//if they conflict ENCA and ENCB will be set on same interrupt vector and you'll have to poll them 
//to figure out which one has changed, and even then it might not work as intended,
// due to the time it takes to run other interrupts
#define	ENCA REGISTER_BIT(PIND,2) 
#define	ENCB REGISTER_BIT(PIND,3)
//use wherever, but still on diff area than ENCA or ENCB to keep things simple
#define	ENCCLK REGISTER_BIT(PIND,1)

//encoder look up table
const int8_t enc_LUT[17] ={0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};

//these are used in an interrupt, but nowhere else that's why they're as static
static uint8_t	old_AB=0,
				old_click=0;
				
volatile uint16_t		TARGET_TEMPERATURE=0;
volatile uint8_t	clicked=0; //make sure this one is volatile

//TARGET_TEMPERATURE is changed with encoder using external interrupts 
ISR(INT0_vect) { //encoder A
old_AB <<= 2;                //remember previous state = left shift by 2
old_AB |= ( ENCA | ENCB<<1 );  //add current states
TARGET_TEMPERATURE+=enc_LUT[( old_AB & 0x0f )];
//for safety
if(TARGET_TEMPERATURE > 1000 || TARGET_TEMPERATURE <0 ){ //if negative
	TARGET_TEMPERATURE=0;
}

}
ISR(INT1_vect) { //encoder B
old_AB <<= 2;                //remember previous state
old_AB |= ( ENCA | ENCB<<1 );  //add current states
TARGET_TEMPERATURE+=enc_LUT[( old_AB & 0x0f )];
//safety
if(TARGET_TEMPERATURE > 1000 || TARGET_TEMPERATURE <0){ //if negative
	TARGET_TEMPERATURE=0;
}	

}
//clicker interrupt
ISR(PCINT2_vect){
old_click <<= 1; 
old_click |=  ENCCLK;  //add current state
	if(old_click&0x02){ //if pulled down
		 //clicked
		 clicked=1; //
		 
	}	          
}

//setting the registers for interrupts
void setup()
{	
	//pd1=encoders button (input), pd2= encoders B (input), pd3= encoders A (input)
	DDRD=0b00000000;
	PORTD=0b00000000; 
		
	//external interrupts
	EICRA=0b00000101; //any logical change generates interrupts
	EIMSK=0b00000011; //mask register
	//EIFR=0b00000000; //set interrupt flags to zero
	
	///PCINT17, pd1 or encoder button interrupt mask, PCIE2 area
	PCICR=0b00000100; //level change causes interrupt pins PCINT24-16
	PCMSK2=0b00000010; //PCINT17 masked
	
}

//also remember to run in main loop
//cli(); //disable interrupts 
//sei(); //enable interrupts
