#include <WiFi.h>
#include <FastLED.h>

/* ===================== HARDWARE CONFIG ===================== */
// CORRECT PINS (Matching your "Old Working Code")
#define TRIG_PIN    5   
#define ECHO_PIN    18
#define LED_PIN     15
#define BUZZER_PIN  12  

#define NUM_LEDS    60

/* ===================== WIFI SETTINGS ===================== */
const char* ssid = "Hostel_1+B"; 
const char* password = "abcd1234";

/* ===================== OBJECTS ===================== */
CRGB leds[NUM_LEDS];
WiFiServer server(80);

/* ===================== ANIMATION CONSTANTS ===================== */
#define ANIM_STATIC 0
#define ANIM_BLINK  1
#define ANIM_TRAIL  2   

/* ===================== CONFIG STRUCTURE ===================== */
struct RangeConfig {
  int minD, maxD;
  uint8_t anim;
  uint8_t r, g, b;
  uint16_t speed;
  uint8_t brightness;
  uint8_t buzzer;
};

/* ===================== DEFAULTS ===================== */
RangeConfig ranges[] = {
  // MinD, MaxD, Anim,        R,   G,   B,   Speed, Bright, Buzz
  { -1,   0,    ANIM_TRAIL,  255, 0,   0,   120,   60,     0 }, // 0: No Echo -> Red Trail
  {  0,   20,   ANIM_BLINK,  0,   255, 0,   250,   150,    2 }, // 1: 0-20cm -> Green Blink
  { 20,   50,   ANIM_TRAIL,  255, 100, 0,   60,    100,    1 }, // 2: 20-50cm -> Orange Trail
  { 50,  -1,    ANIM_TRAIL,  255, 0,   0,   120,   80,     0 }  // 3: 50cm+  -> Red Trail
};
const int RANGE_COUNT = 4;

/* ===================== STATE VARS ===================== */
long distanceCM = -1;
int ledPos = 0;
bool blinkState = false;
unsigned long lastAnim = 0;
unsigned long lastSensor = 0;
unsigned long lastBuzz = 0;

/* ===================== SENSOR LOGIC ===================== */
long readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Timeout 25ms
  long d = pulseIn(ECHO_PIN, HIGH, 25000); 
  
  if (d == 0) return -1; 
  return d * 0.034 / 2;
}

/* ===================== ACTIVE RANGE SELECTOR ===================== */
RangeConfig* activeRange() {
  if (distanceCM < 0) return &ranges[0];

  for (int i = 1; i < RANGE_COUNT; i++) {
    bool minMet = (ranges[i].minD == -1) || (distanceCM >= ranges[i].minD);
    bool maxMet = (ranges[i].maxD == -1) || (distanceCM < ranges[i].maxD);
    if (minMet && maxMet) return &ranges[i];
  }
  return &ranges[0];
}

/* ===================== OUTPUT HANDLERS ===================== */
void handleBuzzer(uint8_t mode) {
  if (mode == 0) { noTone(BUZZER_PIN); return; }
  
  unsigned long now = millis();
  unsigned long interval = (mode == 1) ? 600 : 150; 
  
  if (now - lastBuzz > interval) {
    tone(BUZZER_PIN, (mode == 1 ? 2000 : 3000), 80);
    lastBuzz = now;
  }
}

void updateLEDs() {
  RangeConfig* cfg = activeRange();
  FastLED.setBrightness(cfg->brightness);
  
  // === TRAIL FIX ===
  // Decreased from 50 to 10. 
  // Lower number = Slower Fade = Longer Trail.
  fadeToBlackBy(leds, NUM_LEDS, 10);

  unsigned long now = millis();
  CRGB color(cfg->r, cfg->g, cfg->b);

  if (cfg->anim == ANIM_STATIC) {
    fill_solid(leds, NUM_LEDS, color);
  }
  else if (cfg->anim == ANIM_BLINK) {
    if (now - lastAnim > cfg->speed) {
      blinkState = !blinkState;
      lastAnim = now;
    }
    if (blinkState) fill_solid(leds, NUM_LEDS, color);
  }
  else if (cfg->anim == ANIM_TRAIL) {
    if (now - lastAnim > cfg->speed) {
      ledPos = (ledPos + 1) % NUM_LEDS;
      lastAnim = now;
    }
    leds[ledPos] = color;
  }
  handleBuzzer(cfg->buzzer);
  FastLED.show();
}

/* ===================== SETUP ===================== */
void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500); 
  FastLED.clear();
  FastLED.show();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print("."); 
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP: "); Serial.println(WiFi.localIP());
  
  server.begin();
}

/* ===================== HELPERS ===================== */
int getArg(String req, String key, int def) {
  int idx = req.indexOf(key + "=");
  if (idx == -1) return def;
  int end = req.indexOf("&", idx);
  if (end == -1) end = req.indexOf(" ", idx);
  return req.substring(idx + key.length() + 1, end).toInt();
}

String getConfigJSON() {
  String s = "[";
  for(int i=0; i<RANGE_COUNT; i++) {
    s += "{";
    s += "\"id\":" + String(i) + ",";
    s += "\"minD\":" + String(ranges[i].minD) + ",";
    s += "\"maxD\":" + String(ranges[i].maxD) + ",";
    s += "\"anim\":" + String(ranges[i].anim) + ",";
    s += "\"r\":" + String(ranges[i].r) + ",";
    s += "\"g\":" + String(ranges[i].g) + ",";
    s += "\"b\":" + String(ranges[i].b) + ",";
    s += "\"speed\":" + String(ranges[i].speed) + ",";
    s += "\"bright\":" + String(ranges[i].brightness) + ",";
    s += "\"buzz\":" + String(ranges[i].buzzer);
    s += "}";
    if(i < RANGE_COUNT-1) s += ",";
  }
  s += "]";
  return s;
}

/* ===================== LOOP ===================== */
void loop() {
  // 1. Read Sensor
  if (millis() - lastSensor > 100) {
    distanceCM = readUltrasonic();
    lastSensor = millis();
  }

  // 2. Update Effects
  updateLEDs();

  // 3. Web Server
  WiFiClient client = server.available();
  if (client) {
    unsigned long start = millis();
    while (client.connected() && millis() - start < 100) {
      if (client.available()) {
        String req = client.readStringUntil('\r');
        client.flush();

        if (req.indexOf("/data") >= 0) {
           client.print("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"distance\":" + String(distanceCM) + "}");
        } else if (req.indexOf("/config") >= 0) {
           client.print("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" + getConfigJSON());
        } else if (req.indexOf("/set") >= 0) {
           int id = getArg(req, "id", -1);
           if (id >= 0 && id < RANGE_COUNT) {
             ranges[id].minD = getArg(req, "minD", ranges[id].minD);
             ranges[id].maxD = getArg(req, "maxD", ranges[id].maxD);
             ranges[id].anim = getArg(req, "anim", ranges[id].anim);
             ranges[id].r = getArg(req, "r", ranges[id].r);
             ranges[id].g = getArg(req, "g", ranges[id].g);
             ranges[id].b = getArg(req, "b", ranges[id].b);
             ranges[id].speed = getArg(req, "speed", ranges[id].speed);
             ranges[id].brightness = getArg(req, "bright", ranges[id].brightness);
             ranges[id].buzzer = getArg(req, "buzz", ranges[id].buzzer);
           }
           client.print("HTTP/1.1 200 OK\r\n\r\n");
        } else {
           client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
           // START RAW STRING
           client.print(R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Landing Pad</title>
<style>
body{font-family:sans-serif;background:#020617;color:#e5e7eb;text-align:center;padding:10px}
.card{background:#0f172a;padding:20px;margin:20px auto;max-width:400px;border-radius:14px;box-shadow:0 10px 15px -3px rgba(0,0,0,0.5)}
input,select{width:100%;padding:10px;margin:8px 0;background:#1e293b;color:#fff;border:1px solid #334155;border-radius:6px;box-sizing:border-box}
button{width:100%;padding:12px;background:#22c55e;color:#fff;border:none;border-radius:6px;cursor:pointer;font-weight:bold;margin-top:10px}
label{display:block;margin-top:10px;font-size:0.9rem;color:#94a3b8;text-align:left}
h1{margin:0;font-size:2.5rem}
</style></head><body>
<div class="card"><h1><span id="d">--</span> <small style="font-size:1rem">cm</small></h1></div>
<div class="card">
<label>Select Range to Edit</label>
<select id="id" onchange="L()"><option value=0>Range 0 (No Echo / Error)</option><option value=1>Range 1 (Landing 0-20)</option><option value=2>Range 2 (Approach 20-50)</option><option value=3>Range 3 (Far 50+)</option></select>
<label>Distance Limits (Min / Max)</label>
<div style="display:flex;gap:5px"><input id="minD" placeholder="Min"><input id="maxD" placeholder="Max"></div>
<label>Animation & Color</label>
<select id="anim"><option value=0>Static Color</option><option value=1>Blink</option><option value=2>Trail (Spin)</option></select>
<div style="display:flex;gap:5px"><input id="r" placeholder="R"><input id="g" placeholder="G"><input id="b" placeholder="B"></div>
<label>Speed & Brightness & Buzzer</label>
<div style="display:flex;gap:5px"><input id="speed" placeholder="Speed"><input id="bright" placeholder="Bright"></div>
<select id="buzz"><option value=0>Buzzer Off</option><option value=1>Slow Beep</option><option value=2>Fast Beep</option></select>
<button onclick="S()">Save Configuration</button>
</div>
<script>
let C=[];
function L(){let c=C[document.getElementById('id').value];if(!c)return;['minD','maxD','anim','r','g','b','speed','bright','buzz'].forEach(k=>document.getElementById(k).value=c[k]);}
function S(){let q=`id=${document.getElementById('id').value}`;['minD','maxD','anim','r','g','b','speed','bright','buzz'].forEach(k=>{q+=`&${k}=${document.getElementById(k).value}`});fetch('/set?'+q).then(()=>{alert("Saved!"); fetchConfig();});}
function fetchConfig(){fetch('/config').then(r=>r.json()).then(d=>{C=d;L()});}
setInterval(()=>fetch('/data').then(r=>r.json()).then(d=>document.getElementById('d').innerText=d.distance),500);
fetchConfig();
</script></body></html>
)rawliteral"); 
           // END RAW STRING
        }
        break;
      }
    }
    client.stop();
  }
}