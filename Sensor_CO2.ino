/*

*/

#include <Arduino.h>                         //LIBRERIAS PARA CONXEXION Y MEMORIA
#include <WiFi.h>                            //ESP32 Core WiFi Library  
#include <AsyncTCP.h>                        //
#include <SPIFFS.h>                          //
#include <ESPAsyncWebServer.h>               //CONEXION

#include <HTTPClient.h>
#include <Arduino_JSON.h>

#include <TFT_eSPI.h>                        // Graphics and font library for ILI9341 driver chip PANTALLA
#include <SPI.h>                             // PANTALLA
#include <svm30.h>                           // SENSOR
//#include "EasyBuzzer.h"                      // BUZZER
#include<Ticker.h>                           // TIEMPO
TFT_eSPI tft = TFT_eSPI();                   // Invoke library
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DEBUG false                          //define driver debug false : no messages
#define DELAY 10                             //define number of seconds between display results
#define activar_transistor 32                //BATERIA: pin para activar transistor
#define activar_buzzer 33                    //PARA ACTIVAR TRANSISTOR QUE ACTIVA BUZZER
#define lectura_bateria 35                   //BATERIA: pin para leer el valor de la bateria
#define activar_SyP 25                       //TRANSISTOR para activar sensor y pantalla
//#define CON_WIFI 34                          //PIN PARA CREAR AP 


// constants won't change. They're used here to set pin numbers:
const int CON_WIFI = 34; // the number of the pushbutton pin
const int SHORT_PRESS_TIME = 1000; // 1000 milliseconds
const int LONG_PRESS_TIME  = 1000; // 1000 milliseconds


// Variables will change:
int lastState = LOW;  // the previous state from the input pin
int currentState;     // the current reading from the input pin
unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;
bool isPressing = false;
bool isLongDetected = false;
bool RecibiDatoServer = false; //PRUEBA ALEJANDRA PARA DISPLAY TX



//String internetIP;                           //variable para almacenar la ip cuando hay conexion a internet y que se abra una nueva pestaña con la nueva ip
bool bandera,banderaConectado,banderaDesconectado;
bool banderaInterrupcionWiFi = false;
bool banderaMovil = false;
bool banderaStop = false;
int yourSetpointCO2_alto, yourSetpointCO2_bajo;  //setpoints
String altoDefault = "800";                    //valor default
String bajoDefault = "500";                    //valor default
int COMP=0;                                  //PARA CALIBRACIÓN
int CO2_COMP=0;                              //PARA ALMACENAR VALOR YA CALIBRADO
unsigned int frequency = 1000;               //PARA BUZZER
unsigned int beeps = 1;                      //PARA BUZZER

Ticker actBattery;                           //PARA LEER BATERIA CADA CIERTO TIEMPO
Ticker Fijo;                                 //PARA PROTOTIPO FIJO
Ticker actualizarInfo;                       //PARA ACTUALIZAR LA INFORMACION DE LA PAGINA
Ticker pantalla;                             //PARA APAGAR LA PANTALLA
long sum = 0;                                // Variable que almacena la suma de los valores adquiridos por el ADC para despues hacer un promedio con esos valores
float voltage = 0.0;                         // calculated voltage
float output = 0.0;                          //output value
int Vbateria = 0;                            //Voltaje de bateria ya redondeado
const float battery_max = 2.78; //2.85;               //maximum voltage of battery
const float battery_min = 2.16; //2.5;               //minimum voltage of battery before shutdown

SVM30 svm;                                   // create instance
struct svm_values v;                         // store values

AsyncWebServer server(80);
String sensorName; // = "246F286068D8";                           //MAC del sensor o ID
String nombreSensor;                         //Nombre de sensor
String yourEdificio;
String yourSalon;
const char* ssid = "Sensor CO2";             //Cuando se crea el Access point se crea con este nombre y tambien se puede agregar una contraseña //const char* password = "12345";
const char* ssid2 = "nombreSSID";            //Variables para almacenar las credenciales de acceso a wifi
const char* password2 = "passwordSSID";      //Variables para almacenar las credenciales de acceso a wifi
const char* PARAM_ONE = "SetpointCO2_alto";  //Variables para almacenar setpoint
const char* PARAM_FOUR = "SetpointCO2_bajo"; //Variables para almacenar setpoint
const char* PARAM_TWO = "Edificio";
const char* PARAM_THREE = "Salon";
const char* nombreSSID2 = "";
const char* passwordSSID2= "";

int sensortemperatura,sensorhumedad;

String sensorReadings;
String sensorReadingsArr[6];

const char* serverName = "http://192.168.0.100:8002/api/lecturas/getInfoSensor/";              
const char* serverName2 = "http://192.168.0.100:8002/api/lecturas/guardarDatosSensor/";        

/**************************AQUI EMPIEZA CODIGO HTML PARA SETPOINTS, SSID Y CONTRASEÑA********************************/
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <link rel="icon" href="/favicon">
        <link rel="stylesheet" href="/estilos">
        <title>Suomi Air. Configuracion de conexion</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
    </head>
    <body>
        <div class="center">
            <div style="text-align: center;">
                <img src="/logoair" width="25%">
            </div>
            <h2 style="text-align: center;">Configuración de los valores de conexión</h2>
            <form action="/get" target="hidden-form">
                <table class="tabla">
                    <tr>
                        <td><span class="etiqueta">Nombre de la red</span></td>
                        <td><input type="text" name="nombreSSID" id="nombreSSID" required></td>
                    </tr>
                    <tr>
                        <td><span class="etiqueta">Contraseña</span></td>
                        <td><input type="password" name="passwordSSID" id="passwordSSID" required></td>
                    </tr>
                    <tr>
                        <td><span class="etiqueta">Setpoint CO² alto</span><span class="opcional">(opcional)</span></td>
                        <td><input type="text" name="setpointCO2_alto"></td>
                    </tr>
                    <tr>
                        <td><span class="etiqueta">Setpoint CO² bajo</span><span class="opcional">(opcional)</span></td>
                        <td><input type="text" name="setpointCO2_bajo"></td>
                    </tr>
                </table>
                <div class="centrar">
                    <input type="submit" class="boton" value="Actualizar informacion" onclick="actualizar()">
                </div>
                <div class="centrar2">
                    <button class="boton" onclick = "conectar('on');">Realizar conexion</button>
                </div>
                <iframe style="display:none" name="hidden-form"></iframe>
            </form>
        </div>
        <div class="footer">
            <img src="/logo" width="1%">
            <span style="margin-left: 5px;">Suomi 2022</span>
        </div>
    </body>
    <script type="text/javascript">
        function actualizar() {
            //console.log("NombreSSID: " + document.getElementById("nombreSSID").value);
            //console.log("PasswordSSID: " + document.getElementById("passwordSSID").value);
            if(document.getElementById("nombreSSID").value == null || document.getElementById("passwordSSID").value == null ||
              document.getElementById("nombreSSID").value == "" || document.getElementById("passwordSSID").value == ""){
                alert('Para actualizar valores de conexion es necesario llenar el nombre de la red y la contraseña');
            }else{
                alert("Los valores de conexión se actualizaron exitosamente");
                setTimeout(function(){ document.location.reload(false); }, 500);   
            }
        }
        function conectar(x) {
        var xhr = new XMLHttpRequest();
            xhr.open("GET", "/" + x, true);
            xhr.send();
        }
    </script>
</html>)rawliteral";

/**********************************************************************************************************************************/


void read_id();                              // function prototypes (sometimes the pre-processor does not create prototypes themself on ESPxx)
void read_featureSet();
void read_values();
void CO2();
void CO2X();
void CO2XX(); 
void condicionCO2();
void battery_read();
void leerSensor();
void BATERIA_1O();
void bateriamenor100();
void BATERIA_1OO(); 
void simbolo_wifiCONECTADO(); 
void simbolo_wifiDESCONECTADO();
void simbolo_TX_CONECTADO(); 
void simbolo_TX_DESCONECTADO();
void notFound(AsyncWebServerRequest *request); 
String readFile(fs::FS &fs, const char * path);
void writeFile(fs::FS &fs, const char * path, const char * message);
String processor(const String& var);
String RemoveCharacters(String x);
void initializeSPIFFS(); 
String httpPOSTRequest(const char* serverName);
void variablesArray();
void guardarMaxyMin();
void enviarDatosco2Alto();
void enviarDatosFijo();
void initWiFi(const char* ssid,const char* password); 
void Condfijo_Movil();
void actualizarInformacion();
void prenderPantalla();
void apagarPantalla();
void longPress_ShortPress();
//////////toño
const int  puertoPin = 23;    // the pin that the pushbutton is attached to
// Variables will change:
int puertoPushCounter = 0;   // counter for the number of button presses
int puertoState = 0;         // current state of the button
int lastpuertoState = 0;     // previous state of the button
//////////toño
void setup() 
{
  Serial.begin(115200);
  initializeSPIFFS(); 
  writeFile(SPIFFS, "/SetpointCO2_alto.txt", altoDefault.c_str());
  writeFile(SPIFFS, "/SetpointCO2_bajo.txt", bajoDefault.c_str());
  yourSetpointCO2_alto = readFile(SPIFFS, "/SetpointCO2_alto.txt").toInt();
  Serial.print("*** Your SetpointCO2_alto: ");
  Serial.println(yourSetpointCO2_alto);
  yourSetpointCO2_bajo = readFile(SPIFFS, "/SetpointCO2_bajo.txt").toInt();
  Serial.print("*** Your SetpointCO2_alto: ");
  Serial.println(yourSetpointCO2_alto);
  prenderPantalla();
  pinMode(activar_transistor, OUTPUT);
  pinMode(activar_buzzer, OUTPUT);
  pinMode(activar_SyP, OUTPUT);
  pinMode(lectura_bateria, INPUT);
  pinMode(puertoPin, INPUT);//toño
  pinMode(CON_WIFI, INPUT);
  digitalWrite(CON_WIFI, LOW);
  digitalWrite(activar_buzzer, LOW);
  actBattery.attach(300, battery_read);                                              //Lee el valor de la bateria cada 5 minutos (300s)
  tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);
  svm.EnableDebugging(DEBUG);                                                        // enable debug messages
  svm.begin();   
  Serial.print(F("Driver version : "));
  Serial.println(svm.GetDriverVersion());
  
  if (svm.probe() == true)                                                           // try to detect SVM30 sensors
  Serial.println(F("SVM30 detected"));
  read_id();                                                                         // read and display the ID
  read_featureSet();                                                                 // read SGP30 feature set
  leerSensor();   //QUE LEA SENSOR PARA ENVIAR EL PRIMER VALOR CUANDO SE CONECTE A WIFI
  battery_read();
  sensorName = WiFi.macAddress();
  sensorName = RemoveCharacters(sensorName);
  nombreSensor = "CO2 ID: " + sensorName;
  Serial.print("Nombre sensor: ");
  Serial.println(nombreSensor);
  ssid = nombreSensor.c_str();
  Serial.print(ssid);
  
  String nombreSSID = readFile(SPIFFS, "/nombreSSID.txt");
  nombreSSID2 = nombreSSID.c_str();
  String passwordSSID = readFile(SPIFFS, "/passwordSSID.txt");
  passwordSSID2 = passwordSSID.c_str();

  initWiFi(nombreSSID2,passwordSSID2);
}

void loop()
{
  longPress_ShortPress();
  PUERTO();
  // To access your stored values 
  String nombreSSID = readFile(SPIFFS, "/nombreSSID.txt");
 // Serial.print("*** Your nombreSSID: ");
 // Serial.println(nombreSSID);
  nombreSSID2 = nombreSSID.c_str();

  String passwordSSID = readFile(SPIFFS, "/passwordSSID.txt");
  //Serial.print("*** Your passwordSSID: ");
  //Serial.println(passwordSSID);
  passwordSSID2 = passwordSSID.c_str();

  yourSetpointCO2_alto = readFile(SPIFFS, "/SetpointCO2_alto.txt").toInt();
  //Serial.print("*** Your SetpointCO2_alto: ");
  //Serial.println(yourSetpointCO2_alto);

  yourSetpointCO2_bajo = readFile(SPIFFS, "/SetpointCO2_bajo.txt").toInt();
  //Serial.print("*** Your SetpointCO2_bajo: ");
  //Serial.println(yourSetpointCO2_bajo);

/*
  Serial.println("{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{IP A LA QUE ESTA CONECTADO ACTUALMENTE}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Subnet Mask: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("Gateway IP: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("DNS 1: ");
  Serial.println(WiFi.dnsIP(0));
  Serial.print("DNS 2: ");
  Serial.println(WiFi.dnsIP(1));
  Serial.println("{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}");*/
  leerSensor();

 if (banderaInterrupcionWiFi == true) //( digitalRead(Con_WiFI) == HIGH) //CUANDO SELECCIONE CONECTARSE CON WIFI
  {
      banderaInterrupcionWiFi = false;

      Fijo.detach();           
      prenderPantalla();        
      banderaConectado = false;
      simbolo_wifiDESCONECTADO();
      simbolo_TX_DESCONECTADO(); //PRUEBA ALEJANDRA
      Serial.println("ENTRE A MODO WIFI AP");
      WiFi.mode(WIFI_AP);
      WiFi.softAP(ssid);
      IPAddress IP = WiFi.softAPIP();
      Serial.print("AP IP address: ");
      Serial.println(IP);
      // Send web page with input fields to client
      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html);
      });

      // Send a GET request to <ESP_IP>/get?ssid2=<inputMessage>
      server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
        String inputMessage;
        inputMessage = request->getParam("nombreSSID")->value();
        writeFile(SPIFFS, "/nombreSSID.txt", inputMessage.c_str());
        inputMessage = request->getParam("passwordSSID")->value();
        writeFile(SPIFFS, "/passwordSSID.txt", inputMessage.c_str());
        inputMessage = request->getParam("setpointCO2_alto")->value();
        if(inputMessage != ""){
          writeFile(SPIFFS, "/SetpointCO2_alto.txt", inputMessage.c_str());
        }
        inputMessage = request->getParam("setpointCO2_bajo")->value();
        if(inputMessage != ""){
          writeFile(SPIFFS, "/SetpointCO2_bajo.txt", inputMessage.c_str());
        }
        Serial.println("Se actualizaron los valores de conexion");
        request->send(200, "text/text", "Se actualizaron los valores de conexion");
      });

      server.on("/logoair", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/logoair.png", "image/png");
      });
      server.on("/logo", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/logo.png", "image/png");
      });
      server.on("/favicon", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/favicon.ico", "image/x-icon");
      });
      server.on("/estilos", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/estilos.css", "text/css");
      });

       // Receive an HTTP GET request
      server.on("/on", HTTP_GET, [] (AsyncWebServerRequest *request) {
      bandera = true;
      request->send(200, "text/plain", "ok" );
      });

      server.onNotFound(notFound);
      server.begin();        
  }

 else if(banderaMovil == true)
 {
   banderaMovil = false;
   Serial.println("++++++++++++++ENVIAR DATOS MOVIL++++++++++++++");
   enviarDatosMovil(); 
 }
   
if(bandera == true)//else if
  {
    longPress_ShortPress();
          Serial.println("Estoy dentro de bandera");
          Serial.println("Entre a boton");
    
          WiFi.softAPdisconnect();
          WiFi.disconnect();

          initWiFi(nombreSSID2,passwordSSID2); 
  }

if (banderaConectado == true && (WiFi.status() != WL_CONNECTED))
{
  longPress_ShortPress();
  simbolo_wifiDESCONECTADO();
  simbolo_TX_DESCONECTADO(); //PRUEBA ALEJANDRA
  Serial.println("Reconnecting to WiFi...");
  Serial.println("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%");
  WiFi.disconnect();
  banderaDesconectado = true;
  initWiFi(nombreSSID2,passwordSSID2); 
}

}

void read_values() 
{                                                                                     //@brief : read and display the values from the SVM30
    
    longPress_ShortPress();
    if (svm.GetValues(&v))                                                       
    Serial.print(F("temperature : "));
    Serial.println((float) v.temperature/1000);
    sensortemperatura = (int) v.temperature/1000;
    tft.setTextColor(TFT_CYAN,TFT_BLACK);
    tft.setCursor(90, 15, 4);
    tft.println("CO2");
    tft.setCursor(25, 90, 4);
    tft.println("TEMPERATURA");
    tft.setCursor(15, 116, 6);
    tft.println((int) v.temperature/1000);
    tft.setCursor(201, 116, 4);
    tft.println("C");

    Serial.print(F("Humidity : "));
    Serial.println((float) v.humidity/1000);
    sensorhumedad = (int) v.humidity/1000;
    Serial.println("");
    tft.setTextColor(TFT_CYAN,TFT_BLACK);
    tft.setCursor(25, 165, 4);
    tft.println("HUMEDAD REL.");
    tft.setCursor(15, 191, 6);
    tft.println((int) v.humidity/1000);
    tft.setCursor(194, 191, 4);
    tft.println("%");
}

void CO2() 
{
  longPress_ShortPress();
    Serial.print(F("CO2 equivalent : "));
    Serial.println((float) CO2_COMP);
    tft.setCursor(15, 40, 6);
    tft.println(CO2_COMP);//TOÑO
    tft.setCursor(170, 40, 4);
    tft.println("ppm");
    tft.setTextColor(TFT_BLUE,TFT_BLACK);
    tft.setCursor(94, 40, 6);
    tft.println("      ");
}

void CO2X() 
{
  longPress_ShortPress();
    Serial.print(F("CO2 equivalent : "));
    Serial.println((float) CO2_COMP);
    tft.setCursor(15, 40, 6);
    tft.println(CO2_COMP);//TOÑO
    tft.setCursor(170, 40, 4);
    tft.println("ppm");
}   

void CO2XX() 
{
  longPress_ShortPress();
    Serial.print(F("CO2 equivalent : "));
    Serial.println((float) CO2_COMP);
    tft.setCursor(15, 40, 6);
    tft.println(CO2_COMP);//TOÑO
    tft.setCursor(170, 40, 4);
    tft.println("ppm");
}

void read_id() 
{                                                                                    //@brief: read and display the id of the SGP30 and SHTC1 sensors
  longPress_ShortPress();
  uint16_t buf[3];                                                                   // SGP30 is 3 words, SHTC1 is 1 word
  char  id[15];
  
  if (svm.GetId(SGP30, buf))
  Serial.print(F("\nSGP30 id : "));
  sprintf(id, "%04x %04x %04x", buf[0], buf[1], buf[2]);
  Serial.println(id);

  if (svm.GetId(SHTC1, buf) == true)
  Serial.print(F("SHTC1 id : "));
  // only bit 5:0 matter (source: datasheet)
  sprintf(id, "%04x", buf[0] & 0x3f);
  Serial.println(id);
}

void read_featureSet()
{                                                                                    //@brief: read and display the product feature set of the SGP30 sensor
  longPress_ShortPress();
  char buf[2];
  
  if (svm.GetFeatureSet(buf))

  Serial.print(F("\nSGP30 product type : "));
  Serial.print((buf[0] & 0xf0), HEX);
  
  Serial.print(F(", Product version : "));
  Serial.println(buf[1], HEX);
  Serial.println();
}

void condicionCO2()
{
  longPress_ShortPress();
  if(CO2_COMP < 1000)
  {
    CO2();
  }
  else if(CO2_COMP > 1000 && CO2_COMP < 10000)
  {
   CO2X();
  }
  else
   CO2XX();
}

void battery_read()
{
  longPress_ShortPress();
  for (int i = 0; i < 20; i++)
  {
    digitalWrite(activar_transistor, HIGH);
    //delay(1000);
    sum += analogRead(lectura_bateria);
    delayMicroseconds(10);    
   }
   digitalWrite(activar_transistor,LOW); 
   //delay(1000);
   voltage = sum / (float)20;
   Serial.print("Promedio ADC 20 valores  ");
   Serial.println(voltage);
   voltage = (voltage * 0.000805);  //3.3V / 4096 (ADC 12 bits) nos da 0.00805 el valor de 1 bit.
   Serial.print("voltaje  ");
   Serial.println(voltage);
   voltage = voltage + ((voltage * 7.7) / 100); 
   Serial.print("voltaje con correccion  ");
   Serial.println(voltage);
    
   output = ((voltage - battery_min) / (battery_max - battery_min)) * 100;
   Vbateria = round(output);
   Serial.print("Vbateria: "); 
   Serial.println(Vbateria);
   ///////TOÑO 
    tft.setTextColor(TFT_BLUE,TFT_BLACK);
    tft.setCursor(15, 265, 6);
    tft.println("            ");
    if (Vbateria < 0)
   {
      Vbateria = 0;
      Serial.print("PORCENTAJE DE BATERIA: ");
      Serial.println(Vbateria);
      BATERIA_1OO();
    }
    if (Vbateria > 100)
    {
      Vbateria = 100;
     Serial.print("PORCENTAJE DE BATERIA: ");
     Serial.println(Vbateria);
     BATERIA_1OO();
     }
   else if (Vbateria < 100)
   {
     Serial.print("PORCENTAJE DE BATERIA: ");
     Serial.println(Vbateria);
     BATERIA_1OO();
   }
   sum = 0;
}

void BATERIA_1OO()  
{
  longPress_ShortPress();
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(25, 240, 4);
  tft.println("BATERIA");
  tft.setCursor(15, 265, 6);
  tft.println(Vbateria);
  tft.setCursor(95, 270, 4);
  tft.println("%");
}

void leerSensor()
{
  longPress_ShortPress();
  read_values();                                                                   //read measurement values
  CO2_COMP = v.CO2eq + COMP; 
  if(CO2_COMP < yourSetpointCO2_bajo)
  {
    digitalWrite(activar_buzzer, LOW);
    tft.setTextColor (TFT_CYAN,TFT_BLACK );
    condicionCO2(); 
  }
  else if(CO2_COMP >= yourSetpointCO2_bajo && CO2_COMP <= yourSetpointCO2_alto)
  {
    digitalWrite(activar_buzzer,LOW);
    tft.setTextColor(TFT_YELLOW,TFT_BLACK);
    condicionCO2(); 
  } 
  else if(CO2_COMP > yourSetpointCO2_alto)
  {
    digitalWrite(activar_buzzer, HIGH);
    tft.setTextColor(TFT_RED,TFT_BLACK);
    condicionCO2();  
    if((banderaStop == false) && (sensorReadingsArr[1] == "Fijo"))
    {
      banderaStop = true;
      enviarDatosco2Alto(); 
    }    
   }
}

void simbolo_wifiCONECTADO() 
{
  longPress_ShortPress();
  tft.setTextColor(TFT_GREEN, TFT_BLACK);//#define BLUE2 0x00BFFF
  tft.setCursor(167, 240, 4);
  tft.println("WiFi");
}

void simbolo_wifiDESCONECTADO() 
{
    longPress_ShortPress();
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(167, 240, 4);//240
    tft.println("WiFi");
}
///////////////////////////////////////////////////////////////////////////////////////
void simbolo_TX_CONECTADO() 
{
    longPress_ShortPress();
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(182,275, 4);//275tft.setCursor(15, 275, 4)
    tft.println("TX");
}

void simbolo_TX_DESCONECTADO()
{
    longPress_ShortPress();
    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    tft.setCursor(182,275, 4);//275tft.setCursor(15, 275, 4)
    tft.println("TX");
}
///////////////////////////////////////////////////////////////////////////////////////

void notFound(AsyncWebServerRequest *request) 
{
  longPress_ShortPress();
  request->send(404, "text/plain", "Not found");
}

String readFile(fs::FS &fs, const char * path)
{
  longPress_ShortPress();
 // Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
   // Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  file.close();
  Serial.println(fileContent);
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message)
{
  longPress_ShortPress();
 // Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

String processor(const String& var)      // Replaces placeholder with stored values
{
  longPress_ShortPress();           
  if(var == "nombreSSID"){
    return readFile(SPIFFS, "/nombreSSID.txt");
  }
  else if(var == "passwordSSID"){
    return readFile(SPIFFS, "/passwordSSID.txt");
  }
  else if(var == "SetpointCO2_alto"){
    return readFile(SPIFFS, "/SetpointCO2_alto.txt");
  }
  else if(var == "SetpointCO2_bajo"){
    return readFile(SPIFFS, "/SetpointCO2_bajo.txt");
  }
  return String();
}

String RemoveCharacters(String x)
{
  longPress_ShortPress();
int j = 2;
  for(int i = 2; i < 7; i++ )
  {
  x.remove(j,1);
 // Serial.println(x);
  j = j+2;
  }
  return x;
}

void initializeSPIFFS()             // Initialize SPIFFS
{
  longPress_ShortPress();
  #ifdef ESP32
    if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  #else
    if(!SPIFFS.begin()){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  #endif
}

String httpPOSTRequest(const char* serverName) {
  longPress_ShortPress();
  WiFiClient client;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);

      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      // Data to send with HTTP POST
     // String httpRequestData = "api_key=" + apiKey + "&field1=" + String(random(40));
      String httpRequestData = "&id=" + sensorName ;        
      // Send HTTP POST request
      int httpResponseCode = http.POST(httpRequestData);
   
  String payload = "{}"; 
  
  if (httpResponseCode == 200) {    //(httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
    simbolo_TX_DESCONECTADO(); //PRUEBA ALEJANDRA
  }
  // Free resources
  http.end();

  return payload;
}

void variablesArray()
{
  longPress_ShortPress();

 JSONVar myObject = JSON.parse(sensorReadings);
  
      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }
    
      Serial.print("JSON object = ");
      Serial.println(myObject);
    
      // myObject.keys() can be used to get an array of all the keys in the object
      JSONVar keys = myObject.keys();

      if(keys.length() == 0)
      {
        Serial.println("keys length es 0*******************************************************************************************");  
        sensorReadingsArr[0] = "";
        sensorReadingsArr[1] = "";
        sensorReadingsArr[2] = "800";
        sensorReadingsArr[3] = "500";
        sensorReadingsArr[4] = "";
        sensorReadingsArr[5] = "";
      }
      else 
      {
         //////////////////////////////////////////////////////////////////////////////////////////
         //PRUEBA DISPLAY TX ALEJANDRA
         RecibiDatoServer = true; //PRUEBA ALEJANDRA
         simbolo_TX_CONECTADO(); 
         //PRUEBA ALEJANDRA PRUEBA ALEJANDRA PRUEBA ALEJANDRA
         /////////////////////////////////////////////////////////////////////////////////////////

        
        for (int i = 0; i < keys.length(); i++) 
        {
        JSONVar value = myObject[keys[i]];
        Serial.print(keys[i]);
        Serial.print(" = ");
        Serial.println(value);
        if(i == 1)
        {
           sensorReadingsArr[i] = (value); 
        }
        else
        sensorReadingsArr[i] = double(value);
        }
      }
      Serial.print("1 = ");
      Serial.println(sensorReadingsArr[0]);
      Serial.print("2 = ");
      Serial.println(sensorReadingsArr[1]);
      Serial.print("3 = ");
      Serial.println(sensorReadingsArr[2]);
      Serial.print("4 = ");
      Serial.println(sensorReadingsArr[3]);
      Serial.print("5 = ");
      Serial.println(sensorReadingsArr[4]);
      Serial.print("6 = ");
      Serial.println(sensorReadingsArr[5]);
     
}

void guardarMaxyMin()
{
  longPress_ShortPress();
  writeFile(SPIFFS, "/SetpointCO2_alto.txt", sensorReadingsArr[2].c_str());
  writeFile(SPIFFS, "/SetpointCO2_bajo.txt", sensorReadingsArr[3].c_str());
}

void enviarDatosco2Alto()
{
  longPress_ShortPress();
     prenderPantalla();
     pantalla.attach(5, apagarPantalla);   
      if(WiFi.status()== WL_CONNECTED){
      
      WiFiClient client;
      HTTPClient http; 
       
       http.begin(client, serverName2);

      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");  
      String httpRequestData = "&id_sensor=" + sensorName + "&id_salon=" + sensorReadingsArr[0] + "&co2=" + CO2_COMP  + "&temperatura=" + sensortemperatura + "&humedad=" + sensorhumedad + "";      
      // Send HTTP POST request
      int httpResponseCode = http.POST(httpRequestData);

  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);

  if (httpResponseCode != 200) 
  {
    simbolo_TX_DESCONECTADO(); //PRUEBA ALEJANDRA
  }
  
  Serial.println("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& ");
  Serial.println("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& ");
  Serial.println("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& ");
  
    }
     
    else {
      Serial.println("WiFi Disconnected");
    }
}

void enviarDatosFijo()
{
     longPress_ShortPress();
     banderaStop = false;      //bandera para mandar los datos solo una vez cuando el co2 este alto
     prenderPantalla();
     pantalla.attach(5, apagarPantalla);   
      if(WiFi.status()== WL_CONNECTED){
      
      WiFiClient client;
      HTTPClient http; 
       
       http.begin(client, serverName2);

      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");  
      String httpRequestData = "&id_sensor=" + sensorName + "&id_salon=" + sensorReadingsArr[0] + "&co2=" + CO2_COMP  + "&temperatura=" + sensortemperatura + "&humedad=" + sensorhumedad + "";      
      // Send HTTP POST request
      int httpResponseCode = http.POST(httpRequestData);

  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);

    if (httpResponseCode != 200) 
  {
    simbolo_TX_DESCONECTADO(); //PRUEBA ALEJANDRA
  }
  
  Serial.println("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& ");
  Serial.println("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& ");
  Serial.println("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& ");
  
    }
     
    else {
      Serial.println("WiFi Disconnected");
    }
}

void enviarDatosMovil()
{

  sensorReadings = httpPOSTRequest(serverName); //ESTA ES UNA PRUEBA PARA LEER PRIMERO LOS VALORES DE LA PLATAFORMA ANTES DE ENVIARLOS....
  variablesArray(); //prueba
  guardarMaxyMin(); //prueba
  Condfijo_Movil();       //AQUI ASIGNO TICKER PARA LA CONDICION FIJA
  bandera = false; //prueba

  
  
  if(WiFi.status()== WL_CONNECTED)
  {
     WiFiClient client;
     HTTPClient http; 
     http.begin(client, serverName2);
     // Specify content-type header
     http.addHeader("Content-Type", "application/x-www-form-urlencoded");  
     String httpRequestData = "&id_sensor=" + sensorName + "&id_salon=" + sensorReadingsArr[0] + "&co2=" + CO2_COMP  + "&temperatura=" + sensortemperatura + "&humedad=" + sensorhumedad + "";      
     // Send HTTP POST request
     int httpResponseCode = http.POST(httpRequestData);
     Serial.print("HTTP Response code: ");
     Serial.println(httpResponseCode);

    if (httpResponseCode != 200) 
    {
      simbolo_TX_DESCONECTADO(); //PRUEBA ALEJANDRA
    }
     
     Serial.println("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& ");
     Serial.println("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& ");
     Serial.println("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& ");
   }
   else 
   {
      Serial.println("WiFi Disconnected");
   }
}

void initWiFi(const char* ssid,const char* password) 
{ 
  longPress_ShortPress();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  

  if (WiFi.waitForConnectResult() != WL_CONNECTED) 
  {
    if(banderaDesconectado == true)
    {
      Serial.println("NO SE PUDO CONECTAR A INTERNET con bandera desconectado = true"); 
      simbolo_wifiDESCONECTADO();
      simbolo_TX_DESCONECTADO(); //PRUEBA ALEJANDRA
      bandera = false;
    }
    else
    {
    Serial.println("NO SE PUDO CONECTAR A INTERNET"); 
    simbolo_wifiDESCONECTADO();
    simbolo_TX_DESCONECTADO(); //PRUEBA ALEJANDRA
    bandera = false;
    banderaConectado = false;
    }
    return;
  }

    //actualizarInfo.attach(600, actualizarInformacion);  //COMENTADO PARA PRUEBA
    banderaConectado = true;
    banderaDesconectado = false;
    simbolo_wifiCONECTADO();
    Serial.println("CONECTADO A INTERNET IP: ************************************************************************************************************************************** ");
    Serial.println(WiFi.localIP());

  Serial.println("Estas son las credenciales para ip fija ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Subnet Mask: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("Gateway IP: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("DNS 1: ");
  Serial.println(WiFi.dnsIP(0));
  Serial.print("DNS 2: ");
  Serial.println(WiFi.dnsIP(1));
  Serial.println("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");  

    sensorReadings = httpPOSTRequest(serverName);
    variablesArray();  
    guardarMaxyMin();
    Condfijo_Movil();       //AQUI ASIGNO TICKER PARA LA CONDICION FIJA
    bandera = false;
}

void Condfijo_Movil()
{
  longPress_ShortPress();
  if(sensorReadingsArr[1] == "Fijo") 
  {
     Serial.println("soy un prototipo fijo");
     enviarDatosFijo();   //para enviar datos en cuanto se conecte
     int tiempo = sensorReadingsArr[4].toInt(); //cambiar a entero
     tiempo = tiempo*60;
     Fijo.attach(tiempo, enviarDatosFijo);   
  } 
  else if (sensorReadingsArr[1] == "Movil")
  {
    prenderPantalla();  
  }
  else
  {
    prenderPantalla();
  }
}

void actualizarInformacion()
{
    longPress_ShortPress();
    sensorReadings = httpPOSTRequest(serverName);
    variablesArray();  
    guardarMaxyMin();
    Condfijo_Movil();       //AQUI ASIGNO TICKER PARA LA CONDICION FIJA
}

void prenderPantalla()
{
  longPress_ShortPress();
  digitalWrite(activar_SyP, HIGH);
}

void apagarPantalla()
{
  longPress_ShortPress();
  digitalWrite(activar_SyP, LOW);
  pantalla.detach();
}

////////////////TOÑO
void PUERTO() 
{
  longPress_ShortPress();
  puertoState = digitalRead(puertoPin);
  if(puertoState != lastpuertoState) 
  {
   if(puertoState == HIGH) 
    {
    puertoPushCounter++;
    } 
  }
   lastpuertoState = puertoState;
   if(puertoPushCounter == 1) 
    {
    actBattery.detach();
    tft.setTextColor(TFT_BLUE,TFT_BLACK);
    tft.setCursor(15, 265, 6);
    tft.println("            ");
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(25, 240, 4);
    tft.println("BATERIA");
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(15, 275, 4);
    tft.println("CARGANDO");
    }     
}

void longPress_ShortPress()
{
  // read the state of the switch/button:
  currentState = digitalRead(CON_WIFI);

  if(lastState == LOW && currentState == HIGH) {        // button is pressed
    pressedTime = millis();
    isPressing = true;
    isLongDetected = false;
  } else if(lastState == HIGH && currentState == LOW) { // button is released
    isPressing = false;
    releasedTime = millis();

    long pressDuration = releasedTime - pressedTime;

    if( pressDuration < SHORT_PRESS_TIME )
    {
      Serial.println("++++++++++++++++++++++++++++A short press is detected+++++++++++++++++++++++++++++++++++++");
      if((sensorReadingsArr[1] == "Fijo") || (sensorReadingsArr[1] == ""))
      {
        banderaInterrupcionWiFi = true;
      }
      else if((sensorReadingsArr[1] == "Movil"))
      {
        Serial.print("enviar datos movil");
        banderaMovil = true;
      }
      else
      {
        banderaInterrupcionWiFi = true; 
      }
     }
  }
  if(isPressing == true && isLongDetected == false) 
  {
    long pressDuration = millis() - pressedTime;

    if( pressDuration > LONG_PRESS_TIME ) 
    {
      Serial.println("+++++++++++++++++++++++++++++A long press is detected++++++++++++++++++++++++++++++++++++++");
      isLongDetected = true;
        if(banderaMovil == true)
        {
          banderaMovil = false;
          banderaInterrupcionWiFi = true; 
        }
        else
        {
          banderaInterrupcionWiFi = true;
        }
     }
   }

  // save the the last state
  lastState = currentState;
}
