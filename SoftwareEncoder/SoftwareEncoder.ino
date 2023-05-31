//12181407 이종범 모터제어 Encoder 과제
//Youtube Link: https://youtu.be/5WyZ58QC_jk
//모터에 내장된 엔코더를 회전시켜 나오는 펄스를 아두이노로 읽어, 회전속도외 회전변위를 계산하고, 이를 시리얼 통신으로 출력하는 과제
// 엔코더의 값을 읽는 과정은 4체배 방식을 사용한다.
//결선
//DUE       Motor
//pin25      A상
//pin26      B상
//3.3V       전원
//GND        GND
//pio interrupt 사용 시, multiple definition error가 발생한다면
//\appdata\local\arduino15\packages\hardware\sam\1.6.6(version은 다름)\cores\arduino\WInterrupts.c에서
//WInterrupt.c 파일을 확인한 뒤, 사용하고자 하는 PIO Handler 함수의 우선순위를 내려야 함
//방법, 원하는 함수 앞부분에 __atrribute__((weak)) 적어주면 됨.

#define PI 3.141592653589793 //회전각 변환을 위한 원주율
#define PPR 64 //엔코더의 PPR(Pulsr per Revolution)
void configure_encoder_counter();//엔코더 기능 초기화 및 설정을 위한 함수
int index_c; //encoder의 이동상태를 담는 변수, index로 하면 에러가 발생하기 때문에 다른 이름을 써야 함
uint32_t MicrosSampleTime; //주기적 연산을 하는 시간을 마이크로 단위로 환산하여 저장하는 변수
int32_t cnt1, cnt1_p; //현재 엔코더 카운팅 횟수와 이전 카운트 수를 저장하는 변수
int state, state_p; //현재 논리값과 이전 논리값을 저장하는 변수
uint32_t start_time = 0;
uint32_t end_time;
float sample_time = 0.01;
float speed; //회전속도를 저장하는 변수
float count_to_radian; //엔코더 카운트 수를 라디안으로 변환하여 저장하는 변수
char buffer[128]; //시리얼 통신을 통해 출력하는 버퍼
int changeMtx[16] = {0, -1, 1, 0,
                    1, 0, 0, -1,
                    -1, 0, 0, 1,
                    0, 1, -1, 0}; //이전상태의 논리값과 현재 상태의 논리값을 index로 표현해서 cnt가 더해질지, 빼질지 결정하는 배열

void setup()
{
    Serial.begin(115200);
    configure_encoder_counter();
    MicrosSampleTime = (uint32_t)(sample_time * 1e6);
    count_to_radian = 2 * PI / PPR; //카운트 수를 라디안으로 변환하도록 저장.
    start_time = micros();
    end_time += start_time + MicrosSampleTime;
    cnt1 = 0; //카운트 변수 초기화
    cnt1_p = 0;
}

void loop()
{
    speed = count_to_radian * (cnt1 - cnt1_p) / sample_time; //회전속도를 저장하는 변수의 계산식
    sprintf(buffer, "%d %f\n", cnt1, speed); //buffer 변수에 회전 시 발생하는 펄스의 수, 속도를 저장한다
    Serial.print(buffer); //변위, 속도 출력
    cnt1_p = cnt1;

    while (!((end_time - micros()) & 0x80000000));

    end_time += MicrosSampleTime;
}

void configure_encoder_counter()
{
    PMC->PMC_PCER0 = 1u << ID_PIOD; //clock 인가 pmc_enable_periph_clk()로 대체 가능
    PIOD->PIO_AIMDR = PIO_PD0 | PIO_PD1; //rising, falling edge를 감지하도록 활성화
    PIOD->PIO_IDR = PIO_PD0 | PIO_PD1; //interrupt disable
    PIOD->PIO_IFER = PIO_PD0 | PIO_PD1; //input mode로 동작하므로 input filter 활성화
    PIOD->PIO_ODR = PIO_PD0 | PIO_PD1; //input mode로 동작하기 위해서 output disable 시킴
    PIOD->PIO_SCIFSR = PIO_PD0 | PIO_PD1; //encoder신호는 빠르게 변하므로 SDCIFSR을 활성화 시킴
    PIOD->PIO_IER = PIO_PD0 | PIO_PD1; //마지막으로 interrupt enable 시킨다

    //interrupt enable 시키기 위한 코드
    NVIC_DisableIRQ(PIOD_IRQn);
    NVIC_ClearPendingIRQ(PIOD_IRQn);
    NVIC_SetPriority(PIOD_IRQn, 1);

    NVIC_EnableIRQ(PIOD_IRQn);
}

void PIOD_Handler(void)
{
    PIOD->PIO_ISR; //interrupt 발생 여부 확인

    state = PIOD->PIO_PDSR & (PIO_PD0 | PIO_PD1); //data masking 작업 안하면 비트 이동된 값이 계속 남음 필요없는 값을 제거하기 위해 사용
    
    index_c = (state_p << 2) + state;
    cnt1 += changeMtx[index_c];
    //처음에는 state값을 그대로 인덱스로 사용했음 -> state = (state<<2) + PIOD->PIO_PDSR
    //이렇게 되면 state값에 쓰레기 값이 들어가버려 배열의 인덱스를 넘어가버려 배열의 값이 이상한 값이 나오게 됨
    //따라서 인덱스 값을 저장하는 변수를 별도로 만들고 이전의 state 현재 state값을 저장하는 변수를 별개로 만든 뒤,
    //그 값들을 index에 대입해야 함 -> state_p<<2 + state 이전 값이 2비트 이동하고, 현재 값이 뒤 2비트에 들어가면
    //count 배열에 맞는 index를 구할 수 있음
    state_p = state; //모든 과정이 이루어진 뒤에는, 이전 값을 저장하는 state에 넣어줘야 함.
    //이 과정을 interrupt 발생 시 수행해야 함.

}