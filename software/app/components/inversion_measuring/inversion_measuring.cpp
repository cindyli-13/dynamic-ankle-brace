#include "inversion_measuring.h"

void InversionMeasuring::calibrate(float imu1_accel_x, float imu1_accel_y,
                                   float imu1_accel_z, float imu2_accel_x,
                                   float imu2_accel_y, float imu2_accel_z) {
  // Reference: https://math.stackexchange.com/questions/180418/calculate-rotation-matrix-to-align-vector-a-to-vector-b-in-3d/

  Eigen::Vector3f imu1_g{imu1_accel_x, imu1_accel_y, imu1_accel_z};
  Eigen::Vector3f imu2_g{imu2_accel_x, imu2_accel_y, imu2_accel_z};
  imu1_g.normalize();
  imu2_g.normalize();

  Eigen::Vector3f v = imu2_g.cross(imu1_g);
  float c = imu2_g.dot(imu1_g);
  Eigen::Matrix3f w{{0, -v(2), v(1)},
                    {v(2), 0, -v(0)},
                    {-v(1), v(0), 0}};  //skew-symmetric matrix
  Rot_ = Eigen::Matrix3f::Identity() + w +
         w * w * (1 / (1 + c));  // rotation matrix from {imu2} to {imu1}
}

float InversionMeasuring::get_inversion_speed(
    float imu1_gyro_x, float imu1_gyro_y, float imu1_gyro_z, float imu2_gyro_x,
    float imu2_gyro_y, float imu2_gyro_z) {
  Eigen::Vector3f imu1_gyro_scaled{imu1_gyro_x, imu1_gyro_y, imu1_gyro_z};
  Eigen::Vector3f imu2_gyro_scaled{imu2_gyro_x, imu2_gyro_y, imu2_gyro_z};
  Eigen::Vector3f relative_angular_vel =
      Rot_ * imu2_gyro_scaled - imu1_gyro_scaled;

  // TODO: assume y axis of IMU 1 aligns with inversion axis
  // Verify this on actual setup
  return relative_angular_vel(0);
}

float InversionMeasuring::get_inversion_speed(shared::IMUData& imu_data) {
  return get_inversion_speed(imu_data.imu1.gyro.x, imu_data.imu1.gyro.y,
                             imu_data.imu1.gyro.z, imu_data.imu2.gyro.x,
                             imu_data.imu2.gyro.y, imu_data.imu2.gyro.z);
}
