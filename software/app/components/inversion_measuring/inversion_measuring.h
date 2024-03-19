#pragma once

#include <eigen3/Eigen/Eigen>
#include "shared.h"

class InversionMeasuring {
 public:
  InversionMeasuring() : Rot_(Eigen::Matrix3f::Identity()) {}

  // To calibrate, keep foot still in rest position and record the average accelerometer
  // readings from both IMUs (over ~3 sec). Pass the obtained average readings to
  // the calibrate() method.
  void calibrate(float imu1_accel_x, float imu1_accel_y, float imu1_accel_z,
                 float imu2_accel_x, float imu2_accel_y, float imu2_accel_z);

  // Returns inversion speed in deg/s
  float get_inversion_speed(float imu1_gyro_x, float imu1_gyro_y,
                            float imu1_gyro_z, float imu2_gyro_x,
                            float imu2_gyro_y, float imu2_gyro_z);

  // convenience overload
  float get_inversion_speed(shared::IMUData& imu_data);

 private:
  Eigen::Matrix3f Rot_;
};
