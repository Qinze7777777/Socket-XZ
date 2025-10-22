#include "private/xzmqtt.h"

WiFiClientSecure secureClient;
PubSubClient xzmqtt(secureClient);

String subscribe_topic; 
String publish_topic;   
String host;
int port = 8883;
String username;
String password;
String client_id;
String tts_state;
uint8_t sendhello; // 0: not send, 1: send
uint8_t sendgoodbye = 1;

int xzmqtt_connnected;      //是否已连接


void initXZMQTT(void)
{
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, mqttinfo);
    if (error)
    {
        Serial.println("Failed to deserializeJson str");
        return;
    }
    host = doc["endpoint"].as<String>();
    username = doc["username"].as<String>();
    password = doc["password"].as<String>();
    client_id = doc["client_id"].as<String>();
    subscribe_topic = doc["subscribe_topic"].as<String>();
    publish_topic = doc["publish_topic"].as<String>();

    secureClient.setInsecure();//设置SSL
    xzmqtt.setServer(host.c_str(), port);
    xzmqtt.setCallback(onXZMessageHandler);
    xzmqtt.connect(client_id.c_str(), username.c_str(), password.c_str());
    if (xzmqtt.connected())
    {
        Serial.println("============XZ MQTT Connect============");
        xzmqtt_connnected = 1;
    }
}

void reconnectXZMQTT(void)
{
    while (!xzmqtt.connected())
    {
        Serial.println("reconnecting XZ MQTT");
        if (xzmqtt.connect(client_id.c_str(), username.c_str(), password.c_str()))
        {
            Serial.println("============XZ MQTT Connect============");
            // client.subscribe("test/topic");
        }
        else
        {
            Serial.print("reconnect fail,rc=");
            Serial.print(xzmqtt.state());
        }
    }
}

void loopXZMQTT(void)
{
    xzmqtt.loop();  //MQTT循环
    if (!xzmqtt.connected())
    {
        reconnectXZMQTT();
    }
}

int checkXZMQTT(void)
{
    return xzmqtt_connnected;
}

// void onXZDisconnectHandler(AsyncMqttClientDisconnectReason reason)
// {
//     Serial.println("Disconnected from XZMQTT.");
//     Serial.println("Reason:");
//     if (reason == AsyncMqttClientDisconnectReason::TCP_DISCONNECTED)
//     {
//         Serial.println("TCP_DISCONNECTED");
//     }
//     else if (reason == AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION)
//     {
//         Serial.println("MQTT_UNACCEPTABLE_PROTOCOL_VERSION");
//     }
//     else if (reason == AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED)
//     {
//         Serial.println("MQTT_IDENTIFIER_REJECTED");
//     }
//     else if (reason == AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE)
//     {
//         Serial.println("MQTT_SERVER_UNAVAILABLE");
//     }
//     else if (reason == AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS)
//     {
//         Serial.println("MQTT_MALFORMED_CREDENTIALS");
//     }
//     else if (reason == AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED)
//     {
//         Serial.println("MQTT_NOT_AUTHORIZED");
//     }
//     else if (reason == AsyncMqttClientDisconnectReason::ESP8266_NOT_ENOUGH_SPACE)
//     {
//         Serial.println("ESP8266_NOT_ENOUGH_SPACE");
//     }
//     else if (reason == AsyncMqttClientDisconnectReason::TLS_BAD_FINGERPRINT)
//     {
//         Serial.println("TLS_BAD_FINGERPRINT");
//     }
// }


void onXZMessageHandler(char* topic, byte* payload, unsigned int length)
{
    Serial.print("消息 [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();
    char chars[length];
    memcpy(chars, payload, length);
    String str = String(chars);
    
    Serial.println("receive message");
    Serial.printf("Topic: %s\n", topic);
    Serial.println(str);
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, str);
    if (error)
    {
        Serial.println("Failed to deserializeJson str");
        return;
    }
    if (doc["type"] == "hello")     //开始
    {
        //提取音频信息，并开始录音
        if (sendhello)
        {
            udpinfo.server = doc["udp"]["server"].as<String>();
            udpinfo.port = doc["udp"]["port"].as<int>();
            udpinfo.key = doc["udp"]["key"].as<String>();
            udpinfo.nonce = doc["udp"]["nonce"].as<String>();
            udpinfo.session_id = doc["session_id"].as<String>();
            udpinfo.sample_rate = doc["audio_params"]["sample_rate"].as<int>();
            udpinfo.frame_duration =  doc["audio_params"]["frame_duration"].as<int>();
            //启动录音
            recvhello = 1;
            sendhello = 0;
        }
    }
    if (doc["type"] == "goodbye")   //结束
    {
        userover = 1;
    }
    if (doc["type"] == "tts")       //语音合成
    {
        //start 服务器准备下发tts音频 stop 本次tts结束 sentence_start tts文本
        String text;
        tts_state = doc["state"].as<String>();
        text = doc["text"].as<String>();
        if (text != "")
        {
            Serial.println("tts text:" + text);
        }

        if (tts_state == "start")
        {
            recvstart = 1;
        }
        if (tts_state == "stop")
        {
            // if (autolisten)
            // {
            //     publishListenMsg();
            //     Serial.println("publish listen msg");
            // }   
            recvstop = 1;
        }
    }
    if (doc["type"] == "stt")       //语音识别
    {
        //返回语音识别结果
        String text;
        text = doc["text"].as<String>();
        if (text != "")
        {
            Serial.println("tts text:" + text);
        }
    }
    if (doc["type"] == "llm")       //表情
    {
        //返回表情
        String emotion, text;
        emotion = doc["emotion"].as<String>();
        text = doc["text"].as<String>();
        Serial.println("llm emotion:" + emotion);
        Serial.println("llm text:" + text);
    }
    if (doc["type"] == "iot")       //物联网
    {
        JsonArray commands = doc["commands"];
        for (JsonObject cmd : commands)
        {
            const char *name = cmd["name"];
            const char *method = cmd["method"];
            JsonObject params = cmd["parameters"];
            controlXiaozhiIOT(name, method, params);
        }   
    }
}

void controlXiaozhiIOT(const char *name, const char *method, JsonObject &params)
{
    if (strcmp(name, "Screen") == 0)
    {
        if (strcmp(method, "SetBrightness") == 0)
        {
            int brightness_recv = params["brightness"].as<int>();  // 提取参数
            Serial.println("SetBrightness");
            if (brightness_recv == 0)
            {
                devstate.screen = "ON";
            }
            else
            {
                devstate.screen = "OFF";
            }
            devstate.brightness = String(brightness_recv * 2.55);
        }
    }
    if (strcmp(name, "Speaker") == 0)
    {
        if (strcmp(method, "SetVolume") == 0)
        {
            Serial.println("SetVolume");
            int volume_recv = params["volume"].as<int>();  // 提取参数
            Serial.print("设置音量为: ");
            Serial.println(volume_recv);
            devstate.volume = String(volume_recv);
        }
        
    }
    if (strcmp(name, "Socket") == 0)
    {
        if (strcmp(method, "ResetEnergy") == 0)
        {
            Serial.println("ResetEnergy");
            devstate.E = "0";
        }
    }
    
}

void publishXZMsg(String msg)
{
    xzmqtt.publish(publish_topic.c_str(), msg.c_str());
}

void publishHelloMsg(void)
{
    String str;
    DynamicJsonDocument doc(512);
    // if (xzruning)
    // {
    //     Serial.println("ai is running");
    //     publishStopMsg();
    //     delay(2000);
    // }
    doc["type"] = "hello";
    doc["version"] = 3;
    doc["transport"] = "udp";
    doc["audio_params"]["format"] = "opus";
    doc["audio_params"]["sample_rate"] = 16000;
    doc["audio_params"]["channels"] = 1;
    doc["audio_params"]["frame_duration"] = 60;
    serializeJson(doc, str);
    publishXZMsg(str);
    sendhello = 1;
}

void publishAbortMsg(void)
{
    String str;
    DynamicJsonDocument doc(512);
    doc["type"] = "abort";
    serializeJson(doc, str);
    publishXZMsg(str);
}

void publishListenMsg(void)
{
    String str;
    DynamicJsonDocument doc(512);
    doc["session_id"] = udpinfo.session_id;
    doc["type"] = "listen";
    doc["state"] = "start";
    doc["mode"] = "manual";
    serializeJson(doc, str);
    publishXZMsg(str);
}

void publishAutoListenMsg(void)
{
    String str;
    DynamicJsonDocument doc(512);
    doc["session_id"] = udpinfo.session_id;
    doc["type"] = "listen";
    doc["state"] = "start";
    doc["mode"] = "auto";
    serializeJson(doc, str);
    publishXZMsg(str);
}

void publishStopMsg(void)
{
    String str;
    DynamicJsonDocument doc(512);
    doc["session_id"] = udpinfo.session_id;
    doc["type"] = "listen";
    doc["state"] = "stop";
    serializeJson(doc, str);
    publishXZMsg(str);
}

void publishGoodbyeMsg(void)
{
    String str;
    DynamicJsonDocument doc(512);
    sendgoodbye = 1;
    doc["session_id"] = udpinfo.session_id;
    doc["type"] = "listen";
    doc["state"] = "stop";
    serializeJson(doc, str);
    publishXZMsg(str);
}

void publishIOTMsg(void)
{
    String str;
    DynamicJsonDocument doc(1024); // 容量调大以容纳更多字段
    doc["session_id"] = udpinfo.session_id;
    doc["type"] = "iot";
    doc["update"] = true;

    JsonArray descriptors = doc.createNestedArray("descriptors");

    // Screen
    JsonObject screen = descriptors.createNestedObject();
    screen["name"] = "Screen";
    screen["description"] = "智能插座上的屏幕";
    JsonObject screen_properties = screen.createNestedObject("properties");
    JsonObject brightness = screen_properties.createNestedObject("brightness");
    brightness["description"] = "当前智能插座上的屏幕的亮度百分比";
    brightness["type"] = "number";
    JsonObject screen_methods = screen.createNestedObject("methods");
    JsonObject setbrightness = screen_methods.createNestedObject("SetBrightness");
    setbrightness["description"] = "设置亮度(关闭屏幕就是把亮度设置为0)";
    JsonObject setbrightness_params = setbrightness.createNestedObject("parameters");
    JsonObject params_brightness = setbrightness_params.createNestedObject("brightness");
    params_brightness["description"] = "0到100之间的整数";
    params_brightness["type"] = "number";

    // Speaker
    JsonObject Speaker = descriptors.createNestedObject();
    Speaker["name"] = "Speaker";
    Speaker["description"] = "智能插座上的扬声器";
    JsonObject speaker_properties = Speaker.createNestedObject("properties");
    JsonObject volume = speaker_properties.createNestedObject("volume");
    volume["description"] = "当前音量值";
    volume["type"] = "number";
    JsonObject speaker_methods = Speaker.createNestedObject("methods");
    JsonObject setvolume = speaker_methods.createNestedObject("SetVolume");
    setvolume["description"] = "设置音量";
    JsonObject setvolume_params = setvolume.createNestedObject("parameters");
    JsonObject params_volume = setvolume_params.createNestedObject("volume");
    params_volume["description"] = "0到100之间的整数";
    params_volume["type"] = "number";

    // socket插座
    JsonObject socket = descriptors.createNestedObject();
    socket["name"] = "Socket";
    socket["description"] = "当前智能插座的状态, 包括插座开关、电压(V)、电流(A)、功率(W)以及总耗电量(kWh)";

    JsonObject power_properties = socket.createNestedObject("properties");

    JsonObject switch_socket = power_properties.createNestedObject("switch");
    switch_socket["description"] = "智能插座是否打开";
    switch_socket["type"] = "boolean";

    JsonObject voltage = power_properties.createNestedObject("voltage");
    voltage["description"] = "智能插座的电压(V)";
    voltage["type"] = "number";

    JsonObject current = power_properties.createNestedObject("current");
    current["description"] = "智能插座的电流(A)";
    current["type"] = "number";

    JsonObject power = power_properties.createNestedObject("power");
    power["description"] = "智能插座的功率(W)";
    power["type"] = "number";

    JsonObject energy = power_properties.createNestedObject("energy");
    energy["description"] = "智能插座的累计用电量(kWh)";
    energy["type"] = "number";

    JsonObject power_methods = socket.createNestedObject("methods");
    JsonObject resetEnergy = power_methods.createNestedObject("ResetEnergy");
    resetEnergy["description"] = "重置智能插座的累计用电量";
    resetEnergy.createNestedObject("parameters"); // 空参数对象

    //温湿度
    JsonObject humiture = descriptors.createNestedObject();
    humiture["name"] = "Humiture";
    humiture["description"] = "智能插座上的温湿度传感器";
    JsonObject humiture_properties = humiture.createNestedObject("properties");
    JsonObject temperatue = humiture_properties.createNestedObject("temperatue");
    temperatue["description"] = "当前温度";
    temperatue["type"] = "number";

    JsonObject humidity = humiture_properties.createNestedObject("humidity");
    humidity["description"] = "当前温度";
    humidity["type"] = "number";

    JsonObject humiture_methods = humiture.createNestedObject("methods");


    // 最后序列化并发送
    serializeJson(doc, str);
    publishXZMsg(str);
}

//屏幕亮度、音量、电参数、开关状态、
void publishIOTUpdateMsg(void)
{
    // 创建 JSON 文档
    DynamicJsonDocument doc(512);

    // 添加基本信息
    doc["session_id"] = udpinfo.session_id;
    doc["type"] = "iot";
    doc["update"] = true;

    // 创建 states 数组
    JsonArray states = doc.createNestedArray("states");

    // Screen 状态
    JsonObject screen = states.createNestedObject();
    screen["name"] = "Screen";
    JsonObject screenState = screen.createNestedObject("state");
    screenState["brightness"] = String((int)(devstate.brightness.toFloat() / 2.55));      

    // Speaker 状态
    JsonObject speaker = states.createNestedObject();
    speaker["name"] = "Speaker";
    JsonObject speakerState = speaker.createNestedObject("state");
    speakerState["volume"] = devstate.volume;

    //电参数状态
    JsonObject socket = states.createNestedObject();
    socket["name"] = "Socket";
    JsonObject socketState = socket.createNestedObject("states");
    if (devstate.socket == "1")
    {
        socketState["switch"] = true;
    }
    else
    {
        socketState["switch"] = false;
    }
    socketState["voltage"] = devstate.V.toFloat();
    socketState["current"] = devstate.I.toFloat();
    socketState["power"] = devstate.P.toFloat();
    socketState["energy"] = devstate.E.toFloat();

    // 温湿度
    JsonObject humiture = states.createNestedObject();
    humiture["name"] = "Humiture";
    JsonObject humitureState = humiture.createNestedObject("states");
    humitureState["temperatue"] = "22.5";      
    humitureState["humidity"] = "23.4";      
    // 序列化为字符串并打印
    String jsonStr;
    serializeJson(doc, jsonStr);
    publishXZMsg(jsonStr);
}