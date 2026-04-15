# 🔔 Automated School Bell System

An ESP32-based automatic bell system for schools with real-time clock, schedule management, LED indicators, and web-based control panel.

## 📋 Table of Contents
- [System Overview](#system-overview)
- [Team Members & Contributions](#team-members--contributions)
- [Hardware Requirements](#hardware-requirements)
- [Pin Connections](#pin-connections)
- [Software Features](#software-features)
- [Installation Guide](#installation-guide)
- [API Documentation](#api-documentation)
- [Schedule Management](#schedule-management)
- [Troubleshooting](#troubleshooting)
- [GitHub Collaboration](#github-collaboration)

---

## 🎯 System Overview

This automated bell system replaces traditional mechanical bells with an intelligent electronic system that:
- Rings bells automatically based on pre-set schedule
- Shows real-time clock and next bell time on 1.8" color display
- Provides web interface for remote management
- Stores schedule in non-volatile memory (survives power loss)
- Uses LED indicators for system status

**Microcontroller:** ESP32  
**Display:** ST7735S (160x80 pixels)  
**RTC:** DS3231 (high-precision real-time clock)

 bell, WiFi status
void ringBell(int duration); // Activate buzzer with LED pattern
void heartbeatLED();       // System alive indicator (5 sec blink)
