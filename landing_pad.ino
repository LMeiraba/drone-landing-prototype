#include <WiFi.h>
#include <FastLED.h>

/* ---------- LED ---------- */
#define LED_PIN 15
#define NUM_LEDS 60
CRGB leds[NUM_LEDS];

/* ---------- ULTRASONIC ---------- */
#define TRIG_PIN 5
#define ECHO_PIN 18

/* ---------- WIFI ---------- */
const char* ssid = "Hostel_1+B";
const char* password = "abcd1234";
WiFiServer server(80);

/* ---------- VARIABLES ---------- */
long distanceCM = 0;
uint8_t hue = 0;
int pos = 0;         // LED index for spinning
unsigned long lastBlink = 0;
bool greenOn = false;

/* ---------- ULTRASONIC FUNCTION ---------- */
long readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 25000);
  if (duration == 0) return -1;

  return duration * 0.034 / 2;
}

/* ---------- SETUP ---------- */
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP32 BOOT");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\nWiFi CONNECTED");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.begin();
  Serial.println("HTTP SERVER STARTED");
}

/* ---------- LOOP ---------- */
void loop() {

  /* --- READ DISTANCE --- */
  distanceCM = readUltrasonic(); // Your averaging function

  // Clear LEDs for this frame
  FastLED.clear();

  unsigned long now = millis();

  if (distanceCM > 0 && distanceCM < 20) {
    // ---------- Green blinking ----------
    if (now - lastBlink > 500) {   // blink every 500ms
      greenOn = !greenOn;
      lastBlink = now;
    }
    if (greenOn) {
      for (int i=0;i<NUM_LEDS;i++) leds[i] = CRGB::Green;
    }

  } 
  else if (distanceCM >= 20 && distanceCM < 40) {
    // ---------- Orange spinning ----------
    leds[pos] = CRGB::Orange;
    pos = (pos + 1) % NUM_LEDS;
  } 
  else if (distanceCM >= 40) {
    // ---------- Bright red slow spin ----------
    static unsigned long lastRed = 0;
    static int redPos = 0;
    if (now - lastRed > 150) {   // slower than orange
      redPos = (redPos + 1) % NUM_LEDS;
      lastRed = now;
    }
    leds[redPos] = CRGB::Red;
  }

  FastLED.show();
  pos = (pos + 1) % NUM_LEDS;
  hue += 2;

  /* --- WEB SERVER --- */
  WiFiClient client = server.available();
  if (!client) {
    delay(30);
    return;
  }

  while (!client.available()) delay(1);
  String req = client.readStringUntil('\r');
  client.flush();

  /* --- JSON API --- */
  if (req.indexOf("GET /data") >= 0) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close\n");
    client.print("{\"distance\":");
    client.print(distanceCM);
    client.print("}");
  }

  /* --- MAIN PAGE --- */
  else {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close\n");

    client.print(R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 Landing Pad</title>
<style>
body{
  background:#020617;
  color:#e5e7eb;
  font-family:Segoe UI,sans-serif;
  text-align:center;
  margin-top:50px;
}
.card{
  display:inline-block;
  padding:20px;
  border-radius:16px;
  box-shadow:0 0 30px rgba(0,0,0,.6);
}
h1{margin-bottom:10px;}
.dist{font-size:32px;font-weight:700;}
</style>
</head>

<body>
<div class="card">
  <h1>Landing Pad</h1>
  <div class="dist" id="dist">-- cm</div>
</div>

<script>
function update(){
  fetch('/data')
    .then(r=>r.json())
    .then(d=>{
      document.getElementById('dist').textContent =
        d.distance + " cm";
    });
}
setInterval(update,1000);
update();
</script>
</body>
</html>
)rawliteral");
  }

  client.stop();
}
