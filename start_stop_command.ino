#include "esp_camera.h"
#include "FS.h"
#include "camera_pins.h"
#include "SD_MMC.h"

#define CAMERA_MODEL_AI_THINKER
#define CAPTURE_PIN 4   // GPIO pin to start/stop capturing
#define MIN_FRAME_DELAY 100  // Minimum delay (100ms for 10fps)
unsigned long lastCaptureTime = 0;
bool captureEnabled = false;  // Flag for capturing state

void startCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size = FRAMESIZE_HD;  // Use QVGA for better performance
  config.jpeg_quality = 10;  // Adjust quality (10 = good balance)
  config.fb_count = 2;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera initialization failed!");
    return;
  }
}

void saveImage(fs::FS &fs, const char *path, camera_fb_t *fb) {
  Serial.printf("Saving file: %s\n", path);
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  file.write(fb->buf, fb->len);
  file.close();
  Serial.println("Image saved!");
}

void captureImages() {
  if (millis() - lastCaptureTime >= MIN_FRAME_DELAY) {
    lastCaptureTime = millis();  // Update last capture time
    
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }

    String filename = "/image_" + String(millis()) + ".jpg";
    saveImage(SD_MMC, filename.c_str(), fb);

    esp_camera_fb_return(fb);  // Release memory
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(CAPTURE_PIN, OUTPUT); // Set GPIO 4 as an output
  digitalWrite(CAPTURE_PIN, LOW); // Ensure it starts OFF

  startCamera();

  // Initialize SD Card
  Serial.println("Initializing SD Card...");
  if (!SD_MMC.begin()) {
    Serial.println("SD Card Mount Failed");
    return;
  }
  Serial.println("SD Card initialized successfully!");
}

void loop() {
  // Serial Monitor Control for GPIO 4
  if (Serial.available()) {
    char command = Serial.read();
    if (command == '1') {
      digitalWrite(CAPTURE_PIN, HIGH);  // Turn ON
      captureEnabled = true;  // Enable capturing
      Serial.println("GPIO 4 ON - Capturing Started");
    } else if (command == '0') {
      digitalWrite(CAPTURE_PIN, LOW);   // Turn OFF
      captureEnabled = false;  // Stop capturing
      Serial.println("GPIO 4 OFF - Capturing Stopped");
    }
  }

  // Capture images at 10 FPS if capture is enabled
  if (captureEnabled) {
    captureImages();
  }
}
