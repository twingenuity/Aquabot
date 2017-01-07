// This #include statement was automatically added by the Particle IDE.
#include "OneWire/OneWire.h"

// This #include statement was automatically added by the Particle IDE.
#include "lib1.h"

// This #include statement was automatically added by the Particle IDE.
//#include "OneWire/OneWire.h"

// This #include statement was automatically added by the Particle IDE.
#include "spark-dallas-temperature/spark-dallas-temperature.h"

// This #include statement was automatically added by the Particle IDE.
#include "SparkFunPhant/SparkFunPhant.h"


// -----------------
// Read temperature
// -----------------

// Data wire is plugged into port D4 on Photon
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(D4 );
PRODUCT_ID(2599);  
PRODUCT_VERSION(1); 
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature dallas(&oneWire);
String device_uid = ""; // photon hard coded device id, UID
//Global Variables for temp sensor
double temperature = 0.0;
double temperatureF = 0.0;
//double tempc;
//double tempf;
// global vars for d.o. sensor	
char input_char = '\0';
char output_char = '\0';
String sensorstring = "";
int delay_counter = 0;

///////////PHANT STUFF//////////////////////////////////////////////////////////////////
const char server[] = "ec2-52-40-13-117.us-west-2.compute.amazonaws.com";
const char path[] = "api/metrics/aqua"; 
const char port[] = "8080"; 
PhantRest phant(server, path, port);
/////////////////////////////////////////////////////////////////////////////////////////





void setup(){                                                                
     //Serial.begin(9600);                                                     
     Serial1.begin(9600, SERIAL_8N1);                                                    
     delay(3000);  // allow time for sensor to boot up
     sensorstring.reserve(30);
	 
	Serial.begin(9600); 
	// Register a Particle variable here
	Particle.variable("temperature", &temperature, DOUBLE);
	Particle.variable("temperatureF", &temperatureF, DOUBLE);

	// setup the library
	dallas.begin();

    //Begin posting data immediately instead of waiting for a key press.
	
	// send command to d.o. sensor to put it into reading-on-demand mode (instead of continuous readings every second)
	Serial1.write("C,0"); 
	Serial1.write('\n');   
	 
     }

void loop(){
    { /// get deviceID. code block isolated in brackets
      device_uid=System.deviceID();
      Serial.print("Deviceid: ");
      Serial.println(device_uid);
    }
    // read everything from pc serial terminal, send to serial1 (d.o. sensor). This entire section is only for entering commands
    // at serial terminal, send to d.o. sensor and get response. not for getting automated readings
    if (Serial.available()) {
       while(Serial.available()) {
           input_char = (char)Serial.read();
           // forward all characters EXCEPT endline.
           if (input_char != '\n')
              Serial1.write(input_char);
       }
       
       delay(1000); // give second for d.o. sensor to get command and send back response
       // read everything from serial1 (d.o. sensor board) and send directly, char for char, to serial (pc terminal)
       while(Serial1.available())
           sensorstring = Serial1.readStringUntil('\r');
       
       Serial.println(sensorstring); 
    }
    

       //code to ask d.o. sensor for a reading
       Serial1.write('R');
       Serial1.write('\n');
       delay(1200); //according to documentation, d.o. circuit returns a reading one second after command, so give it 1.2 seconds

       while(Serial1.available())
         sensorstring = Serial1.readStringUntil('\r');
       Serial.println(sensorstring); 
    
	   // Request temperature conversion
       dallas.requestTemperatures();

	   // get the temperature in Celcius
	   float tempc = dallas.getTempCByIndex(0);
	   // convert to double
	   temperature = (double)tempc;
       Serial.println(temperature);
       //Serial.write(tempc);
	   // convert to Fahrenheit
	   float tempf = DallasTemperature::toFahrenheit( tempc );
	   // convert to double
	   temperatureF = (double)tempf;
	   Serial.println(temperatureF);
	   delay(5000);

    if (delay_counter == 50) {
	   postToPhant();
	   delay_counter = 0;
    }
    delay_counter++;
	   //printInfo();
	   //delay(500);

}



//---------------------------------------------------------------
int postToPhant()//sends datat to data.sparkfun.com
{

     phant.add("doxygen", sensorstring);
     phant.add("deviceid", device_uid);
     phant.add("tempc", temperature);
     phant.add("tempf", temperatureF);
	 
	 
     TCPClient client;
     char response[512];
     int i = 0;
     int retVal = 0;

     if (client.connect(server, 8080))
     {
         Serial.println("Posting!");
         client.print(phant.post());
         delay(1000);
         while (client.available())
         {
             char c = client.read();
             Serial.print(c);
             if (i < 512)
                 response[i++] = c;
         }
         if (strstr(response, "200 OK"))
         {
             Serial.println("Post success!");
             retVal = 1;
         }
         else if (strstr(response, "400 Bad Request"))
         {
             Serial.println("Bad request");
             retVal = -1;
         }
         else
         {
             retVal = -2;
         }
     }
     else
     {
         Serial.println("connection failed");
         retVal = -3;
     }
     client.stop();
     return retVal;

}
