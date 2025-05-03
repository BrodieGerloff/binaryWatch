//GPIO control
#define DDRB   *((volatile unsigned char *) 0x37) //Port B direction register
#define PORTB  *((volatile unsigned char *) 0x38) //Port B data register
#define PINB   *((volatile unsigned char *) 0x36) //Port B input pins register
#define MCUCR  *((volatile unsigned char *) 0x55) //MCU control register

//Timer control
#define TCCR0A *((volatile unsigned char *) 0x4A) //Timer/counter control register A
#define TCCR0B *((volatile unsigned char *) 0x53) //Timer/counter control register B
#define TCNT0  *((volatile unsigned char *) 0x52) //Timer/counter register
#define TIMSK  *((volatile unsigned char *) 0x59) //Timer/counter interrupt mask register
#define OCR0A  *((volatile unsigned char *) 0x49) //output compare register A

//Interrupt controls
#define SREG   *((volatile unsigned char *) 0x5F) //AVR status register
#define PCMSK  *((volatile unsigned char *) 0x35) //Pin Change Mask Register
#define GIMSK  *((volatile unsigned char *) 0x5B) //General Interrupt mask register

//Other definitions
#define BUTTON_PIN (1<<4)
#define COMPA_COUNT 244

//Functions
#define CHECKBIT(var, bit) ((var) & (1<<(bit)))
#define SEI() __asm__ __volatile__ ("sei" ::: "memory")
#define SLEEP() __asm__("sleep" )
#define BUTTON_PRESSED() (PINB & BUTTON_PIN)

//Number definitions
#define SECONDS_IN_DAY 86400
#define SECONDS_IN_HOUR 3600
#define SECONDS_IN_MINUTE 60
#define STATE_DELAY_SECONDS 3	//How long the user needs to hold the button to switch states, and how long the LEDS remain on in DISPLAYTIME
#define CYCLES_PER_SECOND 4
#define START_TIME 30600			//Watch starts at 8:30 AM so it's a bit easier to tell where you're starting
#define SECONDS_OF_ERROR 50 	//The timer is not perfect and has some error- see the timer interrupt for more info.
#define SECONDS_IN_DAY_CORRECTED (SECONDS_IN_DAY + SECONDS_OF_ERROR)

//Enums and states
enum watchState
{
	IDLE,					//LEDs are off. Clock is still running
	DISPLAYTIME,	//Turn LEDs on
	UPDATEHOUR,		//Updating the time per hour
	UPDATEMINUTE	//Updating the time per minute
};

enum displayMode
{
	ALL,
	HOURS,
	MINUTES
};

// First item is port direction (DDRB) and second is data out (PORTB)
const short ledHourArray[6][2] =
{
	{0b00000011, 0b00010001}, //LED for hour bit 5 (not used in actual hardware- just for consistency)
	{0b00000011, 0b00010010}, //LED for hour bit 4
	{0b00000110, 0b00010010}, //LED for hour bit 3
	{0b00000110, 0b00010100}, //LED for hour bit 2
	{0b00001100, 0b00010100}, //LED for hour bit 1
	{0b00001100, 0b00011000}, //LED for hour bit 0
};

const short ledMinArray[6][2] =
{
	{0b00000101, 0b00010001}, //LED for minute bit 5
	{0b00000101, 0b00010100}, //LED for minute bit 4
	{0b00001010, 0b00010010}, //LED for minute bit 3
	{0b00001010, 0b00011000}, //LED for minute bit 2
	{0b00001001, 0b00010001}, //LED for minute bit 1
	{0b00001001, 0b00011000}, //LED for minute bit 0
};


// Global Variables
short runCount;
volatile unsigned int timeInSeconds;
volatile enum watchState currentState;
volatile enum displayMode displayState;
volatile unsigned int timeButtonPressed;

//--------------------------------
//	TIMER INTERRUPT
//		Advances the clock time.
//		This must be run four times to advance 1 second.
//		The timer is at lowest 976.5625Hz with a 1024 prescaler, but the compare counter is 8 bit.
//		The most accurate division is comparing to 244; this leaves 4.0023 int/sec or 0.0576% fast, or 49.7664 seconds fast per day
//--------------------------------
void __vector_10(void) __attribute__ ((signal));
void __vector_10(void)
{
	runCount++;
	if (runCount == CYCLES_PER_SECOND) {
		timeInSeconds++;
		runCount = 0;
		TCNT0 = 0;
		//Midnight- reset to 0
		if (timeInSeconds >= SECONDS_IN_DAY_CORRECTED) {
			timeInSeconds = 0;
		}
	}
}

//--------------------------------
//	BUTTON INTERRUPT
//		Actions taken when the button is pressed. Button is active low
//
//		State logic:
//		IDLE - Update to DISPLAYTIME
//		DISPLAYTIME - Nothing. Holding the button is handled by the main loop.
//		UPDATEHOUR - Adds one hour (3600 seconds) to the current time.
//		UPDATEMINUTE - Adds one minute to the current time.
//--------------------------------
void __vector_2(void) __attribute__ ((signal));
void __vector_2(void)
{
	if (BUTTON_PRESSED()) return; //Do nothing when ISR is triggered on depress

	switch (currentState){
		case IDLE:
			currentState = DISPLAYTIME;
			break;
		case UPDATEHOUR:
			timeInSeconds += SECONDS_IN_HOUR;
			if (timeInSeconds >= SECONDS_IN_DAY_CORRECTED) {
				timeInSeconds -= SECONDS_IN_DAY_CORRECTED;
			}
			break;
		case UPDATEMINUTE:
			timeInSeconds += SECONDS_IN_MINUTE;
			if (timeInSeconds >= SECONDS_IN_DAY_CORRECTED) {
				timeInSeconds -= SECONDS_IN_DAY_CORRECTED;
			}
			break;
		default:
			break;
	}
	timeButtonPressed = timeInSeconds;
}

// Function declarations
void setup();
void loop();
void stateHandler(enum displayMode mode, enum watchState timeoutState, enum watchState advanceState);
void lightLeds(enum displayMode mode);
void updateLeds(short portDirection, short dataOut);


// Main Loop
int main()
{
	setup();

	while(1)
	{
		loop();
	}
	return 0;
}

// Setup interrupts, timers, etc.
void setup() {

	//--------------------------------
	//	TIMER INTERRUPT
	//--------------------------------

	//Setup timers
	TCCR0A = 0x00;
	TCCR0A |= (1<<1);        	//WGM0[1:0] set to 10b- compare mode
	TCCR0B = 0x00;
	TCCR0B |= (1<<0)|(1<<2); 	//CS0[2:0] set to 101b - copy system clock with 1024 prescaler.

	//Setup and enable timer interrupt
	OCR0A = COMPA_COUNT;			//Compare value
	TIMSK |= (1<<4);					//Enable compare with compare A register

	//--------------------------------
	//	BUTTON INTERRUPT
	//--------------------------------

	PCMSK |= (1<<4);	//Enable interrupt for pin P4
	GIMSK |= (1<<5);			//Enable GPIO interrupts (pin change interrupt)

	//--------------------------------
	//	GENERAL INTERRUPT SETTINGS
	//--------------------------------

	currentState = IDLE;
	displayState = ALL;
	timeInSeconds = 1000;
	timeButtonPressed = 0;
	DDRB = 0x0F;
	PORTB = 0x10;

	SEI();
}

// The main loop
void loop() {
	switch(currentState){
		case IDLE:
			SLEEP();
			break;
		case DISPLAYTIME:
			stateHandler(ALL, IDLE, UPDATEHOUR);
			break;
		case UPDATEHOUR:
			stateHandler(HOURS, UPDATEHOUR, UPDATEMINUTE);
			break;
		case UPDATEMINUTE:
			stateHandler(MINUTES, UPDATEMINUTE, IDLE);
			break;
		default:
			break;
	}
}

void stateHandler(enum displayMode mode, enum watchState timeoutState, enum watchState advanceState)
{
	lightLeds(mode);
	if ((timeInSeconds - timeButtonPressed) >= STATE_DELAY_SECONDS) {
		//If the user is holding the button for STATE_DELAY_SECONDS seconds- advance state. Button is active low.
		if (BUTTON_PRESSED()) {
			currentState = timeoutState;
		}
		else{
			currentState = advanceState;
		}
		//set last press to now so idling in UPDATEHOURS/UPDATEMINUTES always requires waiting
		timeButtonPressed = timeInSeconds;
	}
}

void lightLeds(enum displayMode mode) {
	//Get the real "time"
	short currentMinuteInDay = timeInSeconds / 60; 		//The number of minutes passed today, only used for calculation
	short currentHour = currentMinuteInDay / 60;		//The current hour in 24 hour time
	short currentMinute = currentMinuteInDay % 60;		//The current minute

	for (short i = 0; i < 6; i++) {
		//Check hour
		if (mode == ALL || mode == HOURS )
		{
			if (CHECKBIT(currentHour, i)) updateLeds(ledHourArray[i][0],ledHourArray[i][1]);
		}
		//Check minutes
		if (mode == ALL || mode == MINUTES )
		{
			if (CHECKBIT(currentMinute, i)) updateLeds(ledMinArray[i][0],ledMinArray[i][1]);
		}
	}
	//Clear outputs
	updateLeds(0x00,0x10);
}

void updateLeds(short portDirection, short dataOut)
{
	DDRB = portDirection;
	PORTB = dataOut;
}

