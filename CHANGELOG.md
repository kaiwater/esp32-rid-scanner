# Changelog

本项目按版本记录变更。详细的分版本说明放在 [`changelogs/`](changelogs/)。

## v0.1.0 — 2026-10-31

首个可工作的 ESP32-S3 手持 Remote ID 扫描器。

- ESP32-S3-DevKitC-1 + ST7735S 128x160 + 4 按键硬件方案
- 纯被动 WiFi Beacon / NAN + BLE RID 接收
- 列表 / 三页详情 / RF SCAN 三种界面
- RSSI、噪声底红黄绿标注，高能量/高干扰加 `*`
- 160x128 sprite 双缓冲，消除整屏闪烁
- WiFi Beacon 链路已用 ESP8266 测试源验证；BLE 链路待真实 BLE 源验证

详细说明：[`changelogs/2026-10-31-v0.1.0.md`](changelogs/2026-10-31-v0.1.0.md)
