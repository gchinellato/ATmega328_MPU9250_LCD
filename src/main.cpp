/*
*************************************************
* @Project: Self Balance
* @Platform: Arduino Nano ATmega328
* @Description: Main thread
* @Owner: Guilherme Chinellato
* @Email: guilhermechinellato@gmail.com
*************************************************
*/

#include "Arduino.h"
#include "IMU/quaternionFilters.h"
#include "IMU/MPU9250.h"
#include "Motion/Motor/motor.h"
#include "LiquidCrystal_I2C/LiquidCrystal_I2C.h"
#include "main.h"

#define SERIAL_DBG true  // Set to true to get Serial output for debugging
#define LCD_DISPLAY true  // Set to true to get Serial output for debugging

//Motors objects
Motor motor1(PWM1_PIN,CW1_PIN,CCW1_PIN);
Motor motor2(PWM2_PIN,CW2_PIN,CCW2_PIN);

//IMU object
MPU9250 myIMU;

//LCD object
LiquidCrystal_I2C lcd(0x20,16,2);

byte custom[8] = {
  0b00111,          // Caractere customized
  0b00101,
  0b00111,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

void setup()
{
  Wire.begin();
  // TWBR = 12;  // 400 kbit/sec I2C speed
  Serial.begin(SERIAL_BAUDRATE);

  lcd.init();                 // Inicializando o LCD
  lcd.backlight();            // Ligando o BackLight do LCD
  //lcd.createChar(5, custom);
  lcd.setCursor(0,0);
  lcd.print("Trab. 2016");
  lcd.setCursor(0,1);
  lcd.print("Ele. Digital");
  delay(1000);
  lcd.clear();

  // Read the WHO_AM_I register, this is a good test of communication
  byte c = myIMU.readByte(MPU9250_ADDRESS, WHO_AM_I_MPU9250);
  Serial.print("MPU9250 "); Serial.print("I AM "); Serial.print(c, HEX);
  Serial.print(" I should be "); Serial.println(0x71, HEX);

  if (c == 0x71) // WHO_AM_I should always be 0x68
  {
    Serial.println("MPU9250 is online...");

    // Start by performing self test and reporting values
    myIMU.MPU9250SelfTest(myIMU.SelfTest);
    Serial.print("x-axis self test: acceleration trim within : ");
    Serial.print(myIMU.SelfTest[0],1); Serial.println("% of factory value");
    Serial.print("y-axis self test: acceleration trim within : ");
    Serial.print(myIMU.SelfTest[1],1); Serial.println("% of factory value");
    Serial.print("z-axis self test: acceleration trim within : ");
    Serial.print(myIMU.SelfTest[2],1); Serial.println("% of factory value");
    Serial.print("x-axis self test: gyration trim within : ");
    Serial.print(myIMU.SelfTest[3],1); Serial.println("% of factory value");
    Serial.print("y-axis self test: gyration trim within : ");
    Serial.print(myIMU.SelfTest[4],1); Serial.println("% of factory value");
    Serial.print("z-axis self test: gyration trim within : ");
    Serial.print(myIMU.SelfTest[5],1); Serial.println("% of factory value");

    // Calibrate gyro and accelerometers, load biases in bias registers
    myIMU.calibrateMPU9250(myIMU.gyroBias, myIMU.accelBias);

    myIMU.initMPU9250();
    // Initialize device for active mode read of acclerometer, gyroscope, and
    // temperature
    Serial.println("MPU9250 initialized for active data mode....");

    // Read the WHO_AM_I register of the magnetometer, this is a good test of
    // communication
    byte d = myIMU.readByte(AK8963_ADDRESS, WHO_AM_I_AK8963);
    Serial.print("AK8963 "); Serial.print("I AM "); Serial.print(d, HEX);
    Serial.print(" I should be "); Serial.println(0x48, HEX);

    // Get magnetometer calibration from AK8963 ROM
    myIMU.initAK8963(myIMU.magCalibration);
    // Initialize device for active mode read of magnetometer
    Serial.println("AK8963 initialized for active data mode....");
    if (SERIAL_DBG)
    {
      //  Serial.println("Calibration values: ");
      Serial.print("X-Axis sensitivity adjustment value ");
      Serial.println(myIMU.magCalibration[0], 2);
      Serial.print("Y-Axis sensitivity adjustment value ");
      Serial.println(myIMU.magCalibration[1], 2);
      Serial.print("Z-Axis sensitivity adjustment value ");
      Serial.println(myIMU.magCalibration[2], 2);
    }
  } // if (c == 0x71)
  else
  {
    Serial.print("Could not connect to MPU9250: 0x");
    Serial.println(c, HEX);
    while(1) ; // Loop forever if communication doesn't happen
  }
}

void loop()
{
    // If intPin goes high, all data registers have new data
    // On interrupt, check if data ready interrupt
    if (myIMU.readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01)
    {
        myIMU.readAccelData(myIMU.accelCount);  // Read the x/y/z adc values
        myIMU.getAres();

        // Now we'll calculate the accleration value into actual g's
        // This depends on scale being set
        myIMU.ax = (float)myIMU.accelCount[0]*myIMU.aRes; // - accelBias[0];
        myIMU.ay = (float)myIMU.accelCount[1]*myIMU.aRes; // - accelBias[1];
        myIMU.az = (float)myIMU.accelCount[2]*myIMU.aRes; // - accelBias[2];

        myIMU.readGyroData(myIMU.gyroCount);  // Read the x/y/z adc values
        myIMU.getGres();

        // Calculate the gyro value into actual degrees per second
        // This depends on scale being set
        myIMU.gx = (float)myIMU.gyroCount[0]*myIMU.gRes;
        myIMU.gy = (float)myIMU.gyroCount[1]*myIMU.gRes;
        myIMU.gz = (float)myIMU.gyroCount[2]*myIMU.gRes;

        myIMU.readMagData(myIMU.magCount);  // Read the x/y/z adc values
        myIMU.getMres();
        // User environmental x-axis correction in milliGauss, should be
        // automatically calculated
        myIMU.magbias[0] = +470.;
        // User environmental x-axis correction in milliGauss TODO axis??
        myIMU.magbias[1] = +120.;
        // User environmental x-axis correction in milliGauss
        myIMU.magbias[2] = +125.;

        // Calculate the magnetometer values in milliGauss
        // Include factory calibration per data sheet and user environmental
        // corrections
        // Get actual magnetometer value, this depends on scale being set
        myIMU.mx = (float)myIMU.magCount[0]*myIMU.mRes*myIMU.magCalibration[0] -
                   myIMU.magbias[0];
        myIMU.my = (float)myIMU.magCount[1]*myIMU.mRes*myIMU.magCalibration[1] -
                   myIMU.magbias[1];
        myIMU.mz = (float)myIMU.magCount[2]*myIMU.mRes*myIMU.magCalibration[2] -
                   myIMU.magbias[2];
    } // if (readBytePU9250_ADDRESS, INT_STATUS) & 0x01)

    // Must be called before updating quaternions!
    myIMU.updateTime();

    // Sensors x (y)-axis of the accelerometer is aligned with the y (x)-axis of
    // the magnetometer; the magnetometer z-axis (+ down) is opposite to z-axis
    // (+ up) of accelerometer and gyro! We have to make some allowance for this
    // orientationmismatch in feeding the output to the quaternion filter. For the
    // MPU-9250, we have chosen a magnetic rotation that keeps the sensor forward
    // along the x-axis just like in the LSM9DS0 sensor. This rotation can be
    // modified to allow any convenient orientation convention. This is ok by
    // aircraft orientation standards! Pass gyro rate as rad/s
    //  MadgwickQuaternionUpdate(ax, ay, az, gx*PI/180.0f, gy*PI/180.0f, gz*PI/180.0f,  my,  mx, mz);
    MahonyQuaternionUpdate(myIMU.ax, myIMU.ay, myIMU.az, myIMU.gx*DEG_TO_RAD,
                         myIMU.gy*DEG_TO_RAD, myIMU.gz*DEG_TO_RAD, myIMU.my,
                         myIMU.mx, myIMU.mz, myIMU.deltat);

     lcd.delt_t = millis() - lcd.count;

     if(lcd.delt_t > DATA_INTERVAL_LCD)
     {
         if(LCD_DISPLAY)
         {
             lcd.setCursor(0,0); // Seta o cursor para o caracter 14, na linha 0
             lcd.print("P=");
             lcd.print(myIMU.pitch,1);
             lcd.setCursor(8,0); // Seta o cursor para o caracter 14, na linha 0
             lcd.print("R=");
             lcd.print(myIMU.roll,1);

             lcd.setCursor(0,1); // Seta o cursor para o caracter 14, na linha 0
             lcd.print("Y=");
             lcd.print(myIMU.yaw,1);
             lcd.setCursor(8,1); // Seta o cursor para o caracter 14, na linha 0
             lcd.print("T=");
             lcd.print(myIMU.temperature,1);
             lcd.print(" C");

             /*lcd.print("->");
             lcd.setCursor(14,0); // Seta o cursor para o caracter 14, na linha 0
             lcd.write(5); // Imprime o byte 5(nosso caracter custom)*/
             //lcd.clear(); //Limpa a tela do LCD
         }
         lcd.count = millis();
     }

     myIMU.delt_t = millis() - myIMU.count;

     if (myIMU.delt_t > DATA_INTERVAL)
     {
        myIMU.tempCount = myIMU.readTempData();  // Read the adc values
        // Temperature in degrees Centigrade
        myIMU.temperature = ((float) myIMU.tempCount) / 333.87 + 21.0;

        // Define output variables from updated quaternion---these are Tait-Bryan
        // angles, commonly used in aircraft orientation. In this coordinate system,
        // the positive z-axis is down toward Earth. Yaw is the angle between Sensor
        // x-axis and Earth magnetic North (or true North if corrected for local
        // declination, looking down on the sensor positive yaw is counterclockwise.
        // Pitch is angle between sensor x-axis and Earth ground plane, toward the
        // Earth is positive, up toward the sky is negative. Roll is angle between
        // sensor y-axis and Earth ground plane, y-axis up is positive roll. These
        // arise from the definition of the homogeneous rotation matrix constructed
        // from quaternions. Tait-Bryan angles as well as Euler angles are
        // non-commutative; that is, the get the correct orientation the rotations
        // must be applied in the correct order which for this configuration is yaw,
        // pitch, and then roll.
        // For more see
        // http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
        // which has additional links.
        myIMU.yaw   = atan2(2.0f * (*(getQ()+1) * *(getQ()+2) + *getQ() *
                    *(getQ()+3)), *getQ() * *getQ() + *(getQ()+1) * *(getQ()+1)
                    - *(getQ()+2) * *(getQ()+2) - *(getQ()+3) * *(getQ()+3));
        myIMU.pitch = -asin(2.0f * (*(getQ()+1) * *(getQ()+3) - *getQ() *
                    *(getQ()+2)));
        myIMU.roll  = atan2(2.0f * (*getQ() * *(getQ()+1) + *(getQ()+2) *
                    *(getQ()+3)), *getQ() * *getQ() - *(getQ()+1) * *(getQ()+1)
                    - *(getQ()+2) * *(getQ()+2) + *(getQ()+3) * *(getQ()+3));
        myIMU.pitch *= RAD_TO_DEG;
        myIMU.yaw   *= RAD_TO_DEG;
        // Declination of Sao Paulo (23° 34' 52"S 46° 37' 23"W) is
        // 	21° 11' W  ± 0° 23' (or 21.34°) on 2016-10-31
        // - http://www.ngdc.noaa.gov/geomag-web/#declination
        myIMU.yaw   -= 21.34;
        myIMU.roll  *= RAD_TO_DEG;

        myIMU.tempCount = myIMU.readTempData();  // Read the adc values
        // Temperature in degrees Centigrade
        myIMU.temperature = ((float) myIMU.tempCount) / 333.87 + 21.0;

        if(SERIAL_DBG)
        {
            // Print acceleration values in milligs!
            /*Serial.print("X-acceleration: "); Serial.print(1000*myIMU.ax);
            Serial.print(" , ");
            Serial.print("Y-acceleration: "); Serial.print(1000*myIMU.ay);
            Serial.print(" , ");
            Serial.print("Z-acceleration: "); Serial.print(1000*myIMU.az);
            Serial.println(" mg ");*/

            // Print gyro values in degree/sec
            /*Serial.print("X-gyro rate: "); Serial.print(myIMU.gx, 3);
            Serial.print(" , ");
            Serial.print("Y-gyro rate: "); Serial.print(myIMU.gy, 3);
            Serial.print(" , ");
            Serial.print("Z-gyro rate: "); Serial.print(myIMU.gz, 3);
            Serial.println(" degrees/sec");*/

            // Print mag values in degree/sec
            /*Serial.print("X-mag: "); Serial.print(myIMU.mx);
            Serial.print(" , ");
            Serial.print("Y-mag: "); Serial.print(myIMU.my);
            Serial.print(" , ");
            Serial.print("Z-mag: "); Serial.print(myIMU.mz);
            Serial.println(" mG");*/

            // Print temperature in degrees Centigrade
            /*Serial.print("Temperature is ");  Serial.print(myIMU.temperature, 1);
            Serial.println(" degrees C");*/

            /*Serial.print("q0 = "); Serial.print(*getQ());
            Serial.print(" qx = "); Serial.print(*(getQ() + 1));
            Serial.print(" qy = "); Serial.print(*(getQ() + 2));
            Serial.print(" qz = "); Serial.println(*(getQ() + 3));*/

            /*Serial.print("Pitch: "); Serial.print(myIMU.pitch, 2);
            Serial.print(" , ");
            Serial.print("Roll: "); Serial.print(myIMU.roll, 2);
            Serial.print(" , ");
            Serial.print("Yaw: "); Serial.print(myIMU.yaw, 2);
            Serial.println(" degree");*/

            /*Serial.print("rate = ");
            Serial.print((float)myIMU.sumCount/myIMU.sum, 2);
            Serial.println(" Hz");*/
        }

        motor1.setSpeedPercentage(myIMU.pitch);
        motor2.setSpeedPercentage(myIMU.pitch);

        myIMU.count = millis();
        myIMU.sumCount = 0;
        myIMU.sum = 0;
    }
}
