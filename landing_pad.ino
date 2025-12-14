#include <WiFi.h>
#include <Adafruit_NeoPixel.h>

/* ------------ LED CONFIG ------------ */
#define LED_PIN     15
#define NUM_LEDS    60
#define BRIGHTNESS  50

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

/* ------------ SENSOR PINS ------------ */
#define TRIG_PIN    5
#define ECHO_PIN    18
#define IR_PIN      34
#define BUZZER_PIN  12

/* ------------ WIFI CONFIG ------------ */
const char* ssid = "L";
const char* password = "123456789";
WiFiServer server(80);

/* ------------ VARIABLES ------------ */
long usDist, irDist, finalDist;
String status = "Idle";

/* ------------ FUNCTIONS ------------ */
long readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long d = pulseIn(ECHO_PIN, HIGH, 25000);
  if (!d) return 400;
  return d * 0.034 / 2;
}

long readIR() {
  int v = analogRead(IR_PIN);
  return map(v, 3000, 1000, 10, 80);
}

void setRing(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < NUM_LEDS; i++)
    strip.setPixelColor(i, strip.Color(r, g, b));
  strip.show();
}

void approachAnim() {
  static int p = 0;
  strip.clear();
  strip.setPixelColor(p, strip.Color(255,140,0));
  strip.show();
  p = (p + 1) % NUM_LEDS;
}

void beep() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(120);
  digitalWrite(BUZZER_PIN, LOW);
}

/* ------------ SETUP ------------ */
void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  strip.begin();
  strip.setBrightness(BRIGHTNESS);

  WiFi.softAP(ssid, password);
  server.begin();
}

/* ------------ LOOP ------------ */
void loop() {
  usDist = readUltrasonic();
  irDist = readIR();
  finalDist = min(usDist, irDist);

  if (finalDist < 20) {
    status = "Landing";
    setRing(0,255,0);
    beep();
  } else if (finalDist < 40) {
    status = "Approaching";
    approachAnim();
  } else {
    status = "Idle";
    strip.clear(); strip.show();
  }

  WiFiClient client = server.available();
  if (!client) return;

  while (!client.available()) delay(1);
  String req = client.readStringUntil('\r');
  client.flush();

  /* ---- JSON API ---- */
  if (req.indexOf("GET /data") >= 0) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close\n");
    client.print("{");
    client.print("\"status\":\"" + status + "\",");
    client.print("\"ultrasonic\":" + String(usDist) + ",");
    client.print("\"ir\":" + String(irDist) + ",");
    client.print("\"final\":" + String(finalDist));
    client.print("}");
  }

  /* ---- MAIN PAGE ---- */
  else {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close\n");
    client.print(R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Landing Pad</title>

<style>
body {
  margin:0;
  background:#0f172a;
  color:#e5e7eb;
  font-family:Segoe UI, sans-serif;
}
.card {
  max-width:360px;
  margin:30px auto;
  background:#020617;
  border-radius:16px;
  padding:20px;
  box-shadow:0 0 30px rgba(0,0,0,.6);
}
h1 { text-align:center; margin-bottom:10px; }
.status {
  text-align:center;
  font-size:22px;
  font-weight:bold;
}
.idle { color:#94a3b8; }
.approach { color:#fb923c; }
.landing { color:#22c55e; }
.data {
  margin-top:15px;
  display:flex;
  justify-content:space-between;
}
.box {
  width:30%;
  background:#020617;
  border:1px solid #1e293b;
  padding:10px;
  border-radius:10px;
  text-align:center;
}
</style>
</head>

<body>
<div class="card">
  <h1>🚁 Landing Pad</h1>
  <div id="status" class="status idle">Idle</div>

  <div class="data">
    <div class="box">
      <div id="us">--</div>
      <small>Ultrasonic</small>
    </div>
    <div class="box">
      <div id="ir">--</div>
      <small>IR</small>
    </div>
    <div class="box">
      <div id="final">--</div>
      <small>Final</small>
    </div>
  </div>
</div>

<script>
function update() {
  fetch('/data')
    .then(r => r.json())
    .then(d => {
      status.textContent = d.status;
      us.textContent = d.ultrasonic + " cm";
      ir.textContent = d.ir + " cm";
      final.textContent = d.final + " cm";

      status.className = "status " +
        (d.status=="Landing" ? "landing" :
         d.status=="Approaching" ? "approach" : "idle");
    });
}
setInterval(update, 1000);
update();
</script>
</body>
</html>
)rawliteral");
  }
  client.stop();
}
