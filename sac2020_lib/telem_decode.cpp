/**
 * Script for decoding telemetry dump from aux computer.
 * Usage: make clean; make decode; ./decode TELEM.DAT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "sac2020_state.h"

static char record[1024];

char* state_name(uint8_t state)
{
    switch (state)
    {
        case VehicleState_t::PRELTOFF: return "PRELTOFF";
        case VehicleState_t::PWFLIGHT: return "PWFLIGHT";
        case VehicleState_t::CRUISING: return "CRUISING";
        case VehicleState_t::CRSCANRD: return "CRSCANRD";
        case VehicleState_t::FALLDROG: return "FALLDROG";
        case VehicleState_t::FALLMAIN: return "FALLMAIN";
        case VehicleState_t::CONCLUDE: return "CONCLUDE";
        default:                       return "UNKNOWN";
    };
}

int main(int ac, char** av)
{
    FILE* p_fin = NULL;
    p_fin = fopen(av[1], "rb");

    FILE* p_fout = NULL;
    std::string fout_name = std::string(av[1]) + ".csv";
    p_fout = fopen(fout_name.c_str(), "w");
    fputs("Time,State,Filtered Altitude,Filtered Velocity,Filtered Acceleration,"
          "Pressure,Temperature,Barometer Altitude,IMU Temperature,"
          "Accel X,Accel Y,Accel Z,Accel Vertical,Gyro X,Gyro Y,Gyro Z,"
          "Quat W,Quat X,Quat Y,Quat Z,LP Altitude\n",
          p_fout);

    MainStateVector_t vec;
    uint32_t packet_count = 0;
    while (fread(&vec, sizeof(vec), 1, p_fin) > 0)
    {
        packet_count++;

        // Write to CSV.
        sprintf(record, "%.4f,%s,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%d,%.4f,%.4f,"
                        "%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n",
                        vec.time, state_name(vec.state), vec.altitude,
                        vec.velocity, vec.acceleration, vec.pressure,
                        vec.temperature, vec.baro_altitude, vec.imu_temp,
                        vec.accel_x, vec.accel_y, vec.accel_z,
                        vec.accel_vertical, vec.gyro_x, vec.gyro_y, vec.gyro_z,
                        vec.quat_w, vec.quat_x, vec.quat_y, vec.quat_z,
                        vec.launchpad_altitude);
        fputs(record, p_fout);
    }

    fclose(p_fin);
    fclose(p_fout);
    printf("Decoded %d telemetry packets\n", packet_count);
}
