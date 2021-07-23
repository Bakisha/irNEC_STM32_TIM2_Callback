// original source code:  https://github.com/perz/irNEC

uint16_t Interrupt_in_microseconds = 64; // should work with values between 16 and 500 (value is in microseconds)

#define irPIN PB14                      // can be any pin


TIM_TypeDef *Instance = TIM2;                       // can be any unused timer
HardwareTimer *MyTim = new HardwareTimer(Instance);


#define LED_PIN LED_BUILTIN


//volatile unsigned long irTmr;
volatile unsigned long irCommand;

bool waitForStart = false;
bool startSet = false;
volatile bool gotIr = false;
volatile bool repeatIr = false;
volatile byte irBits, irAdr,  irData;


#define NECREPEAT 8000
#define NECSTARTHIGH  14000
#define NECSTARTLOW 4000
#define NECONE 1688
#define NECZERO 800
#define NECONEHIGH 2812


bool irPIN_state_current;
bool irPIN_state_prev;
uint32_t irTimer_current;
uint32_t irTimer_prev;
uint32_t irTimer_diff;


/*  Read NEC ir interrupt function
  Use: irTmr 4bytes
    irCommand 4 bytes
    irBits   1 byte
    irData 1 byte   Some knob has been pushed :-) on the remote if gotIr is true read it and handle (check if its the right adress maybe) after handled set gotIr to false
    irAdr 1 byte  The remote has this adress
    and four boolean
    gotIr =  a ircommand is waiting to be handled
    repeatIr = a ircommand AND a repeatsignal has been sent
*****************************************/
void irRead() {

  irTimer_current = irTimer_current + Interrupt_in_microseconds;

  irPIN_state_current = bool(digitalRead(irPIN));

  if ( (irPIN_state_prev == true) & (irPIN_state_current == false) ) { // falling edge

    irTimer_diff = irTimer_current - irTimer_prev;

    if (startSet) {
      if (irTimer_diff > NECONEHIGH) {
        // Serial.print("NEC one high> ");
        //Serial.print(irTimer_diff);
        // Serial.print(" ");
        // Serial.println(irBits);
        if (irTimer_diff > NECREPEAT)
          repeatIr = true;
        startSet = false;
        irBits = 0;

      }
      else if (irTimer_diff > NECONE) {

        bitSet(irCommand, (31 - irBits));
        irBits++;
        //Serial.print("1 ");

      }
      else if (irTimer_diff > NECZERO) {
        bitClear(irCommand, (31 - irBits));
        irBits++;
        // Serial.print("0 ");
      }
      else {
        startSet = false;
        irBits = 0;
        //Serial.print("Nec command error to short");
      }
    }
    else if (irTimer_diff > NECSTARTHIGH) {
      waitForStart = true;
      //Serial.println(">");
      //Serial.print("Nec wait for start");
      //Serial.println(irTimer_diff);
    }
    else if (irTimer_diff > NECSTARTLOW) {
      startSet = true;
      irBits = 0;
    }

    if (irBits == 32) {
      startSet = false;
      irBits = 0;
      gotIr = true;
      irData = (byte)(( irCommand >> 8) & 0xFF);
      //irDataN = (byte)(0xff & irCommand);
      irAdr = (byte)((irCommand >> 24) & 0xFF);


    }


    ////////////////////////////////////////////////////////////
    irTimer_prev = irTimer_current; // remember the time of falling edge

  }
  irPIN_state_prev = irPIN_state_current;

}

void Update_IT_callback(void)
{
  irRead();
}

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  delay(1000);

  pinMode(irPIN, INPUT_PULLUP);

  pinMode(LED_PIN, OUTPUT);

  MyTim->pause();
  MyTim->setOverflow( Interrupt_in_microseconds, MICROSEC_FORMAT);
  MyTim->attachInterrupt(Update_IT_callback);
  MyTim->resume();


  Serial.println("Setup done");
}

void loop() {
  // put your main code here, to run repeatedly:

  if (gotIr) {
    Serial.println(" ");
    Serial.print("Ir data= ");
    Serial.println(irData, HEX);
    Serial.print("Ir adress= ");
    Serial.println(irAdr, HEX);
    gotIr = false;
    digitalWrite(LED_PIN, LOW);
  }
  if (repeatIr) {
    Serial.print("Ir repeat= ");

    Serial.println(irData, HEX);
    repeatIr = false;
    digitalWrite(LED_PIN, HIGH);
  }
  if (irData == 0x30)
    digitalWrite(LED_PIN, LOW);


  if (irData == 0x18)
    digitalWrite(LED_PIN, HIGH);

  delay(100);


}
