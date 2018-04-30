

//use the next 11 lines only once anywhere
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

//define ports for data transfer
#define	HX711_PORT REGISTER_BIT(PORTB,3)
#define HX_CLK_HIGH HX711_PORT = 1
#define HX_CLK_LOW HX711_PORT = 0

#define	HX711_DATA REGISTER_BIT(PINB,4)

uint8_t GAIN=0;
//GAIN is a global variable
//gain is 64 or 128, 25sck bits for 128 and 27 for 64
//gain change is in effect on next conversion
//if gain is changed do a dummy read right after
// to get it right on next conversion READ_HX711();
uint32_t READ_HX711(){

while(HX711_DATA);//wait until ready

uint32_t value2=0;
uint8_t negative=0;
uint8_t sck_amp_add=1;
//here the gain defines how many extra clk cycles are added to the end
//for the HX711
if( GAIN == 128){
	sck_amp_add=1;
}
else if( GAIN == 64){
	sck_amp_add=3;
}
else if( GAIN == 32){
	sck_amp_add=2;
}

//read first 23 bits
for(uint8_t  j=0;j<23;j++){
		HX_CLK_HIGH;
		_delay_us(1);
		value2|=HX711_DATA&0x01; //add current data bit
		value2=value2<<1; //shift to left by one bit 
		HX_CLK_LOW;
		_delay_us(1);
}
//last bit without left shifting
HX_CLK_HIGH;
_delay_us(1);
value2|=HX711_DATA&0x01; //add current data bit
HX_CLK_LOW;

//adds GAIN sck's here
for(uint8_t  i=0; i < sck_amp_add; i++){
	_delay_us(1);
	HX_CLK_HIGH;
	_delay_us(1);
	HX_CLK_LOW;
}

//if ( value2 > 0x80000000 || value2 < 0x7FFFFF) { //if out of bounds
 //  return 0x00000000; 
// }
if (value2&&0x80000000 ) { //if negative
    negative=1;
}

if(negative==1){
//data is in two's complement
value2=~value2; //flip bytes
value2++;//add one		
}
//checked with an oscilloscope, lots of interference
//get rid of some bytes, change/remove the next 3 if/elseif's if feeling lucky
if(GAIN ==128){
value2=value2>>8;
}
else if(GAIN ==64){
value2=value2>>7; 
}
else if(GAIN ==32){
value2=value2>>6; 
}
//
return value2; 
}

//tare is a value you can check on every startup
//or by measuring it once and saving it to EEPROM using eemem
//and reading it from there on startup

// You need to have it read to memory at some point since strain gauges
// have minor resistance variation. Load cells are not perfectly
// "balanced" (in wheatstone bridges) even if they were glued perfectly. 
uint32_t tare(uint8_t times){

uint32_t taring=0;
uint32_t check=0;
for(uint8_t j=0;j<times;){
	
	check=READ_HX711();
	if(check!=0x00000000){
		taring+=check;
		j++;
	}
}
taring=taring/times;
return taring;

}

//here you can get the average by inputting multisampling and tare values
uint32_t hx711average(uint8_t times, uint32_t tare){

uint32_t taring=0;
uint32_t check=0;
for(uint8_t j=0;j<times;){
	
	check=abs(tare-READ_HX711());
	if(check!=0x00000000){
		taring+=check;
		j++;
	}
}
taring=round(taring/times);
return taring;

}
