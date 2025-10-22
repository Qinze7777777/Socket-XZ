#include "private/ota.h"

void getOTA(void)
{
    HTTPClient http;
    String mac_addr;
    IPAddress myIP;
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi 未连接");
        return;
    }
    myIP = WiFi.localIP();
    mac_addr = WiFi.macAddress();
    mac_addr = "f1:82:c1:24:25:12"; //指定mac地址
    http.begin(OTA_URL);
    http.addHeader("Device-Id", mac_addr);
    http.addHeader("Content-Type", "application/json");
    

    // 构建 JSON 请求
    StaticJsonDocument<1024> doc;
    // 填充 payload
    doc["flash_size"] = 16777216;  // 闪存大小 (16MB)
    doc["minimum_free_heap_size"] = 8318916;  // 最小可用堆内存
    doc["mac_address"] = mac_addr;  // 示例 MAC 地址
    doc["chip_model_name"] = "esp32s3";  // 芯片型号

    // chip_info
    JsonObject chip_info = doc.createNestedObject("chip_info");
    chip_info["model"] = 9;
    chip_info["cores"] = 2;
    chip_info["revision"] = 2;
    chip_info["features"] = 18;

    // application
    JsonObject application = doc.createNestedObject("application");
    application["name"] = "xiaozhi";

    // partition_table (空数组)
    JsonArray partition_table = doc.createNestedArray("partition_table");

    // ota
    JsonObject ota = doc.createNestedObject("ota");
    ota["label"] = "factory";

    // board
    JsonObject board = doc.createNestedObject("board");
    board["type"] = "bread-compact-wifi";
    board["ip"] = myIP; 
    board["mac"] = mac_addr;  
    // 序列化 JSON
    String payload;
    serializeJson(doc, payload);

    // 发送 POST 请求
    int httpResponseCode = http.POST(payload);


    String response;
    // 解析返回的数据
    if (httpResponseCode == 200) 
    {
        response = http.getString();
        Serial.println("OTA 服务器返回数据:");
        Serial.println(response);
    } 
    else 
    {
        Serial.printf("OTA 服务器错误: HTTP %d\n", httpResponseCode);
    }
    //解析respondse
    doc.clear();
    deserializeJson(doc, response);
    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
        Serial.println("Failed to deserializeJson str");
        return;
    }
    verifycode = doc["activation"]["code"].as<String>();
    mqttinfo = doc["mqtt"].as<String>();
    Serial.println(verifycode);
    Serial.println(mqttinfo);
    if (verifycode.length() == 6)
    {
        Serial.println("OTA 服务器返回code,10s后继续执行");
        delay(10000);
    }
    
    http.end();
}