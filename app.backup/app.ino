/*
 * Alexa - Heroku - NodeMCU  voice controlled device
 * git clone https://github.com/juneskw/devapp-esp8266
 *
 *  
 */

#include <WebSocketsClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h> 
#include <Hash.h>
#include <dht.h>


/*
 *	TODO: Network and Web service configuration 
 *
 *		1.	define SSID, PASSWORD for wifi Access Point
 *		2.	define HOST for heroku application address.
 */
 
#define	SSID		"3Sticks"
#define	PASSWORD	""
#define	HOST		"webapp-esp8266.herokuapp.com"


/*
 *	TODO: GPIO pin configuration
 *
 *		1.	define GPIO pin no. of nodeMCU  for  DHT11 temperature sensor signal pin
 *		2.	define GPIO pin no. of nodeMCU  for  LED   switch signal pin, ( D0 = 16)
 *			refer to breadboard schematic
 */

#define PIN_DHT 14 
#define PIN_LED 16


/*
 *	TODO:	HTTP server configuration  :   'ws://xxx.xxx.xxx.xxx:8080'
 *
 *		1.	define HTTP service port no
 *		2.	define HTTP service directory
 */

#define PATH	"/ws" 
#define	PORT 	80



dht DHT							;

ESP8266WiFiMulti 	WiFiMulti	;
WebSocketsClient 	webSocket	;
DynamicJsonBuffer 	jsonBuffer	; 

String pstrTriggerName	=""		;
String pstrTriggerValue	=""		;
String pstrCurState		=""		;

int nPingCount		= 	0		;
int bTriggerEnable 	=	0		;

int	count =0;	/* connection count */

void webSocketEvent(WStype_t wstype, uint8_t * payload, size_t length) { 

switch(wstype) {

	case WStype_DISCONNECTED:
		Serial.print  ("event> WebSocket is disconnected ! Connecting ...");
		Serial.println(count);
		
		webSocket.begin(HOST, PORT, PATH);	/*	try to connect again	*/
		webSocket.onEvent(webSocketEvent);
		break;

	case WStype_CONNECTED:
		Serial.println("event> WebSocket is... connected !");
		count =0;
	  
		webSocket.sendTXT("Connected");		/*	send message to server */
		break;

	case WStype_TEXT:
		Serial.println("event> Received TEXT type web socket");

		doWebSocketRequest((char*)payload);
		break;
		
	case WStype_BIN:

		hexdump(payload, length);
		Serial.println("event> Received BIN  type web socket");

		webSocket.sendBIN(payload, length);	/*	send back to server	? */	/*	delete ?  -skw	*/
		break;
}

}


/*
	Arduino Sketch setup()
**/
void setup() {
    
	Serial.begin(115200);
    Serial.setDebugOutput(true);
    
    pinMode(PIN_LED, OUTPUT);
    
	for(uint8_t t = 4; t > 0; t--) {
		delay(1000);
	}
	
	Serial.println();
	
	Serial.print("setup> WiFi connecting. Please wait...");
    
    WiFiMulti.addAP(SSID, PASSWORD);

    //WiFi.disconnect();
    while(WiFiMulti.run() != WL_CONNECTED) {
      Serial.print(".");
        delay(100);
    }
    Serial.println("Connected !");

    webSocket.begin(HOST, PORT, PATH);	/*	websocket connection start		*/
    webSocket.onEvent(webSocketEvent);	/*	websocket event handler setup	*/

}

/*
	Arduino Sketch loop()
**/

void loop() {
    
	readDHT11();	/*	read humidity and temperature */
	
	webSocket.loop();

	/*
		if user have set trigger
	*/
	if ( bTriggerEnable==1 ){
		setTrigger( pstrTriggerName, pstrTriggerValue );
	}

	/*
		A ping is required to keep connection betw. Heroku and ESP at least every 40 seconds.
	*/
    delay(2000);
	if ( nPingCount > 20 ) {
		nPingCount = 0;		/*	count up reset	*/
		
		webSocket.sendTXT("\"heartbeat\":\"keepalive\"");	/*	send ping */
	}
	else {
		nPingCount += 1;	/*	count up pings	*/
	}

}

void doWebSocketRequest( String payload ){

	String jsonResponse = "{\"version\": \"1.0\",\"sessionAttributes\": {},\"response\": {\"outputSpeech\": {\"type\": \"PlainText\",\"text\": \"<text>\"},\"shouldEndSession\": true}}";
	/*
		The response message format in json:
		{ 
			"version": "1.0",
			"sessionAttributes": {},
			"response": {
				"outputSpeech": {
					"type": "PlainText",
					"text": "<text>"		// <<<<<<<-  "<text>" is replaced by jsonResponse.replace()
				},
				"shouldEndSession": true
			}
		}
		
		The payload  message format in json:
		{
			????????
		}
		
	*/

	JsonObject& root 	= jsonBuffer.parseObject(payload);
	
	String 		query 	= root["query"];
	
	String 		message	=	"";
	
	Serial.println(payload);	/*	see what payload is */
            
	if(query == "cmd"){
		Serial.println("Query is command !");
	
		String value = root["value"];	/*	*/

		if       (value=="on" ){		/*	command + "on"	*/
			digitalWrite(PIN_LED, HIGH);	/*	LED ON	*/
			message = "{\"state\":\"ON\"}";
			pstrCurState = "ON";

		}else if (value=="off"){		/*	command + "off"	*/
			digitalWrite(PIN_LED, LOW );	/*	LED OFF	*/
			message = "{\"state\":\"OFF\"}";
			pstrCurState = "OFF";

		}else if (value=="deactivate"){
			bTriggerEnable = 0;			/*	deactivate trigger	*/

										/*	  activate trigger	*/
		}else{
			String object = root["object"];

			//set trigger for temperature or humidity
			pstrTriggerName 	= object;
			pstrTriggerValue 	= value	;
			bTriggerEnable 		= 1;
		}
	
		jsonResponse.replace("<text>", "It is done");		/*	after command is accepted, alexa will say 'it is done' */
		  
	}else if(query == "?"){
		Serial.println("Query is inquiry!");

		String value = root["value"];	/*	*/		
//		Serial.print("value:");Serial.print(value);
		
		int state = digitalRead(PIN_LED);	/*	state = ON or OFF	*/	/*  why read this for ??? -skw*/

		if      (value=="switch"  ){
			if(pstrCurState=="ON"){
				message = "{\"state\":\"ON\"}";
			}else{
				message = "{\"state\":\"OFF\"}";
			}
			/*?? do something with message ?? */
			
		}else if(value=="humidity"){
			Serial.println("target> response of humidity"   );
			jsonResponse.replace("<text>", "current humidity is " + String(DHT.humidity) + " percent");
		  
		}else if(value=="temperature"){  
			Serial.println("target> response of temperature");
			jsonResponse.replace("<text>", "current temperature is " + String( C2F(DHT.temperature) ) + " fahrenheit");
		}
	}else{
		Serial.println("target> command is not recognized");
	}
	
	//jsonResponse.replace("<text>", "Garage door " + instance + " is " + message );
	Serial.print("target>sending response back is : ");
	Serial.println(jsonResponse);

	webSocket.sendTXT(jsonResponse);		/*	send and send again at below ???  -skw */
	
	if(query == "cmd" || query == "?")
	{
		webSocket.sendTXT(jsonResponse);
	}
	
}

void setTrigger(String obj, String val){
	Serial.print("Trigger is set for ");
	Serial.print(val.toFloat());
	Serial.print(" ");
	Serial.print(pstrTriggerName);
	Serial.println("");
  
	if(String("fahrenheit") == obj){
		if(C2F(DHT.temperature)>=val.toFloat()){
			Serial.println("Fahrenheit trigger on!");
			digitalWrite(PIN_LED, HIGH);
		}else{
			digitalWrite(PIN_LED, LOW);
		}
		
	}else if(String("celsius") == obj){
		//C2F(DHT.temperature)
        if(DHT.temperature>=val.toFloat()){
			Serial.println("Celcius trigger on!");
			digitalWrite(PIN_LED, HIGH);
		}else{
			digitalWrite(PIN_LED, LOW);
		}
	}else{
		//DHT.humidity
		if(DHT.humidity>=val.toFloat()){
			Serial.println("Humidity trigger on!");
			digitalWrite(PIN_LED, HIGH);
		}else{
			digitalWrite(PIN_LED, LOW);
		}
	}
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
    
	Serial.print( C2F( DHT.temperature ) ); 	/*	'C -> 'F	*/
    Serial.println("'F  ");
	
	delay(1000);	/*	read once every 2 seconds due to sensor tolerance	*/

  }

/*
	convet 'C -> 'F
**/  
  
double C2F(double celsius){return celsius * 9 / 5 + 32;}
