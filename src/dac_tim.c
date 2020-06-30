#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/dac.h>
#include <libopencm3/cm3/nvic.h>
#include <math.h>
#include <time_stm32.h>

#define M_2PI 6.28318530718

/* Definições de saída paralela para conversor D/A
                 DAC0800LCN */
#define outPin0 GPIO9
#define outPin1 GPIO10
#define outPin2 GPIO11
#define outPin3 GPIO12
#define outPin4 GPIO6
#define outPin5 GPIO7
#define outPin6 GPIO8
#define outPin7 GPIO9

#define outPort0 GPIOA
#define outPort1 GPIOA
#define outPort2 GPIOA
#define outPort3 GPIOA
#define outPort4 GPIOB
#define outPort5 GPIOB
#define outPort6 GPIOB
#define outPort7 GPIOB

volatile int flag_amostra = 0;

void gpio_setup(void);
void tim2_setup(void);
uint8_t floatToChar(float);
void updateOutput(uint8_t);

void tim2_isr(void)
{
	//gpio_toggle(GPIOC, GPIO13); /* LED on/off. */
	flag_amostra = 1; //Indica que é o momento de amostrar
	/* Manutenção do interrupt */
	timer_clear_flag(TIM2,TIM_SR_CC1IF); /* Clear interrrupt flag. */
	timer_set_counter(TIM2, 0); /* Set counter back to the beginning */
}


void gpio_setup(void) {

	/* Enable clock. */
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOC);
	/* Set GPIO13 (in GPIO port C) to 'output push-pull'. */
	gpio_set_mode(GPIOC,GPIO_MODE_OUTPUT_2_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL,GPIO13);
	gpio_set(GPIOC,GPIO13);

	/* Configurações de saída */
	gpio_set_mode(outPort0, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, outPin0);
	gpio_set_mode(outPort1, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, outPin1);
	gpio_set_mode(outPort2, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, outPin2);
	gpio_set_mode(outPort3, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, outPin3);
	gpio_set_mode(outPort4, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, outPin4);
	gpio_set_mode(outPort5, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, outPin5);
	gpio_set_mode(outPort6, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, outPin6);
	gpio_set_mode(outPort7, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, outPin7);
	
	/* Inicializando estado da saída */
	gpio_set(outPort0, outPin0);
	gpio_set(outPort1, outPin1);
	gpio_set(outPort2, outPin2);
	gpio_set(outPort3, outPin3);
	gpio_set(outPort4, outPin4);
	gpio_set(outPort5, outPin5);
	gpio_set(outPort6, outPin6);
	gpio_set(outPort7, outPin7);

}
void tim2_setup(void){
	
	//Configurado para 20KHz de frequência de atualização - 50us de taxa de amostragem	

	rcc_periph_clock_enable(RCC_TIM2);
	timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	timer_set_prescaler(TIM2, 72000000/800000); // frequencia do tick é o denominador
	timer_set_period(TIM2, 10); // quantos ticks dão um período
	timer_set_oc_value(TIM2, TIM_OC1, 10); //contagem até o período p/ trigar o interrupt
	timer_enable_irq(TIM2, TIM_DIER_CC1IE);

//NVIC
	
	nvic_enable_irq(NVIC_TIM2_IRQ);
	nvic_set_priority(NVIC_TIM2_IRQ,1);

	timer_enable_counter(TIM2);	
}

uint8_t floatToChar(float sinFloat)
{
	//sinFloat entra aqui entre -1 e 1
	uint8_t out = (uint8_t)(255*(sinFloat+1)/2);
	return out;
}


void updateOutput(uint8_t sinChar)
{
	if(sinChar&(1<<0))
		gpio_set(outPort0, outPin0);
	else	
		gpio_clear(outPort0, outPin0);

	if(sinChar&(1<<1))
		gpio_set(outPort1, outPin1);
	else	
		gpio_clear(outPort1, outPin1);

	if(sinChar&(1<<2))
		gpio_set(outPort2, outPin2);
	else	
		gpio_clear(outPort2, outPin2);

	if(sinChar&(1<<3))
		gpio_set(outPort3, outPin3);
	else	
		gpio_clear(outPort3, outPin3);

	if(sinChar&(1<<4))
		gpio_set(outPort4, outPin4);
	else	
		gpio_clear(outPort4, outPin4);

	if(sinChar&(1<<5))
		gpio_set(outPort5, outPin5);
	else	
		gpio_clear(outPort5, outPin5);

	if(sinChar&(1<<6))
		gpio_set(outPort6, outPin6);
	else	
		gpio_clear(outPort6, outPin6);

	if(sinChar&(1<<7))
		gpio_set(outPort7, outPin7);
	else	
		gpio_clear(outPort7, outPin7);

}

int main(void)
{
	rcc_clock_setup_in_hse_8mhz_out_72mhz(); 

	gpio_setup();
	tim2_setup();
	time_init(); //Pra usar a biblioteca de delays
	delay_ms(20);
	
	//uint16_t T_count = 0; //Contador de quantos períodos já passaram	
	float t = 0; //Tempo atual 
	uint8_t saida_char = (uint8_t)0; //Valor do seno após conversão de float pra inteiro
	int qtd_pontos = 64;
	char seno_tabela[qtd_pontos];
	int i;
	uint16_t freq_alvo = 100;
	float T_incremento = 1000000/(qtd_pontos*freq_alvo); // Em us
	int cont = 0; //Contagem de pontos do seno_tabela considerados
	for (i=0;i<qtd_pontos;i++)
	{
		seno_tabela[i] = floatToChar(sin(M_2PI*i/qtd_pontos));
	}
	i = 0;


	gpio_clear(GPIOC,GPIO13);

	delay_ms(10);
	while(1)
	{
		if (flag_amostra == 1) //Bateu o timer de amostra, atualizo o valor do seno
		{
			i = i+1; //Incremento de um tick
			t = t+12.5; //Incremento de um tick em us
			//saida_seno = sin(M_2PI*t/((float)T));
			if (t<cont*T_incremento)
			{
				//Nada acontece
			}
			else
			{
				saida_char = seno_tabela[cont];
				cont++;
			}

			if (cont==qtd_pontos) // Se o tempo atual for igual ao período, o seno se repete
			{
				t = 0;
				cont = 0;
				i = 0;
			}		


			/* Saída pro conversor D/A */
			updateOutput(saida_char);
			gpio_toggle(GPIOC, GPIO13);

			flag_amostra = 0; //Resetando o valor
		}
	}
	return 0;
}
