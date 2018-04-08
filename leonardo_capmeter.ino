//leonardo_capmeter.ino
//Arduino Capacitance Meter, implemented on Leonardo
//measuring small capacitance, less than 1uf, using charge transfers

//connection
//
//----- CAP_PIN on Arduino ----------
//                                  |
//                                  |
//                                  |
//                                  |
//                                  = DUT
//                                  |
//                                  |
//                                  |
//                                  |
//                                 GND
//
//

//The meter may need caliberation -> measuring CAP_BASE
//1. connection nothing to CAP_PIN
//2. make CAP_BASE 0 in the code. Upload the revised code on the chip
//3. it will report a total capacitance reading
//4. that capacitance should be slightly higher than CAP_SH, typically 4
//5. update the value of CAP_BASE with that difference obtained in step 3 above
//6. recompile the code
//
//hardware configuration
#define CAP_PIN			A0			//pin connected to external capacitance
#define CAP_DLY			100			//delay time, in ms

#define USE_UART					//use uart for output
//end hardware configuration

//global defines
#define CAP_SH			14			//internal ADC sample and hold capacitance, per ATmel
#define CAP_BASE		4			//base pin capacitance

#define ADC_VBG			0b011110	//ADC mux for 1.1v Vbg/charging CAP_SH
#define ADC_GND			0b011111	//ADC mux for GND/discharging CAP_SH
#define ADC_TEMP		0b100111	//ADC mux for temperature sensor

//global variables

//read adc_ch
uint16_t adc_read(uint8_t adc_ch) {
	uint16_t tmp;
	//change the mux
	ADMUX = (ADMUX &~0x1f) | (adc_ch & 0x1f);	//set mux4..0
	ADCSRB= (ADCSRB&~(1<<5)) | (adc_ch & (1<<5));	//set mux5
	ADCSRA|= (1<<ADSC);			//start the conversion
	while (ADCSRA & (1<<ADSC)) continue;	//wait for ADSC bit to clear
	//read the adc: reading ADCL first, then ADCH
	tmp = ADCL;
	tmp|= (ADCH << 8);
	return tmp;
}

//initialize the cap meter
void cap_init(void) {
	digitalWrite(CAP_PIN, LOW);		//output low on CAP_PIN
	pinMode(CAP_PIN, INPUT);		//floating input for CAP_PIN / DUT
	
	//initialize the adc
	analogReference(INTERNAL);		//use internal reference
	analogRead(CAP_PIN);			//dummy read on the external capacitor (DUT)
}

//measure the capacitance
//CAP_PIN assumed to be output low at start of this routine
//returns total capacitance, including CAP_SH, CAP_BASE and CAP_DUT
double cap_read(void) {
	uint32_t tmp0, tmp1;
	
	//float DUT pin
	pinMode(CAP_PIN, INPUT);
	
	//discharge sample-and-hold capacitor by reading ADC_GND
	adc_read(ADC_GND); delay(1);
	//charge up sample-and-hold capacitor by reading ADC_VBG
	adc_read(ADC_VBG); delay(1); 	//dummy read, to allow time to charge up
	//read voltage across sample-and-hold capacitor
	tmp0 = adc_read(ADC_VBG);
	
	tmp1 = analogRead(CAP_PIN);		//read CAP_PIN - charge transfer starts now
	//turn CAP_PIN output, low - discharge CAP_PIN
	digitalWrite(CAP_PIN, LOW); pinMode(CAP_PIN, OUTPUT);
	
	//now calculate the capacitance
	return sqrt((double) tmp0 / tmp1) * CAP_SH;	//no error handling here
}


void setup() {
	cap_init();						//reset the cap meter
	
	//if we want to output it via the UART
#if defined(USE_UART)
	Serial1.begin(9600);
#endif
}

void loop() {
	double tmp;
	
	tmp = cap_read() - CAP_SH - CAP_BASE;				//read the external capacitor
	if (tmp < 0) tmp = 0;			//no negative capacitance
	
	//uart output
#if defined(USE_UART)
	Serial1.print("Capacitance = "); Serial1.print(tmp); Serial1.print("pf.\n\r");
#endif

	delay(CAP_DLY);					//waste some time, to allow the capacitors to discharge
}
