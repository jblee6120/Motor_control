//12181407 이종범
void configure_adc();
void congigure_pio();
unsigned int data[2]; //adc값을 저장하는 배열 data[0]은 VR1(plotting data), data[1]은 VR2(delay 결정)

float sample_time = 0.01;
uint32_t start_time;
uint32_t MicrosSampleTime;

class Delay
{
	int buffer[71] = { 0 };
	int input_data;
public:
	int delayed_result(int input);
};


Delay delay_class;
void setup() {
	Serial.begin(115200);
	MicrosSampleTime = (uint32_t)(sample_time * 1e6);
	start_time = micros() + MicrosSampleTime;
	configure_adc();
	configure_pio();
}

void loop() {
	adc_start(ADC);

	while ((ADC->ADC_ISR & ADC_ISR_EOC0) & ADC_ISR_EOC0) {
		data[0] = adc_get_channel_value(ADC, ADC_CHANNEL_0);
	}

	while ((ADC->ADC_ISR & ADC_ISR_EOC1) & ADC_ISR_EOC1) {
		data[1] = adc_get_channel_value(ADC, ADC_CHANNEL_1);
		data[1] >>= 9;
	}

	Serial.print((data[0]));
	Serial.print(" ");
	Serial.println(delay_class.delayed_result(data[1]));
	while (!((start_time - micros()) & 0x80000000));
}

void configure_adc()
{
	pmc_enable_periph_clk(ID_ADC);

	adc_disable_all_channel(ADC);

	adc_init(ADC, SystemCoreClock, ADC_FREQ_MAX, ADC_STARTUP_FAST);

	adc_configure_timing(ADC, 1, ADC_SETTLING_TIME_3, 1);
	adc_set_resolution(ADC, ADC_12_BITS);

	adc_enable_channel(ADC, ADC_CHANNEL_0);
	adc_enable_channel(ADC, ADC_CHANNEL_1);
}

void configure_pio()
{
	pmc_enable_periph_clk(ID_PIOD);

	PIOD->PIO_PER = 0x000000FF;
	PIOD->PIO_OER = 0x000000FF;
	PIOD->PIO_IDR = 0X000000FF;
	PIOD->PIO_OWER = 0x000000FF;
	PIOD->PIO_CODR = 0xFFFFFFFF;
}

int Delay::delayed_result(int input)
{
	switch (input) {
	case 0:
		buffer[0] = data[0];
		PIOD->PIO_ODSR = 0x00000001;
		return buffer[0];
		break;

	case 1:
		buffer[10] = data[0];
		for (int i = 0; i <= 10; i++) {
			buffer[i - 1] = buffer[i];
		}
		PIOD->PIO_ODSR = 0x00000003;
		return buffer[0];
		break;

	case 2:
		buffer[20] = data[0];
		for (int i = 0; i <= 20; i++) {
			buffer[i - 1] = buffer[i];
		}
		PIOD->PIO_ODSR = 0x00000007;
		return buffer[0];
		break;

	case 3:
		buffer[30] = data[0];
		for (int i = 0; i <= 30; i++) {
			buffer[i - 1] = buffer[i];
		}
		PIOD->PIO_ODSR = 0x0000000F;
		return buffer[0];
		break;

	case 4:
		buffer[40] = data[0];
		for (int i = 0; i <= 40; i++) {
			buffer[i - 1] = buffer[i];
		}
		PIOD->PIO_ODSR = 0x0000001F;
		return buffer[0];
		break;

	case 5:
		buffer[50] = data[0];
		for (int i = 0; i <= 50; i++) {
			buffer[i - 1] = buffer[i];
		}
		PIOD->PIO_ODSR = 0x0000003F;
		return buffer[0];
		break;

	case 6:
		buffer[60] = data[0];
		for (int i = 0; i <= 60; i++) {
			buffer[i - 1] = buffer[i];
		}
		PIOD->PIO_ODSR = 0x0000007F;
		return buffer[0];
		break;

	case 7:
		buffer[70] = data[0];
		for (int i = 0; i <= 70; i++) {
			buffer[i - 1] = buffer[i];
		}
		PIOD->PIO_ODSR = 0x000000FF;
		return buffer[0];
		break;

	default:
		break;
	}
}