#define SAMPLEPERIODUS 200

const float ANALOG_OFFSET = 358.0f; 
#define SAMPLE_RATE 5000 
#define ENVELOPE_CHECK_RATE 25 
#define SAMPLES_BETWEEN_CHECKS (SAMPLE_RATE / ENVELOPE_CHECK_RATE)

// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

void setup() {
  // Set ADC to 77khz, max for 10bit
  sbi(ADCSRA,ADPS2);
  cbi(ADCSRA,ADPS1);
  cbi(ADCSRA,ADPS0);

  for (byte i = 2; i <= 6; i++) {
    pinMode(i, OUTPUT);
  }
  pinMode(13, OUTPUT);
  
  Serial.begin(115200);
}

float bassFilter(float sample) {
  static float xv[3] = {0,0,0}, yv[3] = {0,0,0};
  xv[0] = xv[1]; xv[1] = xv[2]; 
  xv[2] = sample / 9.1f;
  yv[0] = yv[1]; yv[1] = yv[2]; 
  yv[2] = (xv[2] - xv[0])
      + (-0.7960060012f * yv[0]) + (1.7903124146f * yv[1]);
  return yv[2];
} // 20 - 200hz Single Pole Bandpass IIR Filter


float envelopeFilter(float sample) {
  static float xv[2] = {0,0}, yv[2] = {0,0};
  xv[0] = xv[1]; 
  xv[1] = sample / 160.f;
  yv[0] = yv[1]; 
  yv[1] = (xv[0] + xv[1]) + (0.9875119299f * yv[0]);
  return yv[1];
} // 10hz Single Pole Lowpass IIR Filter

float beatFilter(float sample) {
  static float xv[3] = {0,0,0}, yv[3] = {0,0,0};
  xv[0] = xv[1]; xv[1] = xv[2]; 
  xv[2] = sample / 7.015f;
  yv[0] = yv[1]; yv[1] = yv[2]; 
  yv[2] = (xv[2] - xv[0])
      + (-0.7169861741f * yv[0]) + (1.4453653501f * yv[1]);
  return yv[2];
} // 1.7 - 3.0hz Single Pole Bandpass IIR Filter

void update_leds(float size) {
  static const byte thresholds[] = {5, 10, 20, 30, 40};
  for (byte i = 0; i < 5; i++) {
    digitalWrite(i + 2, size > thresholds[i] ? HIGH : LOW);
  }
}

void create_string(float size, char * string) {
  update_leds(size);
  byte val = constrain(15 + (byte)size, 0, 29);
  for (byte i = 0; i < val; i++) {
    *string++ = '=';
  }
  *string = 0; // end with null
}

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

  return (avg * 1.45f < val - 45.0f); // Simplified check
}

void loop() {
  static unsigned long time = 0;
  static byte sampleCount = 0;
  float sample, value, envelope, beat;
  
  time = micros();

  // Read ADC and center so +-512
  sample = (float)analogRead(A0) - ANALOG_OFFSET;
  value = bassFilter(sample);
  envelope = envelopeFilter(abs(value));

  // Every 200 samples (25hz) filter the envelope 
  if (++sampleCount >= SAMPLES_BETWEEN_CHECKS) {
    beat = beatFilter(envelope);
    char buff[30];
    create_string(beat, buff);
    Serial.println(buff);
    digitalWrite(13, beat_judge(beat) ? LOW : HIGH);
    sampleCount = 0;
  }

  // Maintain sample rate
  while (micros() - time < SAMPLEPERIODUS);
}
