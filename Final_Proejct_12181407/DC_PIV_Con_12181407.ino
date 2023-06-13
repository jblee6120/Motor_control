//12181407 이종범 모터제어 기말과제
//File name: DC_PIV_Con_12181407.ino
//Youtube Link: https://youtu.be/K1CaaRjkgEw
//Arduino DUE에 PIV제어기를 class로 구현하고, DC모터의 PIV 위치제어를 하는 과제
//주기적 연산은 DWT를 이용한 Cycle Counter로 구현하였음.
//클래스에는 PIV 제어를 위한 멤버변수들과 PIV제어값을 출력하는 PIV제어함수 PIV_control()이 있다.

#define DWT_CTRL (*((volatile uint32_t*)(0xE0001000UL)))
#define DWT_CYCCNT (*((volatile uint32_t*)(0xE0001004UL)))
#define PI 3.141592653589793 //원주율
#define PPR 64 //Pulse Per Revolution
#define P 30 //PIV 클래스의 생성자의 매개변수로 들어갈 P Gain
#define I 0.9 //PIV 클래스의 생성자의 매개변수로 들어갈 I Gain
#define V 0.05 //PIV 클래스의 생성자의 매개변수로 들어갈 V Gain
#define A 100 //PIV 클래스의 생성자의 매개변수로 들어갈 Anti-Windup Gain

void DWT_Init(); //DWT를 이용한 Cycle Counter초기화
double speed; //각속도를 저장하는 변수
int32_t cnt1, cnt1_p; //각속도를 계산하기 위해, 현재 카운터값, 이전 카운터 값을 저장하는 변수
double count_to_radian; //카운팅 된 값을 각도인 라디안으로 변환하는 변수
double ref_theta; //각변위 기준값
double ref_theta_p = 0; //이전 시간의 각변위 기준값, 각속도 기준을 계산하기 위해 필요함
double theta, theta_p; //각변위 출력, 각변위 출력의 이전 값.

double duty_ratio, duty_ratio_tmp; //duty ratio 저장
double switching_time = 0.001; //PWM 1kHz에 해당하는 스위칭 시간
double switching_cnt = 84000.0; //스위칭 시간에 해당하는 instruction 수.
uint32_t pin_value; //현재 핀(PC5)의 입력상태를 저장하는 변수
int begin_point, end_point; //pwm의 rising 시점의 시각, falling edge시 시각을 저장하는 변수

double sample_time = 0.005;
uint32_t MicrosSampleTime;
uint32_t MicrosCycle; //샘플링 시간을 마이크로초로 변환 후, 그에 맞는 사이클의 수를 저장하는 변수
uint32_t SampleTimeCycle; //샘플링 시간동안 수행되는 명령어 수를 저장하는 변수
uint32_t start_time, end_time; //주기적 연산의 시작시간과 마지막 시간을 저장하는 변수.


void encoder_init(); //Encoder를 사용하기 위한 timer counter를 초기화하는 함수. 이 과제어서는 software counter 대신, Arduino DUE에 내장된 counter로 각변위를 측정한다
void pwm_init(); //모터에 인가되는 전압을 생성하는 pwm을 수행하기 위해 pwm 기능을 하는 함수.
void pio_init(); //encoder로 사용되는 핀, LW-RCP에서 pwm을 받는 핀을 설정하기 위한 함수

//PIV 제어기를 클래스로 구현. (선언부)
class PIV {
public:
	double Kp, Ki, Kv, Ka; //제어 계수들을 멤버로 선언
	double err_theta, w_err; //각변위, 각속도 오차
	double integral; //오차 적분값을 저장하는 변수
	double r_dot; //각변위 기준치의 미분값을 저장하는 변수
	double ts; //샘플링타임
	double output_max; //saturation을 위한 최대출력 전압
	double output_min; //saturation을 위한 최소출력 전압
	double out_tmp; //anti-windup이전의 출력값
	double out; //anti-windup을 거친 출력값
	double anti_w_err; //anti-windup 이전, 이후 출력간의 오차
	double w_ref; //각속도 기준값
	double PIV_control(); //PIV제어값인 out을 출력하는 제어함수
	PIV(double kp, double ki, double kv, double ka); //생성자
};

void setup() {
	Serial.begin(115200);

	MicrosSampleTime = (uint32_t)(sample_time * 1e6);
	MicrosCycle = SystemCoreClock / 1000000;
	SampleTimeCycle = MicrosCycle * MicrosSampleTime;
	pwm_init();//pwm을 사용하기 위해 원하는 기능으로 초기화
	pio_init(); //pwm 출력핀에 대한 설정, duty ratio 측정을 위해 원하는 기능으로 초기화
	encoder_init(); //encoder 사용을 위해 원하는 기능으로 초기화
	cnt1_p = 0; //초기 카운팅 값은 0으로 초기화
	count_to_radian = 2 * PI / PPR; //라디안으로 변환하는 값 설정

	DWT_Init(); //cycle counter 사용을 위한 DWT 초기화
	start_time = DWT_CYCCNT; //시작시간을 측정하여 저장
	end_time = start_time + SampleTimeCycle; //연산이 끝나는 시간은 샘플링 사이클 이후의 시간이다.
}

PIV PIV_con(P, I, V, A); //PIV 위치제어를 수행하는 인스턴스 선언
uint32_t top = 2100; //pwm에서 top value에 따라 그 주파수가 달라짐, 따라서 top을 2100으로 설정해서
					 //따라서 top 값을 2100으로 설정해, 스위칭 주파수를 20kHz로 만든다.

void loop() {
	double duty = 0; //출력에 인가될 pwm duty ratio
	
	ref_theta = 200.0 / 60.0 * duty_ratio - 500.0/3; //PC5에서 읽어온 DUTY RATIO에서 각변위 기준을 구하는 과정

	cnt1 = TC0->TC_CHANNEL[0].TC_CV; //현재 카운트 값 읽기
	speed = -count_to_radian * (cnt1 - cnt1_p)/sample_time; //각속도 계산
	theta = -count_to_radian * cnt1; //현재 각변위 계산
	
	cnt1_p = cnt1; //이전 카운트에 현재 카운트 값 저장한다. 다음에 다시 루프를 돌 때, 각속도를 계산하기 위해 저장함.
	theta_p = -count_to_radian * cnt1_p; //제어계산을 위한 직전 스텝에서의 각변위 계산

	duty = 100.0 / 12.0 * PIV_con.PIV_control(); //제어기 출력은 [V]이므로 이를 duty ratio로 바꿔서 저장한다. 
	
	if (duty < 0) { //음수가 나오는 경우는 반대방향 회전이다
		duty = -duty; //duty는 pwm에 들어가야 하므로 양수로 바꾼다
		PIOC->PIO_ODSR &= ~PIO_PC3; //dir 핀을 low로 설정해준다
	}

	else //양수의 duty ratio를 가진 경우
	{
		PIOC->PIO_ODSR |= PIO_PC3; //dir pin만 high로 설정해준다
	}
	ref_theta_p = ref_theta; //현재 각속도를 이전 각속도에 저장
	Serial.print(ref_theta); //기준 각변위 출력
	Serial.print(" ");
	Serial.println(theta); //실제 각변위 출력
	PWM->PWM_CH_NUM[0].PWM_CDTYUPD = (uint32_t)(-2100.0 / 100 * duty + 2100.0); //듀티 비를 통해 레지스터에 맞는값으로 변환한다
	PWM->PWM_SCUC = 1; //이 코드를 넣지 않으면 모터가 덜덜 떠는 현상이 발생한다. 따라서 모터드라이버를 지키려면 이 코드를 반드시 넣어야 한다.
	while (!((end_time - DWT_CYCCNT) & 0x80000000)); //샘플링 타임 0.005초까지 흘렀는지 확인.
	end_time += SampleTimeCycle; //다음 시간 업데이트
}

void DWT_Init(){
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT_CYCCNT = 0;
	DWT_CTRL |= 1;
}

void encoder_init(){
	PIOB->PIO_PDR = PIO_PB25 | PIO_PB27; //외부입력을 받아 카운터를 올리기 위해 PIO disable 시킨다.
	PIOB->PIO_ABSR |= (PIO_PB25 | PIO_PB27); //Peripheral B를 사용해야 Timer counter 기능 사용이 가능하므로 Peripheral B로 설정

	pmc_enable_periph_clk(ID_TC0); //Timer Counter 0에 클락 인가

	TC0->TC_CHANNEL[0].TC_CMR = 5; //
	TC0->TC_BMR = (1 << 9) | (1 << 8) | (1 << 12) | (1 << 19) | (30 << 20);

	TC0->TC_CHANNEL[0].TC_CCR = 5;
}

PIV::PIV(double kp, double ki, double kv, double ka){
	ts = sample_time; //샘플링 타임 설정
	//매개변수로 들어온 제어값들을 멤버 변수에 저장
	Kp = kp;
	Ki = ki;
	Kv = kv;
	Ka = ka;
	//제어값 계산과정에서 변경되는 값들은 0으로 초기화 시킨다.
	err_theta = 0;
	w_err = 0;
	integral = 0;
	r_dot = 0;
	//saturation을 위한 변수 초기화
	output_max = 12;
	output_min = -12;
	//출력또한 시작에서 정지해야 안전하므로 0으로설정
	out = 0;
	out_tmp = 0;
	anti_w_err = 0;
	w_ref = 0;
}

//pwm 설정함수 구현부
void pwm_init() {
	PIOC->PIO_PDR = PIO_PC2; //pwm출력을 담당하는 핀은 PIO disable
	PIOC->PIO_ABSR |= PIO_PC2; //Peripheral B기능 활성화

	pmc_enable_periph_clk(ID_PWM); //PWM에 클락인가

	PWM->PWM_DIS = (1U << 0); //pwm disable
	PWM->PWM_CLK &= ~0x0F000F00; //MCK를 사용한다.
	PWM->PWM_CLK &= ~0x00FF0000;
	PWM->PWM_CLK &= ~0x000000FF;
	PWM->PWM_CLK |= (1u << 0); //DIVA = 1

	PWM->PWM_CH_NUM[0].PWM_CMR &= ~0x0000000F; //CPRE를 모두 0으로 클리어한다
	PWM->PWM_CH_NUM[0].PWM_CMR |= ~0x0B; //CLKA를 선택한다.
	PWM->PWM_CH_NUM[0].PWM_CMR |= 1u << 8; //CENTER ALIGNED 방식 사용한다.
	PWM->PWM_CH_NUM[0].PWM_CMR &= ~(1u << 9); //CPOL=0으로 설정

	PWM->PWM_CH_NUM[0].PWM_CMR |= (1u << 16); //DEAD-TIME enable 시킨다. 

	PWM->PWM_CH_NUM[0].PWM_DT = 0x00030003; //채널 0번의 데드타임을 설정한다.

	PWM->PWM_CH_NUM[0].PWM_CPRD = 2100; //pwm이 비활성화 되었으므로 여기에 주기값을 설정한다. 2100이면 스위칭 주파수는 20kHz가 된다.

	PWM->PWM_CH_NUM[0].PWM_CDTY = 0; //cdty에 듀티를 입력

	PWM->PWM_SCM |= 0x00000007;
	PWM->PWM_SCM &= ~0x00030000;

	PWM->PWM_ENA = 1u << 0; //채널 0번을 활성화 시킨다.
}

void pio_init() {
	pmc_enable_periph_clk(ID_PIOC); //pioc에 클락인가
	PIOC->PIO_PER |= PIO_PC3 | PIO_PC5; //pc3,pc5의 pio를 활성화. pc3는 방향결정핀이므로, pc5는 디지털 입력을 받는다.
	PIOC->PIO_OER |= PIO_PC3; //pc3는 출력 활성화
	PIOC->PIO_IDR |= PIO_PC3 | PIO_PC5; //두 핀 전부 인터럽트 비활성화
	PIOC->PIO_OWER |= PIO_PC3; //출력을 할 수 있도록 한다
	PIOC->PIO_CODR |= PIO_PC3; //pc3의 출력을 0으로 설정함.

	PIOC->PIO_ODR |= PIO_PC5; //입력핀이므로 출력은 비활성화
	PIOC->PIO_PUER |= PIO_PC5; //입력핀에 풀업저항 사용
	PIOC->PIO_DIFSR |= PIO_PC5;

	PIOC->PIO_SCIFSR |= PIO_PC5; //빠른 신호에 대한 필터 적용시킴
	PIOC->PIO_IFER |= PIO_PC5; //input filter PD1, PD2에 Enable 시켜준다.

	PIOC->PIO_IER |= PIO_PC5; //PD1, PD2의 interrupt를 enable으로 설정


	NVIC_DisableIRQ(PIOC_IRQn); //PC5에 대한 인터럽트를 NVIC 비활성화
	NVIC_ClearPendingIRQ(PIOC_IRQn);
	NVIC_SetPriority(PIOC_IRQn, 1);

	NVIC_EnableIRQ(PIOC_IRQn); //NVIC 활성화
}

void PIOC_Handler(void) {
	uint32_t status;

	status = PIOC->PIO_ISR; //인터럽트 상태 확인. 상태를 확인하면 ISR은 자동으로 0이 됨.

	pin_value = PIOC->PIO_PDSR; //그 때의 입력핀 상태를 확인.

	if (pin_value & PIO_PC5) { //PC5가 high면 rising edge
		begin_point = DWT_CYCCNT; //그 때의 시간 저장.
	}

	else { //falling edge에서
		end_point = DWT_CYCCNT; //그 때의 시간 저장.
	}
	duty_ratio = (double)(end_point - begin_point) / switching_cnt * 100.0; //듀티비 계산

	if (duty_ratio < 0) {
		duty_ratio = 100 + duty_ratio; //듀티비가 음수일 경우 원래 값을 구하기 위해서 다음과 같은 계산을 한다.
	}

}

double PIV::PIV_control()
{
	//reference position과 output reference간의 오차 구하기
	err_theta = ref_theta - theta;
	r_dot = (ref_theta - ref_theta_p) / ts; //reference의 미분값을 구하기 위해, 즉 각속도를 구한다.
	w_ref = Kp * err_theta + r_dot; //속도 기준치에 대한 값을 계산해준다.

	w_err = w_ref - speed; //실제 회전속도와 기준치와의 오차를 계산
	integral += ts * (Ki * w_err + Ka*anti_w_err); //속도오차에 대한 적분값을 계산한다.
	out_tmp = Kv * w_err + integral; //PIV제어기에 대한 출력을 만들어준다.

	//anti-windup
	if (out_tmp >= output_max) out = output_max; //최대출력을 넘을 시, 최대출력으로 제한
	else if (out_tmp <= output_min) out = output_min; //최소출력보다 더 작을 시, 최소출력으로 제한
	else out = out_tmp; //그 사이라면 그대로 출력 반영
	anti_w_err = out - out_tmp; // anti-windup 오차 계산

	return out;
}