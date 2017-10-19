/**
  ******************************************************************************
  * File Name          : dc.c
  * Description        : Adds a commands to virtual serial port to initialize and work dc motors.
  * Modified on	       : May 21, 2017
  *****************************************************************************
*/
//include files
#include <stdint.h>
#include <stdio.h>
#include "stm32f3xx_hal.h"
#include "common.h"

//Declaring global variables
TIM_HandleTypeDef tim1;
TIM_OC_InitTypeDef sConfig;
GPIO_InitTypeDef GPIO_InitStruct; 
TIM_HandleTypeDef htim17;

//private variables
static uint32_t count = 0;
uint32_t motorRunTime;

// FUNCTION      : dcInit
// DESCRIPTION   : This function initializes dc motor drivers and pwm channels.
// PARAMETERS    : 
//	mode	: checks if the command is passed through in vcp
// RETURNS       : NOTHING
void dcInit(int mode)
{
	//checks if such command exists
	if(mode != CMD_INTERACTIVE) {
    	return;
  	}
	__GPIOF_CLK_ENABLE();
	__GPIOA_CLK_ENABLE();

	//Initialize Timer
	__TIM1_CLK_ENABLE();
	tim1.Instance = TIM1;
	tim1.Init.Prescaler = 72;
	tim1.Init.CounterMode = TIM_COUNTERMODE_UP;
	tim1.Init.Period = 1000;
	tim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	tim1.Init.RepetitionCounter = 0;
	HAL_TIM_Base_Init(&tim1);
	HAL_TIM_Base_Start(&tim1);

	//configure gpio pin for pwm channels
	GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    	GPIO_InitStruct.Pull = GPIO_NOPULL;
    	GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    	GPIO_InitStruct.Alternate = 6;

	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);	

	//configure gpio pin for dc
	GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_4;
    	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;;
    	GPIO_InitStruct.Pull = GPIO_NOPULL;
    	GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    	GPIO_InitStruct.Alternate = 0;

	HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);


	//Initialize PWM
	sConfig.OCMode = TIM_OCMODE_PWM1;
	sConfig.Pulse = 500;
	sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfig.OCNPolarity = TIM_OCNPOLARITY_LOW;
	sConfig.OCFastMode = TIM_OCFAST_DISABLE;
	sConfig.OCIdleState = TIM_OCIDLESTATE_RESET;
	sConfig.OCNIdleState =TIM_OCNIDLESTATE_RESET;

	//Timer 17 clock enable and initialize
    	__TIM17_CLK_ENABLE();

	htim17.Instance = TIM17;
	htim17.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim17.Init.Prescaler = 0x1;
  	htim17.Init.Period = 0x8C9F;
 	htim17.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  	htim17.Init.RepetitionCounter = 0;
  	HAL_TIM_Base_Init(&htim17);


    	//Peripheral interrupt priority setting and enabling
    	HAL_NVIC_SetPriority(TIM1_TRG_COM_TIM17_IRQn, 0, 1);
    	HAL_NVIC_EnableIRQ(TIM1_TRG_COM_TIM17_IRQn);

	//config pwm channels
	HAL_TIM_PWM_ConfigChannel(&tim1,&sConfig, TIM_CHANNEL_1);
	HAL_TIM_PWM_ConfigChannel(&tim1,&sConfig, TIM_CHANNEL_2);
	HAL_TIM_PWM_Init(&tim1);
}

// FUNCTION      : dc
// DESCRIPTION   : This function runs the motor in specified direction with specified speed.
// PARAMETERS    : 
//	mode	: checks if the command is passed through in vcp
// RETURNS       : NOTHING
void dc(int mode)
{
	uint32_t dir, speed;
	int rc;

	//checks if such command exists
	if(mode != CMD_INTERACTIVE) {
		return;
  	}

	//fetch user inputs
	rc = fetch_uint32_arg(&dir);
  	if(rc) {
    		printf("Missing Direction\n");
    		return;
 	}

	//stop timer interupt if it comes to this command for second 
	HAL_TIM_Base_Stop_IT(&htim17);

	//for brake
	if(dir == 0) {
		HAL_Delay(100);
		HAL_GPIO_WritePin(GPIOF,GPIO_PIN_2,0);
		HAL_GPIO_WritePin(GPIOF,GPIO_PIN_4,0);
	}
	else {
		//fetch user inputs only if in on mode
		rc = fetch_uint32_arg(&speed);
  		if(rc) {
    			printf("Missing value for duty cycle\n");
    			return;
 		} 

		//for forward direction
		if(dir == 1) {
			HAL_Delay(100);
			HAL_GPIO_WritePin(GPIOF,GPIO_PIN_2,0);
			HAL_GPIO_WritePin(GPIOF,GPIO_PIN_4,1);
		}

		//for reverse direction
		else if(dir == 2) {
			HAL_Delay(100);
			HAL_GPIO_WritePin(GPIOF,GPIO_PIN_2,1);
			HAL_GPIO_WritePin(GPIOF,GPIO_PIN_4,0);
		}
	}

	//set speed in pwm
	TIM1->CCR1 = speed;

	//start pwm
	HAL_TIM_PWM_Start(&tim1,TIM_CHANNEL_1);
	
} 

// FUNCTION      : dcInter
// DESCRIPTION   : This function runs motor in interrupt mode.
// PARAMETERS    : 
//	mode	: checks if the command is passed through in vcp
// RETURNS       : NOTHING
void dcInter(int mode)
{
	uint32_t dir, speed;

	//checks if such command exists
	if(mode != CMD_INTERACTIVE) {
		return;
  	}
	//fetch user inputs
	fetch_uint32_arg(&dir);
  	fetch_uint32_arg(&speed);
  	fetch_uint32_arg(&motorRunTime);
  	
	//for brake
	if(dir == 0) {
		HAL_GPIO_WritePin(GPIOF,GPIO_PIN_2,0);
		HAL_GPIO_WritePin(GPIOF,GPIO_PIN_4,0);
	}
	//for forward direction
	else if(dir == 1) {
		HAL_GPIO_WritePin(GPIOF,GPIO_PIN_2,0);
		HAL_GPIO_WritePin(GPIOF,GPIO_PIN_4,1);
	}
	//for reverse direction
	else if(dir == 2) {
		HAL_GPIO_WritePin(GPIOF,GPIO_PIN_2,1);
		HAL_GPIO_WritePin(GPIOF,GPIO_PIN_4,0);
	}

	//set speed in pwm
	TIM1->CCR1 = speed;
	//start pwm
	HAL_TIM_PWM_Start(&tim1,TIM_CHANNEL_1);
	//start timer in interrupt base 
	HAL_TIM_Base_Start_IT(&htim17);
} 


// FUNCTION      : TIM17_IRQHandler
// DESCRIPTION   : This function is an interrupt service routine ofr timer 17.
// PARAMETERS    : NOTHING
// RETURNS       : NOTHING
void TIM17_IRQHandler(void)
{
	//calls interrupt handler
	HAL_TIM_IRQHandler(&htim17);
  
	//counts to motorRunTime milliseconds everytime interrupt handles is called
	count ++;
  	if(count > motorRunTime) {
		//stop dc motor and reset count
	  	HAL_GPIO_WritePin(GPIOF,GPIO_PIN_2,0);
		HAL_GPIO_WritePin(GPIOF,GPIO_PIN_4,0);
	  	count = 0;
  	}
}

// FUNCTION      : dcMotion
// DESCRIPTION   : This function reads specified pwm channels.
// PARAMETERS    : 
//	mode	: checks if the command is passed through in vcp
// RETURNS       : NOTHING
void dcMotion(int mode)
{
	
	uint32_t counter = 300;
	uint32_t flag = 300;

	//checks if such command exists
	if(mode != CMD_INTERACTIVE) {
		return;
  	}

	//stop timer interupt if it comes to this command for second 
	HAL_TIM_Base_Stop_IT(&htim17);

	//for forward direction
	HAL_GPIO_WritePin(GPIOF,GPIO_PIN_2,1);
	HAL_GPIO_WritePin(GPIOF,GPIO_PIN_4,0);

	while(flag < 1800) {
		if(flag < 800) {						//ACCELERATING
			TIM1->CCR1 = counter;
			HAL_TIM_PWM_Start(&tim1,TIM_CHANNEL_1);
			counter++;
			HAL_Delay(10);
		}	
		else if(flag > 800 && flag < 1300) {				//CONSTANT
			TIM1->CCR1 = 1000;
			HAL_TIM_PWM_Start(&tim1,TIM_CHANNEL_1);
			HAL_Delay(10);
		}
		else {								//DEACCELERATING
			TIM1->CCR1 = counter;
			HAL_TIM_PWM_Start(&tim1,TIM_CHANNEL_1);
			counter--;
			HAL_Delay(10);
		}
		flag++;	
	}

	//to stop dc motor
	HAL_GPIO_WritePin(GPIOF,GPIO_PIN_2,0);
	HAL_GPIO_WritePin(GPIOF,GPIO_PIN_4,0);

} 

//add commands to virtual communication port
ADD_CMD("dcinit",dcInit,"		Initializes PWM channels")
ADD_CMD("dc",dc," <channel> <value> 	DC motor")
ADD_CMD("dcinter",dcInter,"<channel> <value> <motorRuntime> DC motor with interrupt")
ADD_CMD("dcmotion",dcMotion," 	motion profile in DC motor")
