float sim_time = 0;
uint32_t MicrosSampleTime;
uint32_t start_time, end_time;
float sample_time = 0.01;
void configure_PWM();
void pio_init();
void configure_adc();

void setup() {
	configure_PWM();
	pio_init();
	configure_adc();
	MicrosSampleTime = (uint32_t)(sample_time * 1e6);
	start_time = micros();
	end_time = start_time + MicrosSampleTime;
	Serial.begin(115200);
}

void loop() {
	adc_start(ADC);
	double adc_value = 0;
	int32_t duty = 0;
	
	while ((ADC->ADC_ISR & ADC_ISR_EOC0) & ADC_ISR_EOC0) { //ADC 변환이 끝나면
		adc_value = adc_get_channel_value(ADC, ADC_CHANNEL_0); //ADC 채널 0번의 값을 읽어온다
	}
	duty = (int32_t)(-4200.0 / 4095.0 * adc_value + 2100.0);
	Serial.print(adc_value);
	Serial.print(" ");
	Serial.println(duty);
	if (duty < 0) {
		duty = -duty;
		PIOC->PIO_CODR |= PIO_PC3;
	}
	else {
		PIOC->PIO_SODR |= PIO_PC3;
	}

	PWM->PWM_CH_NUM[0].PWM_CDTYUPD = duty;

	PWM->PWM_SCUC = 1;
	
	while (!((end_time - micros()) & 0x80000000));
	end_time += MicrosSampleTime;
}

void configure_PWM() {
	PIOC->PIO_PDR = PIO_PC2;
	PIOC->PIO_ABSR |= PIO_PC2;
	
	pmc_enable_periph_clk(ID_PWM);

	PWM->PWM_DIS = (1U << 0);
	PWM->PWM_CLK &= ~0x0F000F00;
	PWM->PWM_CLK &= ~0x00FF0000;
	PWM->PWM_CLK &= ~0x000000FF;
	PWM->PWM_CLK |= (1u << 0);

	PWM->PWM_CH_NUM[0].PWM_CMR &= ~0x0000000F;
	PWM->PWM_CH_NUM[0].PWM_CMR |= ~0x0B;
	PWM->PWM_CH_NUM[0].PWM_CMR |= 1u << 8;
	PWM->PWM_CH_NUM[0].PWM_CMR &= ~(1u << 9);

	PWM->PWM_CH_NUM[0].PWM_CMR |= (1u << 16);

	PWM->PWM_CH_NUM[0].PWM_DT = 0x00030003;

	PWM->PWM_CH_NUM[0].PWM_CPRD = 2100;

	PWM->PWM_CH_NUM[0].PWM_CDTY = 0;

	PWM->PWM_SCM |= 0x00000007;
	PWM->PWM_SCM &= ~0x00030000;

	PWM->PWM_ENA = 1u << 0;
}

void pio_init() {
	pmc_enable_periph_clk(ID_PIOC);
	PIOC->PIO_PER |= PIO_PC3;
	PIOC->PIO_OER |= PIO_PC3;
	PIOC->PIO_IDR |= PIO_PC3;
	PIOC->PIO_OWER |= PIO_PC3;
	PIOC->PIO_CODR |= PIO_PC3;
}

void configure_adc() {
	pmc_enable_periph_clk(ID_ADC);

	adc_disable_all_channel(ADC);

	adc_init(ADC, SystemCoreClock, ADC_FREQ_MAX, ADC_STARTUP_FAST);

	adc_configure_timing(ADC, 1, ADC_SETTLING_TIME_3, 1); //ADC 타이밍 설정함수
	adc_set_resolution(ADC, ADC_12_BITS); //ADC의 resolution을 12bit로 설정

	adc_enable_channel(ADC, ADC_CHANNEL_0); //ADC 채널 0번을 활성화
}