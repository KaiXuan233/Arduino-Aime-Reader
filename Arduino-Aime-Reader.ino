#include "cmd.h"

#if defined(__AVR_ATmega32U4__) || defined(ARDUINO_SAMD_ZERO)
#pragma message "当前的开发板是 ATmega32U4 或 SAMD_ZERO"
#define SerialDevice SerialUSB
#define LED_PIN A3

#elif defined(ARDUINO_ESP8266_NODEMCU_ESP12E)
#pragma message "当前的开发板是 NODEMCU_ESP12E"
#define SerialDevice Serial
#define LED_PIN D5
//#define SwitchBaudPIN D4 //修改波特率按钮

#elif defined(ARDUINO_NodeMCU_32S)
#pragma message "当前的开发板是 NodeMCU_32S"
#define SerialDevice Serial
#define LED_PIN 13

#else
#error "未经测试的开发板，请检查串口和阵脚定义"
#endif

bool high_baudrate = true;//high_baudrate=true

void setup() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50);
  FastLED.showColor(0);
  nfc.begin();
  while (!nfc.getFirmwareVersion()) {
    FastLED.showColor(0xFF0000);
    delay(500);
    FastLED.showColor(0);
    delay(500);
  }
  nfc.setPassiveActivationRetries(0x10);//设定等待次数
  nfc.SAMConfig();
  memset(&req, 0, sizeof(req.bytes));
  memset(&res, 0, sizeof(res.bytes));

  SerialDevice.begin(high_baudrate ? 115200 : 38400);
  FastLED.showColor(high_baudrate ? 0x0000FF : 0x00FF00);

#ifdef SwitchBaudPIN
#pragma message "已启用波特率切换功能"
  pinMode(SwitchBaudPIN, INPUT_PULLUP);
#endif
}

void loop() {
  SerialCheck();
  packet_write();
#ifdef SwitchBaudPIN
  if (!digitalRead(SwitchBaudPIN)) {
    high_baudrate = !high_baudrate;
    SerialDevice.flush();
    SerialDevice.begin(high_baudrate ? 115200 : 38400);
    FastLED.showColor(high_baudrate ? 0x0000FF : 0x00FF00);
    delay(2000);
  }
#endif
}

static uint8_t len, r, checksum;
static bool escape = false;

static uint8_t packet_read() {

  while (SerialDevice.available()) {
    r = SerialDevice.read();
    if (r == 0xE0) {
      req.frame_len = 0xFF;
      continue;
    }
    if (req.frame_len == 0xFF) {
      req.frame_len = r;
      len = 0;
      checksum = r;
      continue;
    }
    if (r == 0xD0) {
      escape = true;
      continue;
    }
    if (escape) {
      r++;
      escape = false;
    }
    req.bytes[++len] = r;
    if (len == req.frame_len && checksum == r) {
      return req.cmd;
    }
    checksum += r;
  }
  return 0;
}

static void packet_write() {
  uint8_t checksum = 0, len = 0;
  if (res.cmd == 0) {
    return;
  }
  SerialDevice.write(0xE0);
  while (len <= res.frame_len) {
    uint8_t w;
    if (len == res.frame_len) {
      w = checksum;
    } else {
      w = res.bytes[len];
      checksum += w;
    }
    if (w == 0xE0 || w == 0xD0) {
      SerialDevice.write(0xD0);
      SerialDevice.write(--w);
    } else {
      SerialDevice.write(w);
    }
    len++;
  }
  res.cmd = 0;
}

void SerialCheck() {
  switch (packet_read()) {
    case SG_NFC_CMD_RESET:
      sg_nfc_cmd_reset();
      break;
    case SG_NFC_CMD_GET_FW_VERSION:
      sg_nfc_cmd_get_fw_version();
      break;
    case SG_NFC_CMD_GET_HW_VERSION:
      sg_nfc_cmd_get_hw_version();
      break;
    case SG_NFC_CMD_POLL:
      sg_nfc_cmd_poll();
      break;
    case SG_NFC_CMD_MIFARE_READ_BLOCK:
      sg_nfc_cmd_mifare_read_block();
      break;
    case SG_NFC_CMD_FELICA_ENCAP:
      sg_nfc_cmd_felica_encap();
      break;
    case SG_NFC_CMD_AIME_AUTHENTICATE:
      sg_nfc_cmd_aime_authenticate();
      break;
    case SG_NFC_CMD_BANA_AUTHENTICATE:
      sg_nfc_cmd_bana_authenticate();
      break;
    case SG_NFC_CMD_MIFARE_SELECT_TAG:
      sg_nfc_cmd_mifare_select_tag();
      break;
    case SG_NFC_CMD_MIFARE_SET_KEY_AIME:
      sg_nfc_cmd_mifare_set_key_aime();
      break;
    case SG_NFC_CMD_MIFARE_SET_KEY_BANA:
      sg_nfc_cmd_mifare_set_key_bana();
      break;
    case SG_NFC_CMD_RADIO_ON:
      sg_nfc_cmd_radio_on();
      break;
    case SG_NFC_CMD_RADIO_OFF:
      sg_nfc_cmd_radio_off();
      break;
    case SG_RGB_CMD_RESET:
      sg_led_cmd_reset();
      break;
    case SG_RGB_CMD_GET_INFO:
      sg_led_cmd_get_info();
      break;
    case SG_RGB_CMD_SET_COLOR:
      sg_led_cmd_set_color();
      break;
    case 0:
      break;
    default:
      sg_res_init();
  }
}
