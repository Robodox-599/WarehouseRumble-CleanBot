#include <Encoder.h>
#include <PS2X_lib.h>
#include "SPDMotor.h"

//PS2
#define PS2_DAT        52  //14    
#define PS2_CMD        51  //15
#define PS2_SEL        53  //16
#define PS2_CLK        50  //17

#define pressures   false
#define rumble      false
PS2X ps2x; // create PS2 Controller Class

#define COMPMODE_STANDBY 0
#define COMPMODE_AUTO 1
#define COMPMODE_TELEOP 2

int error = 0;
byte type = 0;

void (* resetFunc) (void) = 0;

SPDMotor *motorLF = new SPDMotor(18, 31, true, 12, 34, 35); // <- Encoder reversed to make +position measurement be forward.
SPDMotor *motorRF = new SPDMotor(19, 38, false, 8, 36, 37); // <- NOTE: Motor Dir pins reversed for opposite operation
SPDMotor *motorLR = new SPDMotor(3, 49, true,  9, 43, 42); // <- Encoder reversed to make +position measurement be forward.
SPDMotor *motorRR = new SPDMotor(2, A1, false, 5, A4, A5); // <- NOTE: Motor Dir pins reversed for opposite operation

void setup()
{
  Serial.begin(9600);
  delay(300) ;//added delay to give wireless ps2 module some time to startup, before configuring it

  //setup pins and settings: GamePad(clock, command, attention, data, Pressures?, Rumble?) check for error
  error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble);

  if (error == 0) {
    Serial.println("Found Controller, configuration successful ");
    Serial.println();
    Serial.println("Left analog stick for forward/back and turning.");
    Serial.println("Right analog stick for sideways movement.");
    Serial.println("Hold both L1 and R1 for full-speed.");
  }
  else if (error == 1)
  {
    Serial.println("No controller found, check PS2 receiver is inserted the correct way around.");
    resetFunc();
  }
  else if (error == 2)
    Serial.println("Controller found but not accepting commands.");

  else if (error == 3)
    Serial.println("Controller refusing to enter Pressures mode, may not support it. ");

  type = ps2x.readType();
  switch (type) {
    case 0:
      Serial.println("Unknown Controller type found ");
      break;
    case 1:
      Serial.println("DualShock Controller found ");
      break;
    case 2:
      Serial.println("GuitarHero Controller found ");
      break;
    case 3:
      Serial.println("Wireless Sony DualShock Controller found ");
      break;
  }
}

int competitionMode = COMPMODE_STANDBY;
int timer = 0;

void loop() {

  switch (competitionMode)
  {
    case COMPMODE_STANDBY:
      ps2x.read_gamepad();

      if(ps2x.Button(PSB_START)) { Serial.println("Going to Auto Mode"); competitionMode = COMPMODE_AUTO;}
    break;
    
    case COMPMODE_AUTO:
      doAutonomousLoop();

      // if(timer > 30 seconds) {competitionMode = COMPMODE_TELEOP;}

      break;

    case COMPMODE_TELEOP:
      doTeleopLoop();

      // if (timer > gametime) {competitionMode = COMPMODE_STANDBY;}
      break;
  }

  delay(20);
}

void doTeleopLoop()
{
  if (error == 1) {
    return;  //skip loop if no controller found
  }
  if (type == 2) {
    return;  //Guitar Hero Controller
  }

  // We have a DualShock Controller that is responding
  ps2x.read_gamepad(); //read controller and set no vibrate

  // Test Encoders
  //  Serial.print(motorLF->getEncoderPosition());
  //  Serial.print(", ");
  //  Serial.print(motorRF->getEncoderPosition());
  //  Serial.print(", ");
  //  Serial.print(motorLR->getEncoderPosition());
  //  Serial.print(", ");
  //  Serial.print(motorRR->getEncoderPosition());
  //  Serial.println();

  if (ps2x.Button(PSB_L1) || ps2x.Button(PSB_R1))
  {
    int LY = ps2x.Analog(PSS_LY);
    int LX = ps2x.Analog(PSS_LX);
    int RX = ps2x.Analog(PSS_RX);

    driveChassisJoystick(LY, LX, RX);
  }
}

void doAutonomousLoop()
{
  driveChassis(0, 0, 255);
  delay(5000);

  driveChassis(0,0,0);
  delay(2000);

  driveChassis(0, 0, -255);
  delay(5000);
  
  driveChassis(0,0,0);
  delay(2000);

}

void driveChassisJoystick(int forRev, int turn, int slide)
{
  driveChassis((forRev * 2)-255, (turn * 2)-255, (slide * 2)-255); 
}


void driveChassis(int forRev, int turn, int slide)
{
  float forwardNormalized = (float)(forRev/255.f);

  forwardNormalized = constrain( forwardNormalized, -1.f, 1.f );
  float multiplier = (ps2x.Button(PSB_L1) && ps2x.Button(PSB_R1)) ? 255.f : 80.f;
  int forward = (int)(pow(forwardNormalized, 2.0) * multiplier);

  // Preserve the direction of movement.
  if ( forwardNormalized < 0 ) {
    forward = -forward;
  }

  int right = -slide / 2;
  int ccwTurn = (turn / 4);

  motorLF->speed(forward + ccwTurn - right);
  motorRF->speed(forward - ccwTurn + right);
  motorLR->speed(forward - ccwTurn - right);
  motorRR->speed(-(forward + ccwTurn + right));

//  Serial.print(motorLF->getSpeed());
//  Serial.print(',');
//  Serial.print(motorRF->getSpeed());
//  Serial.print(',');
//  Serial.print(motorLR->getSpeed());
//  Serial.print(',');
//  Serial.print(motorRR->getSpeed());
//  Serial.println();


  
}
