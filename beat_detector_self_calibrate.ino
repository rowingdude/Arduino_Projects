#define SAMPLEPERIODUS 200
#define SAMPLE_RATE 5000
#define ENVELOPE_CHECK_RATE 25
#define SAMPLES_BETWEEN_CHECKS (SAMPLE_RATE / ENVELOPE_CHECK_RATE)

float ANALOG_OFFSET = 358.0f;

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

float calibrate_analog_offset() {
  float sum = 0;
  for (int i = 0; i < 1000; i++) {
    sum += analogRead(A0);
    delayMicroseconds(1000);
  }
  return sum / 1000;
} // Calibrates the analog offset by averaging 1000 readings

void setup_adc() {
  sbi(ADCSRA,ADPS2);
  cbi(ADCSRA,ADPS1);
  cbi(ADCSRA,ADPS0);
} // Sets up ADC for faster readings

void setup_pins() {
  for (byte i = 2; i <= 6; i++) {
    pinMode(i, OUTPUT);
  }
  pinMode(13, OUTPUT);
} // Configures LED pins as outputs

void setup() {
  setup_adc();
  setup_pins();
  Serial.begin(115200);
  ANALOG_OFFSET = calibrate_analog_offset();
  Serial.print("Calibrated analog offset: ");
  Serial.println(ANALOG_OFFSET);
} // Initializes the system, including ADC setup, pin configuration, and offset calibration

float bassFilter(float sample) {
  static float xv[3] = {0,0,0}, yv[3] = {0,0,0};
  xv[0] = xv[1]; xv[1] = xv[2]; 
  xv[2] = sample / 9.1f;
  yv[0] = yv[1]; yv[1] = yv[2]; 
  yv[2] = (xv[2] - xv[0]) + (-0.7960060012f * yv[0]) + (1.7903124146f * yv[1]);
  return yv[2];
} // Applies a 20-200Hz bandpass filter to isolate bass frequencies

float envelopeFilter(float sample) {
  static float xv[2] = {0,0}, yv[2] = {0,0};
  xv[0] = xv[1]; 
  xv[1] = sample / 160.f;
  yv[0] = yv[1]; 
  yv[1] = (xv[0] + xv[1]) + (0.9875119299f * yv[0]);
  return yv[1];
} // Applies a 10Hz lowpass filter to smooth the signal envelope

float beatFilter(float sample) {
  static float xv[3] = {0,0,0}, yv[3] = {0,0,0};
  xv[0] = xv[1]; xv[1] = xv[2]; 
  xv[2] = sample / 7.015f;
  yv[0] = yv[1]; yv[1] = yv[2]; 
  yv[2] = (xv[2] - xv[0]) + (-0.7169861741f * yv[0]) + (1.4453653501f * yv[1]);
  return yv[2];
} // Applies a 1.7-3.0Hz bandpass filter to isolate beat frequencies

void update_leds(float size) {
  static const byte thresholds[] = {5, 10, 20, 30, 40};
  for (byte i = 0; i < 5; i++) {
    digitalWrite(i + 2, size > thresholds[i] ? HIGH : LOW);
  }
} // Updates LED states based on beat intensity

void create_string(float size, char * string) {
  update_leds(size);
  byte val = constrain(15 + (byte)size, 0, 29);
  for (byte i = 0; i < val; i++) {
    *string++ = '=';
  }
  *string = 0;
} // Creates a visual representation of beat intensity and updates LEDs

bool beat_judge(float val) {
  static float history[10] = {0};
  float avg = 0;
  for (byte i = 0; i < 9; i++) {
    avg += history[i];
  }
  avg /= 9;
  for (byte i = 0; i < 9; i++) {
    history[i] = history[i+1];
  }
  history[9] = val;
  return (avg * 1.45f < val - 45.0f);
} // Determines if a beat has occurred based on recent signal history

void process_audio() {
  float sample = (float)analogRead(A0) - ANALOG_OFFSET;
  float value = bassFilter(sample);
  float envelope = envelopeFilter(abs(value));
  return envelope;
} // Processes raw audio input through filters

void handle_beat(float beat) {
  char buff[30];
  create_string(beat, buff);
  Serial.println(buff);
  if (beat_judge(beat)) {
    digitalWrite(13, LOW);
    Serial.println(">>>");
  } else {
    digitalWrite(13, HIGH);
  }
} // Handles beat detection, visualization, and LED control

void loop() {
  static unsigned long time = 0;
  static byte sampleCount = 0;
  time = micros();
  
  float envelope = process_audio();

  if (++sampleCount >= SAMPLES_BETWEEN_CHECKS) {
    float beat = beatFilter(envelope);
    handle_beat(beat);
    sampleCount = 0;
  }

  while (micros() - time < SAMPLEPERIODUS);
} // Main loop: processes audio, detects beats, and maintains timing
