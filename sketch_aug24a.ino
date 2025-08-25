#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>

// Replace with your WiFi credentials
const char* ssid = "your_ssid_here";        // Example: "MyHomeWiFi"
const char* password = "your_password";     // Example: "mypassword123"

const char* auth_user = "your_login";
const char* auth_pass = "your_password";


WebServer server(8080);

// Simple authentication check
bool authenticate() {
  if (strlen(auth_pass) == 0) {
    return true; // No authentication if password is empty
  }
  if (!server.authenticate(auth_user, auth_pass)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

// Enhanced proxy with better URL handling
void handleProxy() {
  if (!authenticate()) return;
  
  String url = server.arg("url");
  if (url == "") {
    server.send(400, "text/plain", "Missing URL parameter");
    return;
  }
  
  // Smart URL handling using indexOf instead of contains
  if (!url.startsWith("http://") && !url.startsWith("https://")) {
    // Try HTTPS first for better compatibility
    if (url.indexOf("google") >= 0 || url.indexOf("youtube") >= 0 || url.indexOf("facebook") >= 0) {
      url = "https://" + url;
    } else {
      url = "http://" + url;
    }
  }
  
  Serial.println("Fetching: " + url);
  
  HTTPClient http;
  http.begin(url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(15000); // 15 second timeout
  
  // Better headers to mimic real browser
  http.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36");
  http.addHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8");
  http.addHeader("Accept-Language", "en-US,en;q=0.5");
  http.addHeader("Accept-Encoding", "identity"); // Don't use compression
  http.addHeader("DNT", "1");
  http.addHeader("Connection", "keep-alive");
  http.addHeader("Upgrade-Insecure-Requests", "1");
  
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    String payload = http.getString();
    String contentType = http.header("Content-Type");
    
    Serial.println("Success! Got " + String(payload.length()) + " bytes");
    
    // Enhanced URL rewriting
    String proxyBase = "http://" + WiFi.localIP().toString() + ":8080/proxy?url=";
    String originalDomain = extractDomain(url);
    
    // Fix relative URLs
    payload.replace("href=\"/", "href=\"" + proxyBase + originalDomain + "/");
    payload.replace("src=\"/", "src=\"" + proxyBase + originalDomain + "/");
    payload.replace("action=\"/", "action=\"" + proxyBase + originalDomain + "/");
    
    // Fix relative URLs without quotes  
    payload.replace("href=/", "href=" + proxyBase + originalDomain + "/");
    payload.replace("src=/", "src=" + proxyBase + originalDomain + "/");
    
    // Fix protocol-relative URLs
    payload.replace("href=\"//", "href=\"" + proxyBase + "https://");
    payload.replace("src=\"//", "src=\"" + proxyBase + "https://");
    
    // Inject custom JavaScript for better functionality
    String jsInjection = "<script>";
    jsInjection += "document.addEventListener('DOMContentLoaded', function() {";
    jsInjection += "var forms = document.querySelectorAll('form');";
    jsInjection += "forms.forEach(function(form) {";
    jsInjection += "if (form.action && form.action.indexOf('proxy?url=') === -1) {";
    jsInjection += "var originalAction = form.action;";
    jsInjection += "if (originalAction.indexOf('/') === 0) originalAction = '" + originalDomain + "' + originalAction;";
    jsInjection += "form.action = '" + proxyBase + "' + encodeURIComponent(originalAction);";
    jsInjection += "}});";
    jsInjection += "var links = document.querySelectorAll('a');";
    jsInjection += "links.forEach(function(link) {";
    jsInjection += "if (link.href && link.href.indexOf('proxy?url=') === -1 && (link.href.indexOf('http') === 0 || link.href.indexOf('/') === 0)) {";
    jsInjection += "var originalHref = link.href;";
    jsInjection += "if (originalHref.indexOf('/') === 0) originalHref = '" + originalDomain + "' + originalHref;";
    jsInjection += "link.href = '" + proxyBase + "' + encodeURIComponent(originalHref);";
    jsInjection += "}});";
    jsInjection += "});";
    jsInjection += "</script>";
    
    // Insert the JavaScript before closing </head> or at the end of <body>
    if (payload.indexOf("</head>") > 0) {
      payload.replace("</head>", jsInjection + "</head>");
    } else if (payload.indexOf("</body>") > 0) {
      payload.replace("</body>", jsInjection + "</body>");
    } else {
      payload += jsInjection;
    }
    
    server.send(httpCode, contentType, payload);
  } else {
    Serial.println("Failed to fetch URL. Error: " + String(httpCode));
    String errorMsg = "Failed to fetch URL: " + String(httpCode);
    errorMsg += "\n\nTrying to access: " + url;
    errorMsg += "\n\nSuggestions:";
    errorMsg += "\n- Try: httpforever.com";
    errorMsg += "\n- Try: neverssl.com";
    errorMsg += "\n- Try: example.com";
    errorMsg += "\n- Check if site supports HTTP (not just HTTPS)";
    server.send(500, "text/plain", errorMsg);
  }
  
  http.end();
}

// Extract domain from URL
String extractDomain(String url) {
  if (url.startsWith("http://")) {
    url = url.substring(7);
  } else if (url.startsWith("https://")) {
    url = url.substring(8);
  }
  
  int slashIndex = url.indexOf('/');
  if (slashIndex > 0) {
    url = url.substring(0, slashIndex);
  }
  
  return url.startsWith("http") ? url : "http://" + url;
}

// Enhanced web interface
void handleRoot() {
  if (!authenticate()) return;
  
  String html = "<!DOCTYPE html>";
  html += "<html><head>";
  html += "<title>ESP32 Enhanced Proxy</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "* { margin: 0; padding: 0; box-sizing: border-box; }";
  html += "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; padding: 20px; }";
  html += ".container { max-width: 800px; margin: 0 auto; background: rgba(255,255,255,0.95); backdrop-filter: blur(10px); border-radius: 20px; padding: 30px; box-shadow: 0 8px 32px rgba(0,0,0,0.1); }";
  html += "h1 { color: #333; text-align: center; margin-bottom: 30px; font-size: 28px; }";
  html += ".status-card { background: linear-gradient(45deg, #4CAF50, #45a049); color: white; padding: 20px; border-radius: 15px; margin-bottom: 25px; text-align: center; }";
  html += ".input-group { margin-bottom: 20px; }";
  html += "label { display: block; margin-bottom: 8px; color: #333; font-weight: 600; }";
  html += "input[type='text'] { width: 100%; padding: 15px; border: 2px solid #e0e0e0; border-radius: 10px; font-size: 16px; transition: border-color 0.3s; }";
  html += "input[type='text']:focus { outline: none; border-color: #667eea; }";
  html += ".btn-primary { background: linear-gradient(45deg, #667eea, #764ba2); color: white; border: none; padding: 15px 30px; border-radius: 10px; font-size: 16px; font-weight: 600; cursor: pointer; transition: transform 0.2s; width: 100%; margin-bottom: 15px; }";
  html += ".btn-primary:hover { transform: translateY(-2px); box-shadow: 0 5px 15px rgba(0,0,0,0.2); }";
  html += ".btn-test { background: #28a745; color: white; border: none; padding: 8px 15px; border-radius: 5px; font-size: 12px; cursor: pointer; margin: 3px; }";
  html += ".btn-test:hover { background: #218838; }";
  html += ".info-section { background: #f8f9fa; padding: 20px; border-radius: 10px; margin: 20px 0; }";
  html += ".info-section h3 { color: #495057; margin-bottom: 15px; }";
  html += ".feature-list { list-style: none; }";
  html += ".feature-list li { padding: 5px 0; color: #6c757d; }";
  html += ".feature-list li::before { content: '✓ '; color: #28a745; font-weight: bold; }";
  html += "#result { margin-top: 20px; }";
  html += ".loading { text-align: center; padding: 20px; color: #667eea; }";
  html += ".error { background: #f8d7da; color: #721c24; padding: 15px; border-radius: 8px; border: 1px solid #f5c6cb; }";
  html += "</style></head><body>";
  
  html += "<div class='container'>";
  html += "<h1>ESP32 Enhanced Web Proxy</h1>";
  
  html += "<div class='status-card'>";
  html += "<h3>Proxy Status: Online</h3>";
  html += "<p><strong>Home IP:</strong> " + WiFi.localIP().toString() + "</p>";
  html += "<p><strong>Network:</strong> " + String(ssid) + "</p>";
  html += "<p><strong>Uptime:</strong> " + String(millis()/1000) + " seconds</p>";
  html += "</div>";
  
  html += "<div class='input-group'>";
  html += "<label for='urlInput'>Enter Website URL:</label>";
  html += "<input type='text' id='urlInput' placeholder='google.com, youtube.com, news.ycombinator.com' required>";
  html += "</div>";
  
  html += "<button class='btn-primary' onclick='browseWebsite()'>Browse Through Home Connection</button>";
  
  html += "<div class='info-section'>";
  html += "<h3>Quick Test Sites:</h3>";
  html += "<button class='btn-test' onclick='testSite(\"httpforever.com\")'>HTTP Forever</button>";
  html += "<button class='btn-test' onclick='testSite(\"neverssl.com\")'>Never SSL</button>";
  html += "<button class='btn-test' onclick='testSite(\"example.com\")'>Example.com</button>";
  html += "<button class='btn-test' onclick='testSite(\"news.ycombinator.com\")'>Hacker News</button>";
  html += "<button class='btn-test' onclick='testSite(\"reddit.com\")'>Reddit</button>";
  html += "</div>";
  
  html += "<div class='info-section'>";
  html += "<h3>Enhanced Features:</h3>";
  html += "<ul class='feature-list'>";
  html += "<li>Automatic URL rewriting for better navigation</li>";
  html += "<li>JavaScript injection for dynamic link fixing</li>";
  html += "<li>Smart HTTPS/HTTP detection</li>";
  html += "<li>Enhanced browser headers</li>";
  html += "<li>Form submission handling</li>";
  html += "<li>Improved error messages</li>";
  html += "</ul>";
  html += "</div>";
  
  html += "<div class='info-section'>";
  html += "<h3>Access Information:</h3>";
  html += "<p><strong>Local Access:</strong> http://" + WiFi.localIP().toString() + ":8080</p>";
  html += "<p><strong>External Access:</strong> Setup port forwarding on your router</p>";
  html += "<p><strong>Authentication:</strong> " + String(strlen(auth_pass) > 0 ? "Enabled" : "Disabled") + "</p>";
  html += "</div>";
  
  html += "<div id='result'></div>";
  html += "</div>";
  
  // Enhanced JavaScript
  html += "<script>";
  html += "function browseWebsite() {";
  html += "const url = document.getElementById('urlInput').value.trim();";
  html += "if (!url) { alert('Please enter a website URL'); return; }";
  html += "loadSite(url);";
  html += "}";
  
  html += "function testSite(url) {";
  html += "document.getElementById('urlInput').value = url;";
  html += "loadSite(url);";
  html += "}";
  
  html += "function loadSite(url) {";
  html += "const result = document.getElementById('result');";
  html += "result.innerHTML = '<div class=\"loading\">Loading ' + url + ' through your home connection...</div>';";
  html += "fetch('/proxy?url=' + encodeURIComponent(url))";
  html += ".then(response => {";
  html += "if (!response.ok) throw new Error('HTTP ' + response.status);";
  html += "return response.text();";
  html += "})";
  html += ".then(data => {";
  html += "result.innerHTML = '<iframe src=\"data:text/html;charset=utf-8,' + encodeURIComponent(data) + '\" width=\"100%\" height=\"700px\" style=\"border: 2px solid #667eea; border-radius: 10px;\"></iframe>';";
  html += "})";
  html += ".catch(error => {";
  html += "result.innerHTML = '<div class=\"error\">Error loading ' + url + ': ' + error.message + '<br><br>Try: httpforever.com or neverssl.com for testing</div>';";
  html += "});";
  html += "}";
  
  html += "document.getElementById('urlInput').addEventListener('keypress', function(e) {";
  html += "if (e.key === 'Enter') browseWebsite();";
  html += "});";
  html += "</script>";
  
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

// Status endpoint with more info
void handleStatus() {
  if (!authenticate()) return;
  
  String status = "{";
  status += "\"device\": \"ESP32 Enhanced Proxy\",";
  status += "\"version\": \"2.0\",";
  status += "\"ip\": \"" + WiFi.localIP().toString() + "\",";
  status += "\"network\": \"" + String(ssid) + "\",";
  status += "\"signal\": " + String(WiFi.RSSI()) + ",";
  status += "\"uptime\": " + String(millis()) + ",";
  status += "\"freeHeap\": " + String(ESP.getFreeHeap()) + ",";
  status += "\"totalHeap\": " + String(ESP.getHeapSize()) + ",";
  status += "\"authentication\": " + String(strlen(auth_pass) > 0 ? "true" : "false");
  status += "}";
  
  server.send(200, "application/json", status);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=== ESP32 Enhanced Web Proxy v2.0 ===");
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi Connected!");
    Serial.print("Local Access: http://");
    Serial.print(WiFi.localIP());
    Serial.println(":8080");
    Serial.println("Authentication: " + String(strlen(auth_pass) > 0 ? "Enabled" : "Disabled"));
    Serial.println("Enhanced features enabled!");
  } else {
    Serial.println();
    Serial.println("WiFi connection failed!");
    Serial.println("Please check your credentials and try again.");
    return;
  }
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/proxy", handleProxy);
  server.on("/status", handleStatus);
  
  // Handle CORS and 404
  server.onNotFound([]() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(404, "text/plain", "404: Page not found");
  });
  
  server.begin();
  Serial.println("Enhanced web proxy server started!");
  Serial.println("Memory usage: " + String(ESP.getFreeHeap()) + " bytes free");
}

void loop() {
  server.handleClient();
  
  // Connection monitoring
  static unsigned long lastCheck = 0;
  static unsigned long lastMemoryReport = 0;
  
  if (millis() - lastCheck > 30000) {  // Every 30 seconds
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected, reconnecting...");
      WiFi.reconnect();
    }
    lastCheck = millis();
  }
  
  // Memory usage report every 5 minutes
  if (millis() - lastMemoryReport > 300000) {
    Serial.println("Free memory: " + String(ESP.getFreeHeap()) + " bytes");
    lastMemoryReport = millis();
  }
  
  delay(10);
}
