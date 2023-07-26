// このコードは、温湿度センサからのデータを取得し、LCDに表示するさらに、取得したデータを30分おきにmicroSDカードのCSVファイルに保存します。
// 2つのボタンを取り付けており、ボタンを使用して、SDカードのマウントを再試行することができ、また、もう一方のボタンでLCDのバックライトをオン/オフ切り替えることができます。
// RTCモジュールを使用して、データにタイムスタンプを付けることができるため、オフラインの環境で現在時刻と温湿度を紐付けて保存することができます。

// 各ライブラリを読み込んでおります。
#include "FS.h"               // ファイルシステムライブラリ
#include "SD.h"               // SDカードライブラリ
#include "SPI.h"              // SPI通信ライブラリ
#include <Wire.h>             // I2C通信ライブラリ
#include <LiquidCrystal_I2C.h>  // I2C接続LCDライブラリ
#include <DHT.h>              // 温湿度センサライブラリ
#include <RTClib.h>           // RTCライブラリ

// 各ピンとセンサの型の定義をしております。
#define DHTPIN 2              // DHTセンサ接続のピン
#define DHTTYPE DHT22         // DHTセンサの型
#define SCK 18                // SDカードのSCKピン
#define MISO 19               // SDカードのMISOピン
#define MOSI 23               // SDカードのMOSIピン
#define SS 4                  // SDカードのSSピン
#define BUTTON_PIN 15         // 1つ目のボタンのピン(SDカードを再読み込みする際に押すボタン)
#define BUTTON2_PIN 16        // 2つ目のボタンのピン(LCDディスプレイのバックライトを消すボタン)
#define SDA_PIN 25            // I2CのSDAピン
#define SCL_PIN 26            // I2CのSCLピン

// 各モジュール、センサのオブジェクトの初期化をしております。
DHT dht(DHTPIN, DHTTYPE);                  // DHTセンサオブジェクト
LiquidCrystal_I2C lcd(0x27, 16, 2);       // LCDオブジェクト
RTC_DS1307 rtc;                           // RTCオブジェクト
volatile bool sdCardStatus = false;       // SDカードのステータスフラグ
bool lcdStatus = true;                    // LCDのステータスフラグ

unsigned long lastRecordTime = 0;         // 最後にデータを記録した時間
unsigned long lastDisplayUpdateTime = 0;  // 最後に表示を更新した時間

void setup() {
  lcd.init();                 // LCDの初期化
  lcd.backlight();            // LCDのバックライトをオン
  dht.begin();                // DHTセンサの初期化
  Wire.begin();               // I2C通信の開始

  if (!rtc.begin()) {         // RTCの初期化
    lcd.print("Couldn't find RTC");   // RTCが見つからない場合、エラーメッセージ表示
  }

  // RTCの時間調整（コメントアウト）
  // 最初にこのコードを読み込み、ESP32に書き込みます。その後、RTCモジュールに記録されるため、
  // こちらの行をコメントアウトして、再度書き込みます。
  // rtc.adjust(DateTime(2023, 7, 26, 16, 24, 0)); /

  pinMode(BUTTON_PIN, INPUT_PULLUP);        // (SDカードを再読み込みする際に押すボタン)を内部のプルアップ抵抗と接続して、入力として設定
  pinMode(BUTTON2_PIN, INPUT_PULLUP);       // (LCDディスプレイのバックライトを消すボタン)を内部のプルアップ抵抗と接続して、入力として設定

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), tryMountSDCard, FALLING);  // ボタンの割り込み設定

  tryMountSDCard();          // SDカードをマウントする関数を呼び出し
}

void tryMountSDCard() {
  lcd.clear();               // LCDのクリア
  if(!SD.begin()){           // SDカードの初期化
    lcd.print("Card Mount Failed");  // 初期化失敗の場合、エラーメッセージ表示
    sdCardStatus = false;    // SDカードステータスをfalseに設定
    return;
  }
  uint8_t cardType = SD.cardType();  // SDカードのタイプを取得
  if(cardType == CARD_NONE){         
    lcd.print("No SD card attached");  // カードが存在しない場合、エラーメッセージ表示
    sdCardStatus = false;              // SDカードステータスをfalseに設定
  }
  else {
    sdCardStatus = true;               // SDカードが存在する場合、ステータスをtrueに設定
  }
}

void displayData() {
  float h = dht.readHumidity();        // 湿度を取得
  float t = dht.readTemperature();     // 温度を取得

  lcd.setCursor(0, 0);                 // LCDのカーソルを設定
  lcd.print("Temp: ");                 // "Temp:"と表示
  lcd.print(t);                        // 温度を表示
  lcd.print("C");                      // "C"（摂氏）と表示

  lcd.setCursor(0, 1);                 // LCDの次の行にカーソルを移動
  lcd.print("Hum: ");                  // "Hum:"と表示
  lcd.print(h);                        // 湿度を表示
  lcd.print("%");                      // "%"と表示
}

void recordData() {
  DateTime now = rtc.now();            // 現在の時刻を取得
  float h = dht.readHumidity();        // 湿度を取得
  float t = dht.readTemperature();     // 温度を取得

  File dataFile = SD.open("/data.csv", FILE_APPEND); // SDカード上のCSVファイルを開く（または作成）
  if(dataFile){                        // ファイルが正常に開かれた場合
    // ファイルに日時、温度、湿度をCSV形式で書き込む
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
    dataFile.close();                  // ファイルを閉じる
  }
}

void loop() {
  unsigned long currentMillis = millis();  // 現在の時間（ミリ秒）を取得

  if (digitalRead(BUTTON_PIN) == LOW) {    // 1つ目のボタンが押された場合
    tryMountSDCard();                      // SDカードをマウントする関数を呼び出し
    delay(300);                            // 300ミリ秒の遅延
  }

  if (digitalRead(BUTTON2_PIN) == LOW) {   // 2つ目のボタンが押された場合
    lcdStatus = !lcdStatus;                // LCDの状態を切り替える
    delay(300);                            // 300ミリ秒の遅延
    if(lcdStatus) {
      lcd.backlight();                     // LCDバックライトをオン
    } else {
      lcd.noBacklight();                   // LCDバックライトをオフ
    }
  }

  if (sdCardStatus && lcdStatus && currentMillis - lastRecordTime >= 1800000) {
    recordData();                          // 30分ごとにデータを記録
    lastRecordTime = currentMillis;        // 最後にデータを記録した時間を更新
  }

  if (sdCardStatus && lcdStatus && currentMillis - lastDisplayUpdateTime >= 4000) {
    displayData();                         // 4秒ごとにLCDにデータを表示
    lastDisplayUpdateTime = currentMillis; // 最後に表示を更新した時間を更新
  }

  if (!lcdStatus) {                        // LCDがオフの場合、何もしない
  } else if(!sdCardStatus) {               // SDカードが存在しない場合、エラーメッセージを表示
    lcd.setCursor(0, 0);
    lcd.print("Check SD Card...  ");
    lcd.setCursor(0, 1);
    lcd.print("Press button to retry");
  }
}
