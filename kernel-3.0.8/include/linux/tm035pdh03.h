#ifndef __TM035PDH03_H__
#define __TM035PDH03_H__

/**
 * @gpio_lcd_spi_dr: [MANDATORY]
 * @gpio_lcd_spi_dt: [MANDATORY]
 * @gpio_lcd_spi_clk: [MANDATORY]
 * @gpio_lcd_spi_ce: [MANDATORY]
 * @gpio_lcd_reset: [MANDATORY]
 * @v33_reg_name: [OPTIONAL] LCD V3.3 regulator name,
 *                if V3.3 is constantly supplied, set this field to NULL
 */
struct platform_tm035_data {
	int gpio_lcd_spi_dr;
	int gpio_lcd_spi_dt;
	int gpio_lcd_spi_clk;
	int gpio_lcd_spi_ce;
	int gpio_lcd_reset;
	const char *v33_reg_name;
};

#endif	/* __TM035PDH03_H__ */
