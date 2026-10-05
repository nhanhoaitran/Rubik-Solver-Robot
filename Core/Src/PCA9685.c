#include "PCA9685.h"

static float pca9685_pwm_frequency = 50.0f;

static HAL_StatusTypeDef PCA9685_ReadRegister(
		I2C_HandleTypeDef *hi2c,
		uint8_t reg,
		uint8_t *value
		)
{
	if ((hi2c == NULL) || (value == NULL))
	{
		return HAL_ERROR;
	}

	return HAL_I2C_Mem_Read(
			hi2c,
			PCA9685_ADDR,
			reg,
			I2C_MEMADD_SIZE_8BIT,
			value,
			1,
			100
			);
}

static HAL_StatusTypeDef PCA9685_WriteRegister(
		I2C_HandleTypeDef *hi2c,
		uint8_t reg,
		uint8_t value
		)
{
	if(hi2c == NULL)
	{
		return HAL_ERROR;
	}

	return HAL_I2C_Mem_Write(
			hi2c,
			PCA9685_ADDR,
			reg,
			I2C_MEMADD_SIZE_8BIT,
			&value,
			1,
			100
			);
}

static HAL_StatusTypeDef PCA9685_SetBit(
		I2C_HandleTypeDef *hi2c,
		uint8_t reg,
		uint8_t bit,
		uint8_t value
		)
{
	uint8_t data;

	HAL_StatusTypeDef status;
	if (bit > 7U)
	{
		return HAL_ERROR;
	}

	status = PCA9685_ReadRegister(
			hi2c,
			reg,
			&data
			);

	if (status != HAL_OK)
	{
		return status;
	}
	if (value == 0U)
	{
		data &= (uint8_t)~(1U << bit);
	}
	else
	{
		data |= (uint8_t)(1U << bit);
	}
	return PCA9685_WriteRegister(
			hi2c,
			reg,
			data);
}

HAL_StatusTypeDef PCA9685_IsReady(I2C_HandleTypeDef *hi2c)
{
	if (hi2c == NULL)
	{
		return HAL_ERROR;
	}
	return HAL_I2C_IsDeviceReady(
			hi2c,
			PCA9685_ADDR,
			3,
			100);
}

HAL_StatusTypeDef PCA9685_SetPWMFrequency(
		I2C_HandleTypeDef *hi2c,
		float frequency)
{
	uint8_t old_mode;
	uint8_t sleep_mode;
	uint8_t prescale;

	float prescale_value;

	HAL_StatusTypeDef status;

	if(hi2c == NULL)
	{
		return HAL_ERROR;
	}

	/*
	     * PCA9685 prescale valid range is 3...255.
	     *
	     * With a nominal 25 MHz oscillator, useful PWM range is
	     * approximately 24 Hz ... 1526 Hz.
	 */
	if (frequency < 24.0f)
	{
		frequency = 24.0f;
	}
	if (frequency > 1526.0f)
	{
		frequency = 1526.0f;
	}

	prescale_value = (PCA9685_OSC_FREQ / (PCA9685_RESOLUTION * frequency)) - 1.0f;

	prescale = (uint8_t)(prescale_value + 0.5f);
	status = PCA9685_ReadRegister(
			hi2c,
			PCA9685_MODE1,
			&old_mode
			);
	if(status != HAL_OK)
	{
		return status;
	}
	/*
	     * Enter sleep before changing PRE_SCALE.
	     *
	     * Set SLEEP bit (bit 4).
	*/
	sleep_mode = old_mode | (1U << PCA9685_MODE1_SLEEP_BIT);

	status = PCA9685_WriteRegister(
			hi2c,
			PCA9685_MODE1,
			sleep_mode
			);
	if (status != HAL_OK)
	{
		return status;
	}
	/*
	     * Write prescale register.
	*/
	status = PCA9685_WriteRegister(
			hi2c,
			PCA9685_PRE_SCALE,
			prescale
			);
	if (status != HAL_OK)
	{
		return status;
	}

	 /*
	     * Restore MODE1 with SLEEP cleared.
	 */
	status = PCA9685_WriteRegister(
			hi2c,
			PCA9685_MODE1,
			old_mode & (uint8_t)~(1U << PCA9685_MODE1_SLEEP_BIT)
			);
	if (status != HAL_OK)
	{
		return status;
	}

	 /*
	     * Oscillator requires time to stabilize after wake-up.
	 */
	HAL_Delay(1);

	/*
	     * Set RESTART bit.
	 */
	status = PCA9685_SetBit(
			hi2c,
			PCA9685_MODE1,
			PCA9685_MODE1_RESTART_BIT,
			1U);
	if (status != HAL_OK)
	{
		return status;
	}
	pca9685_pwm_frequency =
	            PCA9685_OSC_FREQ /
	            (PCA9685_RESOLUTION *
	            ((float)prescale + 1.0f));

	 return HAL_OK;
}

HAL_StatusTypeDef PCA9685_Init(
        I2C_HandleTypeDef *hi2c,
        float frequency)
{
    HAL_StatusTypeDef status;


    /*
     * First check whether PCA9685 is actually present.
     */
    status = PCA9685_IsReady(hi2c);

    if (status != HAL_OK)
    {
        return status;
    }

    /*
     * Enable Auto-Increment.
     */
    status = PCA9685_SetBit(
            hi2c,
            PCA9685_MODE1,
            PCA9685_MODE1_AI_BIT,
            1U);

    if (status != HAL_OK)
    {
        return status;
    }

    /*
     * Configure PWM frequency.
     */
    status = PCA9685_SetPWMFrequency(
            hi2c,
            frequency);

    if (status != HAL_OK)
    {
        return status;
    }


    return HAL_OK;
}

HAL_StatusTypeDef PCA9685_SetPWM(
        I2C_HandleTypeDef *hi2c,
        uint8_t channel,
        uint16_t on,
        uint16_t off)
{
    uint8_t register_address;
    uint8_t pwm[4];


    if (hi2c == NULL)
    {
        return HAL_ERROR;
    }


    if (channel > PCA9685_MAX_CHANNEL)
    {
        return HAL_ERROR;
    }


    /*
     * PCA9685 has a 12-bit PWM counter:
     *
     * 0 ... 4095
     */
    if ((on > 4095U) || (off > 4095U))
    {
        return HAL_ERROR;
    }


    /*
     * Each channel uses four consecutive registers.
     *
     * Channel 0 starts at 0x06.
     *
     * Channel n:
     *
     * 0x06 + 4*n
     */
    register_address =
            PCA9685_LED0_ON_L +
            (4U * channel);


    /*
     * Split 12-bit values into low and high bytes.
     *
     * Only the lower 4 bits of ON_H / OFF_H contain
     * the counter value.
     */
    pwm[0] = (uint8_t)(on & 0xFFU);
    pwm[1] = (uint8_t)((on >> 8U) & 0x0FU);

    pwm[2] = (uint8_t)(off & 0xFFU);
    pwm[3] = (uint8_t)((off >> 8U) & 0x0FU);


    return HAL_I2C_Mem_Write(
            hi2c,
            PCA9685_ADDR,
            register_address,
            I2C_MEMADD_SIZE_8BIT,
            pwm,
            4,
            100);
}


/*
 * Set servo pulse width directly in microseconds
 */

HAL_StatusTypeDef PCA9685_SetServoPulse(
        I2C_HandleTypeDef *hi2c,
        uint8_t channel,
        float pulse_us)
{
    float period_us;
    float counts_float;

    uint16_t counts;


    if (pca9685_pwm_frequency <= 0.0f)
    {
        return HAL_ERROR;
    }

    period_us =
            1000000.0f /
            pca9685_pwm_frequency;


    /*
     * One full period = 4096 PCA9685 counts.
     *
     * Therefore:
     *
     * counts =
     * pulse_us / period_us * 4096
     */
    counts_float =
            (pulse_us / period_us)
            * PCA9685_RESOLUTION;

    /*
     * Round to nearest count.
     */
    counts =
            (uint16_t)(counts_float + 0.5f);

    return PCA9685_SetPWM(
            hi2c,
            channel,
            0U,
            counts);
}


/*
 * Servo angle with configurable calibration
 */
HAL_StatusTypeDef PCA9685_SetServoAngleCalibrated(
        I2C_HandleTypeDef *hi2c,
        uint8_t channel,
        float angle,
        float min_pulse_us,
        float max_pulse_us)
{
    float pulse_us;


    /*
     * Clamp requested angle.
     */
    if (angle < SERVO_MIN_ANGLE)
    {
        angle = SERVO_MIN_ANGLE;
    }

    if (angle > SERVO_MAX_ANGLE)
    {
        angle = SERVO_MAX_ANGLE;
    }


    if (min_pulse_us >= max_pulse_us)
    {
        return HAL_ERROR;
    }


    /*
     * Linear interpolation:
     *
     * angle = 0°
     *     -> min_pulse_us
     *
     * angle = 180°
     *     -> max_pulse_us
     */
    pulse_us =
            min_pulse_us +
            ((angle / 180.0f) *
            (max_pulse_us - min_pulse_us));


    return PCA9685_SetServoPulse(
            hi2c,
            channel,
            pulse_us);
}


/*
 * Servo angle using default calibration
 */

HAL_StatusTypeDef PCA9685_SetServoAngle(
        I2C_HandleTypeDef *hi2c,
        uint8_t channel,
        float angle)
{
    return PCA9685_SetServoAngleCalibrated(
            hi2c,
            channel,
            angle,
            SERVO_DEFAULT_MIN_US,
            SERVO_DEFAULT_MAX_US);
}
