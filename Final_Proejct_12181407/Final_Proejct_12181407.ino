#define DWT_CTRL (*((volatile uint32_t*)(0xE0001000UL)))
#define DWT_CYCCNT (*((volatile uint32_t*)(0xE0001004UL)))
#define PI 3.141592653589793
#define PPR 64
#define P 30
#define I 0.9
#define V 0.05
#define A 100

void DWT_Init();
float speed;
int32_t cnt1, cnt1_p;
float count_to_radian;
char buffer[128];
float ref_theta; //각변위 기준값
float theta, theta_p; //각변위 출력, 각변위 출력의 이전 값.

float sample_time = 0.005;
uint32_t MicrosSampleTime;
uint32_t MicrosCycle;
uint32_t SampleTimeCycle;
uint32_t start_time, end_time;


void encoder_init();
void pwm_init();

class PIV {
public:
	float Kp, Ki, Kv, Ka;
	float err_theta, err_w;
	float err_theta_p, err_w_p;
	float integral;
	float derivative_theta_ref;
	float ts;
	float output_max;
	float output_min;
	float out_tmp;
	float out;
	float anti_w_err;
	float PIV_control();
	PIV(float kp, float ki, float kv, float ka);
};

void setup() {
	Serial.begin(115200);

	MicrosSampleTime = (uint32_t)(sample_time * 1e6);
	MicrosCycle = SystemCoreClock / 1000000;
	SampleTimeCycle = MicrosCycle * MicrosSampleTime;

	encoder_init();
	cnt1_p = 0;
	count_to_radian = 2 * PI / PPR;

	DWT_Init();
	start_time = DWT_CYCCNT;
	end_time = start_time + SampleTimeCycle;
}

PIV PIV_con(P, I, V, A); //PIV 위치제어를 수행하는 인스턴스 선언

void loop() {
	cnt1 = TC0->TC_CHANNEL[0].TC_CV; //현재 카운트 값 읽기
	speed = count_to_radian * (cnt1 - cnt1_p)/sample_time; //각속도 계산
	theta = count_to_radian * cnt1; //현재 각변위 계산
	
	Serial.print(theta); //각변위, 각속도 출력
	Serial.print(" ");
	Serial.println(speed);
	
	cnt1_p = cnt1; //이전 카운트에 현재 카운트 값 저장한다. 다음에 다시 루프를 돌 때, 각속도를 계산하기 위해 저장함.
	theta_p = count_to_radian * cnt1_p; //제어계산을 위한 직전 스텝에서의 각변위 계산

	PIV_con.PIV_control();

	while (!((end_time - DWT_CYCCNT) & 0x80000000)); //샘플링 타임 0.005초까지 흘렀는지 확인.
	end_time += SampleTimeCycle; //다음 시간 업데이트
}

void DWT_Init(){
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT_CYCCNT = 0;
	DWT_CTRL |= 1;
}

void encoder_init(){
	PIOB->PIO_PDR = PIO_PB25 | PIO_PB27;
	PIOB->PIO_ABSR |= (PIO_PB25 | PIO_PB27);

	pmc_enable_periph_clk(ID_TC0);

	TC0->TC_CHANNEL[0].TC_CMR = 5;
	TC0->TC_BMR = (1 << 9) | (1 << 8) | (1 << 12) | (1 << 19) | (30 << 20);

	TC0->TC_CHANNEL[0].TC_CCR = 5;
}

PIV::PIV(float kp, float ki, float kv, float ka){
	ts = sample_time;
	Kp = kp;
	Ki = ki;
	Kv = kv;
	Ka = ka;
	err_theta = 0;
	err_theta_p = 0;
	err_w = 0;
	err_w_p = 0;
	integral = 0;
	derivative = 0;
	ouput_max = 12;
	output_min = -12;
}

void pwm_init() {
	PIOC->
}

float PIV::PIV_control()
{
	//reference position과 output reference간의 오차 구하기
	err_theta = ref_theta - theta;
	derivative_theta_ref = (theta - theta_p) / ts; //V제어를 위한 각변위 오차 구하기
	err_w = derivative_theta_ref + Kp*err_theta - speed; //속도오차를 구하는 과정. 각변위 오차에 계수를 추가해야 함.

	//anti-windup 부분
	
	out_tmp = Kv * err_w + Ki * integral;

	//anti-windup
	if (out_tmp >= output_max) out = output_max;
	else if (out_tmp <= output_min) out = output_min;
	else out = out_tmp;

	anti_w_err = out - out_tmp;

	integral += ts * (err_w + Ka * anti_w_err); //오차적분, anti-windup된 값도 넣어야 함.

	//theta_p = theta;
}