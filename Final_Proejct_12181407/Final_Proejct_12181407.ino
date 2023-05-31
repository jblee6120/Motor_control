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
float ref_theta; //������ ���ذ�
float theta, theta_p; //������ ���, ������ ����� ���� ��.

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

PIV PIV_con(P, I, V, A); //PIV ��ġ��� �����ϴ� �ν��Ͻ� ����

void loop() {
	cnt1 = TC0->TC_CHANNEL[0].TC_CV; //���� ī��Ʈ �� �б�
	speed = count_to_radian * (cnt1 - cnt1_p)/sample_time; //���ӵ� ���
	theta = count_to_radian * cnt1; //���� ������ ���
	
	Serial.print(theta); //������, ���ӵ� ���
	Serial.print(" ");
	Serial.println(speed);
	
	cnt1_p = cnt1; //���� ī��Ʈ�� ���� ī��Ʈ �� �����Ѵ�. ������ �ٽ� ������ �� ��, ���ӵ��� ����ϱ� ���� ������.
	theta_p = count_to_radian * cnt1_p; //�������� ���� ���� ���ܿ����� ������ ���

	PIV_con.PIV_control();

	while (!((end_time - DWT_CYCCNT) & 0x80000000)); //���ø� Ÿ�� 0.005�ʱ��� �귶���� Ȯ��.
	end_time += SampleTimeCycle; //���� �ð� ������Ʈ
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
	//reference position�� output reference���� ���� ���ϱ�
	err_theta = ref_theta - theta;
	derivative_theta_ref = (theta - theta_p) / ts; //V��� ���� ������ ���� ���ϱ�
	err_w = derivative_theta_ref + Kp*err_theta - speed; //�ӵ������� ���ϴ� ����. ������ ������ ����� �߰��ؾ� ��.

	//anti-windup �κ�
	
	out_tmp = Kv * err_w + Ki * integral;

	//anti-windup
	if (out_tmp >= output_max) out = output_max;
	else if (out_tmp <= output_min) out = output_min;
	else out = out_tmp;

	anti_w_err = out - out_tmp;

	integral += ts * (err_w + Ka * anti_w_err); //��������, anti-windup�� ���� �־�� ��.

	//theta_p = theta;
}