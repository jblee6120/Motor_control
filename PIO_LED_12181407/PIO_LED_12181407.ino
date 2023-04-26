//12181407 이종범 모터제어 PIO과제
// Aruino DUE의 레지스터를 조작해 LED1~LED8에 불이 1~4개가 들어오는 프로그램
//+, -로 delay조절, 숫자로 LED의 갯수를 제어할 수 있다.
// YouTube Link:
//Pin 연결
//	DUE		Peripheral
//	5V			5V
//	GND			GND
//	25(PD0)		LED1
//	26(PD1)		LED2
//	27(PD2)		LED3
//	28(PD3)		LED4
//	14(PD4)		LED5
//	15(PD5)		LED6
//	29(PD6)		LED7
//	11(PD7)		LED8

char data[8] = { 0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80 }; //LED 1개를 차례대로 켜고 끄는 비트를 저장하기 위한 배열
char data_1[8] = { 0x03,0x06,0x0C,0x18,0x30,0x60,0xC0,0x81 }; //LED 2개를 차례대로 켜고 끄는 비트를 저장하기 위한 배열
char data_2[8] = { 0x07,0x0E,0x1C,0x38,0x70,0xE0,0xC1,0x83 }; //LED 3개를 차례대로 켜고 끄는 비트를 저장하기 위한 배열
char data_3[8] = { 0x0F,0x1E,0x3C,0x78,0xF0,0xE1,0xC3,0x87 }; //LED 4개를 차례대로 켜고 끄는 비트를 저장하기 위한 배열
char* data_p = data; //배열의 이름은 포인터이므로 char형 포인터를 활용해 배열의 요소들을 레지스터에 입력해준다.

int cnt = 0; //0~7번까지의 LED상태를 차례대로 넘어가기위한 counter변수
char data_in; //시리얼 통신으로 받은 데이터를 저장하기 위한 변수
int t = 100;

int sample_time; //샘플링 타임, 즉 몇 초동안 딜레이가 되는지 그 값을 저장하는변수. 단위는 ms다.
uint32_t start_time, end_time; //샘플링 주기가 시작되는 시간과 끝나는 시간을 저장하는 변수.

uint32_t MicrosSampleTime; //샘플링타임을 마이크로 초 단위로 환산해서 저장하는 변수.

void setup()
{
	sample_time = 100; //샘플링 타임을 100ms로 설정
	MicrosSampleTime = (uint32_t)(sample_time * 1e3); //ms 단위의 시간을 마이크로 초로 환산한다.

	Serial.begin(115200); //시리얼 통신 시작.

	PMC->PMC_PCER0 = 1u << ID_PIOD; //PIOD에 clock을 입력함

	PIOD->PIO_PER = 0x000000FF; //PIO_PER(PIO Enable Resistor)를 사용해서 PD0~PD7의 PIO를 Enable 시킨다.
	PIOD->PIO_IDR = 0x000000FF; //PD0~PD7의 Interrupt를 disable 시킨다.

	PIOD->PIO_OER = 0x000000FF; //PD0~PD7핀이 Output을 출력할 수 있도록 한다.
	PIOD->PIO_OWER = 0x000000FF;//PD0~PD7핀이 Output을 word 단위로 출력할 수 있도록 한다.

	PIOD->PIO_ODSR = 0x00000000; //PIOD의 output상태를 모두 0으로 초기화
}

void loop()
{
	start_time = micros();//현재시간을 저장
	MicrosSampleTime = (uint32_t)(sample_time * 1e3); //샘플링타임을 마이크로 초 단위로 저장
	end_time = start_time + MicrosSampleTime; //샘플링 주기의 끝을 시작시간+샘플링 시간으로 저장한다.

	if (Serial.available()) //시리얼 통신으로 정보를 받아오면
	{
		data_in = Serial.read(); //시리얼 통신으로 받은 정보를 저장한다.

		switch (data_in) //받은 data로 switch문으로 나눠본다.
		{
		case '+':
			if (sample_time >= 2000) sample_time = 2000; //샘플링 시간이 2초 이상이면, 샘플링 시간을 2초로 한다.
			else sample_time += 100; //범위 내라면, 샘플링 시간을 100ms 증가시킨다.
			break;

		case '-':
			sample_time -= 100; //샘플링 시간을 100ms 줄인다.
			if (sample_time <= 100) sample_time = 100; //100ms 미만이라면, 샘플링 시간을 100ms로 고정시킨다.
			break;
		case '1': //1일 경우
			data_p = data; //data_p 포인터가 data 배열을 가리키도록 한다.
			break;

		case '2': //2를 받은 경우
			data_p = data_1; //data_p 포인터가 2개의 LED를 켜고 끌 수 있는 패턴을 저장하는 배열을 가리키도록 한다.
			break;

		case '3':
			data_p = data_2; //data_p 포인터가 2개의 LED를 켜고 끌 수 있는 패턴을 저장하는 배열을 가리키도록 한다.
			break;

		case '4':
			data_p = data_3; //data_p 포인터가 2개의 LED를 켜고 끌 수 있는 패턴을 저장하는 배열을 가리키도록 한다.
			break;

		default:
			break;

		}
	}
		PIOD->PIO_ODSR = data_p[cnt]; //포인터가 가리키는 배열의 요소를 차례대로 레지스터에 입력해서 LED를 패턴대로 On시킨다.
		cnt++; //count변수 증가
		if (cnt >= 8) cnt = 0; //count 변수가 8을 넘으면 count 변수를 0으로 초기화 시킨다.

	Serial.println(sample_time); //샘플링 타임을 시리얼 통신으로 출력
	while (!((end_time - micros()) & 0x80000000)); //샘플링 주기가 끝날 때까지 대기.
}