#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <LedControl.h>
#include <Wire.h>
#include <MPU6050.h>

// NÚT NHẤN
                      // nút nhấn RESET chỉ cần kết nối phần cứngcứng
#define BUTTON_PIN 32 // 
#define BUTTON2_PIN 33 // nút báo động 
#define BUTTON3_PIN 25  // nút chọn mode hiển thị 7SEGMODE 
// BUZZER VÀ LED CẢNH BÁO 
#define BUZZER_CTRL 19
#define LED_STATUS 23
#define LED_ON_OFF 2
// LED 7 ĐOẠN 
#define DIN_PIN  13
#define CLK_PIN  14
#define CS_PIN   12
#define DHTTYPE  DHT11
LedControl lc = LedControl(DIN_PIN, CLK_PIN, CS_PIN, 1); 
// DHT11
#define DHTPIN   4
DHT dht(DHTPIN, DHTTYPE);
// SW-420
#define SW420_PIN 15
// HC-SR04
#define TRIG_PIN 5
#define ECHO_PIN 18
// MPU6050
MPU6050 mpu;

int led7mode = 0;
bool systemEnabled = false;
bool lastMainButtonState = LOW;
bool warningEnabled = false;
bool lastButton2State = LOW;
bool lastButton3State = LOW;
// Hiển thị 8 chữ số lên LED 7 đoạn
void displayNumberOnLed(int digits[8], bool dots[8]) {
  for (int i = 0; i < 8; i++) {
    if (digits[i] >= 0 && digits[i] <= 9) {
      lc.setDigit(0, 7 - i, digits[i], dots[i]);
    } else if (digits[i] >= 'A') {
      lc.setChar(0, 7 - i, digits[i], dots[i]);
    } else {
      lc.setChar(0, 7 - i, ' ', false);
    }
  }
}

void setup() {
  Serial.begin(115200);
  // DHT11 
  Serial.println("Khởi động cảm biến DHT11...");
  dht.begin();
  /// ĐÈN BÁO ON/OFF 
  pinMode(LED_ON_OFF, OUTPUT);
  // LED 7 ĐOẠN 
  lc.shutdown(0, false);
  lc.setIntensity(0, 10);
  lc.clearDisplay(0);
  // NÚT NHẤN 
  pinMode(BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(BUTTON2_PIN, INPUT_PULLDOWN);
  pinMode(BUTTON3_PIN, INPUT_PULLDOWN);
  // SW-420420
  pinMode(SW420_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  // KHỐI CẢNH BÁO 
  pinMode(BUZZER_CTRL, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);
  digitalWrite(BUZZER_CTRL, LOW);
  digitalWrite(LED_STATUS, LOW);
  // MPU6050 
  Wire.begin(21, 22);
  mpu.initialize();
  if (mpu.testConnection()) {
    Serial.println("MPU6050 kết nối thành công!");
  } else {
    Serial.println("Không thể kết nối với MPU6050.");
    while (1);
  }
}

void loop() {
  // NÚT ON/OFF 
  bool currentMainButtonState = digitalRead(BUTTON_PIN);
  if (currentMainButtonState == HIGH && lastMainButtonState == LOW) {
    systemEnabled = !systemEnabled;
    Serial.print("System is now: ");
    Serial.println(systemEnabled ? "ENABLED" : "DISABLED");
    lc.clearDisplay(0); // Xoá LED khi tắt hệ thống
  }
  lastMainButtonState = currentMainButtonState;
  ///////////////////////////////////////////////////////
  if (systemEnabled) {
  digitalWrite(LED_ON_OFF, HIGH);
  //DHT11
  float nhietDo = dht.readTemperature();
  if (isnan(nhietDo)) {
    Serial.println("Lỗi! Không thể đọc dữ liệu từ cảm biến DHT11");
  } else {
    Serial.print("Nhiệt độ: ");
    Serial.print(nhietDo);
    Serial.println("°C");
  }

  //SW-420
  int state = digitalRead(SW420_PIN);
  if (state == LOW) {
  } else {    
    digitalWrite(BUZZER_CTRL, HIGH);
    digitalWrite(LED_STATUS, HIGH);
    delay(1000);
    digitalWrite(BUZZER_CTRL, LOW);
    digitalWrite(LED_STATUS, LOW);
  }

  //MPU6050
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  float ax_m_s2 = ax / 16384.0 * 9.81;
  static float vx = 0;
  static unsigned long lastTime = millis();
  unsigned long currentTime = millis();
  float dt = (currentTime - lastTime) / 1000.0;
  lastTime = currentTime;

  if (abs(ax) < 17000) {
    vx = 0;
  }
  vx += ax_m_s2 * dt;
  float speedX = abs(vx);
  Serial.print("Tốc độ X: ");
  Serial.print(speedX);
  Serial.println(" m/s");

  // HC-SR04
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.0343 / 2;
  Serial.print("Khoảng cách: ");
  Serial.print(distance);
  Serial.println(" cm");
  ////////////////// BUTTON LOGIC
  // Nút nhấn báo động
  bool currentButton2State = digitalRead(BUTTON2_PIN);
  if (currentButton2State == HIGH && lastButton2State == LOW) {
    warningEnabled = !warningEnabled; // Đảo trạng thái cảnh báo
  }
  lastButton2State = currentButton2State;
  if (warningEnabled) {
    digitalWrite(BUZZER_CTRL, HIGH);
    digitalWrite(LED_STATUS, HIGH);
    delay(200);
    digitalWrite(BUZZER_CTRL, LOW);
    digitalWrite(LED_STATUS, LOW);
    delay(200);
  } else {
    digitalWrite(BUZZER_CTRL, LOW);
    digitalWrite(LED_STATUS, LOW);
  }
  // Nút chọn chế độ hiển thị led 7 đoạn 
  bool currentButton3State = digitalRead(BUTTON3_PIN);
  if (currentButton3State == HIGH && lastButton3State == LOW) {
    led7mode = (led7mode + 1) % 3;
    Serial.print("Chuyển mode LED7: ");
    Serial.println(led7mode);
  }
  //HIỂN THỊ LÊN LED 7 ĐOẠN (đơn giản hóa ví dụ)
  int digits[8];
  bool dots[8] = {false};
  for (int i = 0; i < 8; i++) digits[i] = -1;  // <-- Thêm dòng này
  switch (led7mode) {
    case 0: {
      int temp = (int)(nhietDo * 100);
      digits[4] = (int)nhietDo / 10;
      digits[5] = (int)nhietDo % 10;
      digits[6] = (int)((nhietDo - (int)nhietDo) * 100) /10;
      digits[7] = (int)((nhietDo - (int)nhietDo) * 100) %10;
      for (int i = 0; i < 4; i++) digits[i] = -1;
      dots[5] = true;
      break;
    }
    case 1: {
      int val = (int)(distance * 10000 + 0.5);
      for (int i = 0; i < 8; i++) {
        digits[7 - i] = val % 10;
        val /= 10;
      }
      if (digits[0] == 0) digits[0] = -1;
      if (digits[1] == 0 && digits[0] == -1) digits[1] = -1;
      dots[3] = true;
      break;
    }
    case 2: {
      int val = (int)(speedX * 100 + 0.5);
      for (int i = 0; i < 6; i++) {
        digits[7 - i] = val % 10;
        val /= 10;
      }
      digits[0] = digits[1] = -1;
      if (digits[2] == 0) digits[2] = -1;
      if (digits[2] == -1 && digits[3] == 0) digits[3] = -1;
      if (digits[2] == -1 && digits[3] == -1 && digits[4] == 0) digits[4] = -1;
      dots[5] = true;
      break;
    }
  }
  displayNumberOnLed(digits, dots);
  if (nhietDo > 35 ) {
    digitalWrite(BUZZER_CTRL, HIGH);
    digitalWrite(LED_STATUS, HIGH);
    delay(500); 
    digitalWrite(BUZZER_CTRL, LOW);
    digitalWrite(LED_STATUS, LOW);
  }
  if (distance < 10 ) {
    digitalWrite(BUZZER_CTRL, HIGH);
    digitalWrite(LED_STATUS, HIGH);
    delay(500); 
    digitalWrite(BUZZER_CTRL, LOW);
    digitalWrite(LED_STATUS, LOW);
  }
  if (speedX > 30 ) {
    digitalWrite(BUZZER_CTRL, HIGH);
    digitalWrite(LED_STATUS, HIGH);
    delay(500); 
    digitalWrite(BUZZER_CTRL, LOW);
    digitalWrite(LED_STATUS, LOW);
  }
  delay(500); // 500ms
} else {    digitalWrite(LED_ON_OFF, LOW);}
}
