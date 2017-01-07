// This #include statement was automatically added by the Particle IDE.
#include "lib1.h"

// This #include statement was automatically added by the Particle IDE.
#include "SparkFunPhant/SparkFunPhant.h"

// This #include statement was automatically added by the Particle IDE.
//#include "SparkFunPhant/SparkFunPhant.h"

PRODUCT_ID(2595); //cahngeme 
PRODUCT_VERSION(1);

//Global Variables
String device_uid = ""; // photon hard coded device id, UID
char input_char = '\0';
String ecsensorstring = "";   // electrical conducitivity sensor output string
int second_count = 295;         // counter to allow periodic posting to phant based on time interval
int success = 0;                // flag variable for initialization loops (read mode, response mode)
int failure_count = 0;          // don't report error unless it happens a few times
String device_name = "";

// variables used for parsing EC reading
String ec="";
String tds="";
String sal="";
String sg= "";
String curr_string="";
int nextcomma=-1;
int prevcomma=0;
int lastcomma=0;

///////////PHANT STUFF//////////////////////////////////////////////////////////////////
const char server[] = "ec2-52-40-13-117.us-west-2.compute.amazonaws.com";
const char path[] = "api/metrics/aqua"; 
const char port[] = "8080"; 
PhantRest phant(server, path, port);
/////////////////////////////////////////////////////////////////////////////////////////



void setup(){                                                                
     Serial.begin(9600);                                                     
     Serial1.begin(9600, SERIAL_8N1);
     ecsensorstring.reserve(30);
     ec.reserve(20);
     tds.reserve(20);
     sal.reserve(20);
     sg.reserve(20);
     curr_string.reserve(20);
     device_name.reserve(20);

     delay(3000);  // allow time for sensor to boot up
     // sensor always seems to puke an error on first command after flash so... do one and throw it away
     Serial1.write('i');
     Serial1.write('\r');
     delay(1000);
     // reads string out of buffer to clear buffer, does nothing with string
     while(Serial1.available()) {
        Serial1.readStringUntil('\r');
     }
    
     Serial.println("Initializing sensor configs");
     

//    
    { // set device name. code block isolated in brackets
     Serial1.write("name,?");
     Serial1.write('\r');
     delay(200);
     while(Serial1.available()) {
        device_name=Serial1.readStringUntil('\r');
     }
     device_name=device_name.substring(6);
     Serial.print("Device name: ");
     Serial.println(device_name);
    }

    delay(1000);
    // empty buffer and clear our sensorstring before starting main program
    while(Serial1.available()) {
        Serial1.readStringUntil('\r');
    }
    ecsensorstring="";
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
    second_count++;
    // read everything from serial1 (sensor board) and send directly, char for char, to serial (pc terminal)
    while(Serial1.available()) {
        Serial.println("");
        ecsensorstring = Serial1.readStringUntil('\r');
        Serial.print(ecsensorstring); 
        Serial.write('\n');
        Serial.write('\r');
    }
   
   // force a reading every 5 minutes (300 seconds, main loop has 1 sec delay each pass) for phant post
   if (second_count == 300 ) {
       Serial.println("Taking reading to post to phant");
       Serial1.write('r');  // send r command to request reading
       Serial1.write('\r'); // send carriage return character to end the command
       delay(1000);         // delay 1 sec to allow sensor time to take reading
       while(Serial1.available()) {
          Serial.println("");
          ecsensorstring = Serial1.readStringUntil('\r');
          Serial.print(ecsensorstring); 
          Serial.write('\n');
          Serial.write('\r');
        }
        
       //This section parses the sensor string into the 4 readings
       // code block isolated in brackets
       {
       lastcomma=ecsensorstring.lastIndexOf(',');

       int entry_number=0;
       while (nextcomma < lastcomma ) {
          nextcomma=ecsensorstring.indexOf(',',prevcomma);
          curr_string=ecsensorstring.substring(prevcomma, nextcomma);
          if (entry_number == 0) {
             ec=curr_string;
             Serial.print("ec = ");
             Serial.println(ec);
          }
          if (entry_number == 1) {
             tds=curr_string;
             Serial.print("tds = ");
             Serial.println(tds);
          }
          if (entry_number == 2) {
             sal=curr_string;
             Serial.print("sal = ");
             Serial.println(sal);
          }
          prevcomma=nextcomma+1;
          entry_number++;
       }
       // last substring is sg
       lastcomma++;
       sg=ecsensorstring.substring(lastcomma);
       Serial.print("sg = ");
       Serial.println(sg);
    
        // reset all the kludgey flags
       nextcomma=0;
       prevcomma=0;
       ecsensorstring="";
       curr_string="";
       }  // end sensor string parsing code block
       
       postToPhant();
       second_count = 0; // set time back to 0
       Serial.println("5 min to next reading...");
   }
   { /// get deviceID. code block isolated in brackets
      device_uid=System.deviceID();
      Serial.print("Deviceid: ");
      Serial.println(device_uid);
    }
}

int postToPhant()//sends data to data.sparkfun.com
{
     phant.add("deviceid", device_uid);
     phant.add("ec", ec);
     phant.add("tds",tds);
     phant.add("sal",sal);
     phant.add("sg",sg);
    // phant.add("device_name",device_name);

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
             //Serial.print(c); // comment out to reduce screen chatter. 7/8/2016. cstamper
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