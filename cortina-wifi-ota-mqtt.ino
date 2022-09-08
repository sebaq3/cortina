//6/9/2022 INICIO DEL PROGRAMA
//ver el tema de cmabiar la configuracion despues de haber iniciado con el pin de reset
//separar el archivo en 2 (sin la cortina)






#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#ifdef ESP32
  #include <SPIFFS.h>
#endif

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>    //https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WebServer/README.rst
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

#define TRIGGER_PIN D0 //pin para entrar al wifiespecial
#define ledWifi D4
char nombrehost[34]= "prueba";
const char *ssid = "ESPap";
char pass[34]= "password";

// wifimanager can run in a blocking mode or a non blocking mode
// Be sure to know how to process loops with no delay() if using non blocking
bool wm_nonblocking = false; // change to true to use non blocking

//WiFiManager
//Local intialization. Once its business is done, there is no need to keep it around
WiFiManager wifiManager;
ESP8266WebServer server(8080);
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
unsigned long lastMsg2 = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
char msg2[MSG_BUFFER_SIZE];
int value = 0;

bool borrar = false;

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40] = "192.168.101.233";
char mqtt_port[6] = "1883";
char api_token[34] = "YOUR_API_TOKEN";
char mqtt_user[20] = "mqtt_user";
char mqtt_pass[20] = "mqtt_pass";

char tema_pub[40] = "/home/habitacion3/cortinapub"; //"tema_pub" posicion, % de subida
char tema_sub[40] = "/home/habitacion3/cortinasub"; //% posicion "set_position_topic"
char tema_pubest[40] = "/home/habitacion3/cortinaestadopub"; //subiendo... bajando... parado.. etc... "state_topic"
char tema_hab[40] = "/home/habitacion3/habilitado"; //online - offline "availability_topic"

//flag for saving data
bool shouldSaveConfig = false;

long int time1 = 0;
long int time2 = 0;
long int time3 = 0;
long int time4 = 0;
long int time5 = 0;
long int tsubGlob = 0;
long int tbajGlob = 0;
long int porSub = 0;
long int porBaj = 0;

long int tsub = 0;
long int tbaj = 0;

//const int motor1pin1 = D2;
const int motor1pin1 = D5;          
//const int motor2pin2 = D5;
const int motor2pin2 = D2;
//const int limitar = D6;
const int limitar = D7;
//const int limitab = D7;
const int limitab = D6;
//const int borrar = D0;
int nu;
//const int teclasubirpin = D8;
const int teclasubirpin = D1;
//const int teclabajarpin = D1;
const int teclabajarpin = D8;

bool sinwifi = false;

byte cont = 0;
byte max_intentos = 50;

char *dispo = nombrehost;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

//String SendHTML(uint8_t led1stat,uint8_t led2stat){
  String SendHTML(){
    String ptr = "<!DOCTYPE html> <html>\n";
    ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
    ptr +="<title>LED Control</title>\n";
    ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
    ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
    ptr +=".button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
    ptr +=".button-on {background-color: #1abc9c;}\n";
    ptr +=".button-on:active {background-color: #16a085;}\n";
    ptr +=".button-off {background-color: #34495e;}\n";
    ptr +=".button-off:active {background-color: #2c3e50;}\n";
    ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
    ptr +="</style>\n";
    ptr +="</head>\n";
    ptr +="<body>\n";
    ptr +="<h1>ESP8266 Web Server</h1>\n";
    ptr +="<h3>Using Access Point(AP) Mode</h3>\n";   
    {ptr +="<p></p><a class=\"button button-on\" href=\"/subir\">SUBIR</a>\n";}    
    {ptr +="<p></p><a class=\"button button-off\" href=\"/bajar\">BAJAR</a>\n";}
    ptr +="<p>% porSub porcentaje de subida :";
    ptr += porSub ;
    ptr +="</p>\n";
  
    ptr +="</body>\n";
    ptr +="</html>\n";
    return ptr;
  }
  
void handleRoot() {
  

  server.send(200, "text/html", SendHTML());
}

void handle_subir() {
  subirn(100);
  
  Serial.println("SUBIO POR HTML");
  //server.send(200, "text/html", SendHTML()); 
  server.sendHeader("Location","/");        // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303);                         // Send it back to the browser with an HTTP status 303 (See Other) to redirect
}

void handle_bajar() {  
  bajarn((100));
  
  Serial.println("BAJO POR HTML");
  //server.send(200, "text/html", SendHTML()); 
  server.sendHeader("Location","/");        // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303);                         // Send it back to the browser with an HTTP status 303 (See Other) to redirect
}

void handleNotFound() {
  //digitalWrite(led, 1);
  String message = "NO EXISTE\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  //digitalWrite(led, 0);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msjEntrada;
  msjEntrada = "";
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {

    msjEntrada = msjEntrada + (char)payload[i];
    //Serial.print((char)payload[i]);
  }
  Serial.println(msjEntrada);


  int value  = msjEntrada.toInt() ;

  //value = value * 10 ;

  Serial.println (value);

  if (value < 101 && value > -1) {
    if (value < porSub) {
      bajarn((porSub - value));
    } else {
      if (value > porSub) {
        subirn(value - porSub);
      } else {
        if (value == porSub) { }
      }
    }
  }
}

void reconnect() {
  int error = 0;
  
  // Loop until we're reconnected
  while (!client.connected() ){
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    //String clientId = nombrehost;
    //clientId += String(WiFi.macAddress());
    String clientId = "ESP8266Client-";
    clientId += String(WiFi.macAddress());
    // Attempt to connect
    if (client.connect(clientId.c_str(),mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("tema_pub", "hello world");
      // ... and resubscribe
      client.subscribe(tema_sub);
      sinwifi = false;
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      
      // Wait 5 seconds before retrying
      delay(5000);
      error = error + 1;
      if (!sinwifi){
        if (error > 5 ) {
          Serial.println("ENTRO POR ERROR");
          freno();
          delay(1000);
          moverAbajo();
          while (digitalRead(limitab) != 1) {
            delay(1000);
          }
          freno();
        }
      }  
    }
   
  }
  error = 0;
  
}


void moverArriba() {
  digitalWrite(motor1pin1, LOW);
  digitalWrite(motor2pin2, HIGH);
  Serial.println("ARRIBA");
}
void moverAbajo() {
  digitalWrite(motor1pin1, HIGH);
  digitalWrite(motor2pin2, LOW);
  Serial.println("ABAJO");
}

void freno() {
  digitalWrite(motor1pin1, HIGH);
  digitalWrite(motor2pin2, HIGH);
  //Serial.println("STOP");
}


void calibrar() {

  //EN LA CALIBRACION PRIMERO TIENE Q BAJAR NO
  Serial.println("CALIBRANDO");
  //mover arriba hasta limitar
  snprintf (msg, MSG_BUFFER_SIZE, "%s", "closing");
  client.publish(tema_pubest, msg); //ENVIAR ESTADO
  moverAbajo();
  while (digitalRead(limitab) != 1) {
    delay(800);
  }
  //Serial.println("SALIO DEL PRIMER WHILE");
  freno();
  snprintf (msg, MSG_BUFFER_SIZE, "%s", "closed");
  client.publish(tema_pubest, msg); //ENVIAR ESTADO
  delay(800);


  //mover arriba contando segundos hasta limitar
  snprintf (msg, MSG_BUFFER_SIZE, "%s", "opening");
  client.publish(tema_pubest, msg); //ENVIAR ESTADO
  time1 = millis();
  //time1 = 0 PONELE

  moverArriba();
  while (digitalRead(limitar) != 1) {
    delay(800);
  }
  freno();
  snprintf (msg, MSG_BUFFER_SIZE, "%s", "open");
  client.publish(tema_pubest, msg); //ENVIAR ESTADO
  snprintf (msg, MSG_BUFFER_SIZE, "%s", "closing");
  client.publish(tema_pubest, msg); //ENVIAR ESTADO
  delay(1000); //por seguridad para el rele
  time2 = millis()-1000; //le resto la seguridad
  //time2 = 35 (la diferencia entre time1 y time2 es lo q tardo en subir)

  //mover abajo contando segundos hasta limitab

  moverAbajo();

  while (digitalRead(limitab) != 1) {
    delay(800);
  }
  freno();
  time3 = millis();
  snprintf (msg, MSG_BUFFER_SIZE, "%s", "closed");
  client.publish(tema_pubest, msg); //ENVIAR ESTADO
  delay(3000);

  Serial.println("TARDO EN SUBIR");
  tsub = time2 - time1;
  Serial.println(tsub);

  Serial.println("TARDO EN BAJAR");
  tbaj = time3 - time2;
  Serial.println(tbaj);

  tsubGlob = 0;
  porSub = 0;
  tbajGlob = tbaj;
  porBaj = 100;


  //guardar tiempos
  //saveConfig();
  delay(500);

}

void bajarn(int n) {
  snprintf (msg, MSG_BUFFER_SIZE, "%s", "closing");
  client.publish(tema_pubest, msg); //ENVIAR ESTADO
  time4 = millis();
  Serial.print("tbaj: ");
  Serial.println(tbaj);
  long int tvar;
  tvar = 0;
  tvar = tbaj * n / 100;
  moverAbajo(); //mueve abajo
  while (digitalRead(limitab) != 1 &&  time5 < tvar  ) { //si NO llego al tope y time 5 es menor que eso, quedarse en el while
    delay(1000);
    time5 = millis() - time4;
  }
  freno(); //frena
  if (digitalRead(limitab) == 1) { //esta abajo de todo, 100% cerrada
    tbajGlob = tbaj;
    tsubGlob = 0;
    porBaj = 100;
    porSub = 0;
    snprintf (msg, MSG_BUFFER_SIZE, "%s", "closed");
    client.publish(tema_pubest, msg); //ENVIAR ESTADO
  } else {
    tbajGlob = tbajGlob + time5;
    porBaj = tbajGlob * 100 / tbaj;
    porSub = porSub - n;
    long int tvar2 = 0;
    tvar2 = porSub * tsub / 100;
    tsubGlob = tvar2;

  }
  time4 = 0;
  time5 = 0;
  snprintf (msg, MSG_BUFFER_SIZE, "%s", "stopped");
  client.publish(tema_pubest, msg); //ENVIAR ESTADO
}


void subirn(int n) {

  snprintf (msg, MSG_BUFFER_SIZE, "%s", "opening");
  client.publish(tema_pubest, msg); //ENVIAR ESTADO
  Serial.print("tbaj: ");
  Serial.println(tbaj);
  time4 = millis();
  moverArriba(); //mueve arriba
  long int tvar;
  tvar = 0;
  tvar = tsub * n / 100;
  while (digitalRead(limitar) != 1 &&  time5 < tvar  ) {
    delay(1000);
    time5 = millis() - time4;

  }
  freno(); //frena
  //Serial.println(time5);
  if (digitalRead(limitar) == 1) { //esta toda abierta, 0% cerrada
    tbajGlob = 0;
    tsubGlob = tsub;
    porBaj = 0;
    porSub = 100;
    snprintf (msg, MSG_BUFFER_SIZE, "%s", "open");
    client.publish(tema_pubest, msg); //ENVIAR ESTADO
  } else {
    tsubGlob = tsubGlob + time5;
    porSub = tsubGlob * 100 / tsub;
    porBaj = porBaj - n;
    long int tvar2 = 0;
    tvar2 = porBaj * tbaj / 100;
    tbajGlob = tvar2;


  }
  time4 = 0;
  time5 = 0;
  snprintf (msg, MSG_BUFFER_SIZE, "%s", "stopped");
  client.publish(tema_pubest, msg); //ENVIAR ESTADO

}

void subirtecla() {
  //Serial.println("subirtecla");
  snprintf (msg, MSG_BUFFER_SIZE, "%s", "opening");
  client.publish(tema_pubest, msg); //ENVIAR ESTADO
  time4 = millis();
  moverArriba();
  while (digitalRead(teclasubirpin) == 1 && digitalRead(limitar) != 1) {
    delay(1000);
    time5 = millis() - time4; //tiempo q estuvo andando

    if (digitalRead(limitar) == 1) { ///esta arriba de todo, 100% abierta
      freno(); //frena
      tbajGlob = 0;
      tsubGlob = tsub;
      porBaj = 0;
      porSub = 100;
      delay(3000);
      break;
    }
  }

  freno(); //frena
  tsubGlob = tsubGlob + time5;
  porSub = tsubGlob * 100 / tsub;
  porBaj = 100 - porSub;
  long int tvar2 = 0;
  tvar2 = porBaj * tbaj / 100;
  tbajGlob = tvar2;
  time4 = 0;
  time5 = 0;

}


void bajartecla() {
  //Serial.println("bajartecla");
  snprintf (msg, MSG_BUFFER_SIZE, "%s", "closing");
  client.publish(tema_pubest, msg); //ENVIAR ESTADO
  time4 = millis();
  moverAbajo();
  while (digitalRead(teclabajarpin) == 1 && digitalRead(limitab) != 1) {
    delay(1000);
    time5 = millis() - time4; //tiempo q estuvo andando

    if (digitalRead(limitab) == 1) { //esta toda cerrada, 100% cerrada
      freno(); //frena
      tbajGlob = tbaj;
      tsubGlob = 0;
      porBaj = 100;
      porSub = 0;
      delay(3000);
      break;
    }
  }

  freno(); //frena
  tbajGlob = tbajGlob + time5;
  porBaj = tbajGlob * 100 / tbaj;
  porSub = 100 - porBaj;
  long int tvar2 = 0;
  tvar2 = porSub * tsub / 100;
  tsubGlob = tvar2;


  time4 = 0;
  time5 = 0;

}



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();
  freno();
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(nombrehost);
  ArduinoOTA.setPassword(pass);

  pinMode(motor1pin1, OUTPUT);
  pinMode(motor2pin2, OUTPUT);

  pinMode(limitar, INPUT);
  pinMode(limitab, INPUT);
  //pinMode(borrar, INPUT);

  pinMode(teclasubirpin, INPUT);
  pinMode(teclabajarpin, INPUT);
  
  pinMode(TRIGGER_PIN, INPUT);
  wifiManager.setHostname(nombrehost);
  WiFi.hostname(nombrehost);
  if(wm_nonblocking) wifiManager.setConfigPortalBlocking(false);


  std::vector<const char *> menu = {"wifi","info","param","sep","restart","exit"};
  
  wifiManager.setMenu(menu);

  // set dark theme
  wifiManager.setClass("invert");
  //clean FS, for testing
  //SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    //SPIFFS.remove("/config.json");
    //wifiManager.resetSettings(); 
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

  #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
        DynamicJsonDocument json(2048);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if ( ! deserializeError ) {
  #else
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
  #endif
          Serial.println("\nparsed json");
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(api_token, json["api_token"]);          
          strcpy(mqtt_user, json["mqtt_user"]);
          strcpy(mqtt_pass, json["mqtt_pass"]);
          strcpy(tema_pubest, json["tema_pubest"]);
          strcpy(tema_pub, json["tema_pub"]);
          strcpy(tema_sub, json["tema_sub"]);
          strcpy(tema_hab, json["tema_hab"]);

        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_api_token("apikey", "API token", api_token, 32);
  WiFiManagerParameter custom_mqtt_user("mqttuser", "Mqtt User", mqtt_user, 10);
  WiFiManagerParameter custom_mqtt_pass("mqttpass", "Mqtt Pass", mqtt_pass, 10);
  WiFiManagerParameter custom_tema_pubest("tema_pubest", "Estado (closing, opening...)", tema_pubest, 40);
  WiFiManagerParameter custom_tema_pub("tema_pub", "Postion", tema_pub, 40);
  WiFiManagerParameter custom_tema_sub("tema_sub", "Set Position", tema_sub, 40);
  WiFiManagerParameter custom_tema_hab("tema_hab", "Online - Offline", tema_hab, 40);




  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  //wifiManager.setSTAStaticIPConfig(IPAddress(10, 0, 1, 99), IPAddress(10, 0, 1, 1), IPAddress(255, 255, 255, 0));

  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_api_token);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_pass);
  wifiManager.addParameter(&custom_tema_pubest);
  wifiManager.addParameter(&custom_tema_pub);
  wifiManager.addParameter(&custom_tema_sub);
  wifiManager.addParameter(&custom_tema_hab );

  

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(nombrehost, pass)) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    sinwifi= true;
    //ESP.restart();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(api_token, custom_api_token.getValue());
  strcpy(mqtt_user, custom_mqtt_user.getValue());
  strcpy(mqtt_pass, custom_mqtt_pass.getValue());
  strcpy(tema_pubest, custom_tema_pubest.getValue());
  strcpy(tema_pub, custom_tema_pub.getValue());
  strcpy(tema_sub, custom_tema_sub.getValue());
  strcpy(tema_hab, custom_tema_hab.getValue());
  
  
  
  Serial.println("The values in the file are: ");
  Serial.println("\tmqtt_server : " + String(mqtt_server));
  Serial.println("\tmqtt_port : " + String(mqtt_port));
  Serial.println("\tapi_token : " + String(api_token));
  Serial.println("\tmqtt_user : " + String(mqtt_user));
  Serial.println("\tmqtt_pass : " + String(mqtt_pass));
  Serial.println("\ttema_pubest : " + String(tema_pubest));
  Serial.println("\ttema_pub : " + String(tema_pub));
  Serial.println("\ttema_sub : " + String(tema_sub));
  Serial.println("\ttema_hab : " + String(tema_hab));
    
  

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
  #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    DynamicJsonDocument json(2048);
  #else
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
  #endif
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["api_token"] = api_token;
    json["mqtt_user"] = mqtt_user;
    json["mqtt_pass"] = mqtt_pass;
    json["tema_pubest"] = tema_pubest;
    json["tema_pub"] = tema_pub;
    json["tema_sub"] = tema_sub;
    json["tema_hab"] = tema_hab;
    

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

  #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    serializeJson(json, Serial);
    serializeJson(json, configFile);
  #else
    json.printTo(Serial);
    json.printTo(configFile);
  #endif
    configFile.close();
    //end save
  }


  if (sinwifi){
    Serial.println(WiFi.getMode());
    delay(1000);
    WiFi.mode(WIFI_OFF);  
    WiFi.softAPdisconnect(true);  
    delay(5000);    
    WiFi.softAP("ssid2", pass,6,0,3);
    Serial.println(WiFi.softAPIP());
    Serial.println(WiFi.getMode());
    WiFi.softAP(ssid, pass,6,0,3);
    //Serial.println("DHCP status:");
    //Serial.println(wifi_softap_dhcps_status());
  }
  
    ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();

  
  
  
  calibrar();

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/subir", handle_subir);
  server.on("/bajar", handle_bajar);
 

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  uint16_t mqttport = (uint16_t)strtol(mqtt_port, NULL, 10);
  client.setServer(mqtt_server, mqttport);
  client.setCallback(callback);

  

  /////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////COMENTAR ESTAS LINEAS////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////
  tsub = 60000;
  tbaj = 30000;
  /////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////
  
} //FIN DEL SETUP



void checkButton(){
  // check for button press
  if ( digitalRead(TRIGGER_PIN) == LOW ) {
    // poor mans debounce/press-hold, code not ideal for production
    borrar = true;
    delay(50);
    if( digitalRead(TRIGGER_PIN) == LOW ){
      Serial.println("Button Pressed");
      // still holding button for 3000 ms, reset settings, code not ideaa for production
      delay(3000); // reset delay hold
      if( digitalRead(TRIGGER_PIN) == LOW ){
        Serial.println("Button Held");
        Serial.println("Erasing Config, restarting");
        wifiManager.resetSettings();
        ESP.restart();
      }
      
      // start portal w delay
      Serial.println("Starting config portal");
      wifiManager.setConfigPortalTimeout(120);
      
      if (!wifiManager.startConfigPortal("OnDemandAP","password")) {
        Serial.println("failed to connect or hit timeout");
        delay(3000);
        // ESP.restart();
      } else {
        //if you get here you have connected to the WiFi
        Serial.println("connected...yeey :)");
      }
    }
  borrar = false;  
  }
}

void loop() {
  if(wm_nonblocking) wifiManager.process(); // avoid delays() in loop when non-blocking and other long running code  
  checkButton();
  

  // put your main code here, to run repeatedly:
  if (!borrar) server.handleClient();
  MDNS.update();
  ArduinoOTA.handle();
  
  if (!client.connected()&& !sinwifi ) {
    reconnect();
    Serial.println("ACA");
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg2 > 5000) {
    lastMsg2 = now;
    snprintf (msg, MSG_BUFFER_SIZE, "%s", "online");
    client.publish(tema_hab, msg); //
  }
  
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "%ld", porSub);

    //Serial.print("Publish message: ");
    //Serial.println(msg);
    client.publish(tema_pub, msg); //ENVIAR PORCENTAJE DE SUBIDA/abierto
    //Serial.println("");
    //Serial.print("porSub: ");
    //Serial.println(porSub);
    //Serial.print("porBaj: ");
    //Serial.println(porBaj);
    snprintf (msg, MSG_BUFFER_SIZE, "%s", "stopped");
    client.publish(tema_pubest, msg); //ENVIAR ESTADO
  }
  if (digitalRead(teclabajarpin) == 1) {
    bajartecla();
  }
  if (digitalRead(teclasubirpin) == 1) {
    subirtecla();
  }
  if (porSub == 0) {
    porBaj = 100;
  }

}
