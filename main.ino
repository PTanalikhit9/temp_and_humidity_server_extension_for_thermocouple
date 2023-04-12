
#include <SPI.h>
#include <SD.h>
#include <DHT.h>
#include <TimeLib.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include "mbed.h" //added
#include "MAX31856.h" //added

#define TCP_FLAGS_FIN_V 1    //as declared in net.h
#define TCP_FLAGS_ACK_V 0x10 //as declared in net.h
#define MAX_BUFF       128
#define F_PRECISION      2
#define MISO            50
#define MOSI            51
#define SCK             52
#define CS_SD            4
#define CS_Ether        10
#define SENSOR_NUM       2      
#define TEMP_LIMIT      40      // Unit: deg Celcius
#define TIME_ZONE 9  // GMT+(TIME_ZONE)
#define MEASURE_INTERVAL 5000

void sendNTPpacket(const char * address);
void getCurrentTime();
const char* formatString(char *buf_tmp, const char *fmt, ...);
void MeasureHnT();
void doSomething(int i);

const int AM2302_PIN[]={5,6,7,8,9};     // The program measures data from first SENSOR_NUM sensors connected to this pin numbers.
char buf[MAX_BUFF];
char file_name[100];

DHT dht[SENSOR_NUM];
File myFile;

float h[SENSOR_NUM];
float t[SENSOR_NUM];
int measure_timer=0;
unsigned long epoch_base=0, epoch=0, prev_millis=0;

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 0, 19);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

unsigned int localPort = 8888;       // local port to listen for UDP packets
const char timeServer[] = "time.nist.gov"; // time.nist.gov NTP server
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
EthernetUDP Udp;

void(* resetFunc) (void) = 0;

void setup() 
{
  Serial.begin(9600);
  while (!Serial) 
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  // start Ethernet and UDP
  Ethernet.begin(mac, ip);
  
  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    delay(500);
    resetFunc();
  } else if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
    delay(500);
    resetFunc();
  }
  
  //added
  // Add your code snippet here
  // Hardware serial port over USB micro
  Serial serial(USBTX, USBRX); // this could be modified as the system use the ethernet module ****** (before run)

  //SPI spi(SPIO MOSI,SPIO MISO,SPIO SCK);
  SPI spi(P2_1, P2_2, P2_0); //change pin (before run)
  //----------------------------------------------------------

  //Thermocouples
  MAX31856 Thermocouple(spi, P2_3); //(before run)
  Thermocouple.setThermocoupleType(MAX31856_TCTYPE_T);
  //added
  
  Serial.println(Ethernet.localIP());
  Serial.println(Ethernet.gatewayIP());
      
  Serial.print("Initializing SD card...");
  if (!SD.begin(CS_SD)) {
    Serial.println("initialization failed!");
    delay(500);
    resetFunc();
  }
  Serial.println("initialization done.");

  Udp.begin(localPort);
  
  getCurrentTime();
  epoch_base = epoch - millis()/1000;
  
  
  int Y=year(epoch), M=month(epoch), D=day(epoch), h=hour(epoch), m=minute(epoch), s=second(epoch);
  char file_date[9]="";
  char file_time[7]="";
  file_date[0]=Y/1000 + '0';
  file_date[1]=(Y%1000)/100 + '0';
  file_date[2]=(Y%100)/10 + '0';
  file_date[3]=(Y%10) + '0';
  file_date[4]=M/10 + '0';
  file_date[5]=M%10 + '0';
  file_date[6]=D/10 + '0';
  file_date[7]=D%10 + '0';
  file_date[8]='\0';

  file_time[0]=h/10 + '0';
  file_time[1]=h%10 + '0';
  file_time[2]=m/10 + '0';
  file_time[3]=m%10 + '0';
  file_time[4]=s/10 + '0';
  file_time[5]=s%10 + '0';
  file_time[6]='\0';
  strcat(file_name, file_date);
  strcat(file_name, "/L_");
  strcat(file_name, file_time);
  strcat(file_name, ".csv");
  if(!SD.exists(file_date))
  {
    SD.mkdir(file_date);
  }
  
  Serial.println((String) "File name: "+file_name);
  
  myFile = SD.open(file_name, FILE_WRITE);
  if (!myFile) {
    // if the file didn't open, print an error:
    Serial.println("error opening log file");
    return;
  }
//  for(int i=0 ;i<SENSOR_NUM; i++)
//    myFile.print(formatString(buf, ",Sensor %d,", i+1));
//  myFile.println("");
  
  myFile.print("Time,");
  for(int i=0; i<SENSOR_NUM; i++)
    myFile.print(formatString(buf, "Humidity %d(%),Temperature %d(deg C),", i+1));
    
  myFile.println("");
  myFile.close();

  for(int i=0 ;i<SENSOR_NUM; i++)
    dht[i].setup(AM2302_PIN[i]);
  
  //add the code to plot graph here
  
  prev_millis=millis();
}


void loop() 
{
  if(millis() >= 21600000)  // The program restarts after 6 hours of running.
  {
    resetFunc();
  }
  delay(1);
  epoch = epoch_base + millis()/1000;
  measure_timer++;
  if(millis()-prev_millis>=MEASURE_INTERVAL)
  {
    prev_millis=millis();
    MeasureHnT();
    for(int i=0; i<SENSOR_NUM; i++)
    {
      if(t[i]>=TEMP_LIMIT)
      {
        doSomething(i);
      }
    }
    myFile = SD.open(file_name, FILE_WRITE);
    if (myFile) {
      myFile.print(formatString(buf, "%d-%d-%d %d:%d:%d,", year(epoch), month(epoch), day(epoch), hour(epoch), minute(epoch), second(epoch)));
      //Serial.print("DATA,");
      Serial.print(buf);
      for(int i=0; i<SENSOR_NUM; i++)
      {
        myFile.print(formatString(buf, "%f,%f,", h[i], t[i])); 
        Serial.print(buf);
      }
      myFile.println("");
      Serial.println("");
      myFile.close();      // close
    }
    //Serial.println("SAVEWORKBOOK");
    measure_timer=0;
  }

  // do the Ethernet thing
  EthernetClient client = server.available();
  if(client) 
  {
    int header_num=0;
    char data[20];
    Serial.println("new client");
    // an HTTP request ends with a blank line
    bool currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        //Serial.write(c);
        if(header_num<20-1) data[header_num++]=c;
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the HTTP request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          Serial.print("\nData: ");
          Serial.println(data);
          Serial.println();
          if(strncmp("GET / ", data, 6) == 0)
          {
            // send a standard HTTP response header
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");  // the connection will be closed after completion of the response
            client.println("Refresh: 5");  // refresh the page automatically every 5 sec
            client.println();
            client.println("<!DOCTYPE HTML>");
            client.println("<html>");
            client.println("<head><title>"
                           "Success"
                           "</title>"
                           "<link rel=\"shortcut icon\" href=\"#\"></head>");
            client.println(formatString(buf,"Time : %d/%d/%d %d:%d:%d <br>",year(epoch), month(epoch), day(epoch), hour(epoch), minute(epoch), second(epoch)));
            for(int i=0; i<SENSOR_NUM; i++)
            {
              client.println(formatString(buf, "<br>Sensor %d<br>"
                                             "Temperature: %f deg C, Humidity: %f %%<br>", i+1, t[i], h[i]));
            }
            for(int i=0; i<SENSOR_NUM; i++)
            {
              if(t[i]>=TEMP_LIMIT)
              {
                client.println(formatString(buf, "<br><strong>!!! SENSOR %d DETECTED HIGH TEMPERATURE !!!</strong>", i+1));
              }
              
            }
            client.println(formatString(buf, "<br>Sensor %d<br>"
                                             "Temperature_HJ: %f deg C, Temperature_HJ: %f deg C %%<br>", temperature_TC, temperature_CJ));
            if(temperature_TC>=TEMP_LIMIT)//added
            {
              client.println(formatString(buf, "<br><strong>!!! SENSOR %d DETECTED HIGH TEMPERATURE !!!</strong>"));
            }//added
            break;
          }
          else
          {
            break;
          }
          
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
  
}


// send an NTP request to the time server at the given address
void sendNTPpacket(const char * address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); // NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

void getCurrentTime()
{
  sendNTPpacket(timeServer);
  // wait to see if a reply is available
  delay(1000);
  if (Udp.parsePacket()) {
    // We've received a packet, read the data from it
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    // the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, extract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;

    // now convert NTP time into everyday time:
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years, consider timezone:
    epoch = secsSince1900 - seventyYears + (TIME_ZONE) * 3600UL;
    // print Unix time:
  }
}

/**
 * --------------------------------------------------------------
 * Perform simple printing of formatted data
 * Supported conversion specifiers: 
 *      d, i     signed int
 *      u        unsigned int
 *      ld, li   signed long
 *      lu       unsigned long
 *      f        double
 *      c        char
 *      s        string
 *      %        '%'
 * Usage: %[conversion specifier]
 * Note: This function does not support these format specifiers: 
 *      [flag][min width][precision][length modifier]
 * --------------------------------------------------------------
 */

const char* formatString(char *buf_tmp, const char *fmt, ...) {
  /* buffer for storing the formatted data */
  memset(buf_tmp, 0, MAX_BUFF);
  char *pbuf = buf_tmp;
  /* pointer to the variable arguments list */
  va_list pargs;
  
  /* Initialise pargs to point to the first optional argument */
  va_start(pargs, fmt);
  /* Iterate through the formatted string to replace all 
  conversion specifiers with the respective values */
  while(*fmt) {
    if(*fmt == '%') {
      switch(*(++fmt)) {
        case 'd': 
        case 'i': 
          pbuf += sprintf(pbuf, "%d", va_arg(pargs, int));
          break;
        case 'u': 
          pbuf += sprintf(pbuf, "%u", va_arg(pargs, unsigned int));
          break;
        case 'l': 
          switch(*(++fmt)) {
            case 'd': 
            case 'i': 
              pbuf += sprintf(pbuf, "%ld", va_arg(pargs, long));
              break;
            case 'u': 
              pbuf += sprintf( pbuf, "%lu", 
                               va_arg(pargs, unsigned long));
              break;
          }
          break;
        case 'f': 
          pbuf += strlen(dtostrf( va_arg(pargs, double), 
                                  1, F_PRECISION, pbuf));
          break;
        
        case 'c':
          *(pbuf++) = (char)va_arg(pargs, int);
          break;
        case 's': 
          pbuf += sprintf(pbuf, "%s", va_arg(pargs, char *));
          break;
        case '%':
          *(pbuf++) = '%';
          break;
        default:
          break;
      }
    } else {
      *(pbuf++) = *fmt;
    }
    fmt++;
    
  }
  
  *pbuf = '\0';
  va_end(pargs);
  return buf_tmp;
}

void MeasureHnT()
{
  for(int i=0; i<SENSOR_NUM; i++)
  {
    h[i]=dht[i].getHumidity();
    t[i]=dht[i].getTemperature();
  }
  
  float temperature_TC, temperature_CJ; //added
  //Make loop here, may be while(True){...}
  temperature_TC=Thermocouple.readTC();
  temperature_CJ=Thermocouple.readCJ(); //added
  
  int tmp=0;
  for(int i=0; i<SENSOR_NUM; i++)
    if (isnan(h[i]) || isnan(t[i]))
    {
      Serial.print("DHT sensor read failure!\nFailed sensor num: ");
      Serial.println(i+1);
    }
  
  return;
}



//...
void doSomething(int i)
{
  Serial.println(formatString(buf, "!!! Sensor %d detected high temperature !!!", i+1));
  return;
}
