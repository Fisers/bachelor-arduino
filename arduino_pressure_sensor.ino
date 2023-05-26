# include <SPI.h>

bool busy = false;
int time_measuring = 0;
long pressure = 0; // max 16777215
uint8_t pressure_bytes[3] = {0xFF, 0xFF, 0xFF};
uint8_t temp_bytes[3] = {0xFF, 0xFF, 0xFF};
long temperature = 0; // max 16777215
int i_measure = 0;

volatile int i = 0;
uint8_t myArray[16];
volatile int time_since_last_msg = 0;
bool sending = false;

void reset_buffer() {
  for(int a = 0; a < sizeof(myArray); a++) {
    myArray[a] = 0xFF;
  }
}

void setup()
 {
  Serial.begin (115200);   // set baud rate to 115200 for usart
  pinMode(SS, INPUT);
  pinMode(MOSI, INPUT);
  pinMode(MISO, OUTPUT);
  pinMode(SCK, INPUT);
  SPCR |= _BV(SPE);
  SPI.attachInterrupt();

  reset_buffer();
  SPDR = 0b01000000;
}

void loop(){
  // Handle logging
  if (sending && time_since_last_msg > 1)
  {
    char buffer[1024];
    sprintf(buffer, "Received");
    for(int a = 0; a < sizeof(myArray); a++) {
      if(myArray[a] == 0xFF) break;
      sprintf(buffer, "%s 0x%02x", buffer, myArray[a]); 
    }
    Serial.println(buffer);
    Serial.println("=============================================");

    reset_buffer();

    i=0;
    time_since_last_msg = 0;
    sending = false;
  }
  time_since_last_msg++;

  SPDR = 0b01000000;
  // Handle calculation imitiation
  if(busy && !sending) {
    time_measuring++;
    SPDR = 0b01100000;

    if(time_measuring > 5) {
      busy = false;
      pressure = random(0, 16777215);
      temperature = random(0, 16777215);
      time_measuring = 0;

      Serial.print("! PRESSURE: ");
      Serial.print(pressure);
      Serial.print(" | TEMP: ");
      Serial.println(temperature);

      pressure_bytes[0] = (pressure >> 16) & 0xff;
      pressure_bytes[1] = (pressure >> 8) & 0xff;
      pressure_bytes[2] = pressure & 0xff;

      temp_bytes[0] = (temperature >> 16) & 0xff;
      temp_bytes[1] = (temperature >> 8) & 0xff;
      temp_bytes[2] = temperature & 0xff;

      Serial.println(pressure_bytes[0]);
      Serial.println(pressure_bytes[1]);
      Serial.println(pressure_bytes[2]);
      Serial.println(temp_bytes[0]); // 449252
      Serial.println(temp_bytes[1]);
      Serial.println(temp_bytes[2]);

      SPDR = 0b01000000;
    }
  }
  delay(1);
}

// Interrupt function
ISR (SPI_STC_vect) 
{
  uint8_t c = SPDR;        // read byte from SPI Data Register
  myArray[i++] = c;
  time_since_last_msg = 0;
  sending = true;

  switch(c) {
    case 0x00:
      switch(i_measure) {
        case 0:
          SPDR = pressure_bytes[0];
          break;
        case 1:
          SPDR = pressure_bytes[1];
          break;
        case 2:
          SPDR = pressure_bytes[2];
          break;
        case 3:
          SPDR = temp_bytes[0];
          break;
        case 4:
          SPDR = temp_bytes[1];
          break;
        case 5:
          SPDR = temp_bytes[2];
          break;
      }

      i_measure++;
      return;

    case 0xAA:
      busy = true;
      i_measure = 0;
      return;

    case 0xF0:
      if(busy) return;
      SPDR = pressure_bytes[0];
      i_measure = 1;
      return;
  }
}