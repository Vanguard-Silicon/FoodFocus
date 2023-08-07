#include <HX711.h>
#include <RTClib.h>
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "pitches.h" 11

#define SALT_LIMIT 05   // 5 grams
#define SUGAR_LIMIT 50  // 50 grams
#define OIL_LIMIT 20    // 20 grams

#define DT_LOADCELL_1 13
#define DT_LOADCELL_2 10
#define DT_LOADCELL_3 9

#define SCK_LOADCELLS 8

#define POTENTIOMETER A0

#define PUSH_BUTTON_MULTI 3
#define PUSH_BUTTON_REPORT 2

#define SALT 1
#define SUGAR 2
#define OIL 3

#define BUZZER 11

#define SALT_LED 7
#define SUGAR_LED 6
#define OIL_LED 5

#define SPICE_TOLERANCE_LOWER 1       // 1g sensitivity. Adjust necessarilly.
#define SPICE_TOLERANCE_UPPER 100     // 100g has been set as the maximum amount for one time.
#define SPICE_ADDITION_TOLERANCE 150  // Minimum 150g should be added.

#define MAX_MEMBERS 8
#define POT_VALUE 100000
#define POT_RESISTOR_VALUE 1000  // Change accordingly


bool clockState = true;
bool silentMode = false;
int soundStatus = 0;
int time;
int potPreviousVal;

HX711 scale1, scale2, scale3;
RTC_DS1307 rtc;

char *menu[] = { "<   Sound Profile  >", "<   Member Count   >", "<       Timer      >" };

int h, m, s, yr, mt, dt, dy, olds;  // hous, minutes, seconds, year, month, date of the month, day, previous second
char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
char *MTH[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

float saltConsumption = 0.0, sugarConsumption = 0.0, oilConsumption = 0.0;
float saltWeight = 0.0, sugarWeight = 0.0, oilWeight = 0.0;
LiquidCrystal_I2C lcd(0x27, 20, 4);  // Default address is 0x27 (39).
bool saltBeeped = false, sugarBeeped = false, oilBeeped = false;
byte numMembers = 1;

int alarmTime = 0;
int alarmOnTime = 0;
// unsigned long systemTime;

// notes in the startup sound:
int startupMelody[] = {
  NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5
};

// note durations for the startup sound:
int startupNoteDurations[] = {
  8, 8, 8, 8
};

// Notes for the closing sound
int closingMelody[] = {
  NOTE_C5, NOTE_D5, NOTE_E5, NOTE_F5, NOTE_G5
};

// Note durations for the closing sound
int closingNoteDurations[] = {
  4, 4, 4, 4, 4
};

// Notes for the notification sound
int notificationMelody[] = {
  NOTE_A4, NOTE_E5, NOTE_A5
};

// Note durations for the notification sound
int notificationNoteDurations[] = {
  8, 8, 4
};

// Sound icon character
byte soundIcon[] = {
  B00100,
  B01101,
  B11100,
  B11101,
  B11100,
  B01101,
  B00100,
  B00000
};

// Sound icon character
byte soundOffIcon[] = {
  B10100,
  B01100,
  B11101,
  B11110,
  B11101,
  B01100,
  B10100,
  B00000
};

/////////////////////////////////////////////////////
void showClock() {


  lcd.setCursor(5, 0);
  lcd.print("Food Focus");

  // read all time and date and display
  DateTime now = rtc.now();
  h = now.hour();
  m = now.minute();
  s = now.second();
  yr = now.year();
  mt = now.month();
  dt = now.day();
  dy = now.dayOfTheWeek();
  olds = s;

  // fill the display with all the data
  printbig((h % 10), 3);
  printbig((h / 10), 0);
  printbig((m % 10), 10);
  printbig((m / 10), 7);
  printbig((s % 10), 17);
  printbig((s / 10), 14);

  lcd.setCursor(3, 3);
  lcd.print(MTH[mt - 1]);
  lcd.setCursor(7, 3);
  lcd.print(dt / 10);
  lcd.print(dt % 10);
  lcd.setCursor(10, 3);
  lcd.print(daysOfTheWeek[dy]);

  s = now.second();          // read seconds
  if (olds != s) {           // if seconds changed
    printbig((s % 10), 17);  // display seconds
    printbig((s / 10), 14);
    olds = s;
    if (s == 0) {              // minutes change
      m = now.minute();        // read minutes
      printbig((m % 10), 10);  // display minutes
      printbig((m / 10), 7);
      if (m == 0) {             // hours change
        h = now.hour();         // read hours
        printbig((h % 10), 3);  // dislay hours
        printbig((h / 10), 0);
        if (h == 0) {      // day change
          dt = now.day();  // read day
          dy = now.dayOfTheWeek();
          mt = now.month();  // read month
          yr = now.year();   // read year
          lcd.setCursor(3, 3);
          lcd.print(MTH[mt]);
          lcd.setCursor(7, 3);
          lcd.print(dt / 10);
          lcd.print(dt % 10);
          lcd.setCursor(10, 3);
          lcd.print(daysOfTheWeek[dy]);
        }
      }
    }
  }
}

int potRead() {
  return 1023 - analogRead(A0);
}

void printbig(int i, int x) {
  //  prints each segment of the big numbers

  if (i == 0) {
    lcd.setCursor(x, 1);
    lcd.write(8);
    lcd.write(1);
    lcd.write(2);
    lcd.setCursor(x, 2);
    lcd.write(3);
    lcd.write(4);
    lcd.write(5);
  } else if (i == 1) {
    lcd.setCursor(x, 1);
    lcd.write(8);
    lcd.write(255);
    lcd.print(" ");
    lcd.setCursor(x, 2);
    lcd.print(" ");
    lcd.write(255);
    lcd.print(" ");
  }

  else if (i == 2) {
    lcd.setCursor(x, 1);
    lcd.write(1);
    lcd.write(6);
    lcd.write(2);
    lcd.setCursor(x, 2);
    lcd.write(3);
    lcd.write(7);
    lcd.write(4);
  }

  else if (i == 3) {
    lcd.setCursor(x, 1);
    lcd.write(1);
    lcd.write(6);
    lcd.write(2);
    lcd.setCursor(x, 2);
    lcd.write(4);
    lcd.write(7);
    lcd.write(5);
  }

  else if (i == 4) {
    lcd.setCursor(x, 1);
    lcd.write(3);
    lcd.write(4);
    lcd.write(2);
    lcd.setCursor(x, 2);
    lcd.print("  ");
    lcd.write(5);
  }

  else if (i == 5) {
    lcd.setCursor(x, 1);
    lcd.write(255);
    lcd.write(6);
    lcd.write(1);
    lcd.setCursor(x, 2);
    lcd.write(7);
    lcd.write(7);
    lcd.write(5);
  }

  else if (i == 6) {
    lcd.setCursor(x, 1);
    lcd.write(8);
    lcd.write(6);
    lcd.print(" ");
    lcd.setCursor(x, 2);
    lcd.write(3);
    lcd.write(7);
    lcd.write(5);
  }

  else if (i == 7) {
    lcd.setCursor(x, 1);
    lcd.write(1);
    lcd.write(1);
    lcd.write(5);
    lcd.setCursor(x, 2);
    lcd.print(" ");
    lcd.write(8);
    lcd.print(" ");
  }

  else if (i == 8) {
    lcd.setCursor(x, 1);
    lcd.write(8);
    lcd.write(6);
    lcd.write(2);
    lcd.setCursor(x, 2);
    lcd.write(3);
    lcd.write(7);
    lcd.write(5);
  }

  else if (i == 9) {
    lcd.setCursor(x, 1);
    lcd.write(8);
    lcd.write(6);
    lcd.write(2);
    lcd.setCursor(x, 2);
    lcd.print(" ");
    lcd.write(4);
    lcd.write(5);
  }
}

void playMelody(int melody[], int noteDurations[], int length) {
  if (silentMode == true) { return true; }
  for (int thisNote = 0; thisNote < length; thisNote++) {
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(BUZZER, melody[thisNote], noteDuration);
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    noTone(BUZZER);
  }
}

byte getMembers() {
  return numMembers;
  const float lowerValue = POT_RESISTOR_VALUE / (POT_RESISTOR_VALUE + POT_VALUE) * 5;
  return (int)(((1023 - analogRead(POTENTIOMETER)) / 1023.0 * 4.9) / (5 - lowerValue) * (MAX_MEMBERS) + 1);
  //return (int) ((analogRead(POTENTIOMETER)));
}

// This plays an animation when booting up the device.
void bootup() {
  //return true;
  const int delayTime = 500;
  lcd.clear();
  lcd.setCursor(6, 1);
  lcd.print(" o d");
  delay(delayTime);
  lcd.setCursor(8, 2);
  lcd.print(" o u");
  delay(delayTime);
  lcd.setCursor(6, 1);
  lcd.print("Fo d");
  delay(delayTime);
  lcd.setCursor(8, 2);
  lcd.print("Fo us");
  delay(delayTime);
  lcd.setCursor(6, 1);
  lcd.print("Food");
  delay(delayTime);
  lcd.setCursor(8, 2);
  lcd.print("Focus");
  delay(delayTime * 5);

  lcd.clear();
  lcd.setCursor(2, 2);
  lcd.print("Initializing.");
  delay(delayTime * 3);

  lcd.setCursor(2, 2);
  lcd.print("Initializing..");
  delay(delayTime * 3);

  lcd.setCursor(2, 2);
  lcd.print("Initializing...");
  playMelody(startupMelody, startupNoteDurations, 4);
  lcd.clear();
}

float getWeight(int cellNumber) {
  if (cellNumber == 1)
    return scale1.get_units();
  else if (cellNumber == 2)
    return scale2.get_units();
  else if (cellNumber == 3)
    return scale3.get_units();
}

void calibrateLoadCell(HX711 *scale, float value) {
  scale->set_scale(value);  // This will give answers in grams.
}

void justConsumed(int spice, float usage) {
  // A float is usually 4 bytes.
  if (spice == SALT) {
    saltConsumption += usage;
    saltWeight -= usage;
    EEPROM.put(0, saltConsumption);  // EEPROM address 0 (0 1 2 3)
  }
  if (spice == SUGAR) {
    sugarConsumption += usage;
    sugarWeight -= usage;
    EEPROM.put(sizeof(float), sugarConsumption);  // EEPROM address 4 (4 5 6 7)
  }
  if (spice == OIL) {
    oilConsumption += usage;
    oilWeight -= usage;
    EEPROM.put(2 * sizeof(float), oilConsumption);  // EEPROM address 8 (8 9 10 11)
  }

  // First 12 bytes of the EEPROM is used to store the current usage of the day.
  // Using EEPROM reduces the lost of data in case of a power failure.
}

void justAdded(int spice, float addition) {
  if (spice == SALT) {
    saltWeight += addition;
  }
  if (spice == SUGAR) {
    sugarWeight += addition;
  }
  if (spice == OIL) {
    oilWeight += addition;
  }
}

bool confirmAddition(int spice) {
  //digitalWrite(BUZZER, HIGH);
  playMelody(notificationMelody, notificationNoteDurations, 3);

  unsigned long time = millis();

  lcd.clear();
  String spiceName = "";
  if (spice == SALT) spiceName = "Salt";
  else if (spice == SUGAR) spiceName = "Sugar";
  else if (spice == OIL) spiceName = "Oil";

  lcd.setCursor(0, 0);
  lcd.print("Did you add " + spiceName + "?");
  lcd.setCursor(0, 2);
  lcd.print("Press \"Confirm\"");
  lcd.setCursor(0, 3);
  lcd.print("button to confirm.");

  if (spice == SALT) {
    digitalWrite(SALT_LED, HIGH);
  } else if (spice == SUGAR) {
    digitalWrite(SUGAR_LED, HIGH);
  } else if (spice == OIL) {
    digitalWrite(OIL_LED, HIGH);
  }

  // The program will wait for 5 seconds for the user confirmation
  while (millis() - time <= 5000) {
    //lcd.setCursor(0, 1);
    //lcd.print("first");
    if (millis() - time >= 1000) {
      digitalWrite(BUZZER, LOW);
    }
    if (digitalRead(PUSH_BUTTON_MULTI) == HIGH) {
      // Confirmed.
      if (spice == SALT) {
        digitalWrite(SALT_LED, LOW);
        digitalWrite(BUZZER, LOW);
      } else if (spice == SUGAR) {
        digitalWrite(SUGAR_LED, LOW);
        digitalWrite(BUZZER, LOW);
      } else if (spice == OIL) {
        digitalWrite(OIL_LED, LOW);
        digitalWrite(BUZZER, LOW);
      }
      lcd.clear();
      return true;
    }
  }
  lcd.clear();
  return false;
}

typedef struct {
  byte numOfMembers;
  byte month;
  byte day;
  float saltConsumption;
  float sugarConsumption;
  float oilConsumption;
} consumptionData;

void storeConsumption() {
  DateTime dateTime = rtc.now();
  float consumptionSalt, consumptionSugar, consumptionOil;
  EEPROM.get(0, consumptionSalt);
  EEPROM.get(sizeof(float), consumptionSugar);
  EEPROM.get(2 * sizeof(float), consumptionOil);
  consumptionData consumption = { (byte)getMembers(), (byte)dateTime.month(), (byte)dateTime.day(),
                                  (float)(consumptionSalt), (float)(consumptionSugar), (float)(consumptionOil) };

  int storeIndex = 20 + (dateTime.day() * sizeof(consumptionData));  // The struct has a size of 8 bytes
  EEPROM.put(storeIndex, consumption);
}

typedef struct mA {
  float saltAverage;
  float sugarAverage;
  float oilAverage;
} monthlyAverage;

// Seems like the direct use of the typedef name in function return type is not working.
struct mA calculateTheAverage(int days) {
  float pastMonth[3] = {};
  consumptionData temp;
  for (int i = 1; i <= days; i++) {
    EEPROM.get(20 + i * sizeof(consumptionData), temp);
    pastMonth[SALT - 1] += (float)temp.saltConsumption / temp.numOfMembers;
    pastMonth[SUGAR - 1] += (float)temp.sugarConsumption / temp.numOfMembers;
    pastMonth[OIL - 1] += (float)temp.oilConsumption / temp.numOfMembers;
  }
  // Dividing by 30 is not exactly correct, as not every month has 30 days, specially the current month.
  // But, it has been used for simplicity, otherwise more data such as the current day in the month need to be taken into account.
  static monthlyAverage averages = { pastMonth[SALT - 1] / 30, pastMonth[SUGAR - 1] / 30, pastMonth[OIL - 1] / 30 };

  return averages;
}

float *calculateLinearRegression() {
  int sigma_x = 0;
  int sigma_x2 = 0;
  int sigma_y_salt = 0, sigma_y_sugar = 0, sigma_y_oil = 0;
  int sigma_xy_salt = 0, sigma_xy_sugar = 0, sigma_xy_oil = 0;

  int monthCount;
  EEPROM.get(16, monthCount);
  for (int i = 0; i < monthCount; i++) {
    monthlyAverage temp;
    EEPROM.get(500 + i * sizeof(monthlyAverage), temp);
    sigma_x += i;

    sigma_x2 += i * i;

    sigma_y_salt += temp.saltAverage;
    sigma_y_sugar += temp.sugarAverage;
    sigma_y_oil += temp.oilAverage;

    sigma_xy_salt += i * temp.saltAverage;
    sigma_xy_sugar += i * temp.sugarAverage;
    sigma_xy_oil += i * temp.oilAverage;
  }

  // y = a1 * x + a0
  // a1 - gradient, a0 - intersept

  float a0_salt = (float)((sigma_xy_salt * sigma_x) - (sigma_y_salt * sigma_x2)) / (float)((sigma_x * sigma_x) - ((monthCount - 1) * sigma_x2));
  float a0_sugar = (float)((sigma_xy_sugar * sigma_x) - (sigma_y_sugar * sigma_x2)) / (float)((sigma_x * sigma_x) - ((monthCount - 1) * sigma_x2));
  float a0_oil = (float)((sigma_xy_oil * sigma_x) - (sigma_y_oil * sigma_x2)) / (float)((sigma_x * sigma_x) - ((monthCount - 1) * sigma_x2));

  float a1_salt = (float)((sigma_xy_salt * (monthCount - 1)) - (sigma_y_salt * sigma_x)) / (float)((sigma_x2 * (monthCount - 1)) - (sigma_x * sigma_x));
  float a1_sugar = (float)((sigma_xy_sugar * (monthCount - 1)) - (sigma_y_sugar * sigma_x)) / (float)((sigma_x2 * (monthCount - 1)) - (sigma_x * sigma_x));
  float a1_oil = (float)((sigma_xy_oil * (monthCount - 1)) - (sigma_y_oil * sigma_x)) / (float)((sigma_x2 * (monthCount - 1)) - (sigma_x * sigma_x));

  // Needs to make it static because C/C++ does not return local variable pointers to the outside of the function.
  static float coefficients[] = { a0_salt, a0_sugar, a0_oil, a1_salt, a1_sugar, a1_oil };

  return coefficients;
}

void viewDetailedReport() {

  // DISPLAYING FIRST PAGE
  // Displaying the headings
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Monthly Averages (g)");  // of last three months

  int monthCount;
  EEPROM.get(16, monthCount);
  monthlyAverage avg;

  // Displaying the average in this month
  const int days = 3;  // 3 is the example number of days in the demonstration.
  avg = calculateTheAverage(days);
  lcd.setCursor(0, 1);
  lcd.print("This|");
  lcd.setCursor(5, 1);
  lcd.print(avg.sugarAverage, 1);
  lcd.setCursor(10, 1);
  lcd.print(avg.saltAverage, 1);
  lcd.setCursor(15, 1);
  lcd.print(avg.oilAverage, 1);

  // Displaying the average in the previous month
  EEPROM.get(500 + (monthCount - 1) * sizeof(monthlyAverage), avg);
  lcd.setCursor(0, 2);
  lcd.print("Prev|");
  lcd.setCursor(5, 2);
  lcd.print(avg.sugarAverage, 1);
  lcd.setCursor(10, 2);
  lcd.print(avg.saltAverage, 1);
  lcd.setCursor(15, 2);
  lcd.print(avg.oilAverage, 1);


  // Displaying the average in the month before the previous month
  EEPROM.get(500 + (monthCount - 2) * sizeof(monthlyAverage), avg);
  lcd.setCursor(0, 3);
  lcd.print("Bef|");
  lcd.setCursor(5, 3);
  lcd.print(avg.sugarAverage, 1);
  lcd.setCursor(10, 3);
  lcd.print(avg.saltAverage, 1);
  lcd.setCursor(15, 3);
  lcd.print(avg.oilAverage, 1);

  delay(7000);
  lcd.clear();

  // Apply linear regression to calculate the overall pattern over the recorded days
  float *coefficients;
  coefficients = calculateLinearRegression();
  lcd.setCursor(2, 0);
  lcd.print("Overall Analysis");
  lcd.setCursor(0, 1);
  lcd.print("Sugar");
  lcd.setCursor(7, 1);
  lcd.print("Salt");
  lcd.setCursor(14, 1);
  lcd.print("Oil");

  // lcd.setCursor(0, 2);
  // lcd.print("Grd");  // "Gradient" - Lengthy words cannot be used.
  lcd.setCursor(0, 2);
  lcd.print(coefficients[4], 2);
  lcd.setCursor(7, 2);
  lcd.print(coefficients[3], 2);
  lcd.setCursor(14, 2);
  lcd.print(coefficients[5], 2);

  // lcd.setCursor(0, 3);
  // lcd.print("Grwt");  // "Growth" - Lengthy words cannot be used.

  lcd.setCursor(0, 3);
  float value = coefficients[4] * monthCount + coefficients[1];
  if ((value > 0 && coefficients[4] > 0) || (value < 0 && coefficients[4] < 0))  // Positive gradient and positive value for sugar
    lcd.print("Poor");
  else
    lcd.print("Good");

  lcd.setCursor(7, 3);
  value = coefficients[3] * monthCount + coefficients[0];
  if ((value > 0 && coefficients[3] > 0) || (value < 0 && coefficients[3] < 0))  // Positive gradient and positive value for salt
    lcd.print("Poor");
  else
    lcd.print("Good");

  lcd.setCursor(14, 3);
  value = coefficients[5] * monthCount + coefficients[2];
  if ((value > 0 && coefficients[5] > 0) || (value < 0 && coefficients[5] < 0))  // Positive gradient and positive value for oil
    lcd.print("Poor");
  else
    lcd.print("Good");

  delay(7000);

  lcd.clear();
}

void handlePotentiometer() {
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("------------");
  lcd.setCursor(3, 1);
  lcd.print("| Member     |");
  lcd.setCursor(3, 2);
  lcd.print("| Count :    |");
  lcd.setCursor(14, 2);
  lcd.print(getMembers());
  lcd.setCursor(4, 3);
  lcd.print("------------");

  //playMelody(notificationMelody, notificationNoteDurations, 3);
  tone(BUZZER, NOTE_A1, 16);
  delay(20.8);

  unsigned long time = millis();
  while (millis() - time <= 5000) {  // Wait for 5 seconds.
                                     //lcd.setCursor(0, 1);
                                     //lcd.print("second");
    lcd.setCursor(14, 2);
    lcd.print(getMembers());
    numMembers = getMembers();
    if ((numMembers != getMembers()) && (millis() - time > 100)) {
      if (millis() - time > 200) {
        digitalWrite(BUZZER, HIGH);
        delay(20.8);
        digitalWrite(BUZZER, LOW);
      }
      time = millis();  // Resets time again.
    }
  }
  lcd.clear();
}

void initializeForDemonstration() {
  // Initialize the first three values in EEPROM to zero.
  EEPROM.put(0, 0.0);
  EEPROM.put(sizeof(float), 0.0);
  EEPROM.put(2 * sizeof(float), 0.0);

  // saltWeight = getWeight(SALT);
  // sugarWeight = getWeight(SUGAR);
  // oilWeight = getWeight(OIL);
}

// This function will place some values in the EEPROM as the accumulated data in the previous months.
void initialize() {
  // Put 10 to EEPROM address 15 as the number of months.
  const int monthCount = 10;
  EEPROM.put(16, monthCount);

  // Create dummy list of the average consumptions of past months
  monthlyAverage dummyAverages[monthCount] = { { 4.3, 43.0, 15.0 },
                                               { 2.7, 36.8, 22.4 },
                                               { 5.7, 36.4, 29.6 },
                                               { 2.1, 34.5, 21.0 },
                                               { 3.8, 27.9, 12.3 },
                                               { 2.6, 25.0, 14.5 },
                                               { 3.8, 64.8, 12.4 },
                                               { 2.6, 50.6, 17.3 },
                                               { 4.4, 53.8, 16.2 },
                                               { 4.0, 48.7, 18.8 } };

  // Store the dummy data in the EEPROM
  for (int i = 0; i < monthCount; i++) {
    EEPROM.put(500 + i * sizeof(monthlyAverage), dummyAverages[i]);
  }

  // Create a dummy list for the consumption of this month
  // The values are multiplied by 100.
  consumptionData dummyConsumptions[3] = { { 5, 7, 1, 30.89, 234.76, 86.10 },
                                           { 3, 7, 2, 12.02, 112.34, 48.90 },
                                           { 4, 7, 3, 16.89, 163.39, 82.55 } };

  // Store the dummy data in the EEPROM
  for (int i = 1; i <= 3; i++) {
    EEPROM.put(20 + i * sizeof(consumptionData), dummyConsumptions[i - 1]);
  }
}

void multiLongPress() {
  int longPressTime = 2000;  // 2 seconds
  bool allTime = true;
  unsigned long time = millis();
  while (millis() - time <= longPressTime) {  // Long press for 2 seconds
                                              //lcd.setCursor(0, 1);
                                              //lcd.print("third");
    if (digitalRead(PUSH_BUTTON_MULTI) == LOW) {
      allTime = false;
      break;
    }
  }
  if (allTime) {  // Long press of 2 seconds detected.
    lcd.clear();
    lcd.setCursor(4, 1);
    lcd.print("Resetting...");

    initializeForDemonstration();

    // First three EEPROM cells should contain a valid value for the consumption details.
    EEPROM.get(0, saltConsumption);
    EEPROM.get(sizeof(float), sugarConsumption);
    EEPROM.get(2 * sizeof(float), oilConsumption);

    playMelody(closingMelody, closingNoteDurations, 3);
  }
  lcd.clear();
}

void setup() {
  int sys_Time = int(millis() / 20000);
  pinMode(POTENTIOMETER, INPUT);

  pinMode(SALT_LED, OUTPUT);
  pinMode(SUGAR_LED, OUTPUT);
  pinMode(OIL_LED, OUTPUT);

  pinMode(BUZZER, OUTPUT);

  Serial.begin(9600);

  scale1.begin(DT_LOADCELL_1, SCK_LOADCELLS);
  scale2.begin(DT_LOADCELL_2, SCK_LOADCELLS);
  scale3.begin(DT_LOADCELL_3, SCK_LOADCELLS);

  calibrateLoadCell(&scale1, 418);
  calibrateLoadCell(&scale2, 450);
  calibrateLoadCell(&scale3, 466);

  // To waste some first measurements
  scale1.get_units(10);
  scale2.get_units(10);
  scale3.get_units(10);

  // Setting RTC to the date and time at compile time.
  rtc.begin();

  // THIS MIGHT CAUSE PROBLEMS WHEN ATmega328p IS REMOVED FROM THE ARDUINO BOARD OR DISCONNECT FROM THE PC.
  // In such cases, follow these steps.
  // 1. Connect the RTC only to a Arduino board and run the below code.
  // 2. Disconnet the RTC. (As it has an internal battery, the time will not be lost.)
  // 3. Delete the below line and recompile the code before uploading to the Atmega328p.
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // Int is 2 bytes.
  EEPROM.put(12, (int)rtc.now().day());
  EEPROM.put(14, (int)rtc.now().month());

  saltWeight = getWeight(SALT);
  sugarWeight = getWeight(SUGAR);
  oilWeight = getWeight(OIL);

  lcd.init();
  lcd.clear();
  lcd.backlight();

  // Initialize EEPROM memory to all 0, from 21 to 1023
  for (int i = 21; i < 1024; i++) {
    EEPROM.put(i, (byte)0);
  }

  // Put month counter to cell 16 and update only when a month is renewed.
  EEPROM.put(16, 0);

  numMembers = getMembers();

  // ================ REMEMBER ==================
  // Comment out this line in the actual product code.
  initialize();

  // First three EEPROM cells should contain a valid value for the consumption details.
  EEPROM.get(0, saltConsumption);
  EEPROM.get(sizeof(float), sugarConsumption);
  EEPROM.get(2 * sizeof(float), oilConsumption);

  // Boot-up animation
  bootup();

  // systemTime = millis();

  rtc.begin();
  Serial.begin(9600);
  // set up the LCD's number of columns and rows:
  lcd.init();  // initialize the lcd
  lcd.backlight();
  lcd.clear();

  // *******DEFINE CUSTOM CHARACTERS FOR BIG FONT*****************
  byte A[8] = {
    B00011,
    B00111,
    B01111,
    B01111,
    B01111,
    B01111,
    B01111,
    B01111
  };
  byte B[8] = {
    B11111,
    B11111,
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B11111
  };
  byte C[8] = {
    B11000,
    B11100,
    B11110,
    B11110,
    B11110,
    B11110,
    B11110,
    B11110
  };
  byte D[8] = {
    B01111,
    B01111,
    B01111,
    B01111,
    B01111,
    B01111,
    B00111,
    B00011
  };
  byte E[8] = {
    B11111,
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B11111,
    B11111
  };
  byte F[8] = {
    B11110,
    B11110,
    B11110,
    B11110,
    B11110,
    B11110,
    B11100,
    B11000
  };
  byte G[8] = {
    B11111,
    B11111,
    B11111,
    B00000,
    B00000,
    B00000,
    B00000,
    B00000
  };
  byte H[8] = {
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B11111,
    B11111,
    B11111
  };

  lcd.createChar(8, A);
  lcd.createChar(6, B);
  lcd.createChar(2, C);
  lcd.createChar(3, D);
  lcd.createChar(7, E);
  lcd.createChar(5, F);
  lcd.createChar(1, G);
  lcd.createChar(4, H);

  // set a particular date and time if you want (yyyy,mm,dd,hh,mm,ss)
  //   rtc.adjust(DateTime(2021, 2, 16, 11, 00, 00));


  lcd.setCursor(5, 0);
  lcd.print("Food Focus");


  // read all time and date and display
  DateTime now = rtc.now();
  h = now.hour();
  m = now.minute();
  s = now.second();
  yr = now.year();
  mt = now.month();
  dt = now.day();
  dy = now.dayOfTheWeek();
  olds = s;
/*
  // fill the display with all the data
  printbig((h % 10), 3);
  printbig((h / 10), 0);
  printbig((m % 10), 10);
  printbig((m / 10), 7);
  printbig((s % 10), 17);
  printbig((s / 10), 14);

  lcd.setCursor(3, 3);
  lcd.print(MTH[mt - 1]);
  lcd.setCursor(7, 3);
  lcd.print(dt / 10);
  lcd.print(dt % 10);
  lcd.setCursor(10, 3);
  lcd.print(daysOfTheWeek[dy]);

*/
  lcd.clear();

  time = millis();
  potPreviousVal = potRead();
}

void loop() {
  if (silentMode){lcd.setCursor(19, 0);lcd.print("x");}
  

  if ((int(millis() / 1000) % 20) < 5) {
    if (clockState) {
      lcd.clear();
      clockState = false;
    }
    //lcd.clear();
    showClock();
  }

  if (abs(potPreviousVal - potRead()) > 200) {
    lcd.clear();
    int time = int(millis() / 1000);
    potPreviousVal = potRead();

    while (int(millis() / 1000) - time < 7) {
      lcd.setCursor(4, 0);
      lcd.print("--MAIN MENU--");

      if (potRead() < 341) {

        if (soundStatus != 1){soundStatus = 1; if(!silentMode){tone(BUZZER, NOTE_A1, 16);} delay(20.8);}

        lcd.setCursor(0, 2);
        lcd.print(menu[0]);
        if (digitalRead(3) == 1) {
          delay(200);
          int time = int(millis() / 1000);
          potPreviousVal = potRead();

          while (int(millis() / 1000) - time < 3) {

            if (abs(potPreviousVal - potRead()) > 100) {
              time = int(millis() / 1000);
              potPreviousVal = potRead();
            }

            if (potPreviousVal < 500) {
              if (soundStatus != 2){soundStatus = 2; if(!silentMode){tone(BUZZER, NOTE_A1, 16);} delay(20.8);}

              lcd.setCursor(0, 2);
              lcd.print("Silent Mode: < OFF >");
              if (digitalRead(3) == 1) {
                silentMode = false;
                time = int(millis() / 1000) - 10;
                delay(200);
                potPreviousVal = potRead();
              }
            } else {
              if (soundStatus != 3){soundStatus = 3; if(!silentMode){tone(BUZZER, NOTE_A1, 16);} delay(20.8);}

              lcd.setCursor(0, 2);
              lcd.print("Silent Mode: < ON  >");
              if (digitalRead(3) == 1) {
                silentMode = true;
                time = int(millis() / 1000) - 10;
                delay(200);
                potPreviousVal = potRead();
              }
            }
          }
        }

      } else if (potRead() < 682) {
        if (soundStatus != 4){soundStatus = 4; if(!silentMode){tone(BUZZER, NOTE_A1, 16);} delay(20.8);}

        lcd.setCursor(0, 2);
        lcd.print(menu[1]);
        if (digitalRead(3) == 1) {
          delay(200);
          int time = int(millis() / 1000);
          int potPreviousVal = potRead();

          while (int(millis() / 1000) - time < 3) {

            if (abs(potPreviousVal - potRead()) > 100) {
              if(!silentMode){tone(BUZZER, NOTE_A1, 16);} delay(20.8);
              time = int(millis() / 1000);
              potPreviousVal = potRead();
            }

            lcd.setCursor(0, 2);
            lcd.print("Member Count:  < " + String(int(potPreviousVal / 120) + 1) + " >");
            if (digitalRead(3) == 1) {
              if (soundStatus != 5){soundStatus = 5; playMelody(notificationMelody, notificationNoteDurations, 3); delay(20.8);}

              numMembers = int(potPreviousVal / 120) + 1;
              time = int(millis() / 1000) - 10;
              delay(200);
              lcd.clear();
              lcd.setCursor(0, 2);
              lcd.print("   Member Count: " + String(int(potPreviousVal / 120) + 1));
              delay(1000);
              potPreviousVal = potRead();
            }
          }
        }

      } else {
        if (soundStatus != 6){soundStatus = 6; if(!silentMode){tone(BUZZER, NOTE_A1, 16);} delay(20.8);}

        delay(200);
        lcd.setCursor(0, 2);
        lcd.print(menu[2]);
        if (digitalRead(3) == 1) {
          delay(200);
          int time = int(millis() / 1000);
          int potPreviousVal = potRead();

          while (int(millis() / 1000) - time < 3) {

            if (abs(potPreviousVal - potRead()) > 50) {
              if(!silentMode){tone(BUZZER, NOTE_A1, 16);} delay(20.8);
              time = int(millis() / 1000);
              potPreviousVal = potRead();
            }

            lcd.setCursor(0, 2);
            lcd.print("Timer:  < " + String(int(potPreviousVal / 6) + 1) + "  ");
            lcd.setCursor(14, 2);
            lcd.print(" min >");
            if (digitalRead(3) == 1) {
              if (soundStatus != 7){soundStatus = 7; playMelody(notificationMelody, notificationNoteDurations, 3); delay(20.8);}
              alarmTime = (int(potPreviousVal / 6) + 1) * 60;
              alarmOnTime = int(millis() / 1000);
              time = int(millis() / 1000) - 10;
              delay(200);
              lcd.clear();
              lcd.setCursor(0, 2);
              lcd.print("  Timer in " + String(int(potPreviousVal / 6 + 1)) + " mins");
              delay(1000);
              potPreviousVal = potRead();
            }
          }
        }
      }
    }
    lcd.clear();

    if (alarmTime != 0) {
      //lcd.setCursor(0, 2);
      //lcd.print("Alarm on");
      //delay(400);
      if ((int(millis() / 1000) - alarmOnTime) > alarmTime) {
        //lcd.print("Alarm ON");
        //delay(400);
        alarmTime = 0;
        while (true) {
          digitalWrite(BUZZER, HIGH);
          lcd.print("Time's up");
          delay(400);
          if (digitalRead(3) == 1) {
            break;
          }
        }
        digitalWrite(BUZZER, LOW);
      }
    }

    potPreviousVal = potRead();
  }

  DateTime now = rtc.now();
  int currentDate;
  int currentMonth;
  EEPROM.get(12, currentDate);
  EEPROM.get(14, currentMonth);

  if (now.day() != currentDate) {  // New date has started.
    // Store the consumption so far
    storeConsumption();

    // Reset the daily consumption to zero.
    saltConsumption = 0.0;
    sugarConsumption = 0.0;
    oilConsumption = 0.0;
    // Turn of the indicator LEDs
    digitalWrite(SALT_LED, LOW);
    digitalWrite(SUGAR_LED, LOW);
    digitalWrite(OIL_LED, LOW);

    saltBeeped = false;
    sugarBeeped = false;
    oilBeeped = false;

    // Set the new day to EEPROM
    EEPROM.put(12, (int)rtc.now().day());
  }

  if (now.month() != currentMonth) {
    monthlyAverage averages = calculateTheAverage(28);  // This can be 28, 29 (Feb) or 30, 31. The value 28 is used for ease.
    int monthCount;
    EEPROM.get(16, monthCount);
    EEPROM.put(500 + monthCount * sizeof(monthlyAverage), averages);
    EEPROM.put(16, monthCount + 1);
  }

  float instantSaltWeight = getWeight(SALT);
  float instantSugarWeight = getWeight(SUGAR);
  float instantOilWeight = getWeight(OIL);

  float saltUsage = saltWeight - instantSaltWeight;
  if (saltUsage >= SPICE_TOLERANCE_LOWER && saltUsage <= SPICE_TOLERANCE_UPPER){  // Salt is consumed.
    delay(500);
    instantSaltWeight = getWeight(SALT);
    saltUsage = saltWeight - instantSaltWeight;
    if (saltUsage >= SPICE_TOLERANCE_LOWER && saltUsage <= SPICE_TOLERANCE_UPPER)
      justConsumed(SALT, saltUsage);
  }

  float sugarUsage = sugarWeight - instantSugarWeight;
  if (sugarUsage >= SPICE_TOLERANCE_LOWER && sugarUsage <= SPICE_TOLERANCE_UPPER){  // Sugar is consumed.
    delay(500);
    instantSugarWeight = getWeight(SUGAR);
    sugarUsage = sugarWeight - instantSugarWeight;
    if (sugarUsage >= SPICE_TOLERANCE_LOWER && sugarUsage <= SPICE_TOLERANCE_UPPER)
      justConsumed(SUGAR, sugarUsage);
  }

  float oilUsage = oilWeight - instantOilWeight;
  if (oilUsage >= SPICE_TOLERANCE_LOWER && oilUsage <= SPICE_TOLERANCE_UPPER){  // Oil is consumed.
    delay(500);
    instantOilWeight = getWeight(OIL);
    oilUsage = oilWeight - instantOilWeight;
    if (oilUsage >= SPICE_TOLERANCE_LOWER && oilUsage <= SPICE_TOLERANCE_UPPER)
      justConsumed(OIL, oilUsage);
  }

  // Serial.print(String(saltWeight) + "\t" + String(saltConsumption) + "\t\t");
  // Serial.print(String(sugarWeight) + "\t" + String(sugarConsumption) + "\t\t");
  // Serial.println(String(oilWeight) + "\t" + String(oilConsumption));
  if ((int(millis() / 1000) % 20) > 5) {
    if (clockState == false) {
      lcd.clear();
      clockState = true;
    }
    //lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Today's Usage (g)");
    lcd.setCursor(0, 1);
    lcd.print("Sugar |");
    lcd.setCursor(0, 2);
    lcd.print("Salt  |");
    lcd.setCursor(0, 3);
    lcd.print("Oil   |");

    lcd.setCursor(15, 1);
    lcd.print("/");
    lcd.setCursor(15, 2);
    lcd.print("/");
    lcd.setCursor(15, 3);
    lcd.print("/");

    lcd.setCursor(17, 1);
    lcd.print(SUGAR_LIMIT * getMembers());
    lcd.setCursor(17, 2);
    lcd.print(SALT_LIMIT * getMembers());
    lcd.setCursor(17, 3);
    lcd.print(OIL_LIMIT * getMembers());

    lcd.setCursor(8, 1);
    lcd.print(sugarConsumption, 2);
    lcd.setCursor(8, 2);
    lcd.print(saltConsumption, 2);
    lcd.setCursor(8, 3);
    lcd.print(oilConsumption, 2);
  }

  float saltAddition = instantSaltWeight - saltWeight;
  if (saltAddition >= SPICE_ADDITION_TOLERANCE) {  // Salt is added.
    delay(500);
    instantSaltWeight = getWeight(SALT);
    saltAddition = instantSaltWeight - saltWeight;
    if (saltAddition >= SPICE_ADDITION_TOLERANCE) {
      if (confirmAddition(SALT))
        justAdded(SALT, saltAddition);
    }
  } else {
    digitalWrite(SALT_LED, LOW);
  }

  float sugarAddition = instantSugarWeight - sugarWeight;
  if (sugarAddition >= SPICE_ADDITION_TOLERANCE) {  // Sugar is added.
    delay(500);
    instantSugarWeight = getWeight(SUGAR);
    sugarAddition = instantSugarWeight - sugarWeight;
    if (sugarAddition >= SPICE_ADDITION_TOLERANCE) {
      if (confirmAddition(SUGAR))
        justAdded(SUGAR, sugarAddition);
    }
  } else {
    digitalWrite(SUGAR_LED, LOW);
  }

  float oilAddition = instantOilWeight - oilWeight;
  if (oilAddition >= SPICE_ADDITION_TOLERANCE) {  // Oil is added.
    delay(500);
    instantOilWeight = getWeight(OIL);
    oilAddition = instantOilWeight - oilWeight;
    if (oilAddition >= SPICE_ADDITION_TOLERANCE) {
      if (confirmAddition(OIL))
        justAdded(OIL, oilAddition);
    }
  } else {
    digitalWrite(OIL_LED, LOW);
  }

  if (digitalRead(PUSH_BUTTON_REPORT) == HIGH) {
    playMelody(notificationMelody, notificationNoteDurations, 3);
    viewDetailedReport();
  }

  if (saltConsumption / getMembers() > SALT_LIMIT) {
    digitalWrite(SALT_LED, HIGH);
    if (!saltBeeped) {
      digitalWrite(BUZZER, HIGH);
      delay(200);
      digitalWrite(BUZZER, LOW);
      saltBeeped = true;
    }
  }

  if (sugarConsumption / getMembers() > SUGAR_LIMIT) {
    digitalWrite(SUGAR_LED, HIGH);
    if (!sugarBeeped) {
      digitalWrite(BUZZER, HIGH);
      delay(200);
      digitalWrite(BUZZER, LOW);
      sugarBeeped = true;
    }
  }

  if (oilConsumption / getMembers() > OIL_LIMIT) {
    digitalWrite(OIL_LED, HIGH);
    if (!oilBeeped) {
      digitalWrite(BUZZER, HIGH);
      delay(200);
      digitalWrite(BUZZER, LOW);
      oilBeeped = true;
    }
  }

  /*
  if (getMembers() != numMembers) {  // Potentiometer is being rotated.
    handlePotentiometer();
  }*/

  if (digitalRead(PUSH_BUTTON_MULTI) == HIGH) {
    multiLongPress();
  }
}
