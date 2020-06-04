/* Pinout 
  -RESET ADC0 > PB5  v   > VCC
         ADC3 > PB3      > PB2  ADC1
         ADC2 > PB4      > PB1
              > GND      > PB0
*/ 
#define ODROID_DETECT     A3
#define DECODER_SWITCH    PB4
#define ODROID_OFF    0
#define ODROID_BOOT   1
#define ODROID_RUN    2
#define TIMEOUT_MS    1000
#define ON_DETECTED   100
uint8_t odroidState = ODROID_OFF;
uint32_t odroidBootStart;

void setup() {
    pinMode(ODROID_DETECT, INPUT);
    digitalWrite(DECODER_SWITCH, HIGH);
    pinMode(DECODER_SWITCH, OUTPUT);
}
 
void loop() {

//  uint16_t val = analogRead(ODROID_DETECT);
  uint16_t val = digitalRead(ODROID_DETECT)? ON_DETECTED:0;

  switch(odroidState) 
  {
    case ODROID_OFF:
      digitalWrite(DECODER_SWITCH, HIGH);
      if (val >= ON_DETECTED)
      {
        odroidState = ODROID_BOOT;
        odroidBootStart = millis();
      }
      break;
    case ODROID_BOOT:
      if (val < ON_DETECTED)
        odroidState = ODROID_OFF;
      else if (millis() - odroidBootStart > TIMEOUT_MS) {
        digitalWrite(DECODER_SWITCH, LOW);
        odroidState = ODROID_RUN;
      }  
      break;
    case ODROID_RUN:
      if (val < ON_DETECTED)
        odroidState = ODROID_OFF;
      break;
  }
  delay(10);
  }
