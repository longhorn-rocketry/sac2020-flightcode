#include <photic.h>
#include <SPI.h>
#include <Servo.h>
#include <Wire.h>

#include "sac2020_baro.h"
#include "sac2020_imu.h"
#include "sac2020_main_pins.h"
#include "sac2020_lib.h"

/******************************** CONFIGURATION ********************************/

/**
 * Depth of Kalman gain calculation.
 */
#define KGAIN_CALC_DEPTH   100
/**
 * Variance in altimeter readings determined offboard. TODO
 */
#define POS_VARIANCE -111111111
/**
 * Variance in accelerometer readings determined offboard. TODO
 */
#define ACC_VARIANCE -222222222

/*********************************** GLOBALS **********************************/

/**
 * Hardware wrappers, configured during startup.
 */
Sac2020Imu g_imu;
Sac2020Barometer g_baro;
Servo g_servo1;
Servo g_servo2;

/**
 * Component statuses, set during startup.
 */
Status_t g_baro_status  = Status_t::OFFLINE; // BMP85 barometer.
Status_t g_imu_status   = Status_t::OFFLINE; // BNO055 IMU.
Status_t g_pyro1_status = Status_t::OFFLINE; // Pyro 1.
Status_t g_pyro2_status = Status_t::OFFLINE; // Pyro 2.
Status_t g_fnw_status   = Status_t::OFFLINE; // Flight computer network.

/**
 * Kalman filter for state estimation.
 */
photic::KalmanFilter g_kf;
/**
 * Kalman filter timer.
 */
photic::Metronome g_mtr_kf(10);

/********************************* FUNCTIONS **********************************/

/**
 * Gets the time elapsed since the program began in seconds.
 *
 * @ret     Time since epoch in seconds.
 */
float time()
{
    return millis() / 1000.0;
}

/**
 * Initializes the barometer and aborts on failure.
 */
void init_baro()
{
    if (!g_baro.init())
    {
        fault(PIN_LED_BARO_FAULT, "ERROR :: BMP085 INIT FAILED", g_baro_status);
        return;
    }

    g_baro_status = Status_t::ONLINE;
}

/**
 * Initializes the IMU and aborts on failure.
 */
void init_imu()
{
    if (!g_imu.init())
    {
        fault(PIN_LED_IMU_FAULT, "ERROR :: BNO055 INIT FAILED", g_imu_status);
        return;
    }

    g_imu_status = Status_t::ONLINE;
}

/**
 * Initializes the canard fin servos.
 */
void init_servos()
{
    g_servo1.attach(PIN_SERVO1_PWM);
    g_servo2.attach(PIN_SERVO2_PWM);
}

/**
 * Performs pyro continuity checks.
 */
void check_pyros()
{

}

/**
 * Function run by the LED pulse thread during startup and countdown.
 */
void pulse_thread_func()
{
    while (true)
    {
        pulse_leds(g_led_pulse_conf);
    }
}

/**
 * Lower all status LEDs.
 */
inline void lower_leds()
{
    digitalWrite(PIN_LED_BARO_FAULT, LOW);
    digitalWrite(PIN_LED_IMU_FAULT, LOW);
    digitalWrite(PIN_LED_PYRO1_FAULT, LOW);
    digitalWrite(PIN_LED_PYRO2_FAULT, LOW);
}

/*********************************** SETUP ************************************/

void setup()
{
#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.begin(115200);
    while (!DEBUG_SERIAL);
#endif

    // Initialize subsystems.
    init_baro();
    init_imu();
    init_servos();
    check_pyros();

    // Set up Kalman filter.
    g_kf.set_delta_t(g_mtr_kf.period());
    g_kf.set_sensor_variance(POS_VARIANCE, ACC_VARIANCE);
    g_kf.set_initial_estimate(LAUNCHPAD_ALTITUDE, 0, 0);
    g_kf.compute_kg(KGAIN_CALC_DEPTH);

    // Build status LED pulse configuration.
    g_led_pulse_conf.pulses = SYS_ONLINE_LED_PULSES;
    if (g_baro_status == Status::ONLINE)
    {
        g_led_pulse_conf.pins.push_back(PIN_LED_BARO_FAULT);
    }
    if (g_imu_status == Status::ONLINE)
    {
        g_led_pulse_conf.pins.push_back(PIN_LED_IMU_FAULT);
    }
    if (g_pyro1_status == Status::ONLINE)
    {
        g_led_pulse_conf.pins.push_back(PIN_LED_PYRO1_FAULT);
    }
    if (g_pyro2_status == Status::ONLINE)
    {
        g_led_pulse_conf.pins.push_back(PIN_LED_PYRO2_FAULT);
    }

    // Determine if everything initialized correctly.
    bool ok = g_baro_status  == Status_t::ONLINE &&
              g_imu_status   == Status_t::ONLINE &&
              g_pyro1_status == Status_t::ONLINE &&
              g_pyro2_status == Status_t::ONLINE;

#ifdef USING_FNW
    // Send the aux computer my status.
    token_t tok_status = ok ? FNW_TOKEN_AOK : FNW_TOKEN_ERR;
    FNW_SERIAL.begin(FNW_BAUD);
    FNW_SERIAL.write(&tok_status, sizeof(token_t));

    // Wait for confirmation from aux computer.
    while (FNW_SERIAL.peek() == -1);
    token_t tok = FNW_TOKEN_NIL;
    FNW_SERIAL.readBytes(&tok, sizeof(token_t));

    if (tok != FNW_TOKEN_AOK)
    {
        fault(PIN_LED_FNW_FAULT, "ERROR :: FAILED TO HANDSHAKE WITH AUX",
              g_fnw_status);
    }
    else
    {
        g_fnw_status = Status_t::ONLINE;
        g_led_pulse_conf.pins.push_back(PIN_LED_FNW_FAULT);
    }
#endif

    // Dispatch LED pulse thread.
    int32_t pulse_thread_id = threads.addThread(pulse_thread_func);
    if (pulse_thread_id == -1)
    {
    #ifdef DEBUG_SERIAL
        DEBUG_SERIAL.println("ERROR :: FAILED TO CREATE PULSE THREAD");
    #endif
        exit(1);
    }

    // TODO
    while (true);

    // Join the LED pulse thread to ensure we have full command over DIO during
    // flight without the need to synchronize.
    threads.kill(pulse_thread_id);
    threads.wait(pulse_thread_id, 1000); // Cap wait time at 1000 ms.

    // Lower all LEDs to conserve power.
    lower_leds();
}

/************************************ LOOP ************************************/

void loop()
{
    double t = time();

    if (g_mtr_kf.poll(t))
    {

    }
}
