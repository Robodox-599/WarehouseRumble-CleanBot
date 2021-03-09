#include <Encoder.h>
#include <PS2X_lib.h>
#include "SPDMotor.h"
#include <Servo.h> 

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

#define AUTOMODE_INIT 0
#define AUTOMODE_FORWARD 1
#define AUTOMODE_TURNLEFT 2
#define AUTOMODE_REVERSE 3
#define AUTOMODE_STOP 4
#define AUTOMODE_STANDBY 1000

#define TIMER_AUTONOMOUS_MS 2000
#define TIMER_TELEOP_MS 60000

#define ELEVATOR_UP_BUTTON PSB_PAD_UP
#define ELEVATOR_DOWN_BUTTON PSB_PAD_DOWN

int error = 0;
byte type = 0;
int autoState = AUTOMODE_INIT;

void (* resetFunc) (void) = 0;

SPDMotor *motorLF = new SPDMotor(18, 31, true, 12, 34, 35); // <- Encoder reversed to make +position measurement be forward.
SPDMotor *motorRF = new SPDMotor(19, 38, false, 8, 36, 37); // <- NOTE: Motor Dir pins reversed for opposite operation
SPDMotor *motorLR = new SPDMotor(3, 49, true,  9, 43, 42); // <- Encoder reversed to make +position measurement be forward.
SPDMotor *motorRR = new SPDMotor(2, A1, false, 5, A4, A5); // <- NOTE: Motor Dir pins reversed for opposite operation

Servo demoServo;

void setup()
{
  Serial.begin(57600);
  delay(300) ;//added delay to give wireless ps2 module some time to startup, before configuring it
  setupController();

  demoServo.attach(46);
  demoServo.write(90);

  allStop();

  Serial.println("Going to Standby Mode");
}

int competitionMode = COMPMODE_STANDBY;
unsigned long timerStart = millis();

void loop() {

  switch (competitionMode)
  {
    case COMPMODE_STANDBY:
      ps2x.read_gamepad();

      if (ps2x.ButtonPressed(PSB_START))
      {
        Serial.println("Going to Auto Mode");
        competitionMode = COMPMODE_AUTO;
        autoState = AUTOMODE_INIT;
        timerStart = millis();
      }
      break;

    case COMPMODE_AUTO:
      doAutonomousLoop();

      if (millis() > (timerStart + TIMER_AUTONOMOUS_MS)) {
        Serial.println("Going to Teleop Mode");
        allStop();
        competitionMode = COMPMODE_TELEOP;
        timerStart = millis();
      }
      break;

    case COMPMODE_TELEOP:
      doTeleopLoop();

      if (millis() > (timerStart + TIMER_TELEOP_MS)) {
        Serial.println("Match over, going to Standby");
        allStop();
        competitionMode = COMPMODE_STANDBY;
      }
      break;
  }

  delay(20);
}

int armPosition = 0;
int wristTargetPosition = 0;

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

  // SAMPLE CODE FOR USING TWO BUTTONS TO CONTROL A MOTOR
  //  if(ps2x.Button(ELEVATOR_UP_BUTTON) && ps2x.Button(ELEVATOR_DOWN_BUTTON)) {motorLF->speed(0);}
  //  if(ps2x.Button(ELEVATOR_UP_BUTTON) && !ps2x.Button(ELEVATOR_DOWN_BUTTON)) {motorLF->speed(255);}
  //  if(!ps2x.Button(ELEVATOR_UP_BUTTON) && ps2x.Button(ELEVATOR_DOWN_BUTTON)) {motorLF->speed(-255);}
  //  if(!ps2x.Button(ELEVATOR_UP_BUTTON) && !ps2x.Button(ELEVATOR_DOWN_BUTTON)) {motorLF->speed(0);}

/*
    //ServoSample
    if(ps2x.Button(PSB_PAD_LEFT)) { armPosition = 0; }
    if(ps2x.Button(PSB_PAD_RIGHT)) { armPosition = 180; }

    if(ps2x.ButtonPressed(PSB_PAD_UP)) { armPosition += 20; }
    if(ps2x.ButtonPressed(PSB_PAD_DOWN)) { armPosition -= 20; }
    if(armPosition > 180) { armPosition = 180;}
    if(armPosition < 0) { armPosition = 0;}

    demoServo.write(armPosition);

    
    // WRIST CONTROL EXAMPLE
    // Controls
    if(ps2x.Button(PSB_TRIANGLE)) { wristTargetPosition++; }
    if(ps2x.Button(PSB_CROSS)) { wristTargetPosition--; }

    if(ps2x.Button(PSB_SQUARE)) { wristTargetPosition = 25; }
    if(ps2x.Button(PSB_CIRCLE)) { wristTargetPosition = 500; }

    // Motor
    if(wristTargetPosition > 500) {wristTargetPosition = 500;}
    if(wristTargetPosition < 25) {wristTargetPosition = 25;}
    
    int wristActualPosition = analogRead(25);
    int wristMotorSpeed = 0;

    if(wristActualPosition > wristTargetPosition) { wristMotorSpeed = 100;  }
    if(wristActualPosition < wristTargetPosition) { wristMotorSpeed = -100;  }
    if(wristActualPosition == wristTargetPosition) { wristMotorSpeed = 0;  }

    armMotor->speed(wristMotorSpeed);
  */
  
  // DRIVING
  if (ps2x.Button(PSB_L1) || ps2x.Button(PSB_R1))
  {
    int LY = 255 - ps2x.Analog(PSS_LY);
    int LX = ps2x.Analog(PSS_LX);
    int RX = ps2x.Analog(PSS_RX);

    driveChassisJoystick(LY, LX, RX);
  }
  else
  {
    driveChassis(0, 0, 0);
  }
}

void doAutonomousLoop()
{
  //Serial.print('*');
  static int stateLoopCount = 0;
  static long encoderBookMark = 0;

  stateLoopCount++;

  // Autonomous Mode State Machine!!!1!11!!!
  switch (autoState)
  {
    case AUTOMODE_INIT:
      // All the setup stuff if we want

//      autoState = AUTOMODE_FORWARD;
//      stateLoopCount = 0;
        setNewState(AUTOMODE_FORWARD, stateLoopCount);
      break;
    case AUTOMODE_FORWARD:
      if (stateLoopCount == 1)
      {
        Serial.println("AUTOMODE_FORWARD");
        encoderBookMark = motorLF->getEncoderPosition();
        driveChassis(200, 0, 0);
      }

      if (abs(motorLF->getEncoderPosition() - encoderBookMark) > 1300)
      {
        Serial.println("DONE");
        driveChassis(0, 0, 0);
//        autoState = AUTOMODE_TURNLEFT;
//        stateLoopCount = 0;
        setNewState(AUTOMODE_TURNLEFT, stateLoopCount);
      }
      break;
    case AUTOMODE_TURNLEFT:
      if (stateLoopCount == 1)
      {
        Serial.println("AUTOMODE_TURNLEFT");
        encoderBookMark = motorLF->getEncoderPosition();
        driveChassis(0, -200, 0);
      }

      if (abs(motorLF->getEncoderPosition() - encoderBookMark) > 1300)
      {
        Serial.println("DONE");
        driveChassis(0, 0, 0);
//        autoState = AUTOMODE_STANDBY;
//        stateLoopCount = 0;
        setNewState(AUTOMODE_STANDBY, stateLoopCount);
      }

      break;

    case AUTOMODE_STANDBY:
      allStop();
      //Serial.print('-');
      // Nothing!
      break;
  }
}

void driveChassisJoystick(int forRev, int turn, int slide)
{
  driveChassis((forRev * 2) - 255, (turn * 2) - 255, (slide * 2) - 255);
}

void driveChassis(int forRev, int turn, int slide)
{
  float forwardNormalized = (float)(forRev / 255.f);

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
  motorLR->speed(forward + ccwTurn + right);
  motorRR->speed(-(forward - ccwTurn - right));
}

void setupController()
{

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

void allStop()
{
  driveChassis(0, 0, 0);
  //arm.stop
  //gripper.stop
  //wrist.stop
}

void setNewState(int newState, int& stateLoopCount)
{
  autoState = newState;
  stateLoopCount = 0;
}
