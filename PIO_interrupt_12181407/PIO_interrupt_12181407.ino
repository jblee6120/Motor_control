//12181407 이종범 모터제어 PIO Interrupt 과제
//Youtube Link: https://youtu.be/qtVNFfz8QI8
/*pio interrupt를 이용한 초당 스위치 눌린 횟수를 구하는 프로그램
* 
* 핀 설정
* DUE		Peripheral
* PD1(25)	SW1
  PD2		SW2
  5V		5V
  GND		GND
* 
* 스위치가 눌릴 때 핀의 상태는 Logical 0가 된다. 따라서, falling edge가 발생함
* 그러므로 falling edge 발생 시 interrupt가 걸리게 하는 설정을 해야 함
* 
* idea
* 0.05초의 샘플링 타임을 가지므로 0.05*20=1초이므로 20번의 사이클이 돌면 1초 전의 스위치 눌린 횟수와 현재 눌린 횟수를 비교한 뒤,
* 그 차이를 구한 값을 저장하고 시리얼통신으로 출력한다.
*/

float sample_time;

uint32_t start_time, end_time; //시작시간, 끝나는 시간 변수 설정

uint32_t MicroSampleTime; //샘플링 타임을 마이크로 단위로 저장하는 변수

uint8_t cnt1 = 0; //스위치1이 눌린 총 횟수
uint8_t prior_cnt1 = 0; //1초 전에 눌린 총 횟수
uint8_t next_cnt1 = 0; //현재 눌린 총 횟수

uint8_t cnt2 = 0; //스위치2가 눌린 총 횟수
uint8_t prior_cnt2 = 0; //1초 전에 스위치2가 눌린 총 횟수
uint8_t next_cnt2 = 0; //현재 스위치2가 눌린 총 횟수

uint8_t reset_cnt = 0; //0.05초가 몇 번 지났는지 세는 변수 -> 1초, 즉 20번이 되면 초기화

uint8_t diff1 = 0; //1초 동안 스위치1이 눌린 횟수 차이를 저장하는 변수
uint8_t diff2 = 0; //1초 동안 스위치2가 눌린 횟수 차이를 저장하는 변수

void configure_pio_interrupt(); //interrupt 및 pio설정하는 함수의 머리

void setup()
{
	Serial.begin(115200);

	configure_pio_interrupt(); //interrupt 및 pio설정을 수행

	sample_time = 0.05; //샘플링 타임은 0.05초로 설정
	MicroSampleTime = (uint32_t)(sample_time * 1e6); //샘플링 타임을 마이크로 초 단위로 설정
}


void loop()
{
	start_time = micros(); //시작시간 측정

	end_time = start_time + MicroSampleTime; //현재시간 확인

	reset_cnt++; //0.05초가 지난 것을 확인하는 변수 증가

	if (reset_cnt == 20) //0.05초가 20번 지나면 -> 1초가 지났을때
	{
		next_cnt1 = cnt1; //현재 횟수를 지금까지 눌린 총 횟수로 저장
		diff1 = next_cnt1 - prior_cnt1; //현재까지 눌린 횟수 - 1초 전까지 눌린 횟수
		prior_cnt1 = next_cnt1; //계산이 끝나면 다음 계산을 위해 1초 전에 눌린 횟수를 현재 눌린 횟수로 변경

		next_cnt2 = cnt2; //2번 스위치도 마찬가지로 현재까지 눌린 횟수를 지금까지 눌린 횟수로 저장
		diff2 = next_cnt2 - prior_cnt2; //현재까지 눌린 횟수 - 1초 전까지 눌린 횟수
		prior_cnt2 = next_cnt2; //다음 계산을 위해 1초 전에 눌린 횟수를 현재 눌린 횟수로 저장

		reset_cnt = 0; //다음 1초 계산을 위해 초기화
	}

	Serial.print(diff1); // 1초동안 눌린 횟수들을 시리얼 통신을 통해 출력
	Serial.print(" "); // 각 값들은 스페이스로 구분해야하며
	Serial.println(diff2); // 마지막 변수는 줄바꿈을 해 주어야 한다
	while (!((end_time - micros()) & 0x80000000)); //시간 확인
}

void configure_pio_interrupt()
{
	pmc_enable_periph_clk(ID_PIOD); //PD에 클락 제공

	PIOD->PIO_AIMDR = PIO_PD1 | PIO_PD2; // rising edge falling edge 둘 다 인터럽트로 받도록 설정
	PIOD->PIO_IDR = PIO_PD1 | PIO_PD2; //일단 interrupt를 disalbe 시킨다
	PIOD->PIO_PER = PIO_PD1 | PIO_PD2; //PD1,PD2의 pio를 enable 시킨다
	PIOD->PIO_ODR = PIO_PD1 | PIO_PD2; //PD1, PD2는 입력핀으로 설정해준다.

#if 0
	PIOD->PIO_SCIFSR = PIO_PD1 | PIO_PD2; //빠른 신호에 사용하는 필터
#endif

#if 1
	PIOD->PIO_DIFSR = PIO_PD1 | PIO_PD2; //느린 신호에 사용하는필터
	PIOD->PIO_SCDR = 10;
#endif
	PIOD->PIO_AIMER = PIO_PD1 | PIO_PD2; //인터럽트를 falling edge와 edge/level일때 받겠다고 설정
	PIOD->PIO_ESR = PIO_PD1 | PIO_PD2; //인터럽트를 edge event에만 받겠다고 설정함
	PIOD->PIO_ELSR = ~(PIO_PD1 | PIO_PD2); //PIO_FELLSR은 PIO_ELSR에 의해 edge 검출인지, level 검출인지 달라지므로 edge 검출로 설정한다.
	PIOD->PIO_FELLSR = PIO_PD1 | PIO_PD2; //falling edge 발생 시 인터럽트를 발생시키는 설정

	PIOD->PIO_IFER = PIO_PD1 | PIO_PD2; //input filter PD1, PD2에 Enable 시켜준다.

	PIOD->PIO_IER = PIO_PD1 | PIO_PD2; //PD1, PD2의 interrupt를 enable으로 설정

	NVIC_DisableIRQ(PIOD_IRQn); //Interrupt Disable을 시켜준다.
	NVIC_ClearPendingIRQ(PIOD_IRQn); //pending을 clear 시켜준다.
	NVIC_SetPriority(PIOD_IRQn, 1); //NVIC의 priority를 1로 설정해준다.

	NVIC_EnableIRQ(PIOD_IRQn); //Interrupt를 Enalbe 시켜준다.
}

void PIOD_Handler(void)
{
	uint32_t status, PD_value; //인터럽트가 어느 핀에서 발생했는지 그 값을 저장하는 변수와 그 때의 입력핀의 값을 저장하는 변수 생성

	status = PIOD->PIO_ISR; //interrupt 발생 여부를 저장하는 변수, PIO_ISR은 확인 즉시 초기화 되므로 저장이 필요함

	PD_value = PIOD->PIO_PDSR; //입력핀의 값들을 받아옴

	if (status & PIO_PD1) //PD1에서 인터럽트 발생 시
	{
		cnt1++; //횟수 1 증가
	}

	else if (status & PIO_PD2) // PD2에서 인터럽트 발생 시
	{
		cnt2++; //횟수 1 증가
	}

}