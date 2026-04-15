// api_handlers.h/cpp
String getStatusJSON();
String getScheduleJSON();
bool parseAddRequest(String body, BellEvent* event);
