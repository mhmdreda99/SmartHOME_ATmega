/*
 * main.c
 *
 *  Created on: ??þ/??þ/????
 *      Author: TTMA
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#define  F_CPU 8000000UL
#include <util/delay.h>
#include "Service/Std_Types.h"
#include "Service/Comm_Macros.h"
#include "HAL/lcd/lcd.h"
#include "HAL/keypad/keypad.h"
#include "MCAL/ADC/ADC.h"
#include "MCAL/EXT_Interrupt/External_interrupt.h"
#include "MCAL/Internal_EEPROM/Internal_EEPROM.h"

/************* CONFIGURATIONS*************/

#define  PASSWORD_LENGTH	 10
#define  DEBUG				 0
#define  SETUP				 0
#define  MOTOR_IN1			 1
#define  MOTOR_IN2			 2
#define  SW					 2
#define  MOTOR_EN			 5
#define  BUZZER				 4
#define  NUM_OF_TRIALS_ADDR  25
static char str[100] = {0};
#define LCD_REGISTER_SELECT_PIN						(0)
#define LCD_READ_WRITE_PIN							(1)
#define LCD_ENABLE_PIN								(2)
#define LCD_REGISTER_SELECT_ENABLE					(1)
#define LCD_REGISTER_SELECT_DISABLE					(0)
#define READ_FROM_LCD								(1)
#define WRITE_TO_LCD								(0)
#define LCD_ENABLE									(1)
#define LCD_DISABLE									(0)
#define LCD_FIRST_LINE								(0)
#define LCD_SECOND_LINE								(1)
#define LCD_FUNCTION_8BIT_2LINES   					(0x38)
#define LCD_FUNCTION_4BIT_2LINES   					(0x28)
#define LCD_MOVE_DISP_RIGHT       					(0x1C)
#define LCD_MOVE_DISP_LEFT   						(0x18)
#define LCD_MOVE_CURSOR_RIGHT   					(0x14)
#define LCD_MOVE_CURSOR_LEFT 	  					(0x10)
#define LCD_DISP_OFF   								(0x08)
#define LCD_DISP_ON_CURSOR   						(0x0E)
#define LCD_DISP_ON_CURSOR_BLINK   					(0x0F)
#define LCD_DISP_ON_BLINK   						(0x0D)
#define LCD_DISP_ON   								(0x0C)
#define LCD_ENTRY_DEC   							(0x04)
#define LCD_ENTRY_DEC_SHIFT   						(0x05)
#define LCD_ENTRY_INC_   							(0x06)
#define LCD_ENTRY_INC_SHIFT   						(0x07)
#define LCD_BEGIN_AT_FIRST_RAW						(0x80)
#define LCD_BEGIN_AT_SECOND_RAW						(0xC0)
#define LCD_CLEAR_SCREEN							(0x01)
#define LCD_ENTRY_MODE								(0x06)
#define FALLING_EDGE	3
//************************************** Declaration of functions ***********************/
Std_Return init (void);
Std_Return welcome_message (void);
Std_Return choose_list (void);
Std_Return sign_up (void);
Std_Return log_in (void);
Std_Return change_password(void);
uint8 get_password(uint8* pass);
uint8 check_password(uint8 len, uint8* password);
uint8 get_pin_code(void);
uint8 compare(uint8*, uint8*, uint8, uint8);
Std_Return open_the_door (void);
Std_Return close_the_door (void);
Std_Return try_again (void);
Std_Return open_lights(void);
Std_Return close_lights(void);
uint8 Display_Temp(void);
uint8 adcToVolt(uint16 adcVal);
/*******************************************************************************************/

int main(void)
{
uint8 choice = 0;
	init();

	while(1)
	{
		//Check firstly if the door is permanently locked or not (if the user enters the password 9 times wrong,
		//the door will be locked permanently.
		if(eeprom_read(26) == 0xFF)
		{
			uint8 chk = 0;
			do
			{
				chk = get_pin_code();

			} while (!chk);


			LCD_clear_screen();
			LCD_Send_A_String("Successful");
			LCD_GotoXY(1,0);
			LCD_Send_A_String("operation");
			eeprom_write(26,0);
			eeprom_write(NUM_OF_TRIALS_ADDR,0);
			_delay_ms(1000);
			sign_up();  /**************************************/		}
		//Print Hello message on screen
		welcome_message();

		//Press any key to go to choose list
		Keypad_getkey();

		do
		{
			choose_list();
			choice = Keypad_getkey();
		} while (choice != '1' && choice != '2');

		if(choice == '1')
		{
			log_in();
		}
		else if(choice == '2')
		{
			change_password();

		}

		/***********************************************************************/

		// THIS IS A FUNCTION FOR GARAGE SYSTEM

		uint16 adcValue = 0;
		uint8 voltage = 0;
		adcValue = ADC_Read(0);
				 voltage = adcToVolt(adcValue);
				 if(voltage < 2 ){
				open_the_door();
				 }
				 else{
				close_the_door();
				 }
	}

}


/**************************************************************************************/

//This function initiates the LCD, ADC Modules and ports needed

Std_Return init (void)
{
	uint8 i = 0;							//Just a counter

	LCD_lcd_init();		//Initialize LCD
	LCD_Send_A_Command(0x02);
	LCD_Send_A_Command(LCD_FUNCTION_4BIT_2LINES);

	ADC_init();	                                     //Initialize ADC
	SET_BIT(DDRA,MOTOR_IN1);//PB0 is an output pin
	SET_BIT(DDRA,MOTOR_IN2);//PB1 is an output pin
	SET_BIT(DDRA,MOTOR_EN);//PB3 is an output pin
	SET_BIT(DDRA,BUZZER);//PB4 is an output pin
	SET_BIT(DDRC,0);     //PC0 is an output pin
	SET_BIT(DDRC,1);     //PC1 is an output pin
	SET_BIT(DDRC,2);    // PC2 is an output pin
	SET_BIT(DDRC,3);    //PC3 is an output pin



	set_externalInterrupt(INT2, FALLING_EDGE);		//Enable INT0, FALLING_EDGE
	sei();                                         ////Enable Global Interrupts
	eeprom_write(NUM_OF_TRIALS_ADDR,0);
// INITIAL PASS
#if SETUP
	eeprom_write(1,4);
	for(i = 1; i < 5; i++)
		eeprom_write(i+1, i+48);
	for(i = 0; i < 10; i++)
		eeprom_write(i+27, i+48);
#endif
}

/******************************************************************************************/

/**********************************************************************************/

//This function prints a welcome message to the user

Std_Return welcome_message (void)
{
	LCD_clear_screen ();
	LCD_Send_A_String("  Welcome!");
	LCD_GotoXY(1,0);
    LCD_Send_A_String("MY SMART HOME");
}

/****************************************************************************************/
/*************************************************************************************/

//This function lists the possible choices for user

Std_Return choose_list (void)
{
	LCD_clear_screen();
	LCD_Send_A_String("(1)Log-in");
	   LCD_GotoXY(1,0);
	LCD_Send_A_String("(2)ChangePASSORD");
}

/****************************************************************************************/

/**********************************************************************************/

/*********************************************************************************/

//This function asks the user to enter his desired password two times. If the passwords are
//matched, the password will be saved. Else, It won't and the user should try again.

Std_Return sign_up (void)
{
	uint8 desired_password_1[11] = {0};		//The first entered password is saved here
	uint8 desired_password_2[11] = {0};		//The second entered password is saved here
	uint8 length_1 = 0;						//Length of first entered password
	uint8 length_2 = 0;						//Length of second entered password

	LCD_clear_screen();
	  LCD_GotoXY(0,0);
	LCD_Send_A_String("YournewPassword:");
	  LCD_GotoXY(1,0);
	length_1 = get_password(desired_password_1);

	LCD_clear_screen();
	  LCD_GotoXY(0,0);
	LCD_Send_A_String("ReenterPassword:");
	  LCD_GotoXY(1,0);
	length_2 = get_password(desired_password_2);


	if(compare(desired_password_1,desired_password_2,length_1,length_2))
	{
		uint8 i = 0;		//Just a counter variable
		cli();/**********************************************/

		//Save the length of the password in EEPROM (Location = 0x0001)
		eeprom_write(0x0001,length_1);

		for(i = 0; i < length_1; i++)
		{
			//Save the password in EEPROM (In location from 12 to 22)
			eeprom_write(i+2,desired_password_1[i]);
		}
		sei();      //Enable Global Interrupts

		//Successful operation
		LCD_clear_screen();
		LCD_Send_A_String("Your password ");
		  LCD_GotoXY(1,0);
		LCD_Send_A_String(" is saved");
		_delay_ms(1000);
	}

	else
	{
		//Failed operation, try again
		LCD_clear_screen();
		  LCD_GotoXY(0,0);
		LCD_Send_A_String("Failed_operation");
		LCD_clear_screen();
		  LCD_GotoXY(0,0);
		LCD_Send_A_String("Please,");
		 LCD_GotoXY(1,0);
		LCD_Send_A_String("Try again");
		_delay_ms(1000);
		sign_up();
	}
}

/***************************************************************************************/
/*************************************************************************************************************/

//This function compares password_1 and password_2 and returns '1' if they are matched and '0'
//if matching didn't occur.

uint8 compare(uint8* pass1, uint8* pass2, uint8 len1, uint8 len2)
{
	uint8 i = 0;		// a counter variable

	if(len1 != len2)
	{

#if DEBUG
		LCD_clear_screen ();
		 LCD_GotoXY(0,1);
		LCD_Send_A_String("! = ");
		_delay_ms(1000);
#endif
		return 0;
	}

	else
	{
		for(i = 0; i < len1; i++)
		{
			if(pass1[i] != pass2[i])
			{
#if DEBUG
				LCD_clear_screen ();
				 LCD_GotoXY(0,0);
				LCD_Send_A_String("pass1= ");
				LCD_Send_A_String(itoa((int)pass1[i],(char*)str,10));
				 LCD_GotoXY(1,1);
				LCD_Send_A_String("pass2= ");
				LCD_Send_A_String(itoa((int)pass2[i],(char*)str,10));
				_delay_ms(500);
#endif
				return 0;
			}
			else
			{
#if DEBUG
				LCD_clear_screen ();
				 LCD_GotoXY(0,1);
				LCD_Send_A_String("pass1= ");
				LCD_Send_A_String(itoa((int)pass1[i],str,10));
				 LCD_GotoXY(1,1);
				LCD_Send_A_String("pass2= ");
				LCD_Send_A_String(itoa((int)pass2[i],str,10));
				_delay_ms(2000);
#endif

			}
		}
		return 1;
	}
}

/**************************************************************************/

/*************************************************************************************************************/

//This function tells the user to enter the saved password and then checks if it is right or not
//using check_password function. And then make a decision upon the results

Std_Return log_in (void)
{
	uint8 entered_password[11] = {0};				//The entered password is saved here
	uint8 length = 0;								//Length of entered password

	LCD_clear_screen();
	LCD_Send_A_String("Enter your pass: ");
	LCD_GotoXY(1,0);
	length = get_password(entered_password);

	if(check_password(length, entered_password))
	{
		open_the_door();
		open_lights();
		Display_Temp();

	}
	else
	{
		try_again();
	}
}

/*************************************************************************************************************/

//This function changes the password of user. It firstly asks to enter his old password and if it is right, It
//will ask you to enter your new password two times.

Std_Return change_password(void)
{
	uint8 old_password[10] = {0};		//Old password is stored here
	uint8 length = 0;					//length of old password is stored here

	LCD_clear_screen();
	LCD_Send_A_String("Enter old pass:");
	LCD_GotoXY(1,0);
	length = get_password(old_password);
	if (check_password(length, old_password))
	{
		sign_up();
	}
	else
	{

		change_password();/*****************************/
	}
}

/******************************************************************/
/****************************************************************************/
//This function takes the password from the user

uint8 get_password(uint8* pass)
{
	uint16 cnt = 0; // counter
	uint8 key = 0; // get key pressed
	LCD_Send_A_Command( LCD_DISP_ON_CURSOR );
	LCD_Send_A_Command( LCD_DISP_ON_CURSOR_BLINK);

	for(cnt = 0; cnt <= 10 ; )
	{
		key = Keypad_getkey();

		if(key == '+')
		{
			break;
		}

		else if(key == '?' && cnt > 0)
		{
			LCD_Send_A_Command(LCD_MOVE_CURSOR_LEFT);
			LCD_Send_A_String(' ');
			LCD_Send_A_Command(LCD_MOVE_CURSOR_LEFT);

		cnt --;
		}

		else if(cnt != 10)   /*************/
		{
			pass[cnt++] = key;
			LCD_Send_A_String('*');
		}
	}
	LCD_Send_A_Command(LCD_DISP_ON ); // cursor off


	return cnt;
}
/*******************************************************************/
/*********************************************************************/

//This function compare the entered password with the one saved in the EEPROM. If matching occurs,
//it would return '1'. Else, It would return '0'

uint8 check_password(uint8 len, uint8* password)
{
	uint8 i = 0;				//Just a counter variable

	if(eeprom_read(0x0001) != len)		//Length is stored in EEPROM location 0x0001
	{
		return 0;
	}
	else
	{
		for(i = 0; i < len; i++)
		{
			if(password[i] != eeprom_read(i+2))		//Password is stored in EEPROM from location 2 to 12
			{
				LCD_clear_screen();
				LCD_Send_A_String("error");
				_delay_ms(1000);
				return 0;
			}
		}
		return 1;
	}
}

/****************************************************************************/

/******************************************************************************/

//This function rotates the motor in anti_clock wise direction which means that the door is OPENED

Std_Return open_the_door (void)
{
	SET_BIT(PORTB,MOTOR_EN);  //EN = 1
	SET_BIT(PORTB,MOTOR_IN1);	//IN1 = 1
	CLEAR_BIT(PORTB,MOTOR_IN2);	//IN2 = 0
	SET_BIT(PORTB,BUZZER);
	LCD_clear_screen();
	LCD_Send_A_String(" The door is opened");
	_delay_ms(2000);
	CLEAR_BIT(PORTB,MOTOR_EN);//EN = 0
	CLEAR_BIT(PORTB,BUZZER); //buzzer off
	eeprom_write(NUM_OF_TRIALS_ADDR,0);
}

/*************************************************************************************/

//This function rotates the motor in counter_clock wise direction which means that the door is closed

Std_Return close_the_door (void)
{

	SET_BIT(PORTA,MOTOR_EN);	//EN = 1
	CLEAR_BIT(PORTA,MOTOR_IN1);	//IN1 = 0
	SET_BIT(PORTA,MOTOR_IN2);		//IN2 = 1
	SET_BIT(PORTA,BUZZER);	//buzzer on

	_delay_ms(2000);
	CLEAR_BIT(PORTA,BUZZER); //buzzer off
	CLEAR_BIT(PORTB,MOTOR_EN);

}
 /*******************************************************************************/

/**********************************************************/

//If the voltage on PB2 changes from HIGH to LOW (falling_edge) the door will be closed.
//In reality the interrupt can come from any source .... can be a sensor connected to the door or just
//a when switch is pressed, the door will be closed.

ISR(INT2_vect)
{
	cli();
	close_the_door();
	close_lights();
	sei(); //Enable Global Interrupts
}

/**********************************************************************************/

/*************************************************************************************/

Std_Return try_again (void)
{
	static uint8 number_of_trials = 0;
	number_of_trials = eeprom_read(NUM_OF_TRIALS_ADDR);
	eeprom_write(NUM_OF_TRIALS_ADDR,++number_of_trials);			//This variable is stored in EEPROM location number 25

	if(eeprom_read(NUM_OF_TRIALS_ADDR) == 3 || eeprom_read(NUM_OF_TRIALS_ADDR) == 6)
	{
		uint8 i = 0;
		LCD_clear_screen();
		 LCD_GotoXY(0,0);/**********************/
		LCD_Send_A_String("Wrong password");
		LCD_GotoXY(1,2);
		LCD_Send_A_String("Try again in   ");
		for(i = 59; i > 0; i--)
		{
			LCD_GotoXY(13,2);
			itoa((int)i, str, 10);
			LCD_Send_A_String(str);
			_delay_ms(100);
		}
		log_in();
	}

	else if (eeprom_read(NUM_OF_TRIALS_ADDR) >= 9)
	{
		eeprom_write(26,0xFF);
		uint8 check = 0;
		do
		{
			check = get_pin_code();

		} while (!check);

		LCD_clear_screen();
		LCD_Send_A_String("Successful operation");
		eeprom_write(26,0);
		eeprom_write(NUM_OF_TRIALS_ADDR,0);
		_delay_ms(1000);
		sign_up();
	}

	else
	{
		LCD_clear_screen();
		LCD_GotoXY(0,1);
		LCD_Send_A_String("Wrong password");
		LCD_GotoXY(1,1);
		LCD_Send_A_String("Plz, try again");
		_delay_ms(1000);
		log_in();
	}
}
/************************************************************************/

/********************************************************************************/

//This function checks if the user enters the pin code correctly or not

uint8 get_pin_code(void)
{
	uint8 i = 0;				//Just a counter variable
	uint8 pin_code[10] = {0};
	uint8 length = 0;

	LCD_clear_screen();
	LCD_Send_A_String("The door isclosed");
	LCD_GotoXY(1,0);
	LCD_Send_A_String(" permanently");
	_delay_ms(1000);

	LCD_clear_screen();
	LCD_Send_A_String("Enter pin code:");
	LCD_GotoXY(0,0);
	length = get_password(pin_code);
	for(i = 0; i < 10; i++)/************************************/
		pin_code[i] -= 48;

	if(length != 10)		//pin code is 10 numbers + the ending '*' which acts as an enter.
	{
		LCD_clear_screen();
		LCD_Send_A_String(itoa((int)length,str,10));
		_delay_ms(1000);
		return 0;
	}
	else
	{
		for(i = 0; i < 10; i++)
		{
			if(pin_code[i] != eeprom_read(i+27))		//Password is stored in EEPROM
				                                        //from location 27 to 37
			{
				return 0;
			}
		}
		return 1;
	}
}

/*************************************************************************************/
Std_Return open_lights(void)
{

	SET_BIT(PORTB,4);
	SET_BIT(PORTB,5);
	SET_BIT(PORTB,6);
	SET_BIT(PORTB,7);
}

/***********************************************************************************/
Std_Return close_lights(void)
{
	CLEAR_BIT(PORTB,4);
	CLEAR_BIT(PORTB,5);
	CLEAR_BIT(PORTB,6);
	CLEAR_BIT(PORTB,7);
}
/******************************************************************************/
// this is a function that converts the adc value to volt.

uint8 adcToVolt(uint16 adcVal) {

	return ((adcVal * 5) / 1024);
}
/*************************************************************************/
// this function read analog and convertit to digital in celisuis degrees and display it at lcd
uint8 Display_Temp(void){
uint16 ADC_temp_Value;
uint16 Temp;
uint8 buffer [3];  //displaying digital output as temperature
	LCD_clear_screen ();
	ADC_init();/***********************/
	ADC_temp_Value=ADC_Read(1);
	Temp=ADC_temp_Value/4 ;                    /*since the resolution (2.56/2^10 = 0.0025) is 2.5mV there will be an increment of 4
	                                               for every 10mV input,
                                               that means for every degree raise there will be increment of 4 in digital value.
			                                      So to get the temperature we have to divide ADC output by four.*/
	LCD_clear_screen ();
	LCD_Send_A_Command(0x0F);
	LCD_GotoXY(0,0);
	LCD_Send_A_String("HOME TEMP");

	LCD_GotoXY(1,0);
	LCD_Send_A_String("TEMP(C)=");
	LCD_GotoXY(1,10);
	itoa(Temp,buffer,10); /*command for putting variable number in LCD(variable number, in which character to replace,
	                         which base is variable(ten here as we are counting number in base10))*/

	LCD_Send_A_String(buffer);
	LCD_Send_A_String(" ");
	LCD_GotoXY(0,0);

}
