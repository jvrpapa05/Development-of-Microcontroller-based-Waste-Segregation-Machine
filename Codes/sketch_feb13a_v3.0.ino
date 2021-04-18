// v2.0 - with stepper motor
// v3.0 - Fix stepper motor offset and add button Calibrate and Test.

#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
Servo lid;
LiquidCrystal_I2C lcd(0x27, 20, 4);

const int irA = 2;
const int irB = 3;
boolean irA_bool = false;
boolean irB_bool = false;

const int relay1 = 4;
const int relay2 = 5;

const int hallEffect = 6;

const int stepperDirection = 8;
const int stepperPulses = 7;

const int solenoid = 10;
const int mosfet2 = 11;

const int e18 = 12;
boolean e18Bool = false;

const int buzzer = 13;

const int buttonCalibrate = A0;
const int buttonTest = A1;

const int metalReset = A2;
const int metalDetect = A3;
boolean metalBool = false;

boolean ISR_A = false;
boolean ISR_B = false;
int count = 0;
double timeDifferenceA;
double timeDifferenceB;
double timeA_high;
double timeA_low;
double timeB_high;
double timeB_low;
int typeOfWaste;

//// for motor control
int delayPulse = 100; // value in MICROSECONDS

String degree;
int degreeInt; // convert to integer
int degreePrevious = 0; // store previous degree
int degreeNew;

int revSet = 3200; // motor stepper driver set
int pulsesRev;


//For testing
int testingPos = 0;

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(10);
  Serial.println(F("BEGIN"));

  attachInterrupt(digitalPinToInterrupt(irA), aDetect, CHANGE);
  attachInterrupt(digitalPinToInterrupt(irB), bDetect, CHANGE);

  pinMode(irA, INPUT);
  pinMode(irB, INPUT);

  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  digitalWrite(relay1, LOW);
  digitalWrite(relay2, LOW);

  pinMode(hallEffect, INPUT);

  pinMode(stepperDirection, OUTPUT);
  pinMode(stepperPulses, OUTPUT);

  pinMode(solenoid, OUTPUT);

  lid.attach(9);
  lid.write(0);

  pinMode(mosfet2, OUTPUT);

  pinMode(e18, INPUT_PULLUP);

  pinMode(buzzer, OUTPUT);

  pinMode(buttonCalibrate, INPUT_PULLUP);
  pinMode(buttonTest, INPUT_PULLUP);

  pinMode(metalReset, OUTPUT);
  pinMode(metalDetect, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("AUTOMATIC WASTE");
  lcd.setCursor(0, 1);
  lcd.print("SEGREGATOR SYSTEM");
  lcd.setCursor(0, 2);
  lcd.print("STATUS: Ready");
  lcd.setCursor(0, 3);
  lcd.print("Insert Waste");

  metalCal();

  closeLid();
  stepperCalibrate();
  Beep();
}

void loop() {
  if (digitalRead(e18) == LOW) {
    Serial.println(F("Waste detected. Analyzing."));
    //reset all false alarm values of ISR.
    timeA_high = 0;
    timeA_low = 0;
    timeB_high = 0;
    timeB_low = 0;

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("STATUS: Classifying");
    lcd.setCursor(0, 1);
    lcd.print("Analyzing Waste...");

    getWasteCharacteristics();
    conditions();

    openLid();
    closeLid();

    Beep();

    Serial.println();
    //reset all values
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("AUTOMATIC WASTE");
    lcd.setCursor(0, 1);
    lcd.print("SEGREGATOR SYSTEM");
    lcd.setCursor(0, 2);
    lcd.print("STATUS: Ready");
    lcd.setCursor(0, 3);
    lcd.print("Insert Waste");

    count = 0;
    metalBool = false;
    e18Bool = false;
    irA_bool = false;
    irB_bool = false;
    typeOfWaste = 0;
    timeDifferenceA = 0;
    timeDifferenceB = 0;
  }

  if (digitalRead(buttonTest) == LOW) {
    Serial.print(F("Moving at: "));
    Serial.print(testingPos);
    Serial.println(F(" degrees."));

    rotate(testingPos);

    testingPos = testingPos + 90;
    if (testingPos == 450) {
      testingPos = 0;
    }
  }

  if (digitalRead(buttonCalibrate) == LOW) {
    Serial.println("Resetting the position");
    stepperCalibrate();
    testingPos = 0;
  }

  mainProg();
}


void stepperCalibrate() {
  while (digitalRead(hallEffect) == HIGH) {
    digitalWrite(stepperPulses, HIGH);
    delayMicroseconds(delayPulse);
    digitalWrite(stepperPulses, LOW);
    delayMicroseconds(delayPulse);
  }
}

void conditions() {
  //1 - Metal.
  //2 - Pet Bottles.
  //3 - Candy Plastic Wrapper.
  //4 - Crumpled Paper.

  if (metalBool == LOW) {
    //Serial.println("1");
    typeOfWaste = 1;
  } else if (e18Bool == LOW) {
    //Serial.println("2-A");
    typeOfWaste = 2;
  } else if (digitalRead(irA) == HIGH || digitalRead(irB) == HIGH) {
    //Serial.println("2-B");
    typeOfWaste = 2;
  } else if (timeDifferenceA == 0 && timeDifferenceB < 30) {
    //Serial.println("3-A");
    typeOfWaste = 3;
  } else if (timeDifferenceA < 30 && timeDifferenceB == 0) {
    //Serial.println("3-B");
    typeOfWaste = 3;
  } else if (timeDifferenceA < 10 && timeDifferenceB < 10) {
    //Serial.println("3-C");
    typeOfWaste = 3;
  } else {
    //Serial.println("4");
    typeOfWaste = 4;
  }

  Serial.print(F("tD A: "));
  Serial.println(timeDifferenceA);
  Serial.print(F("tD B: "));
  Serial.println(timeDifferenceB);

  Serial.print(F("Type of Waste: "));
  Serial.println(typeOfWaste);

  lcd.setCursor(0, 3);
  lcd.print("TYPE: ");

  switch (typeOfWaste) {
    case 1:
      Serial.println(F("METAL."));
      lcd.print("Metal");
      rotate(0);
      break;
    case 2:
      Serial.println(F("PET BOTTLE."));
      lcd.print("Pet bottle");
      rotate(90);
      break;
    case 3:
      Serial.println(F("CANDY PLASTIC WRAPPER."));
      lcd.print("Candy Wrapper");
      rotate(180);
      break;
    case 4:
      Serial.println(F("CRUMPLED PAPER."));
      lcd.print("Crumpled Paper");
      rotate(270);
      break;
  }

}


void isr_IRa() {
  if (ISR_A == true) {
    timeDifferenceA = timeA_high - timeA_low;
    if (timeDifferenceA > 0) {
      Serial.println(F("ISR-A Activated."));
      Serial.print(F("Time A High: "));
      Serial.println(timeA_high);
      Serial.print(F("Time A Low: "));
      Serial.println(timeA_low);
      Serial.print(F("Difference A: "));
      Serial.println(timeDifferenceA);
      Serial.println();
    }

    ISR_A = false;
  }
}


void isr_IRb() {
  if (ISR_B == true) {
    timeDifferenceB = timeB_high - timeB_low;
    if (timeDifferenceB > 0) {
      Serial.println(F("ISR-B Activated."));
      Serial.print(F("Time B High: "));
      Serial.println(timeB_high);
      Serial.print(F("Time B Low: "));
      Serial.println(timeB_low);
      Serial.print(F("Difference B: "));
      Serial.println(timeDifferenceB);
      Serial.println();
    }

    ISR_B = false;
  }

}


void getWasteCharacteristics() {

  while (count <= 100) {
    e18Bool = digitalRead(e18);
    metalBool = digitalRead(metalDetect);
    irA_bool = digitalRead(irA);
    irB_bool = digitalRead(irB);
    Serial.println(count);

    lcd.setCursor(0, 2);
    lcd.print("Progress: ");
    lcd.print(count);
    lcd.print("%");

    count++;
  }

  Serial.print(F("E18-D80NK: "));
  Serial.println(e18Bool);
  Serial.print(F("Metal Transducer: "));
  Serial.println(metalBool);
  Serial.print(F("Infrared A: "));
  Serial.println(irA_bool);
  Serial.print(F("Infrared B: "));
  Serial.println(irB_bool);

  isr_IRa();
  isr_IRb();
}

void openLid() {
  Serial.println(F("Open Lid."));
  lid.write(0);
  delay(250);
  digitalWrite(solenoid, HIGH);
  delay(500);
  digitalWrite(solenoid, LOW);
}

void closeLid() {
  Serial.println(F("Close Lid."));
  digitalWrite(solenoid, HIGH);
  lid.write(90);
  delay(500);
  digitalWrite(solenoid, LOW);
  delay(250);
  lid.write(0);
}

void aDetect() {
  if (digitalRead(irA) == LOW) {
    timeA_high = millis();
  } else if (digitalRead(irA) == HIGH) {
    timeA_low = millis();
  }

  ISR_A = true;
}

void bDetect() {
  if (digitalRead(irB) == LOW) {
    timeB_high = millis();
  } else if (digitalRead(irB) == HIGH) {
    timeB_low = millis();
  }

  ISR_B = true;
}


void metalCal() {
  // to calibrate sensor/ in environment.
  digitalWrite(metalReset, HIGH);
  delay(2000);
  digitalWrite(metalReset, LOW);
}


void mainProg() {
  if (Serial.available() > 0) {
    degree = Serial.readString();
    degreeInt = degree.toInt();

    Serial.println(F("----- PROCESSING -----"));
    Serial.print(F("Degree: "));
    Serial.println(degreeInt);

    rotate(degreeInt);
    Serial.println(F("----- DONE -----"));

    Serial.println();
    Serial.println();
  }
}

void dump() {
  Serial.println();
  Serial.println(String("degreeInt -> ") + degreeInt);
  Serial.println(String("degreePrevious -> ") + degreePrevious);
  Serial.println(String("degreeNew -> ") + degreeNew);
}

void rotate(int a) {

  if (degreePrevious < a) {
    digitalWrite(stepperDirection, HIGH);
    degreeNew = a - degreePrevious;
    Serial.println(String("CW Difference: ") + degreeNew);

  } else if (degreePrevious > a) {
    digitalWrite(stepperDirection, LOW);
    degreeNew = degreePrevious - a;
    Serial.println(String("CCW Difference: ") + a);

  } else if (degreePrevious == a) {
    Serial.println(F("NO DIFFERENCE"));
    degreeNew = 0;
  }

  dump();
  Serial.println();

  pulsesRev = map(degreeNew, 0, 360, 0, revSet);

  Serial.print(F("Pulses to Set: "));
  Serial.println(pulsesRev);

  for (int x = 0; x <= pulsesRev; x++) {
    digitalWrite(stepperPulses, HIGH);
    delayMicroseconds(delayPulse);
    digitalWrite(stepperPulses, LOW);
    delayMicroseconds(delayPulse);
  }

  degreePrevious = a;
}


void Beep() {
  digitalWrite(buzzer, HIGH);
  delay(40);
  digitalWrite(buzzer, LOW);
  delay(40);

  digitalWrite(buzzer, HIGH);
  delay(40);
  digitalWrite(buzzer, LOW);
  delay(40);

  digitalWrite(buzzer, HIGH);
  delay(40);
  digitalWrite(buzzer, LOW);
  delay(40);
}
