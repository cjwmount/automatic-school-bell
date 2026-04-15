
---

### 👤 **Sarah: Web Server & API**
**Branch:** `feature/web-server`

```markdown
# Web Server & API Module - School Bell System

## My Role
WiFi connectivity, web dashboard, REST API endpoints, and remote control.

## Files I Created
- `src/network/wifi.cpp/h` - WiFi connection manager
- `src/network/webserver.cpp/h` - HTTP server on port 80
- `src/network/api.cpp/h` - JSON response handlers
- `data/index.html` - Embedded web interface (or PROGMEM string)

## API Endpoints

| Endpoint | Method | Description | Response |
|----------|--------|-------------|----------|
| `/` | GET | Web dashboard | HTML page |
| `/api/status` | GET | System status | `{time, next, ringing, count}` |
| `/api/schedule` | GET | All bell events | `[{hour,minute,duration}]` |
| `/api/add` | POST | Add schedule | `{status:"ok"}` |
| `/api/delete` | POST | Remove schedule | `{status:"ok"}` |
| `/api/ring` | POST | Manual bell trigger | `{status:"ringing"}` |

## Web Interface Features
- Real-time status updates (auto-refresh every 1 sec)
- Visual bell ringing indicator
- Add/delete schedule buttons
- Manual bell trigger
- Mobile-responsive design

## Functions Implemented
- `initWiFi()` - Connects to configured SSID
- `setupWebServer()` - Registers all route handlers
- `handleClient()` - Process incoming requests (called in loop)
- `getStatusJSON()` - Returns current system state

## Testing
✅ WiFi connects to "Capital" network  
✅ Web dashboard accessible at http://[ESP32-IP]  
✅ API returns valid JSON  
✅ Add/delete schedule via web works  
✅ Manual bell rings from browser  

## Dependencies
- WiFi.h
- WebServer.h
- ArduinoJson (for request parsing)

## Merge Status
⏳ Ready to merge with Person 2's main.cpp - API calls schedule functions
