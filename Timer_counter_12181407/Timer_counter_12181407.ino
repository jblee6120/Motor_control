//12181407 이종범 Timer counter 실습과제
//Peripheral board의 LED1~LED8가 하나씩 일정 시간간격을 가지며 켜지는 과제
// YouTube Link: https://youtu.be/WxtgQbj3GfY
//timer counter와 ADC를 이용해 LED가 이동하는 시간간격을 조절해야한다
//연결한 핀 번호
/* Due     Peripheral Board
PD0~PD7 -> LED1~LED8
5V->5V, GND->GND */


/* arm내장 라이브러리가 각자의 기능에 해당하는 이름의 소스코드에 정의되어 있음 ex)adc.c, tc.c, pio.c., pmc.c
이를 이해하기 위해서는 포인터, 클래스, 구조체, 배열을 반드시 이해해야 함
또한 모든 전처리기 변수는 각자의 기능에 해당되는 이름의 헤더파일에 정의되어 있으며, ex)adc.h, tc.h, pio.h, pmc.h
datasheet를 찾아보면 해당 전처리기 변수의 값이나 정확한 용도를 알 수 있다
따라서 기말과제, 예제코드, 과제용 프로그램을 제작할 때에는 가능하면 예제코드 응용+datasheet를 찾아보며 만들어봐야함*/

char state[8] = { 0x01, 0x02,0x04,0x08,0x10,0x20,0x40,0x80 }; //word단위의 led의 상태를 담는 배열, 이는 PIO_ODSR로 입력해야한다
unsigned int data = 0; //ADC변환을 마친 데이터를 저장하는 변수
unsigned int data_past = 0;
double dt = 0.1; //led가 움직이는 시간을 결정하는 변수
void pio_configure(); //pio설정을 하는 함수 초기선언
void timer_counter_configure(); //timer counter를 설정하는 함수 초기선언
void adc_configure(); //adc를 설정하는 함수 초기선언
unsigned int i = 0; //state의 index를 지칭할 변수
uint32_t rc = 0; //Timer counter의 Top value인 rc값을 저장하는 변수

void setup()
{
	Serial.begin(115200); //data가 정상적으로 수신되는지 확인하기 위해 시리얼통신을 실행
	pio_configure(); //pio 설정
	timer_counter_configure(); //timer counter 설정
	adc_configure(); //adc 설정
}

void loop()
{
	adc_start(ADC); //adc 변환을 시작하는 ARM내장 라이브러리 함수 adc.c에 정의되어 있음

	while ((ADC->ADC_ISR & ADC_ISR_EOC0) != ADC_ISR_EOC0);
	//ADC변환 완료시 ADC_ISR에서 인터럽트 상태 확인 EOC0(End Of Conversino 0)이 1이 되면 내부의 논리연산이 0이 된다
	//즉, ADC변환 중에는 해당 while문을 빠져나가지 않는다
	data = adc_get_channel_value(ADC, ADC_CHANNEL_0) >> 10; //변환 완료된 adc값을 가져와서 저장. 해당 함수도 adc.c에 정의되어 있음
	//adc_get_channel_value를 사용하지 않고 datasheet에 있는 좀 더 복잡한 방식으로 adc를 동작해보도록 하자

	switch (data) //data가 정수형으로 0~4이므로 각 경우마다 갱신하는 dt값이 다르다
	{
	case 0:
		dt = 0.125;
		break;

	case 1:
		dt = 0.25;
		break;

	case 2:
		dt = 0.5;
		break;

	case 3:
		dt = 1.0;
		break;

	default:
		break;
	}
	
	if (data < data_past) TC0->TC_CHANNEL[0].TC_CCR |= TC_CCR_SWTRG; //이전 ADC변환값보다 현재 변환값이 더 크면 Timer Counter의 카운터 값이 rc보다 더 크므로 인터럽트가 발생하지 않는다
	//따라서, rc가 더 낮아지는 조건에서는 Timer counter의 카운터 값을 0으로 초기화시켜야 계속해서 인터럽트가 걸릴 수 있다.
	//초기화 시키지 않으면, LED가 멈춰있는 현상이 발생한다.

	rc = (uint32_t)((VARIANT_MCK / 2) * dt);
	TC0->TC_CHANNEL[0].TC_RC = rc - 1;
	Serial.println(data); //adc변환이 정상적인지 확인하기 위해 화면에 출력
	data_past = data; //현재 adc값을 과거 데이터로 저장한다.
	//Serial.print(" ");
	//Serial.println(dt); //dt의 갱신이 정상적인지 확인하기 위해 화면에 출력
	TC0->TC_CHANNEL[0].TC_SR; //Interrupt 상태를 확인해서 인터럽트 플래그를 0으로 만들어준다.
}

void pio_configure()
{
	pmc_enable_periph_clk(ID_PIOD); //PD에 clock 인가. 초반에 배운 내용 중 더 복잡하게 하는 내용이 있음. 참고필요

	PIOD->PIO_PER = 0x000000FF; //PD0~PD7까지 PIO를 enable시킨다
	PIOD->PIO_IDR = 0x000000FF; //interrupt disable

	PIOD->PIO_OER = 0x000000FF; //output enable 출력이 가능하게 만든다
	PIOD->PIO_OWER = 0x000000FF; //output write enable word 단위 출력이 가능하게 한다 PIO_ODSR쓰려면 이거 해야함
}

void timer_counter_configure()
{
	pmc_enable_periph_clk(ID_TC0); //TC0에 clock 인가

	TC0->TC_CHANNEL[0].TC_CMR = TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK1;
	//counter mode register로 wave, waveselmode, timerclock을 설정 관련 전처리변수는 tc.h에 있음. 정 모르겠으면 데이터시트 찾아보면서 16진수로 직접 입력해도 됨
	rc = (uint32_t)((VARIANT_MCK / 2) * dt);
	//counter의 top값을 설정하는 rc의 값을 설정, 84MHz/2*dt clock
	//ex) rc=84MHz/2*0.1=4.2M의 counter 즉 0.05초에 한 번 counter값 갱신 및 interrupt 발생
	TC0->TC_CHANNEL[0].TC_RC = rc - 1; //0부터 시작하므로 1을 뺀다

	TC0->TC_CHANNEL[0].TC_IER = TC_IER_CPCS; //TC_RC에 matching시 interrupt발생하도록 설정
	TC0->TC_CHANNEL[0].TC_IDR = ~TC_IER_CPCS; //나머지는 interrupt 발생하지 않도록 설정

	NVIC_EnableIRQ(TC0_IRQn); //중첩 interrupt 발생하도록 설정 그냥 해야하는 함수임
	TC0->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG; //clock enable, Timer Counter의 카운터 값을 0으로 초기화시킨다.
}

void adc_configure()
{
	pmc_enable_periph_clk(ID_ADC); //ADC에 clock인가 adc.c에 정의

	adc_disable_all_channel(ADC); //모든 ADC에 adc disable adc.c에 정의

	adc_init(ADC, SystemCoreClock, ADC_FREQ_MAX, ADC_STARTUP_FAST); //ADC초기화 함수 adc.c에 정의

	adc_configure_timing(ADC, 1, ADC_SETTLING_TIME_3, 1);

	adc_set_resolution(ADC, ADC_12_BITS); //adc resolution 12bits로 설정

	adc_enable_channel(ADC, ADC_CHANNEL_0); //A0채널만 adc enable설정
}

void TC0_Handler()
{
	unsigned int status; //interrupt 상태를 담는 변수

	status = TC0->TC_CHANNEL[0].TC_SR; //TC_SR에서 나오는 interrupt 상태 저장

	// uint32_t rc_interrupt = (uint32_t)((VARIANT_MCK / 2) * dt); //interrupt 발생 시 rc값을 갱신하기 위한 변수
	// TC0->TC_CHANNEL[0].TC_RC = rc_interrupt - 1; //실시간적으로 값을 바꾸기 위해서는 loop안에서 값을 바꿔야할듯

	PIOD->PIO_ODSR = state[i]; //interrupt 발생 시 LED의 상태를 변경
	i++; //다음 state로 넘어간다
	if (i >= 8) i = 0; //8번은 없으므로 8이 되면 index는 0으로 초기화
}