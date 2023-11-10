#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <drivers/gpio.h>
#include "mt_i2c.h"
#include "ds3231.h"

#define RTC_I2C_SLAVE	      0x68
#define RTC_I2C_SLAVE_ADDRLEN 1
#define DS3231_REG_SECONDS    0x0
#define DS3231_REG_MINUTES    0x1
#define DS3231_REG_HOURS      0x2
#define DS3231_REG_DAY	      0x3
#define DS3231_REG_DATE	      0x4
#define DS3231_REG_MONTH      0x5
#define DS3231_REG_YEAR	      0x6
#define DS3231_REG_MASK	      0xff

const char *day_names[] = { "Mon", "Tue", "Wed", "Thr", "Fir", "Sat", "Sun" };
const char *month_names[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
			      "Jul", "Aug", "Sep", "",	  "",	 "",
			      "",    "",    "",	   "Oct", "Nov", "Dec" };

static void show_current_date(void)
{
	uint8_t val = 0;
	uint8_t D = 0, m = 0, dd = 0, hh = 0, mm = 0, ss = 0, yy = 0;
	uint8_t chip = RTC_I2C_SLAVE;
	int addrlen = RTC_I2C_SLAVE_ADDRLEN;

	mtk_i2c_read_reg(chip, DS3231_REG_SECONDS, addrlen, &val);
	ss = val & DS3231_REG_MASK;
	mtk_i2c_read_reg(chip, DS3231_REG_MINUTES, addrlen, &val);
	mm = val & DS3231_REG_MASK;
	mtk_i2c_read_reg(chip, DS3231_REG_HOURS, addrlen, &val);
	hh = val & DS3231_REG_MASK;
	mtk_i2c_read_reg(chip, DS3231_REG_DAY, addrlen, &val);
	D = val & DS3231_REG_MASK;
	mtk_i2c_read_reg(chip, DS3231_REG_DATE, addrlen, &val);
	dd = val & DS3231_REG_MASK;
	mtk_i2c_read_reg(chip, DS3231_REG_MONTH, addrlen, &val);
	m = val & DS3231_REG_MASK;
	mtk_i2c_read_reg(chip, DS3231_REG_YEAR, addrlen, &val);
	yy = val & DS3231_REG_MASK;
	NOTICE("HW_RTC: [%s %s %x %x:%x:%x %s %x]\n", day_names[D],
	       month_names[m], dd, hh, mm, ss, "CTS", yy);
}

int ds3231_hwrtc_test(void)
{
	uint8_t chip = RTC_I2C_SLAVE;
	int addrlen = RTC_I2C_SLAVE_ADDRLEN;

	mtk_i2c_write_reg(chip, DS3231_REG_SECONDS, addrlen, 0x10);
	show_current_date();
	udelay(1000000);
	show_current_date();
	udelay(1000000);
	show_current_date();

	return 0;
}
