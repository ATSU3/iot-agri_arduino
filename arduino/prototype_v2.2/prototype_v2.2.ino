// このコードは、ESP32を使ってDHT22センサから湿度と温度を読み取り、そのデータをLCDで表示、microSDカードに保存、そしてSupabaseのデータベースに保存する機能を持っています。

// 各ライブラリを読み込んでおります。
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <RTClib.h>
#include <WiFi.h>
#include <ESP32_Supabase.h>

#define DHTPIN 2
#define DHTTYPE DHT22
#define SCK 18  
#define MISO 19 
#define MOSI 23 
#define SS 4    
#define BUTTON_PIN 15  
#define BUTTON2_PIN 16 

// それぞれ、DHTセンサー、LCD、RTCのオブジェクトを作成
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS1307 rtc;

// SDカード、LCDのステータスを管理する変数
volatile bool sdCardStatus = false;
bool lcdStatus = true;

// 最後にデータを記録した時間やLCDを更新した時間を管理する変数
unsigned long lastRecordTime = 0;
unsigned long lastDisplayUpdateTime = 0;

Supabase db; // Supabase用のインスタンスを初期化

// SupabaseのURLとキー
String supabase_url = "{{SET SUPABASE_URL}}";
String anon_key = "{{SET ANNON_KEY}}";

// データを保存するSupabaseのテーブル名
String table = "{{SET TABLE_NAME}}";

// WiFiのSSIDとパスワード
const char *ssid = "{{WIFI_SSID}}";
const char *psswd = "{{WIFI_PASSWORD}}";

// Supabaseへのログイン情報
const String email = "{{SET EMAIL}}";
const String password = "{{SET EMAIL_PASSWORD}}";

void setup() {
  lcd.init();          // LCDの初期化
  lcd.backlight();     // LCDのバックライトをオン
  dht.begin();         // DHTセンサーの初期化

  Wire.begin();        // I2C通信の開始
  
  // WiFiへの接続処理
  WiFi.begin(ssid, psswd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }

  // Supabaseへの接続とログイン
  db.begin(supabase_url, anon_key);
  db.login_email(email, password);  

  if (!rtc.begin()) {
    lcd.print("Couldn't find RTC");  // RTCが見つからない場合のエラーメッセージ
  }

  // RTCの時間調整（コメントアウト）
  // 最初にこのコードを読み込み、ESP32に書き込みます。その後、RTCモジュールに記録されるため、
  // こちらの行をコメントアウトして、再度書き込みます。
  // rtc.adjust(DateTime(2023, 8, 4, 20, 0, 0)); /

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), tryMountSDCard, FALLING);

  tryMountSDCard();  // SDカードの初期化
}

void tryMountSDCard() {
  lcd.clear();
  if(!SD.begin()){
    lcd.print("Card Mount Failed");
    sdCardStatus = false;
    return;
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE){
    lcd.print("No SD card attached");
    sdCardStatus = false;
  }
  else {
    sdCardStatus = true;
  }
}

void sendDataToSupabase(float temperature, float humidity) {
  String JSON = "{ \"temperature\": " + String(temperature) + ", \"humidity\": " + String(humidity) + ", \"area_id\": 5 }";
  bool upsert = false;
  int code = db.insert(table, JSON, upsert);  // Supabaseへのデータ送信
}

void displayData() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // LCDに温度と湿度を表示
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(t);
  lcd.print("C");
  
  lcd.setCursor(0, 1);
  lcd.print("Hum: ");
  lcd.print(h);
  lcd.print("%");
}

void recordData() {
  DateTime now = rtc.now();  // 現在の時刻を取得
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // SDカードにデータを記録
  File dataFile = SD.open("/data.csv", FILE_APPEND);
  if(dataFile){
    dataFile.print(now.year()); dataFile.print("/");
    dataFile.print(now.month()); dataFile.print("/");
    dataFile.print(now.day()); dataFile.print(" ");
    dataFile.print(now.hour()); dataFile.print(":");
    dataFile.print(now.minute()); dataFile.print(":");
    dataFile.print(now.second());
    dataFile.print(",");
    dataFile.print(t);
    dataFile.print(",");
    dataFile.println(h);
    dataFile.close();
  }
}

void loop() {
  unsigned long currentMillis = millis();
  
  // ボタンの入力を検出して、SDカードの状態を確認またはLCDの状態を切り替え
  if (digitalRead(BUTTON_PIN) == LOW) {
    tryMountSDCard();
    delay(300);
  }
  
  if (digitalRead(BUTTON2_PIN) == LOW) {
    lcdStatus = !lcdStatus;
    delay(300);
    if(lcdStatus) {
      lcd.backlight();
    } else {
      lcd.noBacklight();
    }
  }

  // 指定した時間が経過したら、データを記録し、Supabaseに送信
  if (sdCardStatus && currentMillis - lastRecordTime >= 5000) {
    recordData();
    lastRecordTime = currentMillis;
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    sendDataToSupabase(t, h);
  }

  // 指定した時間が経過したら、データをLCDに表示
  if (sdCardStatus && currentMillis - lastDisplayUpdateTime >= 4000) {
    displayData();
    lastDisplayUpdateTime = currentMillis;
  }

  if (!lcdStatus) {
    // LCDがオフの場合は何もしない
  } else if(!sdCardStatus) {
    // SDカードが接続されていない場合は、ユーザーにSDカードをチェックするように促すメッセージを表示
    lcd.setCursor(0, 0);
    lcd.print("Check SD Card...  ");
    lcd.setCursor(0, 1);
    lcd.print("Press button to retry");
  }
}
