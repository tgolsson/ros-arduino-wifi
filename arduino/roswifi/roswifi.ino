/* TODO:                                                                  */
/*    1. Hastighetsreglering på H/V -- PID/Minimum Jerk ---- KLAR         */
/*       4h                                                               */
/*   2. Odometri-uppdatering                           ---- KLAR          */
/*       4h                                                               */


/*    3. Tangentbordsstyrning - Rospy + Nod                               */
/*       4h                                                               */
/*       FÄRDIGT VECKA 1                                                  */
/*   4a: Server på datorn: Ta emot twist-messages, skicka till arduino    */
/*       8h                                                               */
/*   4b: Wifi-skölden --  Ansluta till server, deserialisering på Arduino */
/*       8h  */

// Standard libraries
#include <math.h>

// Headers for this project

#include "roswifi.h"
#include "MoveOrder.h"
const int ENCODER_PIN_L = 22,
    ENCODER_PIN_R = 23,
    LEFT = 0,
    RIGHT = 1,
    pwm_resolution = 4095,
    PWM_CUTOFF_HIGH = 700,
    PWM_CUTOFF_LOW = 10;

const double
    //KP = 80, KI = 20, KD = 0
    KP = 24.0,
    KI = 35.0,
    KD = 0.1,
    V_MAX = 280.0,
    TICKS_ROTATION = 230.4, // From datasheet
    WHB_RADIUS = 0.105,
    WH_RADIUS = 0.045,
    M_PER_PULSE = WH_RADIUS * 2.0 * M_PI / TICKS_ROTATION,
    dT = 1.0/30.0,
    dT_Serial = 1.0/10.0,
    PWM_FACTOR = 0.05,
    SMOOTHING_FACTOR = 0.10;

MotorShieldPins * motors[2];
EncoderStates * enc[2];
PIDParameters * pid_motors[2];
ControlStates * control_motors[2];
long long tOld, tNew, tOld_Serial;
WaypointSet * square;
#ifdef USE_ROS
geometry_msgs::Pose2D pose;
geometry_msgs::Twist previous_vel_msg;
geometry_msgs::Twist plotData;
ros::NodeHandle nh;
ros::Subscriber<geometry_msgs::Twist> twistSubscriber("/robot_0/velocity_commands", &velocityCallback);
ros::Subscriber<std_msgs::Empty> resetSubscriber("/robot_0/reset_odometry", &resetOdometryCallback);
ros::Subscriber<geometry_msgs::Vector3> pidSubscriberL("/robot_0/pid_tune_l", &setPIDCallbackL);
ros::Subscriber<geometry_msgs::Vector3> pidSubscriberR("/robot_0/pid_tune_r", &setPIDCallbackR);
ros::Publisher odometryPublisher("/robot_0/odometry", &pose);
ros::Publisher dataPublisher("/robot_0/plot_data", &plotData);

#else
PoseWithRotation pose;
#endif

int countLoops = 0;

void setup()
{
    delay(1000);
    Serial1.begin(38400); 
    Serial.begin(38400);
    nh.initNode();
    delay(1000);
    nh.subscribe(twistSubscriber);
    nh.subscribe(resetSubscriber);
    nh.subscribe(pidSubscriberL);
    nh.subscribe(pidSubscriberR);
    nh.advertise(odometryPublisher);
    nh.advertise(dataPublisher);

    previous_vel_msg.linear.x = 0.0;
    previous_vel_msg.angular.z = 0.0;

    
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

    pid_motors[LEFT] = new PIDParameters(KP-1, KI+1, KD, pwm_resolution, -pwm_resolution, 0);
    pid_motors[RIGHT] = new PIDParameters(KP, KI, KD, pwm_resolution, -pwm_resolution, 0);
    for (int i = 0; i < 2; i++)
    {
        control_motors[i] = new ControlStates(0, 0, 0, 0, 0, 0, 0, 0, false);
        initMotor(motors[i]);
    }

    Point2D  squarePoints[] = {{0,0},{2,0},{2,2},{0,2},{0,0}};     
    square = new WaypointSet(squarePoints, 5);


    calculateDiffSpeed(0.0, 0.0);


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
    if (controlNew > PWM_CUTOFF_LOW)
    {
        if (controlNew < PWM_CUTOFF_HIGH)
        {
            controlNew = PWM_CUTOFF_HIGH;
        }
    }
    else
    {
        controlNew = 0.0;
    }
    
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
    double pwm_value = c_s->u_ / (double) pwm_resolution * V_MAX * PWM_FACTOR;
    // Sliding sum for encoder speed
    e_s->dp_ = e_s->dp_ * (1-SMOOTHING_FACTOR) + SMOOTHING_FACTOR * (e_s->p_ - e_s->pp_)/(dT);  // dT = 1000 us = 0.001 s
    e_s->pp_ = e_s->p_;

    //Set-point calculation
    double setPoint = minimumJerk(c_s->ti_, (double)tNew, c_s->T_, c_s->ri_, c_s->rf_);
    c_s->r_ = setPoint;

    // Error and error derivative
    double e = (c_s->r_ - e_s->dp_ - pwm_value);
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
    p->I_ = min(max(p->u_min_, p->I_), p->u_max_);
    // Calculate output value
    double ut =  p->Kp_*e + p->Ki_ * p->I_ + p->Kd_ * de;
    // Clamp to maixmum and minimum value before returning
    ut = min(max(p->u_min_, ut), p->u_max_);
    return ut;
    
}

void updatePose(){
    /* Get encoder steps since last check, then save this point for the future */
    double encoderStepsLeft = enc[LEFT]->p_ - enc[LEFT]->pp_;
    double encoderStepsRight = enc[RIGHT]->p_ - enc[RIGHT]->pp_;

    encoderStepsLeft = enc[LEFT]->dp_ * dT;
    encoderStepsRight = enc[RIGHT]->dp_ * dT;
    /* Calculate milli5meters moved since last check */
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
    velLeft /= M_PER_PULSE;
    velRight /= M_PER_PULSE;
   
    control_motors[LEFT]->rf_ = velLeft;
    control_motors[RIGHT]->rf_ = velRight;
    
    double controlOvershoot = max(control_motors[LEFT]->rf_, control_motors[RIGHT]->rf_)/V_MAX;

    if (controlOvershoot > 1.0)
    {
        control_motors[LEFT]->rf_ /= controlOvershoot;
        control_motors[RIGHT]->rf_ /= controlOvershoot;
    }
    for (int i = 0; i < 2; i++)
    {
        control_motors[i]->ri_ = enc[i]->dp_;
        control_motors[i]->r_ = enc[i]->dp_;

        double finishedOfPreviousMotion = (micros() - control_motors[i]->ti_) / (control_motors[i]->T_ - control_motors[i]->ti_);

        if (finishedOfPreviousMotion > 1 || finishedOfPreviousMotion < 0)
        {
            finishedOfPreviousMotion = 0;
        }
        control_motors[i]->ti_ = micros();//
        control_motors[i]->T_ = control_motors[i]->ti_ + abs(control_motors[i]->ri_ - control_motors[i]->rf_) * 0.018*1e6;

        /* double x = (control_motors[i]->T_ - control_motors[i]->ti_) * finishedOfPreviousMotion; */
        /* control_motors[i]->ti_ -= x; */
        /* control_motors[i]->T_ -= x/2.0; */

//        control_motors[i]->ti_ = (control_motors[i]->ti_+micros())/2;
//        control_motors[i]->T_ = control_motors[i]->T_ + dT/2.0; //abs(control_motors[i]->ri_ - control_motors[i]->rf_) * 0.024*1e6;
    }
}

#ifdef USE_ROS

void resetOdometryCallback(const std_msgs::Empty& msg)
{
    pose.x = 0;
    pose.y = 0;    
    pose.theta = 0;
}

void setPIDCallbackL(const geometry_msgs::Vector3& msg)
{
    pid_motors[LEFT]->Kp_ = msg.x;
    pid_motors[LEFT]->Ki_ = msg.y;
    pid_motors[LEFT]->Kd_ = msg.z;
}

void setPIDCallbackR(const geometry_msgs::Vector3& msg)
{
    pid_motors[RIGHT]->Kp_ = msg.x;
    pid_motors[RIGHT]->Ki_ = msg.y;
    pid_motors[RIGHT]->Kd_ = msg.z;
}

void velocityCallback(const geometry_msgs::Twist& msg)
{
    double linear = msg.linear.x;
    double angular = msg.angular.z;
    calculateDiffSpeed(angular, linear);
}
#endif
void loop() {
    tNew = micros();
    long long delta = (tNew - tOld);
    double deltaDouble = (double)delta * 1.0e-6;

    long long deltaSerial = (tNew - tOld_Serial);
    double deltaDoubleSerial = (double)deltaSerial * 1.0e-6;


    
    if (deltaDouble < dT)
    {
        return;
    }
    tOld = tNew;

    nh.spinOnce();

    
    for (int i = 0; i < 2; i++)
    {
        velocityControl(control_motors[i], enc[i], motors[i], pid_motors[i]);
        actuate(control_motors[i]->u_, motors[i]);
    }
    
    updatePose();
    
    if (deltaDoubleSerial > dT_Serial)
    {

        /* geometry_msgs::Twist vel_msg; */
        /* if (!square->finished()){ */
        /*     square->reinit(pose.x, pose.y); */
        /*     vel_msg = square->getVelocities(pose.x, pose.y, pose.theta);         */
        /* } */
    
        /* double diffLin = abs(previous_vel_msg.linear.x - vel_msg.linear.x); */
        /* double diffAng = abs(previous_vel_msg.angular.z - vel_msg.angular.z); */

        /* calculateDiffSpeed(vel_msg.angular.z, vel_msg.linear.x); */
        /* previous_vel_msg.linear.x = vel_msg.linear.x; */
        /* previous_vel_msg.angular.z = vel_msg.angular.z; */
        
        /* Serial.print("Velocity: "); */
        /* Serial.print(vel_msg.linear.x); */
        /* Serial.print(", "); */
        /* Serial.println(vel_msg.angular.z); */
        odometryPublisher.publish(&pose);
        
        plotData.linear.x = enc[LEFT]->dp_;
        plotData.linear.y = enc[RIGHT]->dp_;
        
        plotData.angular.x = control_motors[LEFT]->r_;
        plotData.angular.y = control_motors[RIGHT]->r_;
        
        plotData.linear.z = control_motors[LEFT]->u_;
        plotData.angular.z = control_motors[RIGHT]->u_;
        dataPublisher.publish(&plotData);
        
        tOld_Serial = tNew;
    }

}
