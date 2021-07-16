 /**********************************************************************************************
 * mqttPublish is a SERVICE that posts the raw input 
 * 
 * 
 * 
 **********************************************************************************************/
 #include "IotaWatt.h"
 

 

const int mqttPort = 1883;
const char* mqttUser = "user"; //replace with your mqtt user name
const char* mqttPassword = "password"; //replace with your mqtt password 
const char* mqttServer = "192.168.187.20"; //replace with your mqtt broker IP
PubSubClient MQTTclient(WifiClient); // added for MQTT



void connectMQTT(){
    MQTTclient.setServer(mqttServer, 1883 );

    if ( MQTTclient.connect("ESP8266 Device", mqttUser, mqttPassword)){
    //    Serial.println( " mqtt connected" );
    }
    else {
        Serial.println( "MQTT not connected" );
    }
}


uint32_t mqttPublish(struct serviceBlock* _serviceBlock){
 
trace(T_mqtt,1);
connectMQTT();


DynamicJsonBuffer jsonBuffer;
JsonObject& root = jsonBuffer.createObject();


      JsonObject& wifi = jsonBuffer.createObject();
      wifi.set(F("connecttime"),wifiConnectTime);
      if(wifiConnectTime){
        wifi.set(F("SSID"),WiFi.SSID());
        String ip = WiFi.localIP().toString();
        wifi.set(F("IP"),ip);
        //Serial.printf("SSID: %s, IP: %s\r\n", WiFi.SSID().c_str(), ip.c_str());
        wifi.set(F("channel"),WiFi.channel());
        wifi.set(F("RSSI"),WiFi.RSSI());
        wifi.set(F("mac"), WiFi.macAddress());
      }
      root["wifi"] = wifi;

      JsonArray& channelArray = jsonBuffer.createArray();
      for(int i=0; i<maxInputs; i++){
        if(inputChannel[i]->isActive()){
          JsonObject& channelObject = jsonBuffer.createObject();
          channelObject.set(F("channel"),inputChannel[i]->_name);
          if(inputChannel[i]->_type == channelTypeVoltage){
            channelObject.set(F("Vrms"),statRecord.accum1[i]);
            channelObject.set(F("Hz"),statRecord.accum2[i]);
            channelObject.set("phase", inputChannel[i]->getPhase(inputChannel[i]->dataBucket.volts));
          }
          else if(inputChannel[i]->_type == channelTypePower){
            if(statRecord.accum1[i] > -2 && statRecord.accum1[i] < 2) statRecord.accum1[i] = float(0.0);
            channelObject.set(F("Watts"),String(statRecord.accum1[i],0));
            double pf = statRecord.accum2[i];
            if(pf != 0){
              pf = statRecord.accum1[i] / pf;
            }
            channelObject.set("Pf",pf);
            if(inputChannel[i]->_reversed){
              channelObject.set(F("reversed"),true);
            }
            double volts = inputChannel[inputChannel[i]->_vchannel]->dataBucket.volts;
            double amps = (volts < 50) ? 0 : inputChannel[i]->dataBucket.VA / volts;
            channelObject.set("phase", inputChannel[i]->getPhase(amps));
            channelObject.set("lastphase", inputChannel[i]->_lastPhase);
          }
          channelArray.add(channelObject);
        }
      }
      root["inputs"] = channelArray;

    JsonArray& outputArray = jsonBuffer.createArray();
    Script* script = outputs->first();
    while(script){
      trace(T_WEB,16,1);
      JsonObject& channelObject = jsonBuffer.createObject();
      channelObject.set(F("name"),script->name());
      channelObject.set(F("units"),script->getUnits());
      double value = script->run((IotaLogRecord*)nullptr, &statRecord, 1.0);
      channelObject.set(F("value"),value);
      outputArray.add(channelObject);
      script = script->next();
    }
    trace(T_WEB,16,2);
    root["outputs"] = outputArray;




  String payload = "";
  root["inputs"].printTo(payload);
  char charBuff[payload.length() + 1];
  payload.toCharArray(charBuff,payload.length() + 1);
  MQTTclient.publish("IoTaWatt/inputs", charBuff);

  String outputload = "";
  root["outputs"].printTo(outputload);
  char outBuff[outputload.length() + 1];
  outputload.toCharArray(outBuff,outputload.length() + 1);
  MQTTclient.publish("IoTaWatt/outputs", outBuff);


  String wifiload = "";
  root["wifi"].printTo(wifiload);
  char wifiBuff[wifiload.length() + 1];
  wifiload.toCharArray(wifiBuff,wifiload.length() + 1);
  MQTTclient.publish("IoTaWatt/wifi", wifiBuff);

 return UTCtime() + 2;
 }



 
