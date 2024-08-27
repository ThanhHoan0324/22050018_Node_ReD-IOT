#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Cấu hình chân cho cảm biến DHT
#define DHTPIN 2          // Chân D4 trên ESP8266
#define DHTTYPE DHT11     // Loại cảm biến DHT có thể là DHT11 hoặc DHT22
#define LED_PIN 0         // Chân D3 (GPIO0) cho đèn LED
#define BUTTON_PIN 14     // Chân D5 (GPIO14) cho nút bấm

DHT dht(DHTPIN, DHTTYPE); // Khởi tạo đối tượng DHT

// Thông tin kết nối WiFi
const char* ssid = "v2022";  // Tên WiFi
const char* password = "Giai12468";    // Mật khẩu WiFi

// Thông tin MQTT Broker
const char* mqtt_server = " 192.168.144.201";
const int mqtt_port = 1883;
const char* mqtt_user = "hovantai";          // Tên đăng nhập MQTT (nếu cần)
const char* mqtt_pass = "123456";        // Mật khẩu MQTT (nếu cần)
 // Chủ đề MQTT để gửi dữ liệu
const char* topic_subscribe = "to-esp8266";
const char* topic_publish = "from-esp8266";
const char* topic_publish_temp = "from-esp8266_temp";
const char* topic_publish_humi = "from-esp8266_humi";
const char* topic_publish_button = "esp8266_button_state";

WiFiClient espClient;
PubSubClient client(espClient);

bool ledState = false; // Trạng thái hiện tại của đèn LED và DHT11
bool sensorActive = false; // Trạng thái hiện tại của nút bấm
bool previousSwitchState = HIGH; // Trạng thái trước đó của nút

void setup() {
  // Khởi tạo kết nối Serial để debug
  Serial.begin(115200);
  delay(500); // Đợi một chút để ổn định

  // Kết nối tới mạng WiFi
  Serial.println();
  Serial.print("Đang kết nối tới mạng WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password); // Bắt đầu kết nối WiFi

  while (WiFi.status() != WL_CONNECTED) {
    delay(3000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Đã kết nối WiFi thành công");
  Serial.print("Địa chỉ IP: ");
  Serial.println(WiFi.localIP()); // In địa chỉ IP được cấp

  // Cấu hình MQTT server và cổng
  client.setServer(mqtt_server, mqtt_port);
 

  // Khởi tạo cảm biến DHT
  dht.begin();
  // Khởi tạo chân GPIO cho đèn LED và nút bấm
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Switch sử dụng chế độ kéo lên nội bộ
  digitalWrite(LED_PIN, LOW); // Tắt đèn LED ban đầu
}




void reconnect() {
  // Vòng lặp để đảm bảo kết nối lại MQTT nếu bị ngắt
  while (!client.connected()) {
    Serial.print("Đang kết nối tới MQTT broker...");

    // Tạo một client ID ngẫu nhiên
    String clientId = "mqtt-explorer-ea9c9e01";
    clientId += String(random(0xffff), HEX);

    // Thử kết nối MQTT
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println(" kết nối thành công");
      client.subscribe(topic_subscribe); // Đăng ký chủ đề để nhận lệnh điều khiển
    } else {
      Serial.print(" thất bại với mã lỗi ");
      Serial.print(client.state());
      Serial.println(". Thử lại sau 5 giây");
      delay(5000);
    }
  }
}



void loop() {
  if (!client.connected()) {
    reconnect(); // Kết nối lại MQTT nếu bị ngắt
  }
  client.loop();

  bool currentSwitchState = digitalRead(BUTTON_PIN);

  if (currentSwitchState == LOW && previousSwitchState == HIGH) {
    // Nếu công tắc được nhấn và trạng thái trước đó là HIGH
    sensorActive = !sensorActive; // Chuyển trạng thái hoạt động của cảm biến
    ledState = sensorActive;      // Cập nhật trạng thái của đèn LED
    digitalWrite(LED_PIN, ledState ? HIGH : LOW); // Bật/tắt đèn LED
    

    if (sensorActive) {
      Serial.println("Đèn LED Bật, bắt đầu đo dữ liệu");
    } else {
      Serial.println("Đèn LED Tắt, ngừng đo dữ liệu");
    }


  }

  previousSwitchState = currentSwitchState; // Cập nhật trạng thái trước đó của công tắc

   // Nếu đèn LED đang bật, tiến hành đọc và gửi dữ liệu từ DHT11
  if (sensorActive) {
    // Đọc giá trị độ ẩm và nhiệt độ từ cảm biến DHT
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    // Kiểm tra xem việc đọc có thành công không
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Lỗi khi đọc dữ liệu từ cảm biến DHT!");
      return;
    }

    // In dữ liệu đọc được ra Serial để kiểm tra
    Serial.print("Độ ẩm: ");
    Serial.print(humidity);
    Serial.print(" %\t");
    Serial.print("Nhiệt độ: ");
    Serial.print(temperature);
    Serial.println(" *C");

    String temp = "";
    temp += temperature;
    String humi = "";
    humi += humidity;
    client.publish(topic_publish_temp, temp.c_str());
    client.publish(topic_publish_humi, humi.c_str());
    Serial.println("Gửi dữ liệu thành công");
    Serial.println("- - - - -");

    delay(2000); // Đợi 2 giây trước khi đọc và gửi dữ liệu lần tiếp theo
  }
}

 