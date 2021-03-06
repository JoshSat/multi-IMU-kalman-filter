#include "MPU9250.h"
#include <Wire.h>
#include <Kalman.h> // Source: https://github.com/TKJElectronics/KalmanFilter


  
// an MPU9250 object with the MPU-9250 sensor on I2C bus 0 with address 0x68 and 0x69
MPU9250 IMU0(Wire,0x68);
MPU9250 IMU1(Wire,0x69);
int status;

double accX0, accY0, accZ0;
double gyroX0, gyroY0, gyroZ0;
double accX1, accY1, accZ1;
double gyroX1, gyroY1, gyroZ1;
double temp0, temp1, tempF;
double gyroXangle0, gyroYangle0; // Angle calculate using the gyro only
double compAngleX0, compAngleY0; // Calculated angle using a complementary filter
double kalAngleX0, kalAngleY0; // Calculated angle using a Kalman filter
double gyroXangle1, gyroYangle1; // Angle calculate using the gyro only
double compAngleX1, compAngleY1; // Calculated angle using a complementary filter
double kalAngleX1, kalAngleY1; // Calculated angle using a Kalman filter
double x;
double y;

byte counter = 1; 

uint32_t timer;
Kalman kalmanX0; // Create the Kalman instances 0
Kalman kalmanY0;
Kalman kalmanX1; // Create the Kalman instances 1
Kalman kalmanY1;


void setup() {
  // serial to display data
  Serial.begin(115200);
  while(!Serial) {}

  // start communication with IMU0 
  status = IMU0.begin();
  if (status < 0) {
    Serial.println("IMU initialization unsuccessful");
    Serial.println("Check IMU0 wiring or try cycling power");
    Serial.print("Status: ");
    Serial.println(status);
    while(1) {}
  }
  // start communication with IMU1
    status = IMU1.begin();
  if (status < 0) {
    Serial.println("IMU initialization unsuccessful");
    Serial.println("Check IMU1 wiring or try cycling power");
    Serial.print("Status: ");
    Serial.println(status);
    while(1) {}
  }    
  Serial.println("initialization of sensors done.");
  Serial.println("no issues detected.");
  // setting the accelerometer full scale range to +/-8G 
  IMU0.setAccelRange(MPU9250::ACCEL_RANGE_8G);
  // setting the gyroscope full scale range to +/-500 deg/s
  IMU0.setGyroRange(MPU9250::GYRO_RANGE_500DPS);
  // setting DLPF bandwidth to 20 Hz
  IMU0.setDlpfBandwidth(MPU9250::DLPF_BANDWIDTH_20HZ);
  // setting SRD to 19 for a 50 Hz update rate
  IMU0.setSrd(19);
  // setting the accelerometer full scale range to +/-8G 
  IMU1.setAccelRange(MPU9250::ACCEL_RANGE_8G);
  // setting the gyroscope full scale range to +/-500 deg/s
  IMU1.setGyroRange(MPU9250::GYRO_RANGE_500DPS);
  // setting DLPF bandwidth to 20 Hz
  IMU1.setDlpfBandwidth(MPU9250::DLPF_BANDWIDTH_20HZ);
  // setting SRD to 19 for a 50 Hz update rate
  IMU1.setSrd(19);
  IMU0.readSensor();
  IMU1.readSensor();
  #ifdef RESTRICT_PITCH // Eq. 25 and 26
  double roll0  = atan2(accY0, accZ0) * RAD_TO_DEG;
  double pitch0 = atan(-accX0 / sqrt(accY0 * accY0 + accZ0 * accZ0)) * RAD_TO_DEG;
#else // Eq. 28 and 29
  double roll0  = atan(accY0 / sqrt(accX0 * accX0 + accZ0 * accZ0)) * RAD_TO_DEG;
  double pitch0 = atan2(-accX0, accZ0) * RAD_TO_DEG;
#endif
  kalmanX0.setAngle(roll0); // Set starting angle
  kalmanY0.setAngle(pitch0);
  gyroXangle0 = roll0;
  gyroYangle0 = pitch0;
  compAngleX0 = roll0;
  compAngleY0 = pitch0;

  timer = micros();
  
  #ifdef RESTRICT_PITCH // Eq. 25 and 26
  double roll1  = atan2(accY1, accZ1) * RAD_TO_DEG;
  double pitch1 = atan(-accX1 / sqrt(accY1 * accY1 + accZ1 * accZ1)) * RAD_TO_DEG;
#else // Eq. 28 and 29
  double roll1  = atan(accY1 / sqrt(accX1 * accX1 + accZ1 * accZ1)) * RAD_TO_DEG;
  double pitch1 = atan2(-accX1, accZ1) * RAD_TO_DEG;
#endif
  kalmanX1.setAngle(roll1); // Set starting angle
  kalmanY1.setAngle(pitch1);
  gyroXangle1 = roll1;
  gyroYangle1 = pitch1;
  compAngleX1 = roll1;
  compAngleY1 = pitch1;
///////////////////////////////////////////////////////
  timer = micros();
}

void loop() {
  
  // read the sensor
  IMU0.readSensor();
  IMU1.readSensor();

  accX0 = IMU0.getAccelX_mss();
  accY0 = IMU0.getAccelY_mss();
  accZ0 = IMU0.getAccelZ_mss();
  accX1 = IMU1.getAccelX_mss();
  accY1 = IMU1.getAccelY_mss();
  accZ1 = IMU1.getAccelZ_mss();

  gyroX0 = IMU0.getGyroX_rads();
  gyroY0 = IMU0.getGyroY_rads();
  gyroZ0 = IMU0.getGyroZ_rads();
  gyroX1 = IMU1.getGyroX_rads();
  gyroY1 = IMU1.getGyroY_rads();
  gyroZ1 = IMU1.getGyroZ_rads();

  temp0 = IMU0.getTemperature_C();
  temp1 = IMU1.getTemperature_C();


  double dt = (double)(micros() - timer) / 1000000; // Calculate delta time
  timer = micros();

  // Source: http://www.freescale.com/files/sensors/doc/app_note/AN3461.pdf eq. 25 and eq. 26
  // atan2 outputs the value of -π to π (radians) - see http://en.wikipedia.org/wiki/Atan2
  // It is then converted from radians to degrees
#ifdef RESTRICT_PITCH // Eq. 25 and 26
  double roll0  = atan2(accY0, accZ0) * RAD_TO_DEG;
  double pitch0 = atan(-accX0 / sqrt(accY0 * accY0 + accZ0 * accZ0)) * RAD_TO_DEG;
#else // Eq. 28 and 29
  double roll0  = atan(accY0 / sqrt(accX0 * accX0 + accZ0 * accZ0)) * RAD_TO_DEG;
  double pitch0 = atan2(-accX0, accZ0) * RAD_TO_DEG;
#endif

  double gyroXrate0 = gyroX0 / 131.0; // Convert to deg/s
  double gyroYrate0 = gyroY0 / 131.0; // Convert to deg/s

#ifdef RESTRICT_PITCH
  // This fixes the transition problem when the accelerometer angle jumps between -180 and 180 degrees
  if ((roll0 < -90 && kalAngleX0 > 90) || (roll0 > 90 && kalAngleX0 < -90)) {
    kalmanX0.setAngle(roll0);
    compAngleX0 = roll0;
    kalAngleX0 = roll0;
    gyroXangle0 = roll0;
  } else
    kalAngleX0 = kalmanX0.getAngle(roll0, gyroXrate0, dt); // Calculate the angle using a Kalman filter

  if (abs(kalAngleX) > 90)
    gyroYrate0 = -gyroYrate0; // Invert rate, so it fits the restriced accelerometer reading
  kalAngleY0 = kalmanY0.getAngle(pitch0, gyroYrate0, dt);
#else
  // This fixes the transition problem when the accelerometer angle jumps between -180 and 180 degrees
  if ((pitch0 < -90 && kalAngleY0 > 90) || (pitch0 > 90 && kalAngleY0 < -90)) {
    kalmanY0.setAngle(pitch0);
    compAngleY0 = pitch0;
    kalAngleY0 = pitch0;
    gyroYangle0 = pitch0;
  } else
    kalAngleY0 = kalmanY0.getAngle(pitch0, gyroYrate0, dt); // Calculate the angle using a Kalman filter

  if (abs(kalAngleY0) > 90)
    gyroXrate0 = -gyroXrate0; // Invert rate, so it fits the restriced accelerometer reading
  kalAngleX0 = kalmanX0.getAngle(roll0, gyroXrate0, dt); // Calculate the angle using a Kalman filter
#endif

  gyroXangle0 += gyroXrate0 * dt; // Calculate gyro angle without any filter
  gyroYangle0 += gyroYrate0 * dt;
  //gyroXangle += kalmanX.getRate() * dt; // Calculate gyro angle using the unbiased rate
  //gyroYangle += kalmanY.getRate() * dt;

  compAngleX0 = 0.93 * (compAngleX0 + gyroXrate0 * dt) + 0.07 * roll0; // Calculate the angle using a Complimentary filter
  compAngleY0 = 0.93 * (compAngleY0 + gyroYrate0 * dt) + 0.07 * pitch0;

  // Reset the gyro angle when it has drifted too much
  if (gyroXangle0 < -180 || gyroXangle0 > 180)
    gyroXangle0 = kalAngleX0;
  if (gyroYangle0 < -180 || gyroYangle0 > 180)
    gyroYangle0 = kalAngleY0;

  /* Print Data */
#if 0 // Set to 1 to activate

#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef RESTRICT_PITCH // Eq. 25 and 26
  double roll1  = atan2(accY1, accZ1) * RAD_TO_DEG;
  double pitch1 = atan(-accX1 / sqrt(accY1 * accY1 + accZ1 * accZ1)) * RAD_TO_DEG;
#else // Eq. 28 and 29
  double roll1  = atan(accY1 / sqrt(accX1 * accX1 + accZ1 * accZ1)) * RAD_TO_DEG;
  double pitch1 = atan2(-accX1, accZ1) * RAD_TO_DEG;
#endif

  double gyroXrate1 = gyroX1 / 131.0; // Convert to deg/s
  double gyroYrate1 = gyroY1 / 131.0; // Convert to deg/s

#ifdef RESTRICT_PITCH
  // This fixes the transition problem when the accelerometer angle jumps between -180 and 180 degrees
  if ((roll1 < -90 && kalAngleX1 > 90) || (roll1 > 90 && kalAngleX1 < -90)) {
    kalmanX1.setAngle(roll1);
    compAngleX1 = roll1;
    kalAngleX1 = roll1;
    gyroXangle1 = roll1;
  } else
    kalAngleX1 = kalmanX1.getAngle(roll1, gyroXrate1, dt); // Calculate the angle using a Kalman filter

  if (abs(kalAngleX1) > 90)
    gyroYrate1 = -gyroYrate1; // Invert rate, so it fits the restriced accelerometer reading
  kalAngleY1 = kalmanY1.getAngle(pitch1, gyroYrate1, dt);
#else
  // This fixes the transition problem when the accelerometer angle jumps between -180 and 180 degrees
  if ((pitch1 < -90 && kalAngleY1 > 90) || (pitch1 > 90 && kalAngleY1 < -90)) {
    kalmanY1.setAngle(pitch1);
    compAngleY1 = pitch1;
    kalAngleY1 = pitch1;
    gyroYangle1 = pitch1;
  } else
    kalAngleY1 = kalmanY1.getAngle(pitch1, gyroYrate1, dt); // Calculate the angle using a Kalman filter

  if (abs(kalAngleY1) > 90)
    gyroXrate1 = -gyroXrate1; // Invert rate, so it fits the restriced accelerometer reading
  kalAngleX1 = kalmanX1.getAngle(roll1, gyroXrate1, dt); // Calculate the angle using a Kalman filter
#endif

  gyroXangle1 += gyroXrate1 * dt; // Calculate gyro angle without any filter
  gyroYangle1 += gyroYrate1 * dt;
  //gyroXangle += kalmanX.getRate() * dt; // Calculate gyro angle using the unbiased rate
  //gyroYangle += kalmanY.getRate() * dt;

  compAngleX1 = 0.93 * (compAngleX1 + gyroXrate1 * dt) + 0.07 * roll1; // Calculate the angle using a Complimentary filter
  compAngleY1 = 0.93 * (compAngleY1 + gyroYrate1 * dt) + 0.07 * pitch1;

  // Reset the gyro angle when it has drifted too much
  if (gyroXangle1 < -180 || gyroXangle1 > 180)
    gyroXangle1 = kalAngleX1;
  if (gyroYangle1 < -180 || gyroYangle1 > 180)
    gyroYangle1 = kalAngleY1;

  /* Print Data */
#if 0 // Set to 1 to activate

#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  x = kalAngleX0 + kalAngleX1;
  y = kalAngleY0 + kalAngleY1;
  x = x / 2;
  y = y / 2;
}
