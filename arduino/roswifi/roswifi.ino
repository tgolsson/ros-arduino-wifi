/* TODO:                                                                  */
/*    1. Hastighetsreglering på H/V -- PID/Minimum Jerk                   */
/*       4h                                                               */
/*    2. Odometri-uppdatering                                             */
/*       4h                                                               */
/*    3. Tangentbordsstyrning - Rospy + Nod                               */
/*       4h                                                               */
/*       FÄRDIGT VECKA 1                                                  */
/*   4a: Server på datorn: Ta emot twist-messages, skicka till arduino    */
/*       8h                                                               */
/*   4b: Wifi-skölden --  Ansluta till server, deserialisering på Arduino */
/*       8h                                                               */
#include <math.h>

#include "roswifi.h"
#include "PoseWithRotation.h"
const int ENCODER_PIN_L = 22,
    ENCODER_PIN_R = 23;
const double
    KP = 1.0,
    KI = 0.0,
    KD = 0.0,
    V_MAX = 280.0,
    TICKS_ROTATION = 230.4, // From datasheet
    WHB_RADIUS = 0.105,
    WH_RADIUS = 0.045,
    M_PER_PULSE = 0.045 * 2.0 * M_PI / TICKS_ROTATION;

const double dT = 1.0/30.0;

// Defintions for left and right
const int LEFT = 0,
    RIGHT = 1,
    pwm_resolution = 4095;


MotorShieldPins * motors[2];
EncoderStates * enc[2];
PIDParameters * pid_motors[2];
ControlStates * control_motors[2];
PoseWithRotation pose;
long long tOld, tNew;
static int counter;
void setup()
{
    counter = 0;
    motors[LEFT] = new MotorShieldPins(2, 9, 3, A0, LEFT);
    motors[RIGHT] = new MotorShieldPins(5, 8, 6, A1, RIGHT);

    analogWriteResolution(12);
    analogReadResolution(12);

    pinMode(ENCODER_PIN_L,INPUT);
    pinMode(ENCODER_PIN_R,INPUT);

    enc[LEFT] = new EncoderStates(ENCODER_PIN_L, 0);
    enc[RIGHT] = new EncoderStates(ENCODER_PIN_R, 0);

    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_L), readEncoderLeft, RISING);
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_R), readEncoderRight, RISING);

    digitalWrite(ENCODER_PIN_L,HIGH); // pull-up
    digitalWrite(ENCODER_PIN_R,HIGH); // pull-up

    for (int i = 0; i < 2; i++)
    {
        pid_motors[i] = new PIDParameters(KP, KI, KD, pwm_resolution, -pwm_resolution, 0);
        control_motors[i] = new ControlStates(0, 0, 0, 0, 0, 0, 0, 0, false);
        initMotor(motors[i]);
    }

    calculateDiffSpeed(20.0, 0.0);
        
    
    Serial.begin(9600);
}

void actuate(float control, MotorShieldPins *mps)
{
    // Set motor direction based on sign of control value
    if (mps->SIDE_ == LEFT)
        digitalWrite(mps->DIR_, control < 0 ? LOW : HIGH);
    else
        digitalWrite(mps->DIR_, control < 0 ? HIGH : LOW);

    // Force to positive and make sure it is in allowed control range
    double controlNew = abs(control);
    controlNew = max(min(pwm_resolution, controlNew), 0);

    // Write to pin
    analogWrite(mps->PWM_, controlNew);
}

void readEncoderRight() {
    if (digitalRead(motors[RIGHT]->DIR_))
    {
        enc[RIGHT]->p_--;
    }
    else
    {
        enc[RIGHT]->p_++;
    }
}

void readEncoderLeft() {
    if (digitalRead(motors[LEFT]->DIR_))
    {
        enc[LEFT]->p_++;
    }
    else
    {
        enc[LEFT]->p_--;
    }    
}

void initMotor(MotorShieldPins* pins)
{
    pinMode(pins->DIR_,OUTPUT);
    pinMode(pins->BRK_,OUTPUT);
    pinMode(pins->PWM_,OUTPUT);
    pinMode(pins->CUR_,INPUT);

    switch (pins->SIDE_)
    {
    case LEFT:
        digitalWrite(pins->DIR_,HIGH); 
        break;
    case RIGHT:
        digitalWrite(pins->DIR_,LOW);
        break;
    }
    digitalWrite(pins->BRK_,LOW);
}


//--------------------------------------------------------------------------
void velocityControl(ControlStates* c_s, EncoderStates* e_s, MotorShieldPins* m_pins, PIDParameters* pid_p)
{
    // Sliding sum for encoder speed
    e_s->dp_ = e_s->dp_ * 0.99 + 0.01*(e_s->p_ - e_s->pp_)/(dT);  // dT = 1000 us = 0.001 s
    e_s->pp_ = e_s->p_;

    //Set-point calculation
    double setPoint = minimumJerk(c_s->ti_, (double)tNew, c_s->T_, c_s->ri_, c_s->rf_);
    c_s->r_ = setPoint;

    // Error and error derivative
    double e = (c_s->r_ - e_s->dp_);
    double de = (c_s->e_ - e) / dT;

    // Get PID output
    double ut = pid(e, de, pid_p);

    // Store for next cycle
    c_s->e_ = e;
    c_s->de_ = de;
    c_s->u_ = ut;
    
}


//--------------------------------------------------------------------------
float minimumJerk(float t0, float t, float T, float q0, float qf)
{
    // Calculate t / T. Clamp to [0,1] to prevent value explosion
    double tbyT = min((t-t0)/(T-t0),1);
    // Minimum jerk equation.
    return (q0 + (qf - q0) * (10.0 * pow(tbyT,3.0) - 15.0 * pow(tbyT,4.0) + 6.0 * pow(tbyT,5.0)));
    
}
//--------------------------------------------------------------------------
float pid(float e, float de, PIDParameters* p)
{
    // Update integral term
    p->I_ += e*dT;
    // Calculate output value
    double ut =  p->Kp_*e + p->Ki_ * p->I_ + p->Kd_ * de;
    // Clamp to maixmum and minimum value before returning
    ut = min(max(p->u_min_, ut), p->u_max_);
    return ut;
    
}

void updatePose(){
    /* Get encoder steps since last check, then save this point for the future */
    int encoderStepsLeft = enc[LEFT]->p_ - enc[LEFT]->pp_;
    int encoderStepsRight = enc[RIGHT]->p_ - enc[RIGHT]->pp_;

    /* Calculate millimeters moved since last check */
    double mL = M_PER_PULSE * encoderStepsLeft;
    double mR = M_PER_PULSE * encoderStepsRight;

    /* Calculate movement along x,y in epucks private carthesian system relative to previous position */
    double dy, dx, d, p, delta;
    d = (mR + mL)/2;
    delta = (mR-mL)/(WHB_RADIUS*2);
    /* If abs(delta) is close to 0, use specialized formulas */
    if (sqrt(pow(delta,2)) < 0.001){
        dx = d*cos(delta/2);
        dy = d*sin(delta/2);
    }
    else /* Use the proper formulas */
    {
        p =  d/delta;
        dx = p*sin(delta);
        dy = p*(1-cos(delta));
    }
    /* Translate into global carthesian coordinates */
    pose.x = pose.x + dx * cos (pose.theta) - dy * sin(pose.theta);
    pose.y = pose.y + dx * sin(pose.theta) + dy * cos(pose.theta);
    pose.theta = pose.theta + delta;
    if (pose.theta < -M_PI){
        pose.theta = pose.theta + 2 * PI;
    }
    else if (pose.theta > M_PI){
        pose.theta = pose.theta - 2 * PI;
    }
}


void calculateDiffSpeed(double angVel, double linVel)
{
    double radius = 0;
    double velLeft = 0;
    double velRight = 0;
    if (angVel != 0)
    {
        radius  = linVel / angVel;   
        velLeft = (radius - WHB_RADIUS) * angVel;
        velRight = (radius + WHB_RADIUS) * angVel;
    }
    else
    {
        velLeft = velRight = linVel;
    }
    control_motors[LEFT]->rf_ = velLeft * 1.0 / M_PER_PULSE;
    control_motors[RIGHT]->rf_ = velRight * 1.0 / M_PER_PULSE;
    for (int i = 0; i < 2; i++)
    {
        control_motors[i]->ri_ = enc[i]->dp_;
        control_motors[i]->r_ = enc[i]->dp_;
        // Reset integral

        pid_motors[i]->I_ = 0;

        // Start and end time

        control_motors[i]->ti_ = micros();
        // 0.006  = 3 s / 560 tps -- acceleration constant

        control_motors[i]->T_ = micros() + abs(control_motors[i]->ri_ - control_motors[i]->rf_) * 0.006*1e6;
    }
    
}
void loop() {
    tNew = micros();
    long long delta = (tNew - tOld);
    double deltaDouble = (double)delta * 1.0e-6;
    if (deltaDouble < dT)
    {
        return;
    }
    tOld = tNew;

    updatePose();
    for (int i = 0; i < 2; i++)
    {
        velocityControl(control_motors[i], enc[i], motors[i], pid_motors[i]);
        actuate(control_motors[i]->u_, motors[i]);
    }

    // Print encoder values
    Serial.print("Encoder = (");
    Serial.print(enc[LEFT]->p_);
    Serial.print(", ");
    Serial.print(enc[RIGHT]->p_);
    Serial.println(")");

    // Print position and rotation
    Serial.print("Position (x,y,th): ");
    Serial.print(pose.x);
    Serial.print(", ");
    Serial.print(pose.y);
    Serial.print(", ");
    Serial.println(pose.theta);

    Serial.print("Target value: ");
    Serial.print(control_motors[LEFT]->rf_);
    Serial.print(", ");
    Serial.println(control_motors[RIGHT]->rf_);

    
    Serial.print("Control value: ");
    Serial.print(control_motors[LEFT]->u_);
    Serial.print(", ");
    Serial.println(control_motors[RIGHT]->u_);
    
    Serial.print("Setpoint value: ");
    Serial.print(control_motors[LEFT]->r_);
    Serial.print(", ");
    Serial.println(control_motors[RIGHT]->r_);
}
