// This #include statement was automatically added by the Particle IDE.
#include "lib1.h"

// This #include statement was automatically added by the Particle IDE.
#include "SparkFunPhant/SparkFunPhant.h"

// This #include statement was automatically added by the Particle IDE.
//#include "SparkFunPhant/SparkFunPhant.h"


PRODUCT_ID(2587); //
PRODUCT_VERSION(1);


//Global Variables
String device_uid = ""; // photon hard coded device id, UID
char input_char = '\0';
String phsensorstring = "";   // electrical conducitivity sensor output string
int time_counter = 290;         // counter to allow periodic posting to phant based on time interval

////////////PHANT STUFF//////////////////////////////////////////////////////////////////
const char server[] = "ec2-52-40-13-117.us-west-2.compute.amazonaws.com";
const char path[] = "api/metrics/aqua"; 
const char port[] = "8080"; 
PhantRest phant(server, path, port);
/////////////////////////////////////////////////////////////////////////////////////////


void setup(){                                                                
     Serial.begin(9600);   
     Serial1.begin(9600, SERIAL_8N1);
     phsensorstring.reserve(30);
     delay(3000);  // allow time for sensor to boot up
     // sensor always seems to puke an error on first command after flash so... do one and throw it away
     Serial1.write('i');
     Serial1.write('\r');
     delay(1000);
     // reads string out of buffer to clear buffer, does nothing with string
     while(Serial1.available()) {
        Serial1.readStringUntil('\r');
    }
 { /// get deviceID. code block isolated in brackets
      device_uid=System.deviceID();
      Serial.print("Deviceid: ");
      Serial.println(device_uid);
    }

}



void loop(){
    // read everything from pc serial terminal, send to serial1
    while(Serial.available()) {
        input_char = (char)Serial.read();
        Serial.write(input_char);  // have to force echo it back
        // forward all characters EXCEPT endline.
        if (input_char != '\n') {
           Serial1.write(input_char);
        } 
    }
    delay(1000);
    // read everything from serial1 (sensor board) and send directly, char for char, to serial (pc terminal)
    while(Serial1.available()) {
        Serial.println("");
        phsensorstring = Serial1.readStringUntil('\r');
        Serial.print(phsensorstring); 
        Serial.write('\n');
        Serial.write('\r');
    }

   // force a reading every 5 minutes (300 seconds, main loop has 1 sec delay each pass) for phant post
   if (time_counter == 300 ) {
       Serial.println("Taking reading to post to phant");
       Serial1.write('r');  // send r command to request reading
       Serial1.write('\r'); // send carriage return character to end the command
       delay(1000);         // delay 1 sec to allow sensor time to take reading
       while(Serial1.available()) {
          Serial.println("");
          phsensorstring = Serial1.readStringUntil('\r');
          Serial.print(phsensorstring); 
          Serial.write('\n');
          Serial.write('\r');
        }
       postToPhant();
       time_counter = 0; // set time back to 0
   }
   time_counter++;
}

int postToPhant()//sends data to data.sparkfun.com
{

     phant.add("ph", phsensorstring);
     phant.add("deviceid", device_uid);
     
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