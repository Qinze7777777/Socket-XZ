#include "private/myfile.h"
SemaphoreHandle_t xMutex = xSemaphoreCreateMutex();

void LittleFS_init(void)
{
    SemaphoreHandle_t xMutex = xSemaphoreCreateMutex();
}
int readFileBytes(const String filepath, char *bytes)
{

    int length;
    if (xSemaphoreTake(xMutex, portMAX_DELAY)) // 或 pdMS_TO_TICKS(xxx)
    {
        if (!LITTLEFS.begin())
        {
            Serial.println("An Error has occurred while mounting LittleFS");
            return 0;
        }

        File file = LITTLEFS.open(filepath, "r");
        if (!file)
        {
            Serial.print("Failed to open file for reading:");
            Serial.println(filepath);
            return 0;
        }
        length = file.size();
        file.readBytes(bytes, length);
        file.close();
        LITTLEFS.end();
        xSemaphoreGive(xMutex);
    }
    return length;
}

// String readFile(const char * path)
// {
//     String message = "";
// 	uint8_t buf[256];
//     Serial.printf("Reading file: %s\r\n", path);
//     if(LITTLEFS.begin(true))
// 	{
//         File file = LITTLEFS.open(path);
// 		if(!file || file.isDirectory()){
// 			Serial.println("- failed to open file for reading");
// 		}
// 		Serial.println("- read from file:");
// 		while(file.available()){
// 			file.read(buf, 256);
//             message += String((char*)buf);
// 		}
// 		file.close();
// 		LITTLEFS.end();
//     }
//     else
//     {
//         Serial.println("Failed to read");
//     }
//     Serial.println(message);
// 	return message;
    
// }
String readFile(const String filepath)
{
    String str = "";
    if (xSemaphoreTake(xMutex, portMAX_DELAY)) // 或 pdMS_TO_TICKS(xxx)
    {
        if (!LITTLEFS.begin())
        {
            Serial.println("An Error has occurred while mounting LittleFS");
            xSemaphoreGive(xMutex);
            return str;
        }

        File file = LITTLEFS.open(filepath, "r");
        if (!file)
        {
            Serial.print("Failed to open file for reading:");
            Serial.println(filepath);
            xSemaphoreGive(xMutex);
            return str;
        }

        str = file.readString();
        file.close();
        LITTLEFS.end();
        xSemaphoreGive(xMutex);
    }
    return str;
}
void writeFile(const char * path, const char * message)
{
    if (xSemaphoreTake(xMutex, portMAX_DELAY)) // 或 pdMS_TO_TICKS(xxx)
    {
        if (LITTLEFS.begin(true))
        {
            // Serial.printf("Writing file: %s\r\n", path);
            File file = LITTLEFS.open(path, FILE_WRITE);
            if(!file){
                Serial.println("- failed to open file for writing");
                return;
            }
            if(file.print(message)){
                // Serial.println("- file written");
            } else {
                // Serial.println("- write failed");
            }
            file.close();
            LITTLEFS.end();
        }
        else
        {
            Serial.println("Failed to write");
        }
        xSemaphoreGive(xMutex);
    }
}
void initDevState(void)
{
    devstate.state = "0";
    devstate.V = "0";
    devstate.I = "0";
    devstate.P = "0";
    devstate.E = "0";
    devstate.socket = "1";
    devstate.timernum = "60";
    devstate.timer = "0";
    devstate.IR_node = "0";
    devstate.screen = "1";
    devstate.brightness = "255";
    devstate.volume = "100";
    saveDevState();
}

void initDevConfig(void) 
{
    devconfig.ssid = "英东楼322";
    devconfig.password = "12345678";
    devconfig.mqtt_server = "192.168.11.100";
    devconfig.mqtt_port = "1883";
    devconfig.mqtt_user = "mqtt";
    devconfig.mqtt_password = "123456==";
    devconfig.mqtt_topic = "energy";
    saveDevConfig();
}
void saveDevConfig(void)
{
    StaticJsonDocument<1024> doc;
    doc["ssid"] = devconfig.ssid;
    doc["password"] = devconfig.password;
    doc["mqtt_server"] = devconfig.mqtt_server;
    doc["mqtt_port"] = devconfig.mqtt_port;
    doc["mqtt_user"] = devconfig.mqtt_user;
    doc["mqtt_password"] = devconfig.mqtt_password;
    doc["mqtt_topic"] = devconfig.mqtt_topic;

    String str;
    serializeJson(doc, str);
	Serial.println("devconfig:" + str);
    writeFile("/devconfig", str.c_str());
}

void loadDevConfig(void)
{
    String str = readFile("/devconfig");
    StaticJsonDocument<1024> doc;
    if (str.length() < 10)
    {
        initDevConfig();
        str = readFile("/devconfig");
    }
    DeserializationError error = deserializeJson(doc, str);
    if (error)
    {
        Serial.println("Failed to deserializeJson str");
        return;
    }
    Serial.println("devconfig:" + str);
    devconfig.ssid = doc["ssid"] ? doc["ssid"].as<String>() : "";
    devconfig.password = doc["password"] ? doc["password"].as<String>() : "";
    devconfig.mqtt_server = doc["mqtt_server"] ? doc["mqtt_server"].as<String>() : "";
    devconfig.mqtt_port = doc["mqtt_port"] ? doc["mqtt_port"].as<String>() : "";
    devconfig.mqtt_user = doc["mqtt_user"] ? doc["mqtt_user"].as<String>() : "";
    devconfig.mqtt_password = doc["mqtt_password"] ? doc["mqtt_password"].as<String>() : "";
    devconfig.mqtt_topic = doc["mqtt_topic"] ? doc["mqtt_topic"].as<String>() : "";
   // devconfig.matt_clientId = strdup(WiFi.macAddress().c_str());   //以设备mac地址作为clientid
    devconfig.matt_clientId =  "smart_socket";
    
}

void saveDevState(void)
{
    // static String last_E = "";
    // static String last_socket = "";
    // static String last_timernum = "";
    // static String last_timer = "";
    // static String last_screen = "";
    // static String last_brightness = "";
    StaticJsonDocument<256> doc;
    //如果状态变化，则保存
    // if (last_E != devstate.E || last_socket != devstate.socket || last_timernum != devstate.timernum || last_timer != devstate.timer || last_screen != devstate.screen || last_brightness != devstate.brightness)
    // {
        doc["E"] = devstate.E;
        doc["socket"] = devstate.socket;
        doc["timernum"] = devstate.timernum;
        doc["timer"] = devstate.timer;
        doc["IR_node"] = devstate.IR_node;
        doc["screen"] = devstate.screen;
        doc["brightness"] = devstate.brightness;
        doc["volume"] = devstate.volume;
        String str;
        serializeJson(doc, str);
        writeFile("/devstate", str.c_str());
    // }
    // last_E = devstate.E;
    // last_socket = devstate.socket;
    // last_timernum = devstate.timernum;
    // last_timer = devstate.timer;
    // last_screen = devstate.screen;
    // last_brightness = devstate.brightness;
}



void loadDevState(void)
{
    String str = readFile("/devstate");
    StaticJsonDocument<1024> doc;
    if (str.length() < 10)
    {
        initDevState();
        str = readFile("/devstate");
    }
    DeserializationError error = deserializeJson(doc, str);
    if (error)
    {
        Serial.println("Failed to deserializeJson str");
        return;
    }
    Serial.println("load devstate:" + str);
    devstate.state = "1";
    devstate.E = doc["E"] ? doc["E"].as<String>() : "";
    devstate.socket = doc["socket"] ? doc["socket"].as<String>() : "";
    devstate.timernum = doc["timernum"] ? doc["timernum"].as<String>() : "";
   // devstate.timer = doc["timer"] ? doc["timer"].as<String>() : "";
    devstate.timer = "0";   //上电不恢复定时器开关
    devstate.IR_node = doc["IR_node"] ? doc["IR_node"].as<String>() : "";
    devstate.screen = doc["screen"] ? doc["screen"].as<String>() : "";
    devstate.brightness = doc["brightness"] ? doc["brightness"].as<String>() : "";
    devstate.volume = doc["volume"] ? doc["volume"].as<String>() : "";
    // //执行上电恢复操作
    // if (devstate.socket == "1")
    // {
    //     setRelay(1);
    // }
    // else
    // {
    //     setRelay(0);
    // }
    
}

void delFile(const char * path)
{
    if(LITTLEFS.begin(true))
	{
        if (LITTLEFS.exists(path))
        {
            LITTLEFS.remove(path);
            Serial.println("Del success");
        }
		LITTLEFS.end();
    }
}
void listDir(const char * dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\r\n", dirname);

    if (LITTLEFS.begin(true))
    {
        File root = LITTLEFS.open(dirname);
        if (!root) 
        {
            Serial.println("- failed to open directory");
            LITTLEFS.end(); // 在这里结束文件系统挂载
            return;
        }
        if (!root.isDirectory())
        {
            Serial.println(" - not a directory");
            LITTLEFS.end(); // 在这里结束文件系统挂载
            return;
        }

        File file = root.openNextFile();
        while (file) 
        {
            if (file.isDirectory())
            {
                Serial.print("  DIR : ");
                Serial.println(file.name());
                if (levels) {
                    listDir(file.path(), levels - 1); // 递归调用
                }
            }
            else 
            {
                Serial.print("  FILE: ");
                Serial.print(file.name());
                Serial.print("\tSIZE: ");
                Serial.println(file.size());
            }
            file = root.openNextFile();
        }

        LITTLEFS.end(); // 在这里结束文件系统挂载
    }
}