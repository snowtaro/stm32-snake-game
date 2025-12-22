#include "ds1302.h"
#include "stm32f10x.h"

#define DS1302_PORT     GPIOB
#define DS1302_RCC      RCC_APB2Periph_GPIOB

#define DS1302_RST_PIN  GPIO_Pin_13  // CE (Chip Enable)
#define DS1302_DAT_PIN  GPIO_Pin_11  // I/O (Data)
#define DS1302_CLK_PIN  GPIO_Pin_12  // SCLK (Serial Clock)

// 매크로 함수 (핀 제어)
#define RST_L()     GPIO_ResetBits(DS1302_PORT, DS1302_RST_PIN)
#define RST_H()     GPIO_SetBits(DS1302_PORT, DS1302_RST_PIN)
#define CLK_L()     GPIO_ResetBits(DS1302_PORT, DS1302_CLK_PIN)
#define CLK_H()     GPIO_SetBits(DS1302_PORT, DS1302_CLK_PIN)
#define DAT_L()     GPIO_ResetBits(DS1302_PORT, DS1302_DAT_PIN)
#define DAT_H()     GPIO_SetBits(DS1302_PORT, DS1302_DAT_PIN)
#define DAT_READ()  GPIO_ReadInputDataBit(DS1302_PORT, DS1302_DAT_PIN)

// 레지스터 주소 정의 (Read 주소는 Write + 1)
#define DS_ADDR_SEC     0x80
#define DS_ADDR_MIN     0x82
#define DS_ADDR_HOUR    0x84
#define DS_ADDR_DATE    0x86
#define DS_ADDR_MONTH   0x88
#define DS_ADDR_DAY     0x8A // Weekday
#define DS_ADDR_YEAR    0x8C
#define DS_ADDR_WP      0x8E // Write Protect

// DS1302 RAM: 31 bytes, base 0xC0 (짝수 주소만 write, read는 |1)
#define DS_ADDR_RAM_BASE 0xC0
#define DS_RAM_SIZE      31

// ================================================================
// 2. 내부 헬퍼 함수 선언
// ================================================================
static void DS1302_GPIO_Config(void);
static void DS1302_IO_Output(void);
static void DS1302_IO_Input(void);
static void DS1302_WriteByte(uint8_t dat);
static uint8_t DS1302_ReadByte(void);
static void DS1302_WriteReg(uint8_t addr, uint8_t data);
static uint8_t DS1302_ReadReg(uint8_t addr);
static uint8_t DEC2BCD(uint8_t dec);
static uint8_t BCD2DEC(uint8_t bcd);

// ================================================================
// 3. Public 함수 구현
// ================================================================

void DS1302_Init(void)
{
    DS1302_GPIO_Config();

    // 초기 상태: RST=0, CLK=0
    RST_L();
    CLK_L();

    // 쓰기 방지 해제
    DS1302_WriteReg(DS_ADDR_WP, 0x00);

    {
        uint8_t sec_reg = DS1302_ReadReg(DS_ADDR_SEC | 0x01);
        if (sec_reg & 0x80) {
            // CH=1이면, 같은 초값 유지하면서 CH만 0으로
            uint8_t sec_bcd = sec_reg & 0x7F;
            DS1302_WriteReg(DS_ADDR_SEC, sec_bcd); // CH=0
        }
    }
}

void DS1302_GetTime(DS1302_Time_t *time)
{
    if (time == 0) return;

    uint8_t sec_reg   = DS1302_ReadReg(DS_ADDR_SEC   | 0x01);
    uint8_t min_reg   = DS1302_ReadReg(DS_ADDR_MIN   | 0x01);
    uint8_t hour_reg  = DS1302_ReadReg(DS_ADDR_HOUR  | 0x01);
    uint8_t date_reg  = DS1302_ReadReg(DS_ADDR_DATE  | 0x01);
    uint8_t month_reg = DS1302_ReadReg(DS_ADDR_MONTH | 0x01);
    uint8_t day_reg   = DS1302_ReadReg(DS_ADDR_DAY   | 0x01);
    uint8_t year_reg  = DS1302_ReadReg(DS_ADDR_YEAR  | 0x01);

    // 초 레지스터 bit7 = CH (Clock Halt) -> mask 필요
    time->second = BCD2DEC(sec_reg & 0x7F);
    time->minute = BCD2DEC(min_reg & 0x7F);

    // hour_reg:
    // bit7=0이면 24-hour mode, bit5..0가 hour
    // bit7=1이면 12-hour mode (여기서는 24h 기준으로만 처리)
    time->hour = BCD2DEC(hour_reg & 0x3F);

    time->day   = BCD2DEC(date_reg & 0x3F);
    time->month = BCD2DEC(month_reg & 0x1F);
    time->week  = BCD2DEC(day_reg & 0x07);
    time->year  = BCD2DEC(year_reg);
}

void DS1302_SetTime(DS1302_Time_t *time)
{
    if (time == 0) return;

    // WP 해제
    DS1302_WriteReg(DS_ADDR_WP, 0x00);

    // 24-hour mode 강제(bit7=0), 값도 범위 마스킹
    uint8_t hour_bcd = DEC2BCD(time->hour) & 0x3F; // 0~23만 쓰는 전제

    DS1302_WriteReg(DS_ADDR_YEAR,  DEC2BCD(time->year));
    DS1302_WriteReg(DS_ADDR_MONTH, DEC2BCD(time->month));
    DS1302_WriteReg(DS_ADDR_DATE,  DEC2BCD(time->day));
    DS1302_WriteReg(DS_ADDR_HOUR,  hour_bcd);
    DS1302_WriteReg(DS_ADDR_MIN,   DEC2BCD(time->minute));
    DS1302_WriteReg(DS_ADDR_DAY,   DEC2BCD(time->week));

    // 초 레지스터: CH(bit7)=0 강제 (시계 동작)
    DS1302_WriteReg(DS_ADDR_SEC, (DEC2BCD(time->second) & 0x7F));

    // WP 다시 설정
    DS1302_WriteReg(DS_ADDR_WP, 0x80);
}

void DS1302_ReadRAM(uint8_t offset, uint8_t *buf, uint8_t len)
{
    if (buf == 0 || len == 0) return;
    if (offset >= DS_RAM_SIZE) return;

    if ((uint16_t)offset + (uint16_t)len > DS_RAM_SIZE) {
        len = (uint8_t)(DS_RAM_SIZE - offset);
    }

    for (uint8_t i = 0; i < len; i++) {
        uint8_t addr_w = (uint8_t)(DS_ADDR_RAM_BASE + ((offset + i) * 2)); // write addr (even)
        buf[i] = DS1302_ReadReg(addr_w | 0x01); // read addr
    }
}

void DS1302_WriteRAM(uint8_t offset, const uint8_t *buf, uint8_t len)
{
    if (buf == 0 || len == 0) return;
    if (offset >= DS_RAM_SIZE) return;

    if ((uint16_t)offset + (uint16_t)len > DS_RAM_SIZE) {
        len = (uint8_t)(DS_RAM_SIZE - offset);
    }

    // WP 해제
    DS1302_WriteReg(DS_ADDR_WP, 0x00);

    for (uint8_t i = 0; i < len; i++) {
        uint8_t addr_w = (uint8_t)(DS_ADDR_RAM_BASE + ((offset + i) * 2)); // write addr (even)
        DS1302_WriteReg(addr_w, buf[i]);
    }

    // WP 다시 설정
    DS1302_WriteReg(DS_ADDR_WP, 0x80);
}

// ================================================================
// 4. 내부 헬퍼 함수 구현
// ================================================================

static void DS1302_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(DS1302_RCC, ENABLE);

    // RST, CLK 출력
    GPIO_InitStructure.GPIO_Pin = DS1302_RST_PIN | DS1302_CLK_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DS1302_PORT, &GPIO_InitStructure);

    // DAT 핀 초기 Output
    GPIO_InitStructure.GPIO_Pin = DS1302_DAT_PIN;
    GPIO_Init(DS1302_PORT, &GPIO_InitStructure);
}

static void DS1302_IO_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = DS1302_DAT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DS1302_PORT, &GPIO_InitStructure);
}

static void DS1302_IO_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = DS1302_DAT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // Pull-up 입력
    GPIO_Init(DS1302_PORT, &GPIO_InitStructure);
}

static void DS_Delay() {
  volatile int i;
  for (int i=0;i<50;i++) {__NOP();}
}

static void DS1302_WriteByte(uint8_t dat)
{
    DS1302_IO_Output();

    for (uint8_t i = 0; i < 8; i++)
    {
        CLK_L();
        DS_Delay();
        if (dat & 0x01) DAT_H();
        else            DAT_L();

        CLK_H(); // 상승 에지에서 래치
        DS_Delay();
        dat >>= 1;
        
    }
}

static uint8_t DS1302_ReadByte(void)
{
    uint8_t dat = 0;
    DS1302_IO_Input();

    for (uint8_t i = 0; i < 8; i++)
    {
        dat >>= 1;
        
        CLK_L();
        DS_Delay();
        if (DAT_READ()) dat |= 0x80;

        CLK_H(); // 상승 에지에서 래치
        DS_Delay();
    }
    return dat;
}

static void DS1302_WriteReg(uint8_t addr, uint8_t data)
{
    RST_L();
    CLK_L();
    RST_H();

    DS1302_WriteByte(addr);
    DS1302_WriteByte(data);

    RST_L();
}

static uint8_t DS1302_ReadReg(uint8_t addr)
{
    uint8_t data;

    RST_L();
    CLK_L();
    RST_H();

    DS1302_WriteByte(addr);
    data = DS1302_ReadByte();

    RST_L();
    return data;
}

static uint8_t BCD2DEC(uint8_t bcd)
{
    return (uint8_t)(((bcd >> 4) * 10) + (bcd & 0x0F));
}

static uint8_t DEC2BCD(uint8_t dec)
{
    return (uint8_t)(((dec / 10) << 4) | (dec % 10));
}
