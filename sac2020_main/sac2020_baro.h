#ifndef SAC2020_BARO_H
#define SAC2020_BARO_H

#include <Adafruit_BMP085.h>
#include <photic.h>

class Sac2020Barometer final : public photic::Barometer
{
public:
    /**
     * Initializes the BMP085.
     *
     * @ret     True if successful, false otherwise.
     */
    bool init()
    {
        return m_bmp085.begin(0x00);
    }

    /**
     * Reads in all data from the BMP085.
     *
     * @ret     Always returns true.
     */
    bool update()
    {
        m_data.pressure = m_bmp085.readPressure();
        m_data.temperature = m_bmp085.readTemperature();
        m_data.altitude = m_bmp085.readAltitude();

        return true;
    }

private:
    /**
     * BMP085 driver.
     */
    Adafruit_BMP085 m_bmp085;
};

#endif