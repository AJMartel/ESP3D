/*
  espcom.cpp - esp3d communication serial/tcp/etc.. class

  Copyright (c) 2014 Luc Lebosse. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "config.h"
#include "espcom.h"
#include "command.h"
#include "webinterface.h"

#ifdef ESP_OLED_FEATURE
#include "esp_oled.h"
 bool  ESPCOM::block_2_oled = false;
#endif

#ifdef TCP_IP_DATA_FEATURE
WiFiServer * data_server;
WiFiClient serverClients[MAX_SRV_CLIENTS];
#endif

bool ESPCOM::block_2_printer = false;

void ESPCOM::bridge(bool async)
{
//be sure wifi is on to proceed wifi function
    if ((WiFi.getMode() != WIFI_OFF) || wifi_config.WiFi_on) {
//read tcp port input
#ifdef TCP_IP_DATA_FEATURE
        ESPCOM::processFromTCP2Serial();
#endif
    }
//read serial input
ESPCOM::processFromSerial();
}
long ESPCOM::readBytes (tpipe output, uint8_t * sbuf, size_t len)
{
	 switch (output) {
#ifdef USE_SERIAL_0
    case SERIAL_PIPE:
        return Serial.readBytes(sbuf,len);
        break;
#endif
#ifdef USE_SERIAL_1
    case SERIAL_PIPE:
        return Serial1.readBytes(sbuf,len);
        break;
#endif
#ifdef USE_SERIAL_2
    case SERIAL_PIPE:
        return Serial2.readBytes(sbuf,len);
        break;
#endif
	}
}
long ESPCOM::baudRate(tpipe output)
{
	long br = 0;
	 switch (output) {
#ifdef USE_SERIAL_0
    case SERIAL_PIPE:
        br = Serial.baudRate();
        break;
#endif
#ifdef USE_SERIAL_1
    case SERIAL_PIPE:
        br = Serial1.baudRate();
        break;
#endif
#ifdef USE_SERIAL_2
    case SERIAL_PIPE:
        br = Serial2.baudRate();
        break;
#endif
	}
#ifdef ARDUINO_ARCH_ESP32
    //workaround for ESP32
    if (br == 115201) {
        br = 115200;
    }
    if (br == 230423) {
        br = 230400;
    }
#endif
return br;
}
size_t ESPCOM::available(tpipe output){
	 switch (output) {
#ifdef USE_SERIAL_0
    case SERIAL_PIPE:
        return Serial.available();
        break;
#endif
#ifdef USE_SERIAL_1
    case SERIAL_PIPE:
        return Serial1.available();
        break;
#endif
#ifdef USE_SERIAL_2
    case SERIAL_PIPE:
        return Serial2.available();
        break;
#endif
	}
}
size_t   ESPCOM::write(tpipe output, uint8_t d){
	if ((DEFAULT_PRINTER_PIPE == output) && (block_2_printer || CONFIG::is_locked(FLAG_BLOCK_M117))) return 0;
    if ((SERIAL_PIPE == output) && CONFIG::is_locked(FLAG_BLOCK_SERIAL))return 0;
 switch (output) {
#ifdef USE_SERIAL_0
    case SERIAL_PIPE:
        return Serial.write(d);
        break;
#endif
#ifdef USE_SERIAL_1
    case SERIAL_PIPE:
        return Serial1.write(d);
        break;
#endif
#ifdef USE_SERIAL_2
    case SERIAL_PIPE:
        return Serial2.write(d);
        break;
#endif
	}	
}
void ESPCOM::flush (tpipe output)
{
	 switch (output) {
#ifdef USE_SERIAL_0
    case SERIAL_PIPE:
        Serial.flush();
        break;
#endif
#ifdef USE_SERIAL_1
    case SERIAL_PIPE:
        Serial1.flush();
        break;
#endif
#ifdef USE_SERIAL_2
    case SERIAL_PIPE:
        Serial2.flush();
        break;
#endif
	}
}

void ESPCOM::print (const __FlashStringHelper *data, tpipe output, AsyncResponseStream  *asyncresponse)
{
    String tmp = data;
    ESPCOM::print (tmp.c_str(), output, asyncresponse);
}
void ESPCOM::print (String & data, tpipe output, AsyncResponseStream  *asyncresponse)
{
    ESPCOM::print (data.c_str(), output, asyncresponse);
}
void ESPCOM::print (const char * data, tpipe output, AsyncResponseStream  *asyncresponse)
{
	if ((DEFAULT_PRINTER_PIPE == output) && ( block_2_printer || CONFIG::is_locked(FLAG_BLOCK_M117))) return;
    if ((SERIAL_PIPE == output) && CONFIG::is_locked(FLAG_BLOCK_SERIAL))return;
#ifdef TCP_IP_DATA_FEATURE
    if ((TCP_PIPE == output) && CONFIG::is_locked(FLAG_BLOCK_TCP))return;
#endif
#ifdef WS_DATA_FEATURE
    if ((WS_PIPE == output) && CONFIG::is_locked(FLAG_BLOCK_WSOCKET))return;
#endif
#ifdef ESP_OLED_FEATURE
    if ((OLED_PIPE == output) && CONFIG::is_locked(FLAG_BLOCK_OLED))return;
#endif
    switch (output) {
#ifdef USE_SERIAL_0
    case SERIAL_PIPE:
        Serial.print (data);
        break;
#endif
#ifdef USE_SERIAL_1
    case SERIAL_PIPE:
        Serial1.print (data);
        break;
#endif
#ifdef USE_SERIAL_2
    case SERIAL_PIPE:
        Serial2.print (data);
        break;
#endif
#ifdef TCP_IP_DATA_FEATURE
    case TCP_PIPE:
        ESPCOM::send2TCP (data);
        break;
#endif
    case WEB_PIPE:
        if (asyncresponse != NULL) {
            asyncresponse->print (data);
        }
        break;
#ifdef WS_DATA_FEATURE
    case WS_PIPE:
		{
		
        }
        break;
#endif

#ifdef ESP_OLED_FEATURE
    case OLED_PIPE:
		{
		if (!ESPCOM::block_2_oled) {
			if(!((data=="\n")||(data=="\r")||(data=="\r\n"))) {
				OLED_DISPLAY::print(data);
				OLED_DISPLAY::update_lcd();	
				}
			}
        }
        break;
#endif
	case PRINTER_PIPE:
		{
#ifdef ESP_OLED_FEATURE
		OLED_DISPLAY::setCursor(0, 48);
		if(!((data=="\n")||(data=="\r")||(data=="\r\n")))ESPCOM::print(data, OLED_PIPE);
#endif
        if(!((data=="\n")||(data=="\r")||(data=="\r\n")))ESPCOM::print ("M117 ", DEFAULT_PRINTER_PIPE);
        ESPCOM::print (data, DEFAULT_PRINTER_PIPE);
        }
        break;
    default:
        break;
    }
}
void ESPCOM::println (const __FlashStringHelper *data, tpipe output, AsyncResponseStream  *asyncresponse)
{
    ESPCOM::print (data, output, asyncresponse);
#ifdef TCP_IP_DATA_FEATURE
    ESPCOM::print ("\r", output, asyncresponse);
#endif
    ESPCOM::print ("\n", output, asyncresponse);
}
void ESPCOM::println (String & data, tpipe output, AsyncResponseStream  *asyncresponse)
{
    ESPCOM::print (data, output, asyncresponse);
#ifdef TCP_IP_DATA_FEATURE
    ESPCOM::print ("\r", output, asyncresponse);
#endif
    ESPCOM::print ("\n", output, asyncresponse);
}
void ESPCOM::println (const char * data, tpipe output, AsyncResponseStream  *asyncresponse)
{
    ESPCOM::print (data, output, asyncresponse);
#ifdef TCP_IP_DATA_FEATURE
    ESPCOM::print ("\r", output, asyncresponse);
#endif
    ESPCOM::print ("\n", output, asyncresponse);
}


#ifdef TCP_IP_DATA_FEATURE
void ESPCOM::send2TCP (const __FlashStringHelper *data, bool async)
{
    String tmp = data;
    ESPCOM::send2TCP (tmp.c_str(), async);
}
void ESPCOM::send2TCP (String data, bool async)
{
    ESPCOM::send2TCP (data.c_str(), async);
}
void ESPCOM::send2TCP (const char * data, bool async)
{
    if (!async) {
        for (uint8_t i = 0; i < MAX_SRV_CLIENTS; i++) {
            if (serverClients[i] && serverClients[i].connected() ) {
                serverClients[i].write (data, strlen (data) );
                delay (0);
            }
        }
    }
}
#endif

bool ESPCOM::processFromSerial (bool async)
{
    uint8_t i;
    //check UART for data
    if (ESPCOM::available(DEFAULT_PRINTER_PIPE)) {
        size_t len = ESPCOM::available(DEFAULT_PRINTER_PIPE);
        uint8_t sbuf[len+1];
        sbuf[len] = '\0';
        ESPCOM::readBytes (DEFAULT_PRINTER_PIPE, sbuf, len);
#ifdef TCP_IP_DATA_FEATURE
        if (!async &&  !CONFIG::is_locked(FLAG_BLOCK_TCP)) {
            if ((WiFi.getMode() != WIFI_OFF)  || !wifi_config.WiFi_on) {
                //push UART data to all connected tcp clients
                for (i = 0; i < MAX_SRV_CLIENTS; i++) {
                    if (serverClients[i] && serverClients[i].connected() ) {
                        serverClients[i].write (sbuf, len);
                        delay (0);
                    }
                }
            }
        }
#endif
#ifdef WS_DATA_FEATURE
         if (!CONFIG::is_locked(FLAG_BLOCK_WSOCKET))web_interface->web_socket.textAll(sbuf, len);
#endif
        //process data if any
        COMMAND::read_buffer_serial (sbuf, len);
        return true;
    } else {
        return false;
    }
}
#ifdef TCP_IP_DATA_FEATURE
void ESPCOM::processFromTCP2Serial()
{
    uint8_t i, data;
    //check if there are any new clients
    if (data_server->hasClient() ) {
        for (i = 0; i < MAX_SRV_CLIENTS; i++) {
            //find free/disconnected spot
            if (!serverClients[i] || !serverClients[i].connected() ) {
                if (serverClients[i]) {
                    serverClients[i].stop();
                }
                serverClients[i] = data_server->available();
                continue;
            }
        }
        //no free/disconnected spot so reject
        WiFiClient serverClient = data_server->available();
        serverClient.stop();
    }
    //check clients for data
    //to avoid any pollution if Uploading file to SDCard
    if (!((web_interface->blockserial)  || CONFIG::is_locked(FLAG_BLOCK_TCP) || CONFIG::is_locked(FLAG_BLOCK_SERIAL))) {
        for (i = 0; i < MAX_SRV_CLIENTS; i++) {
            if (serverClients[i] && serverClients[i].connected() ) {
                if (serverClients[i].available() ) {
                    //get data from the tcp client and push it to the UART
                    while (serverClients[i].available() ) {
                        data = serverClients[i].read();
                        ESPCOM::write(DEFAULT_PRINTER_PIPE, data);
                        COMMAND::read_buffer_tcp (data);
                    }
                }
            }
        }
    }
}
#endif
