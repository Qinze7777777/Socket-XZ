#include "private/mywifi.h"
WebServer server(80);
HTTPUpdateServer httpUpdater;
int connectWifi(void)
{
    unsigned char count = 0;
    String ssid = "";
    String password = "";
    if (devconfig.ssid == "" || devconfig.password == "")
    {
        Serial.println("------------ERROR: NO WIFI CONFIG ENTER CONFIG PAGE"); 
        configDev();
        return 0;
    }
    else
    {
        ssid = devconfig.ssid;
        password = devconfig.password;
    }
    // WiFi.mode(WIFI_STA);
    // WiFi.setAutoConnect(true);
    // WiFi.setAutoReconnect(true);
    // WiFi.persistent(true);
    WiFi.begin(ssid, password);
    Serial.print("connecting wifi...");
    while (!WiFi.isConnected())
    {
        count++;
        if (count > 23)
        {
            Serial.println("------------ERROR: WIFI CONNECT FAILED");
            //如果连接失败,将devconfig中的wifi信息清空
            devconfig.ssid = "";
            devconfig.password = "";
            saveDevConfig();
            Serial.println("restart 3 seconds later");
            sleep(3);
            ESP.restart();  
            return 0;
        }
        if (count > 10)
        {
            // WiFi.disconnect(true);
            // WiFi.mode(WIFI_OFF);
            // WiFi.mode(WIFI_STA);
            WiFi.begin(ssid, password, count - 10);
        }
        Serial.print(".");
        delay(500);
    }
    Serial.println("");

    IPAddress myIP = WiFi.localIP();
    Serial.print("IP:");
    Serial.println(myIP);
    return 1;
}

void configDev(void)
{
   
    Serial.println("configDev: APname:wificonfig, APpwd:12345678");
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    WiFi.softAP("wificonfig", "12345678");
    Serial.println("AP setup Done.");

    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address:");
    Serial.println(myIP);

    if (MDNS.begin("wificonfig"))
    {
        Serial.println("MDNS responder started");
    }

    server.on("/", showIndex);
    server.on("/config", HTTP_GET, getConfigFromIndex);
    server.onNotFound(handleNotFound);
    httpUpdater.setup(&server);
    server.begin();
    MDNS.addService("http", "tcp", 80);
    Serial.println("HTTP server started");
    while (1)
    {
        server.handleClient();
        vTaskDelay(pdMS_TO_TICKS(100));
        // MDNS.update();
    }
}


void handleNotFound(void)
{
    String html = "File Not Found\n\n";
    html += "URI: ";
    html += server.uri();
    html += "\nMethod: ";
    html += (server.method() == HTTP_GET) ? "GET" : "POST";
    html += "\nArguments: ";
    html += server.args();
    html += "\n";
    for (uint8_t i = 0; i < server.args(); i++)
    {
        html += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", html);
}

void getConfigFromIndex(void)
{
    devconfig.ssid = server.arg("ssid");
    devconfig.password = server.arg("password");
    devconfig.mqtt_server = server.arg("mqtt_server");
    devconfig.mqtt_port = server.arg("mqtt_port");
    devconfig.mqtt_user = server.arg("mqtt_user");
    devconfig.mqtt_password = server.arg("mqtt_password");
    devconfig.mqtt_topic = server.arg("mqtt_topic");

    devconfig.ssid.trim();
    devconfig.password.trim();
    devconfig.mqtt_server.trim();
    devconfig.mqtt_port.trim();
    devconfig.mqtt_user.trim();
    devconfig.mqtt_password.trim();
    devconfig.mqtt_topic.trim();

    Serial.println("=========get config from index=========");
    Serial.println("ssid:" + devconfig.ssid);
    Serial.println("password:" + devconfig.password);
    Serial.println("mqtt_server:" + devconfig.mqtt_server);
    Serial.println("mqtt_port:" + devconfig.mqtt_port);
    Serial.println("mqtt_user:" + devconfig.mqtt_user);
    Serial.println("mqtt_password:" + devconfig.mqtt_password);
    Serial.println("mqtt_topic:" + devconfig.mqtt_topic);

    saveDevConfig();

    Serial.println("restart 3 seconds later");
    sleep(3);
    ESP.restart();
}

void showIndex(void)
{
    String html = readFile("/index.html");
    html.replace("{ssid}", devconfig.ssid);
    html.replace("{password}", devconfig.password);
    html.replace("{mqtt_server}", devconfig.mqtt_server);
    html.replace("{mqtt_port}", devconfig.mqtt_port);
    html.replace("{mqtt_user}", devconfig.mqtt_user);
    html.replace("{mqtt_password}", devconfig.mqtt_password);
    html.replace("{mqtt_topic}", devconfig.mqtt_topic);

    server.send(200, "text/html", html);
}
