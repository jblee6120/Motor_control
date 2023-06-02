#define DWT_CTRL (*((volatile uint32_t*)(0xE0001000UL)))
#define DWT_CYCCNT (*((volatile uint32_t*)(0xE0001004UL)))
#define PI 3.141592653589793
#define PPR 64
#define P 30
#define I 0.9
#define V 0.05
#define A 100

void DWT_Init();
double speed;
int32_t cnt1, cnt1_p;
double count_to_radian;
char buffer[128];
double ref_theta; //각변위 기준값
double ref_theta_p = 0;
double theta, theta_p; //각변위 출력, 각변위 출력의 이전 값.

double sim_time = 0;
double sample_time = 0.005;
uint32_t MicrosSampleTime;
uint32_t MicrosCycle;
uint32_t SampleTimeCycle;
uint32_t start_time, end_time;


void encoder_init();
void pwm_init();
void pio_init();
void configure_adc();
class PIV {
public:
	double Kp, Ki, Kv, Ka;
	double err_theta, w_err;
	double err_theta_p, err_w_p;
	double integral;
	double r_dot;
	double ts;
	double output_max;
	double output_min;
	double out_tmp;
	double out;
	double anti_w_err;
	double w_ref;
	double PIV_control();
	PIV(double kp, double ki, double kv, double ka);
};

void setup() {
	Serial.begin(115200);

	MicrosSampleTime = (uint32_t)(sample_time * 1e6);
	MicrosCycle = SystemCoreClock / 1000000;
	SampleTimeCycle = MicrosCycle * MicrosSampleTime;
	pwm_init();
	pio_init();
	configure_adc();
	encoder_init();
	cnt1_p = 0;
	count_to_radian = 2 * PI / PPR;

	DWT_Init();
	start_time = DWT_CYCCNT;
	end_time = start_time + SampleTimeCycle;
}

PIV PIV_con(P, I, V, A); //PIV 위치제어를 수행하는 인스턴스 선언
uint32_t top = 2100;
void loop() {
	double duty = 0;
	double adc_value = 0;
	
	sim_time += sample_time;
	
	ref_theta = 100*sin(5*sim_time);
	//ref_theta = 100;
	cnt1 = TC0->TC_CHANNEL[0].TC_CV; //현재 카운트 값 읽기
	speed = -count_to_radian * (cnt1 - cnt1_p)/sample_time; //각속도 계산
	theta = -count_to_radian * cnt1; //현재 각변위 계산
	
	cnt1_p = cnt1; //이전 카운트에 현재 카운트 값 저장한다. 다음에 다시 루프를 돌 때, 각속도를 계산하기 위해 저장함.
	theta_p = -count_to_radian * cnt1_p; //제어계산을 위한 직전 스텝에서의 각변위 계산

	duty = 100.0 / 12.0 * PIV_con.PIV_control();
	
	if (duty < 0) {
		duty = -duty;
		PIOC->PIO_ODSR &= ~PIO_PC3;
	}

	else
	{
		PIOC->PIO_ODSR |= PIO_PC3;
	}
	ref_theta_p = ref_theta;
	Serial.print(ref_theta);
	Serial.print(" ");
	Serial.println(theta);
	PWM->PWM_CH_NUM[0].PWM_CDTYUPD = (uint32_t)(-2100.0 / 100 * duty + 2100.0);
	PWM->PWM_SCUC = 1;
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

PIV::PIV(double kp, double ki, double kv, double ka){
	ts = sample_time;
	Kp = kp;
	Ki = ki;
	Kv = kv;
	Ka = ka;
	err_theta = 0;
	err_theta_p = 0;
	w_err = 0;
	err_w_p = 0;
	integral = 0;
	r_dot = 0;
	output_max = 12;
	output_min = -12;
	out = 0;
	out_tmp = 0;
	anti_w_err = 0;
	w_ref = 0;
}

void pwm_init() {
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

double PIV::PIV_control()
{
	//reference position과 output reference간의 오차 구하기
	err_theta = ref_theta - theta;
	r_dot = (ref_theta - ref_theta_p) / ts;
	w_ref = Kp * err_theta + r_dot;

	w_err = w_ref - speed;
	integral += ts * (Ki * w_err + Ka*anti_w_err);
	out_tmp = Kv * w_err + integral;

	//anti-windup
	if (out_tmp >= output_max) out = output_max;
	else if (out_tmp <= output_min) out = output_min;
	else out = out_tmp;
	anti_w_err = out - out_tmp;
	return out;
	//theta_p =
}