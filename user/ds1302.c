#include "ds1302.h"
#include "stm32f10x.h"

// ================================================================
// 1. 핀 설정 (회로에 맞게 수정하세요)
// ================================================================
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
// 3. Public 함수 구현 (헤더 파일에 정의된 함수)
// ================================================================

void DS1302_Init(void)
{
    DS1302_GPIO_Config();

    // 초기 상태: RST=0, CLK=0
    RST_L();
    CLK_L();

    // 쓰기 방지 해제 (WP 비트 클리어)
    DS1302_WriteReg(DS_ADDR_WP, 0x00);
    
    // 필요 시 오실레이터 정지 비트 해제 등의 초기화 코드를 넣을 수 있음
    // 예: 초 레지스터의 최상위 비트(CH)가 1이면 시계가 멈춤 -> 0으로 설정 필요
}

void DS1302_GetTime(DS1302_Time_t *time)
{
    // 각 레지스터 읽기 (Read 주소 = Write 주소 | 0x01)
    uint8_t sec_reg  = DS1302_ReadReg(DS_ADDR_SEC | 0x01);
    uint8_t min_reg  = DS1302_ReadReg(DS_ADDR_MIN | 0x01);
    uint8_t hour_reg = DS1302_ReadReg(DS_ADDR_HOUR | 0x01);
    uint8_t date_reg = DS1302_ReadReg(DS_ADDR_DATE | 0x01);
    uint8_t month_reg= DS1302_ReadReg(DS_ADDR_MONTH | 0x01);
    uint8_t day_reg  = DS1302_ReadReg(DS_ADDR_DAY | 0x01);
    uint8_t year_reg = DS1302_ReadReg(DS_ADDR_YEAR | 0x01);

    // BCD -> 10진수 변환 후 구조체에 저장
    // 초 레지스터의 7번 비트는 CH(Clock Halt)이므로 마스킹(0x7F) 처리 필요
    time->second = BCD2DEC(sec_reg & 0x7F);
    time->minute = BCD2DEC(min_reg);
    // 12/24 시간 모드에 따라 다를 수 있으나, 일반적으로 24시간 모드 가정
    time->hour   = BCD2DEC(hour_reg & 0x3F); 
    time->day    = BCD2DEC(date_reg);
    time->month  = BCD2DEC(month_reg);
    time->week   = BCD2DEC(day_reg); // 요일 (1~7)
    time->year   = BCD2DEC(year_reg);
}

// (추가) 시간을 설정하는 함수도 필요할 것입니다. (헤더에 선언 필요)
void DS1302_SetTime(DS1302_Time_t *time)
{
    DS1302_WriteReg(DS_ADDR_WP, 0x00); // 쓰기 방지 해제

    DS1302_WriteReg(DS_ADDR_YEAR, DEC2BCD(time->year));
    DS1302_WriteReg(DS_ADDR_MONTH, DEC2BCD(time->month));
    DS1302_WriteReg(DS_ADDR_DATE, DEC2BCD(time->day));
    DS1302_WriteReg(DS_ADDR_HOUR, DEC2BCD(time->hour));
    DS1302_WriteReg(DS_ADDR_MIN, DEC2BCD(time->minute));
    DS1302_WriteReg(DS_ADDR_DAY, DEC2BCD(time->week));
    DS1302_WriteReg(DS_ADDR_SEC, DEC2BCD(time->second)); // 0x80을 쓰면 CH=0 되어 오실레이터 시작

    DS1302_WriteReg(DS_ADDR_WP, 0x80); // 쓰기 방지 설정
}

// ================================================================
// 4. 내부 헬퍼 함수 구현
// ================================================================

static void DS1302_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(DS1302_RCC, ENABLE);

    // RST, CLK는 항상 출력
    GPIO_InitStructure.GPIO_Pin = DS1302_RST_PIN | DS1302_CLK_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DS1302_PORT, &GPIO_InitStructure);

    // DAT 핀은 초기 Output 설정
    GPIO_InitStructure.GPIO_Pin = DS1302_DAT_PIN;
    GPIO_Init(DS1302_PORT, &GPIO_InitStructure);
}

// 데이터 핀을 출력 모드로 변경
static void DS1302_IO_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = DS1302_DAT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DS1302_PORT, &GPIO_InitStructure);
}

// 데이터 핀을 입력 모드로 변경
static void DS1302_IO_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = DS1302_DAT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // Pull-up 입력
    GPIO_Init(DS1302_PORT, &GPIO_InitStructure);
}

static void DS1302_WriteByte(uint8_t dat)
{
    uint8_t i;
    DS1302_IO_Output();

    for (i = 0; i < 8; i++)
    {
        CLK_L(); // 클럭 Low
        if (dat & 0x01) DAT_H();
        else            DAT_L();
        
        dat >>= 1;
        CLK_H(); // 클럭 High (상승 에지에서 데이터 래치)
    }
}

static uint8_t DS1302_ReadByte(void)
{
    uint8_t i, dat = 0;
    DS1302_IO_Input();

    for (i = 0; i < 8; i++)
    {
        dat >>= 1;
        CLK_H(); // 클럭 High
        CLK_L(); // 클럭 Low (하강 에지에서 데이터 출력됨)
        
        if (DAT_READ()) dat |= 0x80;
    }
    return dat;
}

static void DS1302_WriteReg(uint8_t addr, uint8_t data)
{
    RST_L();
    CLK_L();
    RST_H(); // 통신 시작

    DS1302_WriteByte(addr); // 주소 전송
    DS1302_WriteByte(data); // 데이터 전송

    RST_L(); // 통신 종료
}

static uint8_t DS1302_ReadReg(uint8_t addr)
{
    uint8_t data;
    
    RST_L();
    CLK_L();
    RST_H(); // 통신 시작

    DS1302_WriteByte(addr); // 주소 전송
    data = DS1302_ReadByte(); // 데이터 수신

    RST_L(); // 통신 종료
    return data;
}

// BCD -> Decimal 변환 (예: 0x15 -> 15)
static uint8_t BCD2DEC(uint8_t bcd)
{
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

// Decimal -> BCD 변환 (예: 15 -> 0x15)
static uint8_t DEC2BCD(uint8_t dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}