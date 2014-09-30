//In this version i changed the initial chunk size to 3 bytes to increase
//feed rate control.



//Let us assign the timers
#include <TimerOne.h>

#define BufferSize 600
//Apperantly we cannot make this buffer as much as we we want
//3000 results in no function at arduino. Yet it uploads everything correctly

//Size od the chunks sent 
#define ChunkSize 30
unsigned int ChunkCounter=0;

//Mode for timer counter
// We need to reset the motor counter with drive simultanously
#define MotorModeValue 20000
#define SubtractValue 15000

//Counter to ask about data to system
int Count_Ask=0;
#define NumberOfEmptySpacesBeforeAsking 300  //*100*Drive us


#define LedPin 13

int count =0;
int dlyms= 100;
int turncount1=0;
int turncount2=0;

int CounterForNumOfPotReadings=0;
int NumOfPotReadingsToBeAveraged=8;

int CounterTimer=0;
#define PotTime 50  // *100 usec
int DriveTime = 50, Drive=50;

//Pins Digital
//Motor1
int Mot11=2,Mot12=3,Mot13=4,Mot14=5, Mot1En1=6, Mot1En2=7;
//Motor2
int Mot21=8,Mot22=9,Mot23=10,Mot24=11, Mot2En1=12, Mot2En2=13;
//Pot reading
int pot1=A0, pot2=A1, potVal1, potVal2;
long data;

// int lookupFull[4] = {1,2,4,8};
int lookupHalf[8] = {1,3,2,6,4,12,8,9};



//BufferVariables
byte DataOfMotion[BufferSize];
int start = 0; int endpoint=0;


void setup()
{
  
  //PinModes
  pinMode(Mot11, OUTPUT);
  pinMode(Mot12, OUTPUT);
  pinMode(Mot13, OUTPUT);
  pinMode(Mot14, OUTPUT);
  pinMode(Mot1En1, OUTPUT);
  pinMode(Mot1En2, OUTPUT);
  pinMode(Mot21, OUTPUT);
  pinMode(Mot22, OUTPUT);
  pinMode(Mot23, OUTPUT);
  pinMode(Mot24, OUTPUT);
  pinMode(Mot2En1, OUTPUT);
  pinMode(Mot2En2, OUTPUT);
  
  pinMode(LedPin, OUTPUT); // LED
  
  
  //Set Enable Pins as High
  digitalWrite(Mot1En1, HIGH);
  digitalWrite(Mot1En2, HIGH);
  digitalWrite(Mot2En1, HIGH);
  digitalWrite(Mot2En2, HIGH);
  
 // digitalWrite(LedPin,LOW);
  
   Serial.begin(115200); 
   Serial.setTimeout(5);
   
   Init_Buffer();
   /*
    int dly = word((byte)3,(byte)50);
   
   Serial.print("Oh, crap!!");
   Serial.print(dly);
   Serial.print("          ");
   Serial.println(((byte)3*256+(byte)50));
   */
   //Let us Attach the pot readings to a timer interrupt in UNO
   Timer1.initialize(100);         // initialize timer1, and set a  2000 microsecond period
   Timer1.attachInterrupt(Schecular);  // attaches PotRead() as a timer overflow interrupt
}

void loop()
{
    
  if(Serial.available()>0)
  {
   
  byte tempbyte[3];
  char anychar[3];
  Serial.readBytes(anychar,3);
  //First element for direction of data
  tempbyte[0]=(byte)anychar[0];
  //Second element is high byte of int, Third is low byte of int
  tempbyte[1]=(byte)anychar[1];
  tempbyte[2]=(byte)anychar[2];  
  AddElement(tempbyte);
  
  //If the first element is a byte 90 this means arduino should initilize its buffer
  if(tempbyte[0]==(byte)90)
  {
    Init_Buffer();
    //Show Us the initilization
    /*
    digitalWrite(LedPin,HIGH);
    delay(100);
    digitalWrite(LedPin,LOW);
    */
  }
/*
  if (Serial.read() == '\n') 
    { 
      //No need to echo back
      /*
      Serial.println(dlyms);
      
    }
*/ 
  }
  
}

void Schecular()
{
  
     CounterTimer++;
     
   
           
      if(CounterTimer % PotTime==0)
          {PotRead();}
  
      if(CounterTimer >= DriveTime)
           {MotionRead(); }        
  
}

void TakeModesOfTimeCounters()
{
      //Drive Timer will be higher than Counter Timer, so we need to check it before
     if(DriveTime>=MotorModeValue)
     {
       DriveTime=DriveTime-SubtractValue;
       CounterTimer= CounterTimer-SubtractValue;
     }
         
}


void MotionRead()
{
  //Do not pass the end point
  if(start == endpoint)
  {
    //Just delay te movement a safe time interval
    DriveTime=CounterTimer+Drive;
    
    //Why the heck are we even asking. Just wait
    /*
    //Ask for another chunk of data
    Count_Ask++;
    if(Count_Ask>=NumberOfEmptySpacesBeforeAsking)
    {
     Count_Ask=0; 
      Serial.println("30003000");
    }
    */
    //Mode the counter
    TakeModesOfTimeCounters();
    return;
  }
  //Second&Third Element is time delay for next step
  int dly = word(DataOfMotion[start+1],DataOfMotion[start+2]);
  //if(dly<15*256)
    DriveTime=CounterTimer+dly;
        //Mode the counter
    TakeModesOfTimeCounters();
  //else
  //  DriveTime=CounterTimer+DataOfMotion[start+2];
  //Clear it
  DataOfMotion[start+1]=0;
  DataOfMotion[start+2]=0;
  
    
   //Calculation of next motor movement
   
   //x=0 for <9 case
   if( DataOfMotion[start]<9)
   {
    //No action 
   }
   //X=1
   else if (DataOfMotion[start]<18 )
   {
       turncount1=turnit(false,false,turncount1);
   }
   //X=-1
   else
   {
     turncount1=turnit(true,false,turncount1);
   }
   
   
   //Y=0  for mod 9 <3 case
   if((DataOfMotion[start]%9)<3)
   {
    //No action     
   }
   else if((DataOfMotion[start]%9)<6)
   {
     turncount2=turnit(false,false,turncount2);
   }
   else
   {
     turncount2=turnit(true,false,turncount2);
   }
   
   DataOfMotion[start]=0;
   start+=3;
   if(start >=BufferSize)
     start= start%BufferSize;
   
   
   //Output the motor movement signals
   driveMot1(lookupHalf[turncount1]);
   driveMot2(lookupHalf[turncount2]);
   
   //Send the new data request if this chunk is about to finish
   ChunkCounter++;
   //Make sure it is >= not >
   if(ChunkCounter>=ChunkSize)
   {
    ChunkCounter=0;
    Serial.println("30003000");
    
   }
}


void driveMot1(int an)
{
 digitalWrite(Mot11,an&1);
digitalWrite(Mot12,an&2);
digitalWrite(Mot13,an&4);
digitalWrite(Mot14,an&8);
}

void driveMot2(int an)
{
 digitalWrite(Mot21,an&1);
digitalWrite(Mot22,an&2);
digitalWrite(Mot23,an&4);
digitalWrite(Mot24,an&8);
}
// turnit turns a stepper motor in a given direction 
//with either half step or full step.
// dir: TRUE then CCW , FALSE CW
//steps: TRUE half step, FALSE full step
int turnit(bool dir, bool steps,int turncount)
{
  int adding;
  if(steps)
    adding =1;
  else
    adding = 2;
    
  if(dir)
    turncount=turncount+adding;
  else
    turncount=turncount-adding;
  
  //For forward
  if(turncount>=8)
    turncount = turncount%8;
  // For Backward
  if( turncount ==-1 )
     turncount=7; 
  else if(turncount <=-1)
      turncount=6; 
  // Now since we manipulated the counter, we can drive
  return turncount;
}

//Function for Pot Read 
void PotRead()
{
  
    potVal1+=analogRead(pot1);
    //Serial.println(potVal1);
    potVal2+=analogRead(pot2);
    //Serial.println(potVal2);
    CounterForNumOfPotReadings++;
    // Just Print The Data Whenever multiples of NumOfPot...Averaged is reached
    if(CounterForNumOfPotReadings%NumOfPotReadingsToBeAveraged ==0)
    {
      //Average them
      potVal1=potVal1/NumOfPotReadingsToBeAveraged;
      potVal2=potVal2/NumOfPotReadingsToBeAveraged;
      data=(long)potVal1*10000+(long)potVal2;
       Serial.println(data);
      //Clear the averages
      potVal1=0;
      potVal2=0;
      
    }
}


void Init_Buffer()
{
  for(int i=0; i<BufferSize; i++)
  {
     DataOfMotion[i]=(byte)0;    
  }
  //Initilize the counters
  start=0;
  endpoint=0;
  ChunkCounter=0;
  
}

void AddElement( byte addbyte[] )
{
 DataOfMotion[endpoint]=addbyte[0];
 DataOfMotion[endpoint+1]=addbyte[1];
 DataOfMotion[endpoint+2]=addbyte[2];
 endpoint+=3; 
 if(endpoint>=BufferSize)
   endpoint= endpoint%BufferSize;
 }
 
 
