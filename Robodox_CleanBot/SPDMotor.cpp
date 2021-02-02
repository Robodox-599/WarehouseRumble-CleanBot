#include "SPDMotor.h"

SPDMotor::SPDMotor(int encoderA, int encoderB, bool encoderReversed, int motorPWM, int motorDir1, int motorDir2)
: _encoder{new Encoder(encoderA, encoderB)}, _encoderReversed{encoderReversed}, _motorPWM{motorPWM}, _motorDir1{motorDir1}, _motorDir2{motorDir2}
{
//  _encoder = new Encoder(encoderA, encoderB);

  pinMode( _motorPWM, OUTPUT );
  pinMode( _motorDir1, OUTPUT );
  pinMode( _motorDir2, OUTPUT );
}

// Set the PWM speed and direction pins.
// pwm = 0, stop (no active control)
// pwm = 1 to 255, proportion of CCW rotation
// pwm = -1 to -255, proportion of CW rotation
void SPDMotor::speed( int speedPWM )
{
  _speed = speedPWM;
  if( speedPWM == 0 ) {
    digitalWrite(_motorDir1,LOW);
    digitalWrite(_motorDir2,LOW);
    analogWrite( _motorPWM, 255);
  } else if( speedPWM > 0 ) {
    digitalWrite(_motorDir1, LOW );
    digitalWrite(_motorDir2, HIGH );
    analogWrite( _motorPWM, speedPWM < 255 ? speedPWM : 255);
  } else if( speedPWM < 0 ) {
    digitalWrite(_motorDir1, HIGH );
    digitalWrite(_motorDir2, LOW );
    analogWrite( _motorPWM, (-speedPWM) < 255 ? (-speedPWM): 255);
  }
}

// Activate a SHORT BRAKE mode, which shorts the motor drive EM, clamping motion.
void SPDMotor::hardStop()
{
    _speed = 0;
    digitalWrite(_motorDir1,HIGH);
    digitalWrite(_motorDir2,HIGH);
    analogWrite( _motorPWM, 0);
}

// Get the current speed.
int SPDMotor::getSpeed()
{
    return _speed;
}

// Get the current rotation position from the encoder.
long SPDMotor::getEncoderPosition()
{
  long position = _encoder->read();
  return _encoderReversed ? -position : position;
}
