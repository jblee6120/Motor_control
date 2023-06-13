#define DWT_CTRL (*((volatile uint32_t*)(0xE0001000UL)))
#define DWT_CYCCNT (*((volatile uint32_t*)(0xE0001004UL)))

double duty_ratio, duty_ratio_tmp; //duty ratio 저장
double switching_time = 0.001;
double sample_time = 0.005;
double switching_cnt = 84000.0;

uint32_t pin_value;
uint32_t MicrosSampleTime;
uint32_t MicrosCycle;
uint32_t SampleTimeCycle;
uint32_t start_time, end_time;

int begin_point, end_point;

void DWT_Init();
void pio_init();

void setup() {
	Serial.begin(115200);

	MicrosSampleTime = (uint32_t)(sample_time * 1e6);
	MicrosCycle = SystemCoreClock / 100000;
	SampleTimeCycle = MicrosCycle * MicrosSampleTime;

	DWT_Init();
	pio_init();
	start_time = DWT_CYCCNT;
	end_time = start_time + SampleTimeCycle;

}

void loop() {
	/*Serial.print(begin_point);
	Serial.print(" ");*/
	Serial.println(duty_ratio);
	
	while (!((end_time - DWT_CYCCNT) & 0x80000000)); //샘플링 타임 0.005초까지 흘렀는지 확인.
	end_time += SampleTimeCycle; //다음 시간 업데이트
}

void DWT_Init() {
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT_CYCCNT = 0;
	DWT_CTRL |= 1;
}


void pio_init() {
	pmc_enable_periph_clk(ID_PIOC);

	PIOC->PIO_AIMDR |= PIO_PC5; //인터럽트를 falling edge와 edge/level일때 받겠다고 설정
	 PIOC->PIO_IDR |= PIO_PC5;
	 PIOC->PIO_PER |= PIO_PC5;
	PIOC->PIO_ODR |= PIO_PC5;
	PIOC->PIO_PUER |= PIO_PC5;

	//PIOC->PIO_SCIFSR |= PIO_PC5;

	PIOC->PIO_IFER |= PIO_PC5; //input filter PD1, PD2에 Enable 시켜준다.

	PIOC->PIO_IER |= PIO_PC5; //PD1, PD2의 interrupt를 enable으로 설정


	NVIC_DisableIRQ(PIOC_IRQn);
	NVIC_ClearPendingIRQ(PIOC_IRQn);
	NVIC_SetPriority(PIOC_IRQn, 1);

	NVIC_EnableIRQ(PIOC_IRQn);
}

void PIOC_Handler(void) {
	uint32_t status;

	status = PIOC->PIO_ISR;

	pin_value = PIOC->PIO_PDSR;

	if (pin_value & PIO_PC5) {
		begin_point = DWT_CYCCNT;
	}

	else { 
		end_point = DWT_CYCCNT;
	}
	duty_ratio = (double)(end_point - begin_point)/switching_cnt*100.0;

	if (duty_ratio < 0) {
		duty_ratio = 100+duty_ratio;
	}
	
}