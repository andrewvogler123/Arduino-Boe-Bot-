/*
 * Roam_with_Ping_on_Servo_Turret
 * BOE Shield-Bot application roams with Ping))) mounted on servo turret.
 * Source: http://learn.parallax.com/BoeShield/RoamingPingShieldBot
 *
 * IMPORTANT: This is an early revision.  Please check back periodically, 
 * and if you find any bugs, please email me at alindsay@parallax.com.
 *
 * Created May 17th, 2012
 * by Andy Lindsay
 * v0.82
 */
                                                                                
#include <Servo.h>                                 // Include servo library

Servo servoLeft;                                   // Servo object instances
Servo servoRight;
Servo servoTurret;  

const int servoLeftPin = 13;                       // I/O Pin constants
const int servoRightPin = 12;
const int servoTurretPin = 11;
const int pingPin = 10;
const int piezoPin = 4;

const int msPerTurnDegree = 6;                     // For maneuvers  
const int tooCloseCm = 16;                         // For distance decisions
const int bumpedCm = 6;
int ccwLim = 1400;                                 // For turret servo control
int rtAngle = 900;
const int usTocm = 29;                             // Ping))) conversion constant

int numBlackLines = 0; 
int PreviousWhiteVal = 1;
int blackLine = 0;
int longBlackline = 0;
int shortBlackline = 0;
int whiteLine = 0;


// Aequence of turret positions.
int sequence[] = {0, 2, 4, 6, 8, 10, 9, 7, 5, 3, 1};

// Declare array to store that many cm distance elements using pre-calculated
// number of elements in array.
const int elements = sizeof(sequence);
int cm[elements];

// Pre-calculate degrees per adjustment of turret servo. 
const int degreesTurret = 180/(sizeof(sequence)/sizeof(int)-1);

int i = -1;                                        // Global index
int sign = 1;                                      // Sign of increments
int theta = -degreesTurret;                        // Set up initial turret angle

void setup()                                       // Built-in initialization block
{
  pinMode(7, INPUT);    // s5
  pinMode(6, INPUT);  // s4
  pinMode(5, INPUT);  // s3
  pinMode(3, INPUT);  //s2
  tone(piezoPin, 3000, 1000);                      // Play tone for 1 second
  delay(1000);                                     // Delay to finish tone
  
  Serial.begin(9600);                              // Open serial connection

  servoLeft.attach(servoLeftPin);                  // Attach left signal to pin 13
  servoRight.attach(servoRightPin);                // Attach right signal to pin 12
  servoTurret.attach(servoTurretPin);              // Attach turret signal to pin 12
  maneuver(0, 0, 1000);                            // Stay still for 1 second
  turret(0);                                       // Set turret to 0 degrees
}  
 
void loop()                                        // Main loop auto-repeats
{
  int s5 = irDetect(7);
  int s4 = irDetect(6);
  int s3 = irDetect(5);
  int s2 = irDetect(3);  

   if ((s2 == 1) || (s3 == 1) ||  (s4 == 1) || (s5 == 1))
    {
    blackLine++;
    Serial.println(blackLine);
    delay(200);
    PreviousWhiteVal = 0;
   
    if (((s3 == 1) && (s2 == 0) && (s5 == 0)) || ((s4 == 1) && (s2 == 0) && (s5 == 0)))
     {
      maneuver(30, 30, 1);
     }    
   /*if ((s3 == 0) && (s4 == 0))
  {
    PreviousWhiteVal = 1;
    maneuver(80, 80, 1);
  }*/
  else if (((s4 && s5) == 0) && ((s2 && s3) == 1))
  {
    maneuver(30, 0, 1);
  }
   else if (((s4 && s5) == 1) && ((s2 && s3) == 0))
  {
    maneuver(0, 30, 1);
  }
   else if (((s4 && s5) == 0) && ((s2 || s3) == 1))
  {
    maneuver(30, 0, 1);
  }
  else if (((s4 || s5) == 1) && ((s2 && s3) == 0))
  {
    maneuver(0, 30, 1);
  }
       }
  else if((s2 == 0) && (s3 == 0) && (s4 == 0) && (s5 == 0))
  {
  maneuver(80, 80);                              // Go full speed forward UFN
  i++;                                             // Increment turret index

  // Advance turret servo to next position in sequence and wait for it to get there.
  theta = sequence[i] * degreesTurret;       
  turret(theta);
  delay(100);                                

  cm[i] = cmDistance();                            // Measure cm from this turret angle
  }
  // If object in +/- 36 degrees from center is within tooCloseCm threshold... 
  if ((sequence[i]>=3) && (sequence[i]<=7) && (cm[i] < tooCloseCm))
  {
    maneuver(0, 0);                                // Stop moving 
    int theta = findOpening();                     // Get opening (in terms of sequence element
    Serial.println("");
    Serial.print("findOpening result ");
    Serial.println(theta);
   
    theta *= degreesTurret;                        // Convert sequence element to degree angle
    
    // Convert degree angle to time the BOE Shield-Bot wheels will have to turn to face
    // direction of turret.
    int turnAngleTime = (90 - theta) * msPerTurnDegree;
    
    if(turnAngleTime < 0)                          // If negative turning angle, 
    {
      maneuver(-200, 200, -turnAngleTime);         // then rotate CCW for turningAngleTime ms
    }
    else                                           // If positive turning angle, 
    {
      maneuver(200, -200, turnAngleTime);          // then rotate CW for turningAngleTime ms 
    }
    maneuver(200, 200);                            // Start going forward again
  }   

  if(i == 10)                                      // If turret at max, go back to zero 
  {
    i = -1;
  }  
}

 int irDetect(int irReceiverPin)
{
  int ir = digitalRead(irReceiverPin);       // IR receiver -> ir variable
  delay(1);                                  // Down time before recheck
  return ir;                                 // Return 1 no detect, 0 detect
}     


/*
 * Control BOE Shield-Bot servo direction, speed, set and forget version.
 * Parameters: speedLeft - left servo speed
 *             speedRight - right servo speed
 *             Backward  Linear  Stop  Linear   Forward
 *             -200      -100......0......100       200
 */ 
void maneuver(int speedLeft, int speedRight)
{
  // Call maneuver with just 1 ms blocking; servos will keep going indefinitely.
  maneuver(speedLeft, speedRight, 1);              
}

/*
 * Control BOE Shield-Bot servo direction, speed and maneuver duration.   
 * Parameters: speedLeft - left servo speed
 *             speedRight - right servo speed
 *             Backward  Linear  Stop  Linear   Forward
 *             -200      -100......0......100       200
 *             msTime - time to block code execution before another maneuver
 * Source:     http://learn.parallax.com/ManeuverFunction
 */ 
void maneuver(int speedLeft, int speedRight, int msTime)
{
  servoLeft.writeMicroseconds(1500 + speedLeft);   // Set Left servo speed
  servoRight.writeMicroseconds(1500 - speedRight); // Set right servo speed
  if(msTime==-1)                                   // if msTime = -1
  {                                  
    servoLeft.detach();                            // Stop servo signals
    servoRight.detach();   
  }
  delay(msTime);                                   // Delay for msTime
}

/*
 * Position the horn of a Parallax Standard Servo
 * Parameter: degreeVal in a range from 90 to -90 degrees. 
 */ 
void turret(int degreeVal)
{
  servoTurret.writeMicroseconds(ccwLim - rtAngle + (degreeVal * 10)); //servoTurret.writeMicroseconds(ccwLim - rtAngle + (degreeVal * 10));
}

/*
 * Get cm distance measurment from Ping Ultrasonic Distance Sensor
 * Returns: distance measurement in cm.   
 */ 
int cmDistance()
{
  int i=0;
  int distance = 0;                                // Initialize distance to zero    
  do                                               // Loop in case of zero measurement
  {
    i++;
    int us = ping(pingPin);                        // Get Ping))) microsecond measurement
    distance = convert(us, usTocm);                // Convert to cm measurement
    delay(3);                                      // Pause before retry (if needed)
    if (i>10)
    {
      Serial.print("In cmDistance, value is ");
      Serial.println(distance);
    }
  }
  while(distance == 0 && i<20);    
  Serial.print("EXITING cmDistance, i is ");
  Serial.println(i);  
  return distance;                                 // Return distance measurement
}

/*
 * Converts microsecond Ping))) round trip measurement to a useful value.
 * Parameters: us - microsecond value from Ping))) echo time measurement.
 *             scalar - 29 for us to cm, or 74 for us to in.
 * Returns:    distance measurement dictated by the scalar.   
 */ 
int convert(int us, int scalar)
{
    return us / scalar / 2;                        // Echo round trip time -> cm
}

/*
 * Initiate and capture Ping))) Ultrasonic Distance Sensor's round trip echo time.
 * Parameter: pin - Digital I/O pin connected to Ping)))
 * Returns:   duration - The echo time in microseconds 
 * Source:    Ping by David A. Mellis, located in File -> Examples -> Sensors
 * To-Do:     Double check timing against datasheet
 */ 
long ping(int pin)
{
  long duration;                                   // Variables for calculating distance
  pinMode(pin, OUTPUT);                            // I/O pin -> output
  digitalWrite(pin, LOW);                          // Start low
  delayMicroseconds(2);                            // Stay low for 2 us
  digitalWrite(pin, HIGH);                         // Send 5 us high pulse
  delayMicroseconds(5);                            
  digitalWrite(pin, LOW);                          
  pinMode(pin, INPUT);                             // Set I/O pin to input 
  duration = pulseIn(pin, HIGH, 25000);            // Measure echo time pulse from Ping)))
  return duration;                                 // Return pulse duration
}    
          
/*
 * Find an opening in system's 180-degree field of distance detection.
 * Returns:    Distance measurement dictated by the scalar
 * To-do:      Clean up and modularize
 *             Incorporate constants
 */ 
int findOpening()
{
                                                    
  int Ai;                                          // Initial turret angle   
  int Af;                                          // Final turret angle
  int k = sequence[i];                             // Copy sequence[i] to local variable
  int ki = k;                                      // Second copy of current sequence[i] 
  int inc;                                         // Increment/decrement variable 
  int dt;                                          // Time increment
  int repcnt = 0;                                  // Repetitions count
  int sMin;                                        // Minimum distance measurement
  

  // Increment or decrement depending on where turret is pointing
  if(k <= 5)                                       
  {
    inc = 1;
  }  
  else
  {
    inc = -1;
  }
  Serial.print("START inc ");
  Serial.println(inc);
  
  Serial.print("sequence[i] ");
  Serial.println(sequence[i]);
  Serial.print("START i ");
  Serial.println(i);
  Serial.print("sizeof(sequence) ");
  Serial.println(sizeof(sequence));

  // Rotate turret until an opening becomes visible.  If it reaches servo limit, 
  // turn back to first angle of detection and try scanning toward other servo limit.
  // If still no opening, rotate robot 90 degrees and try again.
  do
  {
    repcnt++;                                        // Increment repetition count 
    Serial.println("START DO");
    Serial.print("repcnt ");
    Serial.println(repcnt);
    
    if(repcnt > ((sizeof(sequence) / sizeof(int)))*2)// If no opening after two scans
    {
      Serial.println("No Opening detected");
      maneuver(-500, -500, 1200);                     // Back up, turn, and stop to try again
      maneuver(100, -150, 90*6);
      maneuver(0, 0, 1);
    }
    k += inc;                                        // Increment/decrement k
    if(k == -1)                                      // Change inc/dec value if limit reached
    {
      Serial.println("k is -1");
      k = ki;
      inc = -inc;
      dt = 250;                                      // Turret will take longer to get there
    }
    if(k == 11)
    {
      Serial.println("k is 11");
      k = ki;
      inc = -inc;
      dt = 250;
    }  
    // Look for index of next turret position
    i = findIn(k, sequence, sizeof(sequence)/sizeof(int));
    Serial.print("NOW, i is ");
    Serial.println(i);
    Serial.print("sequence[i] ");
    Serial.println(sequence[i]);
    Serial.print("degreesTurret ");
    Serial.println(degreesTurret);
    
    // Set turret to next position
    int theta = sequence[i] * degreesTurret;
    turret(theta);                                   // Position turret      
    delay(dt);                                       // Wait for it to get there
    Serial.print("theta ");
    Serial.println(theta);
    dt = 100;                                        // Reset for small increment turret movement

    cm[i] = cmDistance();                            // Take Ping))) measurement
    
    Serial.print("END of DO, cm[i] ");
    Serial.println(cm[i]);
  }
  while(cm[i] < 16);                                 // Keep checking to edge of obstacle
  
  Serial.println("");
  Serial.println("OUT OF FIRST WHILE LOOP");
  Serial.println("");
  
  sMin = 1000;                                       // Initialize minimum distance to impossibly large value
  for(int t = 0; t <= 10; t++)
  {  
    if(sMin > cm[t]) sMin = cm[t];                   // Use sMin to track smallest distance
  }
  if(sMin < 6)                                       // If less than 6 cm, back up a little
  {
    maneuver(-200, -200, 350);
    k = -1;                                          // Get turret ready to start over
  }  
  maneuver(0, 0);                                    // Stay still indefinitely
  
  Serial.println("AFTER maneuver(0,0)");
  
  // Keep rotating turret until another obstacle that's under 30 cm is detected or the turret
  // reaches the servo limit.  Keep track of the maximum distance and the angle when this portion
  // of the scan started and stopped.

  Ai = sequence[i];                                  // Save initial angle when obstacle disappeared from view
 
  k = sequence[i];                                   // Make a copy of the turret position again
  
  int aMax = -2;                                     // Initialize maximum distance measurements to impossibly small values
  int cmMax = -2;
  
  Serial.println("START 2nd DO LOOP");
  Serial.print("k at this point ");
  Serial.println(k);
  Serial.print("inc is ");
  Serial.println(inc);
  
  do                                                 // Loop for scan
  {
    if (k == 10 || k == 0)
    {
      Serial.print("FORCE k = 5");
      k = 5;
    }
    k += inc;                                        // Inc/dec turret position
    // Look up index for turret position
    Serial.print("findIn parameters k and num_elements ");
    Serial.print(k);
    Serial.print(" ");
    Serial.println(sizeof(sequence)/sizeof(int));
    Serial.print("sequence[0-10] ");
    Serial.print(sequence[0]);
    Serial.print(" ");
    Serial.print(sequence[1]);
    Serial.print(" ");
    Serial.print(sequence[2]);
    Serial.print(" ");
    Serial.print(sequence[3]);
    Serial.print(" ");
    Serial.print(sequence[4]);
    Serial.print(" ");
    Serial.print(sequence[5]);
    Serial.print(" ");
    Serial.print(sequence[6]);
    Serial.print(" ");
    Serial.print(sequence[7]);
    Serial.print(" ");
    Serial.print(sequence[8]);
    Serial.print(" ");
    Serial.print(sequence[9]);
    Serial.print(" ");
    Serial.print(sequence[10]);
    Serial.println("");
    i = findIn(k, sequence, sizeof(sequence)/sizeof(int));
    Serial.print("i IZ ");
    Serial.println(i);
    int theta = sequence[i] * degreesTurret;         // Position turret
    turret(theta);
    delay(100);
    
    cm[i] = cmDistance();                            // Measure distance
  
    if(cm[i]>cmMax)                                  // Keep track of max distance and angle(max distance)
    {
      cmMax = cm[i];
      aMax = sequence[i]; 
    }  
    Serial.print("END of 2nd DO, cm[i] ");
    Serial.println(cm[i]);
    Serial.print("sequence[i] ");
    Serial.println(sequence[i]);
    Serial.print(" i is ");
    Serial.println(i);
  }
  while((cm[i] > 30)&&(sequence[i]!=0)&&(sequence[i]!=10)); 
  
  Serial.println("");
  Serial.println("OUT OF 2ND WHILE LOOP");
  Serial.println("");
  // Keep repeating while the distance measurement > 30 cm, and the turret is not near its 
  // mechanical limits.
  
  Af = sequence[i];                                  // Record final turret position  
  int A = Ai + ((Af-Ai)/2);                          // Calculate middle of opening

  // Set turret index for straight ahead in prep for turn.  
  i = findIn(5, sequence, sizeof(sequence)/sizeof(int));
  int theta = sequence[i] * degreesTurret;           
  turret(theta);
  
  if(sMin < 7)                                       // Turn further if too close   
  {
    if (A < aMax) return aMax; else return A;
  }
  else
  {
    if (A < aMax) return A; else return aMax;
  }    
}  
 
/*
 * Finds the first instance of a value in an array.
 * Parameters: value to search for
 *             array[] to search in
 *             elements - number of elements to search
 * Returns:    index where the matching element was found    
 */ 
int findIn(int value, int array[], int elements)
{
  for(int i = 0; i < elements; i++)
  {
    if(value == array[i]) return i;
  }
  return -1;
}  



