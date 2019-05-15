#include <ESP8266WiFi.h>
#include <dht.h>

#define	SSID		"3Sticks"
#define	PASSWORD	""

/*
 *	TODO: GPIO pin configuration
 *
 *		1.	define GPIO pin no. of nodeMCU  for  DHT11 temperature sensor signal pin
 *		2.	define GPIO pin no. of nodeMCU  for  PIN_LED   switch signal pin, ( D0 = 16)
 *			refer to breadboard schematic
 */

#define PIN_DHT D5 
#define PIN_LED D0

const char* ssid = "3Sticks";
const char* password = "";

unsigned char status_led=0;
WiFiServer server(80);

String web;
dht    DHT;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  server.begin();
  Serial.println("Server started");
  Serial.println(WiFi.localIP());
}

void loop() {

	readDHT11();	/*	read humidity and temperature */

  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  
  Serial.println("new client");
  while(!client.available())
  {
    delay(1);
  }
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();
  if (req.indexOf("/ledoff") != -1)
  {
    status_led=0;   
    digitalWrite(PIN_LED,LOW);
    Serial.println("PIN_LED OFF");
  }
  else if(req.indexOf("/ledon") != -1)
  {
    status_led=1;
    digitalWrite(PIN_LED,HIGH);
    Serial.println("PIN_LED ON");
  }
  
  
  buildWebPage();
  client.print(web);
  
}  


void buildWebPage()
{
  web = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
  web += "<html>\r\n";
  web += "<head> <meta http-equiv=\"refresh\" content=\"1\" /> </head>" ;
  web += "<body>\r\n";
  web += "<h1>Switch LED, Temperature and Humidity Control & Status Dashboard </h1>\r\n";
  web += "<p>\r\n";

  web += "Current Temperature is : " ;
  web += String(DHT.temperature);
  web += "<p>\r\n";  
  web += "Current Humidity    is : " ;
  web += String(DHT.humidity   );
  web += "<p>\r\n";
  
  /*  PIN_LED status  */
  if(status_led==1)
      web += "PIN_LED On\r\n";
  else
      web += "PIN_LED Off\r\n";
      
  web += "</p>\r\n";
  web += "</p>\r\n";

  /*  Button PIN_LED On   */
  web += "<a href=\"/ledon\">\r\n";
  web += "<button>PIN_LED On</button >\r\n";
  web += "</a>\r\n";
  web += "</p>\r\n";

  /*  Button PIN_LED OFF  */
  web += "<a href=\"/ledoff\">\r\n";
  web += "<button>PIN_LED Off</button >\r\n";
  web += "</a>\r\n";
  
  web += "</body>\r\n";
  web += "</html>\r\n";
  

}


/*
	Read humidity and temperature from DHT11 sensor
**/

void readDHT11(){

  DHT.read11(PIN_DHT);			/*	read value */
	
  Serial.print("humidity = ");	/*	humidity	*/
  Serial.print(DHT.humidity);
  Serial.print("%  ");
    
	Serial.print("temperature = ");	/*	temperature	*/
  Serial.print(DHT.temperature); 
  Serial.print("'C  ");

  Serial.println("")  ;    

	delay(1000);	/*	read once every 2 seconds due to sensor tolerance	*/

  }
