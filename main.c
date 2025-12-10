#include <msp430.h>
#include "peripherals.h"

#define CALADC12_15V_30C *((unsigned int *) 0x1A1A)
#define CALADC12_15V_85C *((unsigned int *) 0x1A1C)

// ----------------- Global Variables ----------------- 
volatile unsigned long global_time_seconds = 29089811;
volatile unsigned long tempReadings = 0;
volatile unsigned char new_second_event = 0;
volatile float degC_per_bit;
const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
float tempC[36];
enum DISPLAY_STATE {RUN = 0, EDIT_DAY = 1, EDIT_MONTH = 2, EDIT_HOUR = 3, EDIT_MINUTE = 4, EDIT_SEC = 5};
unsigned int currentMonth, currentDay, hr, min, sec;
int initial_scroll_value = 0;
int MAX_ADC_VALUE = 4095;

// ------------------ Function Prototypes ------------------
void initButtons(void);
void configure_timer_a2(void);
void config_temp_sensor(void);
void config_scroll_wheel(void);

// functions to poll s1 and s2
unsigned char s1Clicked(void);
unsigned char s2Clicked(void);

// scroll wheel reading/handling
unsigned int getScrollWheelReading(void);
int handle_scroll_value(int adc_value, int divisions);

// displaying various data on LCD
void displayTime(unsigned long int inTime);
void displayTemp(float inAvgTempC);
void displayEditScreen(void);

// polls mcu's temperature sensor and updates tempC
void read_temp(void);
// iterates through tempC array and averages values
float get_temp_avg(void);

// helper functions
void floatTempToArray(float temp, char unit, char* tempStr);
void breakDownTime(unsigned long int inTime);
unsigned long int reconstructSeconds(void);
uint8_t clamp(uint8_t val, uint8_t min, uint8_t max);


#pragma vector=TIMER2_A0_VECTOR
__interrupt void Timer_A2_ISR(void) {
	global_time_seconds++;
	//elapsedSeconds++;
	new_second_event = 1;
}

void main(void)
{
	WDTCTL = WDTPW | WDTHOLD;    // Stop watchdog timer

	initLeds();
	initButtons();
	configDisplay();
	configure_timer_a2();
	config_temp_sensor();
	config_scroll_wheel();

	__enable_interrupt();

	Graphics_clearDisplay(&g_sContext);

	unsigned char previous_s1 = 0;
    unsigned char previous_s2 = 0;

    unsigned char startEdit = 0;
    unsigned char exitEdit = 0;


	enum DISPLAY_STATE state = RUN;
	while (1)
	{
		unsigned char s1_clicked = s1Clicked();
		unsigned char s2_clicked = s2Clicked();

		if (s1_clicked == 1 && previous_s1 == 0) {
		    startEdit = 1;
		} else {
		    startEdit = 0;
		}

		if (s2_clicked == 1 && previous_s2 == 0) {
		    exitEdit = 1;
		} else {
		    exitEdit = 0;
		}

		previous_s1 = s1_clicked;
		previous_s2 = s2_clicked;
		unsigned int raw_val = getScrollWheelReading();
		int newVal = -1;

		switch(state) {
			case RUN:
				if (new_second_event) {
					new_second_event = 0;
					displayTime(global_time_seconds);

					// reads new temp, calculates new avg, updates temp display
					read_temp();
					displayTemp(get_temp_avg());
				}

				if (startEdit) {
					breakDownTime(global_time_seconds);
					state = EDIT_MONTH;
					Graphics_clearDisplay(&g_sContext);
					initial_scroll_value = raw_val;
				}
				break;
			case EDIT_MONTH:
				//Need scroll wheel logic here
				displayEditScreen();

				newVal = handle_scroll_value(raw_val, 12);

				if (newVal != -1)
					currentMonth = clamp(newVal, 0, 11);

				if (startEdit) {
					state = EDIT_DAY;
					initial_scroll_value = raw_val;
				}
				if (exitEdit) {
					global_time_seconds = reconstructSeconds();
					state = RUN;
					Graphics_clearDisplay(&g_sContext);
				}
				break;
			case EDIT_DAY:
				//Need scroll wheel logic here
				displayEditScreen();
				
				newVal = handle_scroll_value(raw_val, daysInMonth[currentMonth]);

				if (newVal != -1) 
					currentDay = clamp(newVal, 1, daysInMonth[currentMonth]);

				if (startEdit) {
					state = EDIT_HOUR;
					initial_scroll_value = raw_val;
				}
				if (exitEdit) {
					global_time_seconds = reconstructSeconds();
					state = RUN;
					Graphics_clearDisplay(&g_sContext);
				}
				break;
			case EDIT_HOUR:
				//Need scroll wheel logic here
				displayEditScreen();

				newVal = handle_scroll_value(raw_val, 24);

				if (newVal != -1) 
					hr = clamp(newVal, 0, 23);

				if (startEdit) {
					state = EDIT_MINUTE;
					initial_scroll_value = raw_val;
				}
				if (exitEdit) {
					global_time_seconds = reconstructSeconds();
					state = RUN;
					Graphics_clearDisplay(&g_sContext);
				}
				break;
			case EDIT_MINUTE:
				//Need scroll wheel logic here
				displayEditScreen();

				newVal = handle_scroll_value(raw_val, 60);

				if (newVal != -1) 
					min = clamp(newVal, 0, 59);

				if (startEdit) {
					state = EDIT_SEC;
					initial_scroll_value = raw_val;
				}
				if (exitEdit) {
					global_time_seconds = reconstructSeconds();
					state = RUN;
					Graphics_clearDisplay(&g_sContext);
				}
				break;
			case EDIT_SEC:
				//Need scroll wheel logic here
				displayEditScreen();

				newVal = handle_scroll_value(raw_val, 60);

				if (newVal != -1) 
					sec = clamp(newVal, 0, 59);

				if (startEdit) {
					state = EDIT_MONTH;
					initial_scroll_value = raw_val;
				}
				if (exitEdit) {
					global_time_seconds = reconstructSeconds();
					state = RUN;
					Graphics_clearDisplay(&g_sContext);
				}
				break;
		}
	}
}

void initButtons(void) {
	// S1 (P2.1)
	P2DIR &= ~BIT1;
	P2REN |= BIT1;
	P2OUT |= BIT1;

	// S2 (P1.1)
	P1DIR &= ~BIT1;
	P1REN |= BIT1;
	P1OUT |= BIT1;
}

void configure_timer_a2() {
	TA2CTL = TASSEL_1 + MC_1 + ID_0 + TACLR; // ACLK, Up Mode, /1 divider

	// 32768 Hz / 1 Hz = 32768 ticks
	TA2CCR0 = 32767; //32768 - 1 = 32767

	TA2CCTL0 |= CCIE;
}

void config_temp_sensor(void) {
	degC_per_bit = ((float) (85.0 - 30.0)) / ((float) (CALADC12_15V_85C - CALADC12_15V_30C));
	REFCTL0 &= ~REFMSTR;

	ADC12CTL0 = ADC12SHT0_9 | ADC12REFON | ADC12ON | ADC12MSC;
	ADC12CTL1 = ADC12SHP + ADC12CONSEQ_1;
	ADC12MCTL1 = ADC12SREF_1 + ADC12INCH_10 + ADC12EOS;
	__delay_cycles(100);
	ADC12CTL0 |= ADC12ENC;
}

void config_scroll_wheel(void) {
	ADC12MCTL0 = ADC12SREF_1 + ADC12INCH_0;
	P6SEL |= BIT0;
}

unsigned char s1Clicked(void) {
	return !(P2IN & BIT1);
}

unsigned char s2Clicked(void) {
	return !(P1IN & BIT1);
}

unsigned int getScrollWheelReading(void) {
//	ADC12CTL0 &= ~ADC12ENC;
//
//	P6SEL |= BIT0;
//	ADC12CTL0 = ADC12SHT0_10 + ADC12ON;
//	ADC12CTL1 = ADC12CSTARTADD_0 + ADC12SHP;
//	ADC12MCTL0 = ADC12SREF_0 + ADC12INCH_0 + ADC12EOS;
//
//	ADC12CTL0 |= (ADC12SC | ADC12ENC);

	ADC12CTL0 &= ~ADC12SC;
	ADC12CTL0 |= ADC12SC;

	while (ADC12CTL1 & ADC12BUSY)
		__no_operation();

	/**
	//------------------------------------------------------------
	ADC12CTL0 &= ~ADC12SC;
	ADC12CTL0 |= ADC12SC;

	// waits for conversion to finish before reading into in_temp
	while(ADC12CTL1 & ADC12BUSY)
		__no_operation();
	in_temp = ADC12MEM0;
	//------------------------------------------------------------
	**/

	return ADC12MEM0 & 0x0FFF;
}

int handle_scroll_value(int adc_value, int divisions) {
	if(adc_value > initial_scroll_value-5 && adc_value < initial_scroll_value+5){
		return -1;
	}
	initial_scroll_value = -1;

	return adc_value / (MAX_ADC_VALUE / (divisions + 1));
}

void displayTime(unsigned long int inTime) {
	const char *monthNames[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

	sec = inTime % 60;
	min = (inTime / 60) % 60;
	hr = (inTime / 3600) % 24;

	unsigned long totalDays = inTime / 86400;
	currentMonth = 0;
	currentDay = 0;
	unsigned int i = 0;
	for (i = 0; i < 12; i++) {
		if (totalDays < daysInMonth[i]) {
			currentMonth = i;
			currentDay = totalDays + 1;
			break;
		} else {
			totalDays -= daysInMonth[i];
		}
	}

	char dateStr[7];
	char timeStr[9];

	//Build date string: MMM DD
	dateStr[0] = monthNames[currentMonth][0];
	dateStr[1] = monthNames[currentMonth][1];
	dateStr[2] = monthNames[currentMonth][2];
	dateStr[3] = ' ';

	//Build day string
	dateStr[4] = (currentDay / 10) + '0'; // Tens digit
	dateStr[5] = (currentDay % 10) + '0'; // Ones digit
	dateStr[6] = '\0';


	// Hours
	timeStr[0] = (hr / 10) + '0';
	timeStr[1] = (hr % 10) + '0';
	timeStr[2] = ':';

	// Minutes
	timeStr[3] = (min / 10) + '0';
	timeStr[4] = (min % 10) + '0';
	timeStr[5] = ':';

	// Seconds
	timeStr[6] = (sec / 10) + '0';
	timeStr[7] = (sec % 10) + '0';
	timeStr[8] = '\0';

	Graphics_drawStringCentered(&g_sContext, (uint8_t *)dateStr, AUTO_STRING_LENGTH, 48, 25, OPAQUE_TEXT);
	Graphics_drawStringCentered(&g_sContext, (uint8_t *)timeStr, AUTO_STRING_LENGTH, 48, 40, OPAQUE_TEXT);

	Graphics_flushBuffer(&g_sContext);
}

void displayTemp(float inAvgTempC){
	float inAvgTempF = (inAvgTempC * (9.0/5.0)) + 32;

	char tempCStr[8];
	char tempFStr[8];

	floatTempToArray(inAvgTempC, 'C', tempCStr);
	floatTempToArray(inAvgTempF, 'F', tempFStr);

	Graphics_drawStringCentered(&g_sContext, (uint8_t *)tempCStr, AUTO_STRING_LENGTH, 48, 65, OPAQUE_TEXT);
	Graphics_drawStringCentered(&g_sContext, (uint8_t *)tempFStr, AUTO_STRING_LENGTH, 48, 75, OPAQUE_TEXT);

	Graphics_flushBuffer(&g_sContext);

}

void displayEditScreen(void) {
    const char *monthNames[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

	char dateStr[7];
	char timeStr[9];

	//Build date string: MMM DD
	dateStr[0] = monthNames[currentMonth][0];
	dateStr[1] = monthNames[currentMonth][1];
	dateStr[2] = monthNames[currentMonth][2];
	dateStr[3] = ' ';

	//Build day string
	dateStr[4] = (currentDay / 10) + '0'; // Tens digit
	dateStr[5] = (currentDay % 10) + '0'; // Ones digit
	dateStr[6] = '\0';


	// Hours
	timeStr[0] = (hr / 10) + '0';
	timeStr[1] = (hr % 10) + '0';
	timeStr[2] = ':';

	// Minutes
	timeStr[3] = (min / 10) + '0';
	timeStr[4] = (min % 10) + '0';
	timeStr[5] = ':';

	// Seconds
	timeStr[6] = (sec / 10) + '0';
	timeStr[7] = (sec % 10) + '0';
	timeStr[8] = '\0';

	Graphics_drawStringCentered(&g_sContext, (uint8_t *)dateStr, AUTO_STRING_LENGTH, 48, 25, OPAQUE_TEXT);
	Graphics_drawStringCentered(&g_sContext, (uint8_t *)timeStr, AUTO_STRING_LENGTH, 48, 40, OPAQUE_TEXT);

	Graphics_flushBuffer(&g_sContext);
}

void read_temp(void) {
	tempReadings++;
	unsigned int in_temp;
	// calculates what index to store array at, -1 to ensure it starts storing at 0
	int index = (tempReadings - 1) % 36;

	ADC12CTL0 &= ~ADC12SC;
	ADC12CTL0 |= ADC12SC;

	// waits for conversion to finish before reading into in_temp
	while(ADC12CTL1 & ADC12BUSY)
		__no_operation();
	in_temp = ADC12MEM1 & 0X0FFF;

	// calculates temp in C and stores in array
	tempC[index] = (float) (((long) in_temp - CALADC12_15V_30C) * degC_per_bit + 30.0);
}

float get_temp_avg(void) {
	// uses elapsed seconds to determine how much of array to iterate through
	int maxIndex = (tempReadings < 36) ? tempReadings : 36;

	// sums all temperature readings
	float sum = 0;
	int i;
	for (i = 0; i < maxIndex; i++) {
		sum += tempC[i];
	}

	// divides temp readings by number of readings to get avg
	return (sum / (float) maxIndex);
}

void floatTempToArray(float temp, char unit, char* tempStr){

	tempStr[0] = (int)(temp / 100) + '0';
	tempStr[1] = (((int)temp % 100) / 10) + '0';
	tempStr[2] = ((int)temp % 10) + '0';
	tempStr[3] = '.';
	tempStr[4] = ((int)(temp*10) % 10) + '0';
	tempStr[5] = ' ';
	tempStr[6] = unit;
	tempStr[7] = '\0';
}

void breakDownTime(unsigned long int inTime) {
	unsigned long totalDays = inTime / 86400;
	sec = inTime % 60;
	min = (inTime / 60) % 60;
	hr = (inTime / 3600) % 24;

	currentMonth = 0;
	currentDay = 0;

	int i;
	for (i = 0; i < 12; i++) {
		if (totalDays < daysInMonth[i]) {
			currentMonth = i;
			currentDay = totalDays + 1;
			break;
		} else {
			totalDays -= daysInMonth[i];
		}
	}
}

unsigned long int reconstructSeconds(void) {
	unsigned long int total_seconds = 0;

	int i;
	for(i = 0; i < currentMonth; i++) {
		total_seconds += daysInMonth[i] * 86400;
	}

	total_seconds += (currentDay - 1) * 86400;
	total_seconds += hr * 3600;
	total_seconds += min * 60;
	total_seconds += sec;

	return total_seconds;
}

uint8_t clamp(uint8_t val, uint8_t min, uint8_t max) {
	if (val < min)
		return min;
	else if (val > max)
		return max;
	else
		return val;
}
