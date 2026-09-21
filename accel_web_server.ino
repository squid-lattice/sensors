/*
  Accelerometer -> Web Browser (Arduino UNO R4 WiFi)
  ---------------------------------------------------
  Connects to your phone's hotspot, runs a small web server on the
  Arduino, and serves a page that polls /data every 100ms and shows
  a live readout + graph of X/Y/Z.

  Uses an ADXL345 accelerometer (Adafruit_ADXL345_U library) over I2C.

  SETUP:
    1. Fill in your hotspot's SSID and password below.
    2. Wire the ADXL345 to the R4's I2C pins (SDA/SCL) and power.
    3. Install the "Adafruit ADXL345" and "Adafruit Unified Sensor"
       libraries via Library Manager if you haven't already.
    4. Upload, open Serial Monitor (115200 baud) to see the IP address
       it prints once connected.
    5. On your phone/laptop (connected to the SAME hotspot), open
       that IP address in a browser, e.g. http://192.168.x.x/
*/

#include <WiFiS3.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// ---------- WiFi credentials ----------
const char ssid[] = "Galaxy S10cc06";
const char pass[] = "poluq853";

WiFiServer server(80);

// ---------- Latest sensor values ----------
float accX = 0.0;
float accY = 0.0;
float accZ = 0.0;

// ---------- HTML page served to the browser ----------
const char htmlPage[] PROGMEM = R"====(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Accelerometer</title>
  <style>
    body { font-family: sans-serif; background:#111; color:#eee; text-align:center; margin:0; padding:1rem; }
    h1 { font-size: 1.2rem; font-weight: normal; color:#888; }
    .readout { display:flex; justify-content:center; gap:1.5rem; margin:1rem 0; }
    .val { font-size:1.8rem; font-variant-numeric: tabular-nums; }
    .label { font-size:0.9rem; color:#888; }
    .x { color:#ff6b6b; } .y { color:#6bcB77; } .z { color:#4d96ff; }
    canvas { background:#1a1a1a; border-radius:8px; width:100%; max-width:700px; height:250px; }
  </style>
</head>
<body>
  <h1>Live Accelerometer Data</h1>
  <div class="readout">
    <div><div class="val x" id="vx">0.00</div><div class="label">X</div></div>
    <div><div class="val y" id="vy">0.00</div><div class="label">Y</div></div>
    <div><div class="val z" id="vz">0.00</div><div class="label">Z</div></div>
  </div>
  <canvas id="chart" width="700" height="250"></canvas>

<script>
const N = 150; // points of history shown
const hist = { x: [], y: [], z: [] };
const canvas = document.getElementById('chart');
const ctx = canvas.getContext('2d');

function pushVal(arr, v) {
  arr.push(v);
  if (arr.length > N) arr.shift();
}

function draw() {
  const w = canvas.width, h = canvas.height;
  ctx.clearRect(0, 0, w, h);

  // gridline at 0
  ctx.strokeStyle = '#333';
  ctx.beginPath();
  ctx.moveTo(0, h/2);
  ctx.lineTo(w, h/2);
  ctx.stroke();

  const range = 20.0; // +/- m/s^2 shown on screen (ADXL345 at +/-2g range tops out around 19.6 m/s^2)
  function plot(arr, color) {
    ctx.strokeStyle = color;
    ctx.lineWidth = 2;
    ctx.beginPath();
    arr.forEach((v, i) => {
      const x = (i / N) * w;
      const y = h/2 - (v / range) * (h/2);
      i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
    });
    ctx.stroke();
  }
  plot(hist.x, '#ff6b6b');
  plot(hist.y, '#6bcb77');
  plot(hist.z, '#4d96ff');
}

async function poll() {
  try {
    const res = await fetch('/data');
    const d = await res.json();
    document.getElementById('vx').textContent = d.x.toFixed(2);
    document.getElementById('vy').textContent = d.y.toFixed(2);
    document.getElementById('vz').textContent = d.z.toFixed(2);
    pushVal(hist.x, d.x);
    pushVal(hist.y, d.y);
    pushVal(hist.z, d.z);
    draw();
  } catch (e) {
    // silently retry
  }
  setTimeout(poll, 20);
}
poll();
</script>
</body>
</html>
)====";

void setup() {
  Serial.begin(115200);

  // ---- Accelerometer init ----
  if (!accel.begin()) {
    Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
    while (1);
  }
  accel.setRange(ADXL345_RANGE_2_G);
  // ---------------------------------------------------------------

  Serial.print("Connecting to ");
  Serial.println(ssid);

  int status = WL_IDLE_STATUS;
  while (status != WL_CONNECTED) {
    status = WiFi.begin(ssid, pass);
    delay(2000);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Connected!");
  Serial.print("Open this in your browser: http://");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  readAccelerometer();
  handleClient();
}

void readAccelerometer() {
  accel.setDataRate(ADXL345_DATARATE_400_HZ);
  sensors_event_t event;
  accel.getEvent(&event);

  accX = event.acceleration.x;
  accY = event.acceleration.y;
  accZ = event.acceleration.z;

  // Keep printing to Serial too, for debugging over USB
  Serial.print(accX);
  Serial.print(",");
  Serial.print(accY);
  Serial.print(",");
  Serial.println(accZ);
}

void handleClient() {
  WiFiClient client = server.available();
  if (!client) return;

  // Read the request line
  String request = client.readStringUntil('\r');
  client.flush();

  if (request.indexOf("GET /data") >= 0) {
    String json = "{\"x\":" + String(accX, 3) +
                  ",\"y\":" + String(accY, 3) +
                  ",\"z\":" + String(accZ, 3) + "}";

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.println();
    client.println(json);
  } else {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println(htmlPage);
  }

  delay(1);
  client.stop();
}
