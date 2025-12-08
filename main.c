#include <msp430.h>
#include "peripherals.h"

#define CALADC12_15V_30C *((unsigned int *) 0x1A1A)
#define CALADC12_15V_85C *((unsigned int *) 0x1A1C)

// ----------------- Global Variables ----------------- 
volatile unsigned long global_time_seconds = 29089811;
volatile unsigned long elapsedSeconds = 0;
volatile unsigned char new_second_event = 0;
volatile float degC_per_bit;
float tempC[36];
enum DISPLAY_STATE {RUN = 0, EDIT_DAY = 1, EDIT_MONTH = 2, EDIT_HOUR = 3, EDIT_MINUTE = 4, EDIT_SEC = 5};
unsigned int edit_month, edit_day, edit_hour, edit_min, edit_sec;
int initial_scroll_value = 0;
int MAX_ADC_VALUE = 4096;

// ------------------ Function Prototypes ------------------
void initButtons(void);
void configure_timer_a2(void);
void config_temp_sensor(void);

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


#pragma vector=TIMER2_A0_VECTOR
__interrupt void Timer_A2_ISR(void) {
	global_time_seconds++;
	elapsedSeconds++;
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

	__enable_interrupt();

	Graphics_clearDisplay(&g_sContext);

	enum DISPLAY_STATE state = RUN;
	while (1)
	{
		unsigned char s1_clicked = s1Clicked();
		unsigned char s2_clicked = s2Clicked();
		switch(state) {
			case RUN:
				if (new_second_event) {
					new_second_event = 0;
					displayTime(global_time_seconds);

					// reads new temp, calculates new avg, updates temp display
					read_temp();
					displayTemp(get_temp_avg());
				}

				if (s1_clicked) {
					breakDownTime(global_time_seconds);
					state = EDIT_MONTH;
					Graphics_clearDisplay(&g_sContext);
				}
				break;
			case EDIT_MONTH:
				//Need scroll wheel logic here
				displayEditScreen();

				if (s1_clicked) {
					state = EDIT_DAY;
				}
				if (s2_clicked) {
					global_time_seconds = reconstructSeconds();
					state = RUN;
					Graphics_clearDisplay(&g_sContext);
				}
				break;
			case EDIT_DAY:
				//Need scroll wheel logic here
				displayEditScreen();

				if (s1_clicked) {
					state = EDIT_HOUR;
				}
				if (s2_clicked) {
					global_time_seconds = reconstructSeconds();
					state = RUN;
					Graphics_clearDisplay(&g_sContext);
				}
				break;
			case EDIT_HOUR:
				//Need scroll wheel logic here
				displayEditScreen();

				if (s1_clicked) {
					state = EDIT_MINUTE;
				}
				if (s2_clicked) {
					global_time_seconds = reconstructSeconds();
					state = RUN;
					Graphics_clearDisplay(&g_sContext);
				}
				break;
			case EDIT_MINUTE:
				//Need scroll wheel logic here
				displayEditScreen();

				if (s1_clicked) {
					state = EDIT_SEC;
				}
				if (s2_clicked) {
					global_time_seconds = reconstructSeconds();
					state = RUN;
					Graphics_clearDisplay(&g_sContext);
				}
				break;
			case EDIT_SEC:
				//Need scroll wheel logic here
				displayEditScreen();

				if (s1_clicked) {
					state = EDIT_MONTH;
				}
				if (s2_clicked) {
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

	ADC12CTL0 = ADC12SHT0_9 | ADC12REFON | ADC12ON;
	ADC12CTL1 = ADC12SHP;
	ADC12MCTL0 = ADC12SREF_1 + ADC12INCH_10;
	__delay_cycles(100);
	ADC12CTL0 |= ADC12ENC;
}

unsigned char s1Clicked(void) {
	return !(P2IN & BIT1);
}

unsigned char s2Clicked(void) {
	return !(P1IN & BIT1);
}

unsigned int getScrollWheelReading(void) {
	ADC12CTL0 &= ~ADC12ENC;

	P6SEL |= BIT0;
	ADC12CTL0 = ADC12SHT0_10 + ADC12ON;
	ADC12CTL1 = ADC12CSTARTADD_0 + ADC12SHP;
	ADC12MCTL0 = ADC12SREF_0 + ADC12INCH_0 + ADC12EOS;

	ADC12CTL0 |= (ADC12SC | ADC12ENC);

	while (ADC12CTL1 & ADC12BUSY)
		__no_operation();

	return ADC12MEM0;
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
	const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	unsigned int sec = inTime % 60;
	unsigned int min = (inTime / 60) % 60;
	unsigned int hr = (inTime / 3600) % 24;

	unsigned long totalDays = inTime / 86400;
	unsigned int currentMonth = 0;
	unsigned int currentDay = 0;
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
	return;
}

void read_temp(void) {
	unsigned int in_temp;
	// calculates what index to store array at, -1 to ensure it starts storing at 0
	int index = (elapsedSeconds - 1) % 36;

	ADC12CTL0 &= ~ADC12SC;
	ADC12CTL0 |= ADC12SC;

	// waits for conversion to finish before reading into in_temp
	while(ADC12CTL1 & ADC12BUSY)
		__no_operation();
	in_temp = ADC12MEM0;

	// calculates temp in C and stores in array
	tempC[index] = (float) (((long) in_temp - CALADC12_15V_30C) * degC_per_bit + 30.0);
}

float get_temp_avg(void) {
	// uses elapsed seconds to determine how much of array to iterate through
	int maxIndex = (elapsedSeconds < 36) ? elapsedSeconds : 36;

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
	const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	unsigned long totalDays = inTime / 86400;
	edit_sec = inTime % 60;
	edit_min = (inTime / 60) % 60;
	edit_hour = (inTime / 3600) % 24;

	edit_month = 0;
	edit_day = 0;

	int i;
	for (i = 0; i < 12; i++) {
		if (totalDays < daysInMonth[i]) {
			edit_month = i;
			edit_day = totalDays + 1;
			break;
		} else {
			totalDays -= daysInMonth[i];
		}
	}
}

unsigned long int reconstructSeconds(void) {
	const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	unsigned long int total_seconds = 0;

	int i;
	for(i = 0; i < edit_month; i++) {
		total_seconds += daysInMonth[i] * 86400;
	}

	total_seconds += (edit_day - 1) * 86400;
	total_seconds += edit_hour * 3600;
	total_seconds += edit_min * 60;
	total_seconds += edit_sec;

	return total_seconds;
}

