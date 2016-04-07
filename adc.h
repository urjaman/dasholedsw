#define ADC_CH_MB 0
#define ADC_CH_SB 1
uint16_t adc_read_mb(void);
uint16_t adc_read_sb(void);
int16_t adc_read_diff(void);
uint16_t adc_read_minv(uint8_t ch);
uint16_t adc_read_maxv(uint8_t ch);

void adc_print_v(unsigned char* buf, uint16_t v);
void adc_init(void);
void adc_run(void);
uint16_t adc_to_dV(uint16_t v);
uint16_t adc_from_dV(uint16_t v);
void adc_print_dV(unsigned char* buf, uint16_t v);

// LOTS of places need to be changed if ADC_MUX_CNT!=2.
#define ADC_MUX_CNT 2
extern int16_t adc_calibration_diff[ADC_MUX_CNT];

