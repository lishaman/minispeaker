#ifndef __KD301_M03545_0317A_H__
#define __KD301_M03545_0317A_H__

/**
 * @gpio_lcd_spi_dr: [MANDATORY]
 * @gpio_lcd_spi_dt: [MANDATORY]
 * @gpio_lcd_spi_clk: [MANDATORY]
 * @gpio_lcd_spi_ce: [MANDATORY]
 * @gpio_lcd_reset: [MANDATORY]
 * @v33_reg_name: [OPTIONAL] LCD V3.3 regulator name,
 *                if V3.3 is constantly supplied, set this field to NULL
 */
struct platform_kd301_data {
	int gpio_lcd_spi_dr;
	int gpio_lcd_spi_dt;
	int gpio_lcd_spi_clk;
	int gpio_lcd_spi_ce;
	int gpio_lcd_reset;
	const char *v33_reg_name;
};

#endif	/* __KD301_M03545_0317A_H__ */
