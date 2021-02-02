#ifndef SPDMOTOR_H
#define SPDMOTOR_H

#include <Encoder.h>

class SPDMotor {
 public:
  SPDMotor(int encoderA, int encoderB, bool encoderReversed, int motorPWM, int motorDir1, int motorDir2);
  void speed( int pwm );
  void hardStop();
  int getSpeed();
  long getEncoderPosition();

 private:
    Encoder *_encoder;
    bool _encoderReversed;
    int _motorPWM, _motorDir1, _motorDir2;
    int _speed;
};

#endif
