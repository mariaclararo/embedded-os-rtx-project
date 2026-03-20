#include "stm32f10x.h"
#include <cmsis_os.h>
#include "Board_LED.h"

/*----------------------------------------------------------------------------
	Pinagens
------------------------------------------------------------------------------*/
//LEDs
#define LED1 0  //PA0
#define LED2 1  //PA1
#define LED3 2  //PA2
#define LED4 15 //PA15
#define LED5 8  //PA8
#define LED6 6  //PA6
#define LED7 5  //PA5
#define LED8 11 //PA11
//LCD display
#define LCD_RS 15
#define LCD_EN 12
//Switches
#define SW1 12  //PB12
#define SW2 13  //PB13
#define SW3 14  //PB14
#define SW4 15  //PB15
#define SW5 5   //PB5
#define SW6 4   //PB4
#define SW7 3   //PB3
#define SW8 3   //PA3
#define SW9 4   //PA4
#define SW10 8  //PB8
#define SW11 9  //PB9
#define SW12 11 //PB11
#define SW13 10 //PB10
#define SW14 7  //PA7
#define SW15 15 //PC15
#define SW16 14 //PC14
#define SW17 13 //PC13
//Potentiometer
#define POT 1 //PB1
//Buzzer
#define BUZ 0 //PB0
#define VEL_CHANGE osKernelSysTickMicroSec(1000000) //1s de intervalo pra valer mudança
/*----------------------------------------------------------------------------
  Valores de delay, velocidade e variáveis globais
 *---------------------------------------------------------------------------*/
const int padrao = 1000; //velocidade padrao
volatile int vel = 1000, i = 0;

uint32_t lastTime = 0, currentTime;
uint16_t adc_value;

char *botao, *funcao, *tecla, *desc;
/*----------------------------------------------------------------------------
  Escopo das funções
 *---------------------------------------------------------------------------*/
void delay(volatile unsigned int count);
void delay_us(uint16_t t);
void delay_ms(uint16_t t);
void lcd_putValue(unsigned char value);
void lcd_command(unsigned char cmd);
void lcd_data(unsigned char data);
void lcd_print(char * str);
void lcd_init();
void acendePares();
void acendeImpares();
int bin_gray(int bin);
void acendeGray(int gray);
void leitura_Pot();
void acende_Pot();
void toca_buzzer();
void buzzer_off();
void lcd();

/*----------------------------------------------------------------------------
  Escopo das thredas
 *---------------------------------------------------------------------------*/
void bot_thread (void const *argument);
void func1_thread(void const *argument);
void func2_thread(void const *argument);
void func3_thread(void const *argument);
void pot_thread(void const *argument);
void func4_thread(void const *argument);
void vel_thread(void const *argument);
void lcd_thread(void const *argument);

/*----------------------------------------------------------------------------
  RedPill setup routine
 *---------------------------------------------------------------------------*/
void setup_RedPill(){

	//int16_t swa, swb, swc;  //Variables to read the switches according to the port it is connected
	RCC->APB2ENR |= RCC_APB2ENR_AFIOEN; // Enable AF clock
	AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
	osDelay(100);

	RCC->APB2ENR |= 0xFC |(1<<9);		//ENABLE clocks for GPIOs and ADC1
	//Setting up outputs for leds
	ADC1->CR2 = 1;	/* ADON = 1 (power-up) */
	ADC1->CR2 |=(1<<2); //enable calibration
	ADC1->SMPR2 = 1<<3; /* SMP1 = 001 */
	osDelay(1);	/* wait 1us to make sure the adc module is stable */
	GPIOA->CRL = 0x43344333;	//PA3, PA4 and PA7: inputs (switches)
	GPIOA->CRH = 0x33333333;  //PA8 - PA15: outputs (leds)

	//Settig up inputs for switches
	GPIOB->CRL = 0x4444440B; //PB0 set for output+alternate wave form, since it is connected to buzzer.
	GPIOB->CRH = 0x44444444;
	GPIOC->CRH = 0x44444444;
	GPIOC->ODR = 0xFFFFFFFF; //set pull-up in GPIOC

	osDelay(1); //wait for I/O setup
	GPIOA->ODR &=~(1<<LCD_RS); //Turn off LED4
	osDelay(1); //wait for LED4 to turn off
	
	lcd_init();
	GPIOA->ODR = 0x00000000;
}

/*----------------------------------------------------------------------------
  Simple delay routine
 *---------------------------------------------------------------------------*/
void delay(volatile unsigned int count) {
    while(count--) {
        __asm("nop");
    }
}
void delay_us(uint16_t t)
{
	uint16_t i;
	volatile unsigned long l = 0;
	for(i = 0; i < t; i++)
		for(l = 0; l < 6; l++)
		{
		}
}
void delay_ms(uint16_t t)
{
	uint16_t i;
	volatile unsigned long l = 0;
	for(i = 0; i < t; i++)
		for(l = 0; l < 6000; l++)
		{
		}
}

/*----------------------------------------------------------------------------
  funcs LCD
 *---------------------------------------------------------------------------*/
void lcd_putValue(unsigned char value)
{
	uint16_t aux;
	aux = 0x0000; //clear aux
	GPIOA->BRR = (1<<5)|(1<<6)|(1<<8)|(1<<11); /* clear PA5, PA6, PA8, PA11 */
	aux = value & 0xF0;
	aux = aux>>4;
	GPIOA->BSRR = ((aux&0x0008)<<8) | ((aux&0x0004)<<3) | ((aux&0x0002)<<5) | ((aux&0x0001)<<8);
	
	GPIOA->ODR |= (1<<LCD_EN); /* EN = 1 for H-to-L pulse */
	delay_ms(3);			/* make EN pulse wider */
	GPIOA->ODR &= ~ (1<<LCD_EN);	/* EN = 0 for H-to-L pulse */
	delay_ms(1);			/* wait	*/

	GPIOA->BRR = (1<<5)|(1<<6)|(1<<8)|(1<<11); /* clear PA5, PA6, PA8, PA11 */
	aux = 0x0000; //clear aux
	aux = value & 0x0F;
	GPIOA->BSRR = ((aux&0x0008)<<8) | ((aux&0x0004)<<3) | ((aux&0x0002)<<5) | ((aux&0x0001)<<8);
	
	GPIOA->ODR |= (1<<LCD_EN); /* EN = 1 for H-to-L pulse */
	delay_ms(3);			/* make EN pulse wider */
  GPIOA->ODR &= ~(1<<LCD_EN);	/* EN = 0 for H-to-L pulse */
  delay_ms(1);			/* wait	*/
}

void lcd_command(unsigned char cmd)
{
	GPIOA->ODR &= ~ (1<<LCD_RS);	/* RS = 0 for command */
	lcd_putValue(cmd);
}

void lcd_data(unsigned char data)
{
	GPIOA->ODR |= (1<<LCD_RS);	/* RS = 1 for data */
	lcd_putValue(data); 
}

void lcd_print(char * str)
{
  unsigned char i = 0;

	while(str[i] != 0) /* while it is not end of string */
	{
		lcd_data(str[i]); /* show str[i] on the LCD */
		i++;
	}
}

void lcd_init()
{
	delay_ms(15);
	GPIOA->ODR &= ~(1<<LCD_EN);	/* LCD_EN = 0 */
	delay_ms(3); 			/* wait 3ms */
	lcd_command(0x33); //lcd init.
	delay_ms(5);
	lcd_command(0x32); //lcd init.
	delay_us(3000);
	lcd_command(0x28); // 4-bit mode, 1 line and 5x8 charactere set
	delay_ms(3);
	lcd_command(0x0e); // display on, cursor on
	delay_ms(3);
	lcd_command(0x01); // display clear
	delay_ms(3);
	lcd_command(0x06); // move right
	delay_ms(3);
}



/*----------------------------------------------------------------------------
	Implementação das funções
------------------------------------------------------------------------------*/
void acendePares()
{
			GPIOA->ODR &= ~(1<<LED1);
			GPIOA->ODR &= ~(1<<LED3);
			GPIOA->ODR &= ~(1<<LED5);
			GPIOA->ODR &= ~(1<<LED7);
	
			GPIOA->ODR |= (1<<LED2);	 // led 1 - PA0
			GPIOA->ODR |= (1<<LCD_RS); 
		  GPIOA->ODR |= (1<<LED6);  // led 5 - PA8
		  GPIOA->ODR |= (1<<LED8);  // led 7 - PA5
}

void acendeImpares()
{
			GPIOA->ODR &= ~(1<<LED2);
			GPIOA->ODR &= ~(1<<LCD_RS);
			GPIOA->ODR &= ~(1<<LED6);
			GPIOA->ODR &= ~(1<<LED8);
	
			GPIOA->ODR |= (1<<LED1);	 // led 1 - PA0
			GPIOA->ODR |= (1<<LED3); 
		  GPIOA->ODR |= (1<<LED5);  // led 5 - PA8
		  GPIOA->ODR |= (1<<LED7);
}

int bin_gray(int bin)
{
	//xor entre bin e bin>>1
	return bin ^(bin>>1);
}

void acendeGray(int gray)
{
		if (gray & (1 << 7)) GPIOA->ODR |=  (1 << LED1); else GPIOA->ODR &= ~(1 << LED1);
    if (gray & (1 << 6)) GPIOA->ODR |=  (1 << LED2); else GPIOA->ODR &= ~(1 << LED2);
    if (gray & (1 << 5)) GPIOA->ODR |=  (1 << LED3); else GPIOA->ODR &= ~(1 << LED3);
    if (gray & (1 << 4)) GPIOA->ODR |=  (1 << LED4); else GPIOA->ODR &= ~(1 << LED4);
    if (gray & (1 << 3)) GPIOA->ODR |=  (1 << LED5); else GPIOA->ODR &= ~(1 << LED5);
    if (gray & (1 << 2)) GPIOA->ODR |=  (1 << LED6); else GPIOA->ODR &= ~(1 << LED6);
    if (gray & (1 << 1)) GPIOA->ODR |=  (1 << LED7); else GPIOA->ODR &= ~(1 << LED7);
    if (gray & (1 << 0)) GPIOA->ODR |=  (1 << LED8); else GPIOA->ODR &= ~(1 << LED8);
}

void leitura_Pot()
{
		ADC1->SQR3 = 9;
		ADC1->CR2 |= (1 << 30);
		delay(2);
		while((ADC1->SR&(1<<1)) == 0);
		uint16_t read = ADC1->DR;
		if (read >= 0 && read <= 511) adc_value = 1;
    else if (read >= 512 && read <= 1023) adc_value = 2;
    else if (read >= 1024 && read <= 1535) adc_value = 3;
    else if (read >= 1536 && read <= 2047) adc_value = 4;
    else if (read >= 2048 && read <= 2559) adc_value = 5;
    else if (read >= 2560 && read <= 3071) adc_value = 6;
    else if (read >= 3072 && read <= 3583) adc_value = 7;
    else if (read >= 3584 && read <= 4095) adc_value = 8;

}

void acende_Pot()
{
		switch(adc_value)
		{	
			case 1:
			{
				GPIOA->ODR |=(1<<LED1);
				GPIOA->ODR &= ~((1<<LED2)|(1<<LED3)|(1<<LED4)|(1<<LED5)|(1<<LED6)|(1<<LED7)|(1<<LED8));
				break;
			}
			case 2:
			{
				GPIOA->ODR |= (1 << LED1) | (1 << LED2);
				GPIOA->ODR &= ~((1<<LED3)|(1<<LED4)|(1<<LED5)|(1<<LED6)|(1<<LED7)|(1<<LED8));
				break;	
			}
			case 3:
			{
				GPIOA->ODR |= (1 << LED1) | (1 << LED2) | (1<<LED3);
				GPIOA->ODR &= ~((1<<LED4)|(1<<LED5)|(1<<LED6)|(1<<LED7)|(1<<LED8));
				break;	
			}
			case 4:
			{
				GPIOA->ODR |= (1 << LED1) | (1 << LED2) | (1<<LED3)| (1<<LED4);
				GPIOA->ODR &= ~((1<<LED5)|(1<<LED6)|(1<<LED7)|(1<<LED8));
				break;	
			}		
			case 5:
			{
				GPIOA->ODR |= (1 << LED1) | (1 << LED2) | (1<<LED3)| (1<<LED4) | (1<<LED5);
				GPIOA->ODR &= ~((1<<LED6)|(1<<LED7)|(1<<LED8));
				break;	
			}	
			case 6:
			{
				GPIOA->ODR |= (1 << LED1) | (1 << LED2) | (1<<LED3)| (1<<LED4) | (1<<LED5) | (1<<LED6);
				GPIOA->ODR &= ~((1<<LED7)|(1<<LED8));
				break;	
			}	
			case 7:
			{
				GPIOA->ODR |= (1 << LED1) | (1 << LED2) | (1<<LED3)| (1<<LED4) | (1<<LED5) | (1<<LED6) | (1<<LED7);
				GPIOA->ODR &= ~((1<<LED8));
				break;	
			}	
			case 8:
			{
				GPIOA->ODR |= (1 << LED1) | (1 << LED2) | (1<<LED3)| (1<<LED4) | (1<<LED5) | (1<<LED6) | (1<<LED7) | (1<<LED8);
				break;	
			}	
			default:
			{
				GPIOA->ODR = 0x00000000;
				break;
			}
		}
}

void toca_buzzer()
{
		uint32_t freq [8] = {275999, 244598, 218479, 206286, 183611, 164217, 148001, 140677};
		int faixa = adc_value; //8 faixas
		uint32_t val = freq[faixa];
	
		RCC->APB1ENR |= (1 << 1);  // Habilita o clock do Timer TIM3
		TIM3->CCR2 = 5000;
		TIM3->CCER = 0x1 << 8;
		TIM3->CCMR2 = 0x0030;
		TIM3->ARR = val;
		TIM3->CR1 = 1;
		osDelay(10);
}

void buzzer_off() {
    TIM3->CR1 = 0;
    TIM3->CCER &= ~(1 << 8);
}

void lcd()
{
		lcd_init();
		lcd_command(0x01);
		lcd_command(0x80);
		lcd_print("Func:");      
		lcd_print(funcao);
		lcd_print("|Bot:");
		lcd_print(botao);
    lcd_command(0xC0);
		lcd_print("SW:");      
		lcd_print(tecla);
		lcd_print(desc);
}

/*----------------------------------------------------------------------------
 Define the thread handles and thread parameters
 *---------------------------------------------------------------------------*/
osThreadId main_ID, func1_ID, func2_ID, func3_ID, pot_ID, func4_ID, bot_ID, vel_ID, lcd_ID;
osThreadDef(func1_thread, osPriorityNormal, 1, 0);
osThreadDef(func2_thread, osPriorityNormal, 1, 0);
osThreadDef(func3_thread, osPriorityNormal, 1, 0);
osThreadDef(pot_thread, osPriorityAboveNormal, 1, 0);
osThreadDef(func4_thread, osPriorityNormal, 1, 0);
osThreadDef(bot_thread, osPriorityHigh, 1, 0);
osThreadDef(vel_thread, osPriorityAboveNormal, 1, 0);
osThreadDef(lcd_thread, osPriorityRealtime, 1, 0);
/*----------------------------------------------------------------------------
  Threads
 *---------------------------------------------------------------------------*/
void func1_thread(void const *argument)
{
	osEvent stopEvt1; //para monitorar se recebeu sinal de parada
	for(;;)
	{
		osSignalWait(0x10, osWaitForever); //espera pressionar SW1
		while(1)
		{
			acendePares();
			osDelay(vel);
			acendeImpares();
			osDelay(vel);
			
			stopEvt1 = osSignalWait(0x01, 0); //não fica esperando esse sinal, mas verifica toda vez (timeOut = 0)
      if (stopEvt1.status == osEventSignal) { //verifica se o evento retornado é um sinal
         break;
      }
		}
	}
}

void func2_thread(void const *argument)
{
	osEvent stopEvt2;
	int gray;
	for(;;)
	{
		osSignalWait(0x20, osWaitForever);
		GPIOA->ODR = 0x00000000;
		while(1)
		{
			stopEvt2 = osSignalWait(0x02, 0);
      if (stopEvt2.status == osEventSignal) break;
			//gray = bin_gray(i);
			gray = i ^(i>>1);
			acendeGray(gray);
			i = (i+1) % 256; //volta à 0 depois dos 8 bits, como buffer circular
			
			osDelay(vel);
		}
	}
}
	

void func3_thread(void const *argument)
{
	osEvent stopEvt3;
	for(;;)
	{
		osSignalWait(0x30, osWaitForever);
		while(1)
		{
			acende_Pot();
			osDelay(vel); //usei vel para faciliar
			stopEvt3 = osSignalWait(0x03, 0);
      if (stopEvt3.status == osEventSignal) break;
		}
	}
}

void pot_thread(void const *argument)
{
	for(;;)
	{
		leitura_Pot();
		osDelay(10);
	}
}

void func4_thread(void const *argument)
{
	osEvent stopEvt4;
	for(;;)
	{
		osSignalWait(0x40, osWaitForever);
		
		while(1)
		{
			toca_buzzer();
			osDelay(vel);
			stopEvt4 = osSignalWait(0x04, 0);
      if (stopEvt4.status == osEventSignal)
			{				
				buzzer_off();
				break;
			}
		}
	}
}

void vel_thread(void const *argument)
{
	osEvent stopEvt5;
	for(;;)
	{
		osSignalWait(0x50, osWaitForever);
		
		while(1)
		{
			vel = adc_value*200;
			stopEvt5 = osSignalWait(0x05, 0);
      if (stopEvt5.status == osEventSignal) break;
			osDelay(10);
		}
	}
}

void lcd_thread(void const *argument)
{
	for(;;)
	{
		osSignalWait(0x11, osWaitForever);
		lcd();
		GPIOA->ODR = 0x00000000;
	}
}

void bot_thread(void const *argument) {
	int lastState1 = 1, lastState2 = 1, lastState3 = 1, lastState4 = 1, lastState5 = 1, lastState6 = 1, lastState7 = 1, lastState8 = 1, lastState9 = 1, lastState10 = 1, lastState11 = 1;
	int current1, current2, current3, current4, current5, current6, current7, current8, current9, current10, current11;
	int control = 0;

	for (;;) {
		current1 = (GPIOB->IDR & (1 << SW1)) ? 1 : 0;
		current2 = (GPIOB->IDR & (1 << SW2)) ? 1 : 0;
		current3 = (GPIOB->IDR & (1 << SW3)) ? 1 : 0;
		current4 = (GPIOB->IDR & (1 << SW4)) ? 1 : 0;
		current5 = (GPIOB->IDR & (1 << SW5)) ? 1 : 0; //botao Y -> "dobra" velocidade == reduz o delay
		current6 = (GPIOB->IDR & (1 << SW6)) ? 1 : 0; //botao A -> "diminui" vel == dobra delay
		current7 = (GPIOB->IDR & (1 << SW7)) ? 1 : 0; //Botão X – Retorna a velocidade ao valor-padrão definido no código
		current8 = (GPIOA->IDR & (1 << SW8)) ? 1 : 0; //Botão B – Define a velocidade de acordo com o valor estabelecido no potenciômetro
		current9 = (GPIOB->IDR & (1 << SW10)) ? 1 : 0; //0,5s (L)
		current10 = (GPIOB->IDR & (1 << SW11)) ? 1 : 0; //2s(M)
		current11 = (GPIOB->IDR & (1 << SW12)) ? 1 : 0; //10s(N)
		
		if(!current1 && lastState1) //ativa função 1, para as outras
		{
			funcao = "1";
			botao = "C";
			tecla = "1";
			desc = " Par Impar";
			osSignalClear(func1_ID, 0x01); //tira todos os sinais que bloqueiam func
			osSignalClear(func2_ID, 0x20);
			osSignalClear(func3_ID, 0x30);
			osSignalClear(func4_ID, 0x40);
			//bloquear outras funcs
			osSignalSet(func2_ID, 0x02); //func2
			osSignalSet(func3_ID, 0x03); //func3
			osSignalSet(func4_ID, 0x04); //func4
			osSignalSet(lcd_ID, 0x11); //lcd
			
			//limpar leds e executar a funcao
			osSignalSet(func1_ID, 0x10);
			
		}
		
		
		if(!current2 && lastState2) //ativa função 2
		{
			i = 0;
			funcao = "2";
			botao = "D";
			tecla = "2";
			desc = " Pisca Gray";
			osSignalClear(func2_ID, 0x02); //tira todos os sinais que bloqueiam func
			osSignalClear(func1_ID, 0x10);
			osSignalClear(func3_ID, 0x30);
			osSignalClear(func4_ID, 0x40);
			//bloquear outras funcs
			osSignalSet(func1_ID, 0x01); //func1
			osSignalSet(func3_ID, 0x03); //func3
			osSignalSet(func4_ID, 0x04); //func4
			osSignalSet(lcd_ID, 0x11); //lcd

			//limpar leds e executar a funcao
			GPIOA->ODR = 0x00000000;
			osSignalSet(func2_ID, 0x20);
		}
		
		
		if(!current3 && lastState3) //ativa função 3
		{
			//ledsOFF();
			funcao = "3";
			botao = "E";
			tecla = "3";
			desc = " Liga c/Pot";
			osSignalClear(func3_ID, 0x03); //tira todos os sinais que bloqueiam func
			osSignalClear(func1_ID, 0x10);
			osSignalClear(func2_ID, 0x20);
			osSignalClear(func4_ID, 0x40);
			//bloquear outras funcs
			osSignalSet(func1_ID, 0x01); //func1
			osSignalSet(func2_ID, 0x02); //func2
			osSignalSet(func4_ID, 0x04); //func4
			osSignalSet(lcd_ID, 0x11); //lcd
			
			//limpar leds e executar a funcao
			osSignalSet(func3_ID, 0x30);
		}
		
		if(!current4 && lastState4) //ativa função 3
		{
			funcao = "4";
			botao = "F";
			tecla = "4";
			desc = " Led Buzzer";
			osSignalClear(func4_ID, 0x04); //tira todos os sinais que bloqueiam func
			osSignalClear(func3_ID, 0x03); //tira todos os sinais que bloqueiam func
			osSignalClear(func1_ID, 0x10);
			osSignalClear(func2_ID, 0x20);
			osSignalClear(func3_ID, 0x30);
			
			//bloquear outras funcs
			osSignalSet(func1_ID, 0x01); //func1
			osSignalSet(func2_ID, 0x02); //func2
			osSignalSet(lcd_ID, 0x11); //lcd
			
			//limpar leds e executar a funcao
			osSignalSet(func4_ID, 0x40);
			osSignalSet(func3_ID, 0x30);
		}
		
		if(!current5 && lastState5)
		{
			//ledsOFF();
			botao = "Y";
			tecla = "5";
			osSignalSet(lcd_ID, 0x11); //lcd
			currentTime= osKernelSysTick(); //retorna o tempo atual desde o início do sistema
			if((currentTime - lastTime) > VEL_CHANGE) //tempo entre uma mudança e outra tem que ser de 1 segundo
			{
				lastTime = currentTime;
				vel /=2;
			}
		}
		if(!current6 && lastState6)
		{
			botao = "A";
			tecla = "6";
			osSignalSet(lcd_ID, 0x11); //lcd
			currentTime= osKernelSysTick(); //retorna o tempo atual desde o início do sistema
			if((currentTime - lastTime) > VEL_CHANGE) //tempo entre uma mudança e outra tem que ser de 1 segundo
			{
				lastTime = currentTime;
				vel *=2;
			}
		}
		if(!current7 && lastState7)
		{
			botao = "X";
			tecla = "7";
			osSignalSet(lcd_ID, 0x11); //lcd
			vel =padrao;
		}
		
		if(!current8 && lastState8)
		{
			if(control == 0)
			{
				botao = "B AT";
				tecla = "8";
				osSignalSet(lcd_ID, 0x11); //lcd
				osSignalClear(vel_ID, 0x05);
				osSignalSet(vel_ID, 0x50); //init a func
				control = 1;
			}
			else if(control)
			{
				botao = "B DES";
				tecla = "8";
				osSignalSet(lcd_ID, 0x11); //lcd
				osSignalClear(vel_ID, 0x50);
				osSignalSet(vel_ID, 0x05); //para a func
				control = 0;
			}
		}
		
		if(!current9 && lastState9)
		{
			botao = "L";
			tecla = "10";
			osSignalSet(lcd_ID, 0x11); //lcd
			vel =500;
		}
		
		if(!current10 && lastState10)
		{
			botao = "M";
			tecla = "11";
			osSignalSet(lcd_ID, 0x11); //lcd
			vel =2000;
		}
		
		if(!current11 && lastState11)
		{
			botao = "N";
			tecla = "12";
			osSignalSet(lcd_ID, 0x11); //lcd
			vel =10000;
		}
		
		lastState1 = current1;
		lastState2 = current2;
		lastState3 = current3;
		lastState4 = current4;
		lastState5 = current5;
		lastState6 = current6;
		lastState7 = current7;
		lastState8 = current8;
		lastState9 = current9;
		lastState10 = current10;
		lastState11 = current11;
		osDelay(10);
	}
}
	
/*----------------------------------------------------------------------------
 Main
 *---------------------------------------------------------------------------*/

int main (void) 
{
	osKernelInitialize ();                    // initialize CMSIS-RTOS
	setup_RedPill();
	func1_ID = osThreadCreate(osThread(func1_thread), NULL);
	func2_ID = osThreadCreate(osThread(func2_thread), NULL);
	pot_ID = osThreadCreate(osThread(pot_thread), NULL);
	func3_ID = osThreadCreate(osThread(func3_thread), NULL);
	bot_ID = osThreadCreate(osThread(bot_thread), NULL);
	func4_ID = osThreadCreate(osThread(func4_thread), NULL);
	vel_ID = osThreadCreate(osThread(vel_thread), NULL);
	lcd_ID = osThreadCreate(osThread(lcd_thread), NULL);

	osKernelStart ();                         // start thread execution 
	while(1)
	{
		;
	}
}
