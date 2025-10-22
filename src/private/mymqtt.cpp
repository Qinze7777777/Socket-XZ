#include "private/mymqtt.h"

AsyncMqttClient mqtt;
int mqtt_connnected;

void initHassMQTT(void)
{
    if (devconfig.mqtt_topic == String("") || devconfig.mqtt_server == String("") || devconfig.mqtt_port == String("") || devconfig.mqtt_user == String("") || devconfig.mqtt_password == String(""))
    {
        Serial.println("--------------HASS MQTT Config Error Enter Config!--------------");
        configDev();
    }
    String willString = devconfig.mqtt_topic + String("/stat");
    const char *topic = strdup(willString.c_str());
    const char *clientId = strdup(devconfig.matt_clientId.c_str());   
    mqtt.setClientId(clientId);
    mqtt.onConnect(onConnectHandler);
    mqtt.onDisconnect(onDisconnectHandler);
    mqtt.onMessage(onMessageHandler);
    mqtt.setKeepAlive(60).setCleanSession(true).setWill(topic, 0, false, "{\"state\":\"0\"}");
    mqtt.setServer(devconfig.mqtt_server.c_str(), devconfig.mqtt_port.toInt());
    if ((devconfig.mqtt_user != String("")) && (devconfig.mqtt_password != String("")))
    {
        mqtt.setCredentials(devconfig.mqtt_user.c_str(), devconfig.mqtt_password.c_str());
    }
    mqtt.connect();
}

int checkHassMQTT(void)
{
    return mqtt_connnected;
}

void onConnectHandler(bool sessionPresent)
{
    mqtt.subscribe((devconfig.mqtt_topic + String("/control")).c_str(), 2);     //订阅控制主题，用于homeassistant控制
    mqtt.subscribe((devconfig.mqtt_topic + String("/control/screen")).c_str(), 2);     //订阅屏幕控制主题，用于homeassistant控制屏幕亮度
    Serial.println("============HASS MQTT Connect============");
    mqtt_connnected = 1;
    publishHassMQTTdiscoverys();
}

void onDisconnectHandler(AsyncMqttClientDisconnectReason reason)
{
    Serial.println("Disconnected from HASS MQTT.");
    Serial.println("Reason:");
    if (reason == AsyncMqttClientDisconnectReason::TCP_DISCONNECTED)
    {
        Serial.println("TCP_DISCONNECTED");
        //尝试重连
        mqtt.connect();
        Serial.println("reconnecting");
    }
    else if (reason == AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION)
    {
        Serial.println("MQTT_UNACCEPTABLE_PROTOCOL_VERSION");
    }
    else if (reason == AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED)
    {
        Serial.println("MQTT_IDENTIFIER_REJECTED");
    }
    else if (reason == AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE)
    {
        Serial.println("MQTT_SERVER_UNAVAILABLE");
    }
    else if (reason == AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS)
    {
        Serial.println("MQTT_MALFORMED_CREDENTIALS");
    }
    else if (reason == AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED)
    {
        Serial.println("MQTT_NOT_AUTHORIZED");
    }
    else if (reason == AsyncMqttClientDisconnectReason::ESP8266_NOT_ENOUGH_SPACE)
    {
        Serial.println("ESP8266_NOT_ENOUGH_SPACE");
    }
    else if (reason == AsyncMqttClientDisconnectReason::TLS_BAD_FINGERPRINT)
    {
        Serial.println("TLS_BAD_FINGERPRINT");
    }
}

void onMessageHandler(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
    char chars[len];
    strlcpy(chars, payload, len + 1);
    String str = String(chars);
    
    Serial.println("HASS receive message");
    // 如果是retain消息，相当于say hello，直接丢弃
    if (properties.retain)
    {
        return;
    }
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, str);
    if (error)
    {
        Serial.println("Failed to deserializeJson str");
        return;
    }
    if (String(topic) == (devconfig.mqtt_topic + String("/control")))    //如果是control主题
    {
        String kind = doc["kind"].as<String>();
        String content = doc["content"].as<String>();
        if (kind == "switch")  //控制插座开关
        {
            if (content == "1")
            {
                // 开
                devstate.socket = "1";
                Serial.println("switch on");
            }
            else
            {
                devstate.socket = "0";
                Serial.println("switch off");
                // 关
            }
        }
        if (kind == "timernum")  //设置定时器
        {
            if (content != "")
            {
                devstate.timernum = content.toInt();
            }
            
        }
        if (kind == "timer")  //开关定时器
        {
            if (content == "1")
            {
                devstate.timer = "1";
                setSocketTimer(devstate.timernum.toInt(), true);
                Serial.println("timer open");
            }
            else
            {
                devstate.timer = "0";
                setSocketTimer(devstate.timernum.toInt(), false);
                Serial.println("timer close ");
            }
        }
        if (kind == "learn")  //红外学习
        {
            if (content == "1")
            {
                // 开
                Serial.println("learn");
            }
        }
        if (kind == "clear")  //清空电能
        {
            if (content == "1")
            {
                // 开
                Serial.println("clear");
                devstate.E = "0";
            }
        }
        if (kind == "reset")  //复位
        {
            if (content == "1")
            {
                mqtt.disconnect();
                ESP.restart();
            }
        }
    }
    else if (String(topic) == (devconfig.mqtt_topic + String("/control/screen")))   //如果是control/screen主题(屏幕专用)
    {
        String state = doc["state"].as<String>();
        String brightness = doc["brightness"].as<String>();
        if (state == "ON")
        {
            // 打开背光
            devstate.screen = "ON";
            Serial.println("screen on");
        }
        else
        {
            //关闭背光
            devstate.screen = "OFF";
            Serial.println("screen off");
        }
        if (brightness != "")
        {
            // 设置背光亮度
            devstate.brightness = String(brightness);
            Serial.println("brightness:" + brightness);
        }
    }
    saveDevState();
}

void publishHassMQTTMessage(String topic, DynamicJsonDocument &doc)
{
    String jsonstr;
    topic.replace("{topic}", devconfig.mqtt_topic);
    serializeJson(doc, jsonstr);
    doc.clear();
    jsonstr.replace("{topic}", devconfig.mqtt_topic);
    mqtt.publish(topic.c_str(), 0, false, jsonstr.c_str());
    delay(20);
}

void publishHassMQTTdiscovery(String topic, DynamicJsonDocument &doc)
{
    DynamicJsonDocument docDevice(1024);

    String clientId = devconfig.matt_clientId;
    clientId.replace("-", "_");
    clientId.toLowerCase();
    topic.replace("{clientId}", clientId);

    String name = clientId;
    name.toUpperCase();
    docDevice["name"] = name;
    // docDevice["model"] = model;
    // docDevice["sw_version"] = version;
    // docDevice["manufacturer"] = "oldfox126";
    docDevice["identifiers"] = "{clientId}";

    #ifdef MQTTCLEAR
        Serial.print("MQTT Clear config=");
        Serial.println(topic);
        mqtt.publish(topic.c_str(), 2, false, "{}");
        delay(20);
        return;
    #endif

    doc["device"] = docDevice;
    String jsonstr;
    serializeJson(doc, jsonstr);
    docDevice.clear();
    doc.clear();

    jsonstr.replace("{clientId}", clientId);
    jsonstr.replace("{topic}", devconfig.mqtt_topic);
    mqtt.publish(topic.c_str(), 0, false, jsonstr.c_str());
    delay(20);
}

void publishHassMQTTdiscoverys(void)
{
    DynamicJsonDocument doc(1024);
    // 设备状态
    String topic = String("homeassistant/binary_sensor/{clientId}_state/config");
    doc["name"] = "设备状态";
    doc["unique_id"] = "{clientId}_state";
    doc["state_topic"] = "{topic}/stat";
    doc["qos"] = "0";
    doc["device_class"] = "running";
    doc["value_template"] = "{{ value_json.state }}";
    doc["payload_on"] = "1";
    doc["payload_off"] = "0";
    publishHassMQTTdiscovery(topic, doc);

    // // version
    // topic = String("homeassistant/sensor/{clientId}_version/config");
    // doc["name"] = "version";
    // doc["unique_id"] = "{clientId}_version";
    // doc["state_topic"] = "{topic}/stat";
    // doc["value_template"] = "{{ value_json.version }}";
    // publishMQTTdiscovery(topic, doc);

    // 设备时间
    topic = String("homeassistant/sensor/{clientId}_time/config");
    doc["name"] = "设备时间";
    doc["unique_id"] = "{clientId}_time";
    doc["state_topic"] = "{topic}/stat";
    doc["value_template"] = "{{ value_json.time }}";
    publishHassMQTTdiscovery(topic, doc);

    // 倒计时
    topic = String("homeassistant/sensor/{clientId}_timeout/config");
    doc["name"] = "倒计时";
    doc["unique_id"] = "{clientId}_timeout";
    doc["state_topic"] = "{topic}/stat";
    doc["value_template"] = "{{ value_json.timeout }}";
    doc["unit_of_measurement"] = "s";
    publishHassMQTTdiscovery(topic, doc);

    // 定时器时间(下面那个只能改不能显示)
    topic = String("homeassistant/sensor/{clientId}_timernumber/config");
    doc["name"] = "定时器时间";
    doc["unique_id"] = "{clientId}_timernumber";
    doc["state_topic"] = "{topic}/stat";
    doc["value_template"] = "{{ value_json.timernumber }}";
    doc["unit_of_measurement"] = "s";
    publishHassMQTTdiscovery(topic, doc);

    // 屏幕
    // topic = String("homeassistant/light/{clientId}_brightness/config");
    // doc["name"] = "屏幕";
    // doc["unique_id"] = "{clientId}_brightness";
    // doc["state_topic"] = "{topic}/screen";
    // doc["command_topic"] = "{topic}/control/screen";
    // doc["brightness"] = "true";
    // doc["schema"] = "json";
    // doc["qos"] = "0";
    topic = String("homeassistant/light/{clientId}_brightness/config");
    doc["name"] = "屏幕";
    doc["unique_id"] = "{clientId}_brightness";
    doc["state_topic"] = "{topic}/screen";
    doc["command_topic"] = "{topic}/control/screen";
    doc["brightness"] = true;
    doc["schema"] = "json";
    doc["qos"] = "0";

    // 添加 RGB 功能
    // doc["rgb"] = true;
    // doc["rgb_command_topic"] = "{topic}/control/rgb";
    // doc["rgb_state_topic"] = "{topic}/state/rgb";
    publishHassMQTTdiscovery(topic, doc);

    // 插座开关
    topic = String("homeassistant/switch/{clientId}_switch/config");
    doc["name"] = "插座开关";
    doc["unique_id"] = "{clientId}_switch";
    doc["state_topic"] = "{topic}/stat";
    doc["command_topic"] = "{topic}/control";
    doc["payload_on"] = "{\"kind\":\"switch\",\"content\":\"1\"}";
    doc["payload_off"] = "{\"kind\":\"switch\",\"content\":\"0\"}";
    doc["qos"] = "0";
    doc["value_template"] = "{{ value_json.switch }}";
    doc["state_on"] = "1";
    doc["state_off"] = "0";
    publishHassMQTTdiscovery(topic, doc);

    // 定时时间
    topic = String("homeassistant/text/{clientId}_timernum/config");
    doc["name"] = "定时时间(秒)";
    doc["unique_id"] = "{clientId}_timernum";
    doc["state_topic"] = "{topic}/stat";
    doc["command_topic"] = "{topic}/control";
    doc["qos"] = "0";
    doc["value_template"] = "{{ value_json.timernum }}";
    doc["command_template"] = "{\"kind\":\"timernum\",\"content\":\"{{ value }}\"}";
    publishHassMQTTdiscovery(topic, doc);

    // 定时开关
    topic = String("homeassistant/switch/{clientId}_timer/config");
    doc["name"] = "定时开关";
    doc["unique_id"] = "{clientId}_timer";
    doc["state_topic"] = "{topic}/stat";
    doc["command_topic"] = "{topic}/control";
    doc["payload_on"] = "{\"kind\":\"timer\",\"content\":\"1\"}";
    doc["payload_off"] = "{\"kind\":\"timer\",\"content\":\"0\"}";
    doc["qos"] = "0";
    doc["value_template"] = "{{ value_json.timer }}";
    doc["state_on"] = "1";
    doc["state_off"] = "0";
    publishHassMQTTdiscovery(topic, doc);

    // // 红外学习
    // topic = String("homeassistant/button/{clientId}_learn/config");
    // doc["name"] = "红外学习";
    // doc["unique_id"] = "{clientId}_learn";
    // doc["command_topic"] = "{topic}/control";
    // doc["qos"] = "0";
    // doc["command_template"] = "{\"kind\":\"learn\",\"content\":\"1\"}";
    // publishMQTTdiscovery(topic, doc);

    //清空电能记录
    topic = String("homeassistant/button/{clientId}_clear/config");
    doc["name"] = "清空电能记录";
    doc["unique_id"] = "{clientId}_clear";
    doc["command_topic"] = "{topic}/control";
    doc["qos"] = "0";
    doc["command_template"] = "{\"kind\":\"clear\",\"content\":\"1\"}";
    publishHassMQTTdiscovery(topic, doc);

    //重启设备
    topic = String("homeassistant/button/{clientId}_reset/config");
    doc["name"] = "重启设备";
    doc["unique_id"] = "{clientId}_reset";
    doc["command_topic"] = "{topic}/control";
    doc["qos"] = "0";
    doc["command_template"] = "{\"kind\":\"reset\",\"content\":\"1\"}";
    publishHassMQTTdiscovery(topic, doc);

    // 电压
    topic = String("homeassistant/sensor/{clientId}_voltage/config");
    doc["name"] = "负载电压";
    doc["unique_id"] = "{clientId}_voltage";
    doc["state_topic"] = "{topic}/stat";
    doc["value_template"] = "{{ value_json.voltage }}";
    doc["unit_of_measurement"] = "V";
    publishHassMQTTdiscovery(topic, doc);

    // 电流
    topic = String("homeassistant/sensor/{clientId}_current/config");
    doc["name"] = "负载电流";
    doc["unique_id"] = "{clientId}_current";
    doc["state_topic"] = "{topic}/stat";
    doc["value_template"] = "{{ value_json.current }}";
    doc["unit_of_measurement"] = "mA";
    publishHassMQTTdiscovery(topic, doc);

    // 功率
    topic = String("homeassistant/sensor/{clientId}_power/config");
    doc["name"] = "负载功率";
    doc["unique_id"] = "{clientId}_power";
    doc["state_topic"] = "{topic}/stat";
    doc["value_template"] = "{{ value_json.power }}";
    doc["unit_of_measurement"] = "W";
    publishHassMQTTdiscovery(topic, doc);

    // 电能
    topic = String("homeassistant/sensor/{clientId}_energy/config");
    doc["name"] = "所耗电能";
    doc["unique_id"] = "{clientId}_energy";
    doc["state_topic"] = "{topic}/stat";
    doc["value_template"] = "{{ value_json.energy }}";
    doc["unit_of_measurement"] = "kWh";
    publishHassMQTTdiscovery(topic, doc);

    //温度
    topic = String("homeassistant/sensor/{clientId}_temperature/config");
    doc["name"] = "环境温度";
    doc["unique_id"] = "{clientId}_temperature";
    doc["state_topic"] = "{topic}/stat";
    doc["value_template"] = "{{ value_json.temperature }}";
    doc["unit_of_measurement"] = "°C";
    publishHassMQTTdiscovery(topic, doc);

    //湿度
    topic = String("homeassistant/sensor/{clientId}_humidity/config");
    doc["name"] = "环境湿度";
    doc["unique_id"] = "{clientId}_humidity";
    doc["state_topic"] = "{topic}/stat";
    doc["value_template"] = "{{ value_json.humidity }}";
    doc["unit_of_measurement"] = "%RH";
    publishHassMQTTdiscovery(topic, doc);

    //红外遥控
    topic = String("homeassistant/fan/{clientId}_hongwai/config");
    doc["name"] = "红外遥控";
    doc["unique_id"] = "{clientId}_hongwai";
    doc["command_topic"] = "{topic}/control/hongwai";
    doc["state_topic"] = "{topic}/hongwai";
    publishHassMQTTdiscovery(topic, doc);
}

void publishDevState(void)
{
    String topic = String("{topic}/stat");
    DynamicJsonDocument doc(1024);
    //更新设备运行状态
    doc["state"] = devstate.state;
    //更新插座状态
    doc["switch"] = devstate.socket;
    //更新定时器开关
    doc["timer"] = devstate.timer;
    //更新定时器时长
    // doc["timernum"] = devstate.timernum;
    // publishMQTTMessage(topic, doc);
    //更新倒计时
    doc["timeout"] = devstate.countdown.toInt();
    //更新定时器时长
    doc["timernumber"] = devstate.timernum;
    //更新设备时间
    doc["time"] = getDateTime();
    //更新电压
    doc["voltage"] = devstate.V;
    //更新电流
    doc["current"] = devstate.I;
    //更新功率
    doc["power"] = devstate.P;
    //更新电能
    doc["energy"] = devstate.E;
    //更新温度
    doc["temperature"] = devstate.temperature;
    //更新湿度
    doc["humidity"] = devstate.humidity;
    publishHassMQTTMessage(topic, doc);
    //更新屏幕状态
    topic = String("{topic}/screen");
    doc["state"] = devstate.screen;
    doc["brightness"] = devstate.brightness.toInt();
    publishHassMQTTMessage(topic, doc);
}