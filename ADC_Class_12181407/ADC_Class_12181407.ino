//12181407 이종범 ADC, Class 과제
//YouTube Link: https://youtu.be/c2NTLrKJDSE
//Abstract
//2개의 가변저항으로 adc값을 읽고, 딜레이 양을 조절하여 딜레이된 데이터와 원본을 class를 이용해 출력하는 과제
//결선
// Arduino DUE		Peripheral Board
//		25					LED1
//		26					LED2
//		27					LED3
//		28					LED4
//		14					LED5	
//		15					LED6
//		29					LED7
//		12					LED8
//		A6					ANA2
//		A7					ANA1

void configure_adc(); //adc 설정을 초기화 하는 함수
void congigure_pio(); //pio 설정을 초기화 하는 함수, LED의 출력이 필요하기 때문이다.
unsigned int data[2]; //adc값을 저장하는 배열 data[0]은 VR1(plotting data), data[1]은 VR2(delay 결정)

float sample_time = 0.01; //주기적 연산을 수행하는 시간을 0.01초로 설정
uint32_t start_time; //시작시간
uint32_t MicrosSampleTime; //샘플링타임을 마이크로 단위로 환산한 값을 저장하는 변수

class Delay //딜레이를 구현할 Delay 클래스
{
	int buffer[71] = { 0 }; //최대 0.7초까지의 딜레이가 필요하므로 0.01*70=0.7 즉, 70개의 배열이 필요
	int input_data; //딜레이 양을 결정지을 인풋값을 저장하는 멤버변수
public:
	int delayed_result(int input); //딜레이 된 결과를 출력하는 멤버함수
};


Delay delay_class; //딜레이를 수행하는 객체 선언
void setup() {
	Serial.begin(115200);
	MicrosSampleTime = (uint32_t)(sample_time * 1e6); //샘플링 타임을 마이크로 초 단위로 환산
	start_time = micros() + MicrosSampleTime; //다음 연산의 시작시간을 설정
	configure_adc(); //adc 설정 초기화
	configure_pio(); //pio 설정 초기화
}

void loop() {
	adc_start(ADC); //ADC를 변환을 시작하는 함수

	while ((ADC->ADC_ISR & ADC_ISR_EOC0) & ADC_ISR_EOC0) { //ADC 변환이 끝나면
		data[0] = adc_get_channel_value(ADC, ADC_CHANNEL_0); //ADC 채널 0번의 값을 읽어온다
	}

	while ((ADC->ADC_ISR & ADC_ISR_EOC1) & ADC_ISR_EOC1) { //ADC 1번의 변환이 끝나면
		data[1] = adc_get_channel_value(ADC, ADC_CHANNEL_1); //ADC 채널 1번의 값을 얻어온다
		data[1] >>= 9; //ADC 1번은 ANA2이므로 딜레이 양을 0~7로 만들기 위해 왼쪽으로 9비트 이동시킨다.
	}

	Serial.print((data[0])); //ADC값 출력
	Serial.print(" "); //다음 값을 출력하기 위해 스페이스 넣기
	Serial.println(delay_class.delayed_result(data[1]));//클래스를 통해 구현된 딜레이된 값을 출력한다.
	while (!((start_time - micros()) & 0x80000000)); //다음 연산시간이 될 때까지 대기
}

void configure_adc() //ADC 설정함수
{
	pmc_enable_periph_clk(ID_ADC); //ADC에 clock 인가

	adc_disable_all_channel(ADC); //모든 ADC 채널을 disable 시킨다.

	adc_init(ADC, SystemCoreClock, ADC_FREQ_MAX, ADC_STARTUP_FAST); //ADC의 클락에 SystemCoreClock을 사용하고, Startup은 빠르게 설정

	adc_configure_timing(ADC, 1, ADC_SETTLING_TIME_3, 1); //ADC 타이밍 설정함수
	adc_set_resolution(ADC, ADC_12_BITS); //ADC의 resolution을 12bit로 설정

	adc_enable_channel(ADC, ADC_CHANNEL_0); //ADC 채널 0번을 활성화
	adc_enable_channel(ADC, ADC_CHANNEL_1); //ADC 채널 1번을 활성화
}

void configure_pio()
{
	pmc_enable_periph_clk(ID_PIOD); //PIOD에 클락 인가

	PIOD->PIO_PER = 0x000000FF; //PD0~PD7까지 PIO를 enable 시킨다.
	PIOD->PIO_OER = 0x000000FF; //Output을 enable 시킨다.
	PIOD->PIO_IDR = 0X000000FF; //interrupt는 사용하지 않으므로, interrupt는 disable 시킨다.
	PIOD->PIO_OWER = 0x000000FF; //PIO_ODSR을 사용하기 위해, PIO_OWER을 활성화시킨다.
	PIOD->PIO_CODR = 0xFFFFFFFF; //시작 전에, 모든 핀을 0으로 초기화.
}

int Delay::delayed_result(int input) //딜레이를 구현할 멤버함수 구현부
{
	switch (input) { //ADC로 인풋값을 받아와서(0~7)
	case 0:
		buffer[0] = data[0]; //0번 배열이 그대로 출력이 되기 때문에, 그대로 0번 버퍼에 현재 ADC값을 넣어준다
		PIOD->PIO_ODSR = 0x00000001; //0일경우, LED는 1개만 출력
		return buffer[0]; //0번 버퍼의 값을 출력
		break;

	case 1: //1일 경우 0.1초 딜레이
		buffer[10] = data[0]; //버퍼 하나당, 0.01초 딜레이므로 10번째 버퍼에 현재 데이터를 넣어준다
		for (int i = 0; i <= 10; i++) {
			buffer[i - 1] = buffer[i]; //10번 이동시키면 0.1초 뒤에 현재 데이터가 나오게 된다.
		}
		PIOD->PIO_ODSR = 0x00000003; //1은 LED 2개 on
		return buffer[0];
		break;

	case 2: //2일 경우 0.2초 딜레이
		buffer[20] = data[0]; //버퍼 하나당, 0.01초 딜레이므로 20번째 버퍼에 현재 데이터를 넣어준다
		for (int i = 0; i <= 20; i++) {
			buffer[i - 1] = buffer[i]; //20번 이동시키면 0.2초 뒤에 현재 데이터가 나오게 된다.
		}
		PIOD->PIO_ODSR = 0x00000007; //2은 LED 3개 on
		return buffer[0];
		break;

	case 3: //3일 경우 0.3초 딜레이
		buffer[30] = data[0]; //버퍼 하나당, 0.01초 딜레이므로 30번째 버퍼에 현재 데이터를 넣어준다
		for (int i = 0; i <= 30; i++) {
			buffer[i - 1] = buffer[i]; //30번 이동시키면 0.3초 뒤에 현재 데이터가 나오게 된다.
		}
		PIOD->PIO_ODSR = 0x0000000F; //3은 LED 4개 on
		return buffer[0];
		break;

	case 4: //4일 경우 0.4초 딜레이
		buffer[40] = data[0]; //버퍼 하나당, 0.01초 딜레이므로 40번째 버퍼에 현재 데이터를 넣어준다
		for (int i = 0; i <= 40; i++) {
			buffer[i - 1] = buffer[i]; //40번 이동시키면 0.4초 뒤에 현재 데이터가 나오게 된다.
		}
		PIOD->PIO_ODSR = 0x0000001F; //4는 LED 5개 on
		return buffer[0];
		break;

	case 5: //5일 경우 0.5초 딜레이
		buffer[50] = data[0]; //버퍼 하나당, 0.01초 딜레이므로 50번째 버퍼에 현재 데이터를 넣어준다
		for (int i = 0; i <= 50; i++) {
			buffer[i - 1] = buffer[i]; //50번 이동시키면 0.5초 뒤에 현재 데이터가 나오게 된다.
		}
		PIOD->PIO_ODSR = 0x0000003F; //5는 LED 6개 on
		return buffer[0];
		break;

	case 6: //6일 경우 0.6초 딜레이
		buffer[60] = data[0]; //버퍼 하나당, 0.01초 딜레이므로 60번째 버퍼에 현재 데이터를 넣어준다
		for (int i = 0; i <= 60; i++) {
			buffer[i - 1] = buffer[i]; //60번 이동시키면 0.6초 뒤에 현재 데이터가 나오게 된다.
		}
		PIOD->PIO_ODSR = 0x0000007F; //6은 LED 7개 on
		return buffer[0];
		break;

	case 7: //6일 경우 0.7초 딜레이
		buffer[70] = data[0]; //버퍼 하나당, 0.01초 딜레이므로 70번째 버퍼에 현재 데이터를 넣어준다
		for (int i = 0; i <= 70; i++) {
			buffer[i - 1] = buffer[i]; //30번 이동시키면 0.7초 뒤에 현재 데이터가 나오게 된다.
		}
		PIOD->PIO_ODSR = 0x000000FF; //7은 LED 8개 on
		return buffer[0];
		break;

	default:
		break;
	}
}