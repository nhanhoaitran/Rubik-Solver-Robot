#ifndef INC_PCA9685_H
#define INC_PCA9685_H

#include "stm32f4xx_hal.h"

/* PCA9685 I2C address */
#define PCA9685_ADDR					(0x40U << 1)

/* PCA9685 registers */
#define PCA9685_MODE1					0x00U
#define PCA9685_MODE2					0x01U

#define PCA9685_LED0_ON_L				0x06U
#define PCA9685_PRE_SCALE 				0xFEU

/* MODE1 bit positions */
#define PCA9685_MODE1_SLEEP_BIT			4U
#define PCA9685_MODE1_AI_BIT			5U
#define PCA9685_MODE1_RESTART_BIT		7U

/* PCA9685 characteristics */
#define PCA9685_OSC_FREQ				25000000.0f
#define PCA9685_RESOLUTION				4096.0f

#define PCA9685_MIN_CHANNEL				0U
#define PCA9685_MAX_CHANNEL				15U

/* Default servo configuration */
#define SERVO_DEFAULT_MIN_US			500.0f
#define SERVO_DEFAULT_MAX_US			2500.0f

#define SERVO_MIN_ANGLE					0.0f
#define SERVO_MAX_ANGLE					180.0f

/* Public functions */

HAL_StatusTypeDef PCA9685_Init(
		I2C_HandleTypeDef *hi2c,
		float frequency
		);

HAL_StatusTypeDef PCA9685_SetPWMFrequency(
		I2C_HandleTypeDef *hi2c,
		float frequency
		);

HAL_StatusTypeDef PCA9685_SetPWM(
		I2C_HandleTypeDef *hi2c,
		uint8_t channel,
		uint16_t on,
		uint16_t off
		);

HAL_StatusTypeDef PCA9685_SetServoPulse(
		I2C_HandleTypeDef *hi2c,
		uint8_t channel,
		float pulse_us
		);

HAL_StatusTypeDef PCA9685_SetServoAngle(
		I2C_HandleTypeDef *hi2c,
		uint8_t channel,
		float angle
		);

HAL_StatusTypeDef PCA9685_SetServoAngleCalibrated(
        I2C_HandleTypeDef *hi2c,
        uint8_t channel,
        float angle,
        float min_pulse_us,
        float max_pulse_us
		);

HAL_StatusTypeDef PCA9685_IsReady(
		I2C_HandleTypeDef *hi2c
		);

#endif


