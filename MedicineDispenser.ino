
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <time.h>

ESP8266WebServer server(80);

// Structure to store tablet data
struct Tablet {
  String name;
  int quantity;
  String schedule;
  int dosesPerDay;
  String times[3];
};

Tablet tablets[4];
String mode = "Dispensing"; // Default mode

void saveToEEPROM() {
  int addr = 0;
  for (int i = 0; i < 4; i++) {
    EEPROM.write(addr++, tablets[i].quantity);
    for (int j = 0; j < 3; j++) {
      EEPROM.write(addr++, tablets[i].times[j].toInt());
    }
  }
  EEPROM.commit();
}

void loadFromEEPROM() {
  int addr = 0;
  for (int i = 0; i < 4; i++) {
    tablets[i].quantity = EEPROM.read(addr++);
    for (int j = 0; j < 3; j++) {
      tablets[i].times[j] = String(EEPROM.read(addr++));
    }
  }
}

String htmlEscape(String data) {
  data.replace("&", "&amp;");
  data.replace("<", "&lt;");
  data.replace(">", "&gt;");
  data.replace("\"", "&quot;");
//  data.replace(""", "&quot;");
  data.replace("'", "&#39;");
  return data;
}

String generateForm() {
  String page = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  page += "<style>body{font-family:Arial;background:#f0f0f0;padding:20px}h2{color:#333}";
  page += "input,select,button{margin:5px;display:block;width:100%;padding:8px;box-sizing:border-box}";
  page += "label{font-weight:bold;margin-top:10px}hr{margin:20px 0}</style></head><body>";
  page += "<h2>Medicine Dispenser Setup</h2>";

  page += "<form action='/submit' method='POST'>";
  page += "<label>Mode:</label>";
  page += "<input type='hidden' name='mode' value='" + mode + "'>";
  page += "<label class='switch'><input type='checkbox' onchange='toggleMode(this)'";
  if (mode == "Refilling") page += " checked";
  page += "><span>Refilling Mode</span></label><hr>";

  if (mode == "Refilling") {
    for (int i = 0; i < 4; i++) {
      page += "<label>Tablet Type " + String(i + 1) + ":</label>";
      page += "<select name='tablet" + String(i) + "'>";
      page += "<option>Dolo</option><option>Vicks</option><option>Vit C</option><option>Vit D</option><option>Paracetamol</option></select>";
      page += "<label>Quantity (max 32):</label><input type='number' name='quantity" + String(i) + "' max='32'>";
      page += "<label>Schedule:</label><select name='schedule" + String(i) + "'><option>Daily</option><option>Alternate Days</option></select>";
      page += "<label>Doses per day (max 3):</label><select name='doses" + String(i) + "' onchange='showTimes(this, " + String(i) + ")'>";
      page += "<option value='1'>1</option><option value='2'>2</option><option value='3'>3</option></select>";
      page += "<label>Time 1:</label><input type='time' name='time" + String(i) + "_1'>";
      page += "<label>Time 2:</label><input type='time' name='time" + String(i) + "_2'>";
      page += "<label>Time 3:</label><input type='time' name='time" + String(i) + "_3'>";
      page += "<hr>";
    }
    page += "<button type='submit'>Submit</button>";
  } else {
    for (int i = 0; i < 4; i++) {
      page += "<p><b>Type " + String(i + 1) + ":</b> " + htmlEscape(tablets[i].name) + " | Pieces Left: " + String(tablets[i].quantity) + "</p>";
    }
    page += "<hr><label>Set Date & Time:</label><input type='datetime-local' name='setdatetime'>";
    page += "<button type='submit'>Update Time</button>";
  }

  page += "</form><script>";
  page += "function toggleMode(cb){window.location.href='/?mode='+ (cb.checked?'Refilling':'Dispensing');}";
  page += "function showTimes(sel, i){let val=sel.value;for(let t=2;t<=3;t++){";
  page += "let timeInput=document.getElementsByName('time'+i+'_'+t)[0];";
  page += "timeInput.style.display=(t<=val)?'block':'none';}}";
  page += "</script></body></html>";
  return page;
}

void handleRoot() {
  if (server.hasArg("mode")) {
    mode = server.arg("mode");
  }
  server.send(200, "text/html", generateForm());
}

void handleFormSubmit() {
  for (int i = 0; i < 4; i++) {
    tablets[i].name = server.arg("tablet" + String(i));
    tablets[i].quantity = server.arg("quantity" + String(i)).toInt();
    tablets[i].schedule = server.arg("schedule" + String(i));
    tablets[i].dosesPerDay = server.arg("doses" + String(i)).toInt();
    for (int j = 0; j < 3; j++) {
      tablets[i].times[j] = server.arg("time" + String(i) + "_" + String(j + 1));
    }
  }
  saveToEEPROM();
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  loadFromEEPROM();

  WiFi.softAP("MedDispenser", "12345678");
  server.on("/", handleRoot);
  server.on("/submit", HTTP_POST, handleFormSubmit);
  server.begin();
  Serial.println("ESP8266 Server started");
}

void loop() {
  server.handleClient();
}
