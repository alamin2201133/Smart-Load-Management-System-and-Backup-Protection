#include <Wire.h>
#include <Adafruit_INA219.h>
#include <LiquidCrystal_I2C.h>

#define BT_SERIAL Serial 

LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_INA219 inaLoad(0x40); 

// --- HARDWARE PIN CONFIGURATION ---
const int optocouplerPin = 2;  // PC817 Mains Connection Tracker
const int alertSystemPin = 3;  // Piezo Buzzer + Signaling Warning LED
const int relayL1 = 5;         // Matrix Rail 1 Out
const int relayL2 = 6;         // Matrix Rail 2 Out
const int relayL3 = 7;         // Matrix Rail 3 Out
const int sourceRelayPin = 8;  // Power Line Switchover Relay

// --- SYSTEM STATE SIGNALS ---
bool usingBattery = false; 
bool l1State = true;
bool l2State = true;
bool l3State = true;
bool systemOvercurrent = false;
bool inaConnected = false;     

int controlMode = 0; // 0 = AUTO, 1 = CUSTOM
int priority1 = 1;   // 1 = High, 2 = Medium, 3 = Low
int priority2 = 2;   
int priority3 = 3;   

// --- VARIABLE PROTECTION FACTOR ---
float maxCurrentThreshold = 0.70; // Overcurrent default starts at 0.70A
bool limitModifiedByUser = false;  // Tracks if user modified current limit access via web slider

unsigned long lastTelemetryTime = 0;
const unsigned long TELEMETRY_INTERVAL = 500; // Strict 500ms tracking window

void setup() {
  BT_SERIAL.begin(9600); 
  
  pinMode(optocouplerPin, INPUT_PULLUP);
  pinMode(alertSystemPin, OUTPUT); 
  pinMode(relayL1, OUTPUT);
  pinMode(relayL2, OUTPUT);
  pinMode(relayL3, OUTPUT);
  pinMode(sourceRelayPin, OUTPUT);

  digitalWrite(sourceRelayPin, HIGH); // Default state: MAINS
  usingBattery = false;

  digitalWrite(relayL1, LOW);  
  digitalWrite(relayL2, LOW);
  digitalWrite(relayL3, LOW);
  digitalWrite(alertSystemPin, LOW); 

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Smart load sysm"); 
  delay(2000);
  
  if (!inaLoad.begin()) {
    lcd.clear(); 
    lcd.print("INA219 ERROR!");
    lcd.setCursor(0, 1);
    lcd.print("RUNNING BYPASS...");
    inaConnected = false;
    delay(2000);
  } else {
    inaLoad.setCalibration_32V_2A(); 
    inaConnected = true;
  }
  lcd.clear();
}

void loop() {
  float vLoad = 0.0;
  float currentA = 0.0;
  float current_raw = 0.0;

  if (inaConnected) {
    vLoad = inaLoad.getBusVoltage_V();
    current_raw = inaLoad.getCurrent_mA();
    currentA = current_raw / 1000.0;
    if (currentA < 0) currentA = 0.0; 
  } else {
    if (millis() % 4000 < 20) {
      Wire.begin();
      if (inaLoad.begin()) {
        inaLoad.setCalibration_32V_2A();
        inaConnected = true;
      }
    }
  }

  bool adapterAlive = (digitalRead(optocouplerPin) == LOW); 

  // Source Switchover Logic Block
  if (!usingBattery && !adapterAlive) {
    digitalWrite(sourceRelayPin, LOW); // Shift source selection to BATTERY
    usingBattery = true;
    systemOvercurrent = false; 
    lcd.clear();
    return;
  } 
  else if (usingBattery && adapterAlive) {
    digitalWrite(sourceRelayPin, HIGH); // Revert to MAINS power lines
    usingBattery = false;
    systemOvercurrent = false;
    l1State = true; l2State = true; l3State = true; 
    lcd.clear();
    return;
  }

  bool noSourceActive = (usingBattery && vLoad < 2.0);

  // =================================================================
  // CASCADING OVERCURRENT PROTECTION SYSTEM (WHILE LOOP FIX)
  // =================================================================
  if (usingBattery && inaConnected && !noSourceActive) {
    
    // Check if current exceeds the limit AND at least one load is still turned ON
    if (currentA > maxCurrentThreshold && (l1State || l2State || l3State)) {
      systemOvercurrent = true;
      
      // Trigger warning alert once
      digitalWrite(alertSystemPin, HIGH); 
      lcd.setCursor(0, 0); lcd.print("WARNING! OVERCR ");
      delay(1200); 
      digitalWrite(alertSystemPin, LOW); 

      // CASCADING LOOP: Keep shedding loads until current drops OR all loads are dead
      while (currentA > maxCurrentThreshold && (l1State || l2State || l3State)) {
        shedNextLoad(); // Drop the lowest priority load active right now
        setLoadOutputs(l1State, l2State, l3State); // Apply physical relay state
        
        delay(150); // Let the circuit settle down before reading current again
        
        // Re-read current sensor instantly to see if more loads need to step down
        current_raw = inaLoad.getCurrent_mA();
        currentA = current_raw / 1000.0;
        if (currentA < 0) currentA = 0.0;
      }
    }
  } 
  else if (!usingBattery) {
    l1State = true; l2State = true; l3State = true;
    systemOvercurrent = false;
  }

  if (noSourceActive) {
    setLoadOutputs(false, false, false);
    currentA = 0.000; 
  } else {
    setLoadOutputs(l1State, l2State, l3State);
  }

  // LCD Layout Configuration Engine
  lcd.setCursor(0, 0);
  if (noSourceActive) {
    lcd.print("Connect Source  ");
  } else {
    if (usingBattery) {
      lcd.print("M:bat");
    } else {
      lcd.print("M:main");
    }
    lcd.print(limitModifiedByUser ? "-M " : "-A "); 
    lcd.print(maxCurrentThreshold, 2); lcd.print("A  "); 
  }

  lcd.setCursor(0, 1);
  if (noSourceActive) {
    lcd.print("V:0.0   C:0.000A");
  } else if (inaConnected) {
    lcd.print("V:"); lcd.print(vLoad, 1);
    lcd.print(" C:"); lcd.print(currentA, 3); lcd.print("A ");
  } else {
    lcd.print("BYPASS SAFE MODE");
  }

  // Telemetry Packaging Desk
  if (millis() - lastTelemetryTime >= TELEMETRY_INTERVAL) {
    lastTelemetryTime = millis();
    
    BT_SERIAL.print("V:");  BT_SERIAL.print(noSourceActive ? 0.0 : vLoad, 1);   BT_SERIAL.print(",");
    BT_SERIAL.print("C:");  BT_SERIAL.print(noSourceActive ? 0.000 : currentA, 3); BT_SERIAL.print(",");
    
    if (noSourceActive) {
      BT_SERIAL.print("S:NOSOURCE,");
    } else {
      BT_SERIAL.print("S:");  BT_SERIAL.print(usingBattery ? "BACKUP" : "MAIN"); BT_SERIAL.print(",");
    }
    
    BT_SERIAL.print("OC:"); BT_SERIAL.print(systemOvercurrent ? "1" : "0"); BT_SERIAL.print(",");
    BT_SERIAL.print("LV:0,"); 
    BT_SERIAL.print("L1:"); BT_SERIAL.print((l1State && !noSourceActive) ? "1" : "0"); BT_SERIAL.print(",");
    BT_SERIAL.print("L2:"); BT_SERIAL.print((l2State && !noSourceActive) ? "1" : "0"); BT_SERIAL.print(",");
    BT_SERIAL.print("L3:"); BT_SERIAL.print((l3State && !noSourceActive) ? "1" : "0");
    BT_SERIAL.println(); 
  }

  // Command Desk Decoder
  if (BT_SERIAL.available() > 0) {
    String command = BT_SERIAL.readStringUntil('\n'); 
    command.trim(); 

    if (command == "RESET") { 
      systemOvercurrent = false; l1State = true; l2State = true; l3State = true;
      limitModifiedByUser = false; maxCurrentThreshold = 0.70; 
      digitalWrite(alertSystemPin, LOW); lcd.init(); 
    }
    else if (command == "AUTO") { 
      controlMode = 0; l1State = true; l2State = true; l3State = true; systemOvercurrent = false; 
    }
    else if (command == "CUSTOM") { 
      controlMode = 1; 
    }
    else if (command.startsWith("AMP")) {
      String valueString = command.substring(3); 
      valueString.trim(); 
      float parsedValue = valueString.toFloat();
      if (parsedValue >= 0.10 && parsedValue <= 2.00) { 
        maxCurrentThreshold = parsedValue; 
        limitModifiedByUser = true; 
      }
    }
    else if (command == "P1H") priority1 = 1; else if (command == "P1M") priority1 = 2; else if (command == "P1L") priority1 = 3;
    else if (command == "P2H") priority2 = 1; else if (command == "P2M") priority2 = 2; else if (command == "P2L") priority2 = 3;
    else if (command == "P3H") priority3 = 1; else if (command == "P3M") priority3 = 2; else if (command == "P3L") priority3 = 3;
  }
  delay(10); 
}

void shedNextLoad() {
  if (controlMode == 0) {
    // Fixed auto hierarchy sequence: Load 3 -> Load 2 -> Load 1
    if (l3State) { l3State = false; return; }
    if (l2State) { l2State = false; return; }
    if (l1State) { l1State = false; return; }
  } else {
    // Custom dynamic matrix sorting engine
    int targetShed = -1;
    int lowestPriorityVal = -1; 

    if (l1State && priority1 > lowestPriorityVal) { lowestPriorityVal = priority1; targetShed = 1; }
    if (l2State && priority2 > lowestPriorityVal) { lowestPriorityVal = priority2; targetShed = 2; }
    if (l3State && priority3 > lowestPriorityVal) { lowestPriorityVal = priority3; targetShed = 3; }

    if (targetShed == 1) l1State = false;
    if (targetShed == 2) l2State = false;
    if (targetShed == 3) l3State = false;
  }
}

void setLoadOutputs(bool l1, bool l2, bool l3) {
  digitalWrite(relayL1, l1 ? LOW : HIGH); 
  digitalWrite(relayL2, l2 ? LOW : HIGH);
  digitalWrite(relayL3, l3 ? LOW : HIGH);
}