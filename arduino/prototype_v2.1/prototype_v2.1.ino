// このコードは、ESP32を使ってDHT22センサから湿度と温度を読み取り、それをSupabaseのデータベースに保存するものです。

// 各ライブラリを読み込んでおります。
#include <ESP32_Supabase.h>  // ESP32でSupabaseを使用するためのライブラリ
#include <WiFi.h>            // ESP32のWiFi機能を使用するためのライブラリ
#include <DHT.h>             // DHTセンサを読み取るためのライブラリ

// DHTセンサの設定
#define DHTPIN 2            // DHTセンサが接続されているピン番号
#define DHTTYPE DHT22       // 使用するDHTセンサのタイプ


DHT dht(DHTPIN, DHTTYPE); // DHTセンサのインスタンスを初期化

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

void setup()
{

  Serial.begin(9600); // シリアル通信の開始

  dht.begin(); // DHTセンサの初期化

  // WiFiへの接続を開始
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, psswd);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }

  Serial.println("Connected!"); // WiFi接続が完了したら表示

  db.begin(supabase_url, anon_key); // Supabaseの初期化

  db.login_email(email, password); // 登録しているemailでSupabaseにログイン

}

void loop()
{
  // DHTセンサから湿度と温度を読み取る
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // 温度、湿度、エリアIDをJSON形式で作成
  String JSON = "{ \"temperature\": " + String(temperature) + ", \"humidity\": " + String(humidity) + ", \"area_id\": 1 }";
  
  // JSONデータをSupabaseに保存
  bool upsert = false;
  int code = db.insert(table, JSON, upsert);
  
  // 応答コードとセンサの値をシリアルモニタに表示
  Serial.println(code);
  Serial.println(humidity);
  Serial.println(temperature);
  
  // 10秒待機
  delay(10000);
}
