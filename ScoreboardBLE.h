#ifndef SCOREBOARD_BLE_H
#define SCOREBOARD_BLE_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

struct ScoreboardState {
  uint8_t scoreHome = 0;
  uint8_t scoreAway = 0;
  uint8_t inning    = 1;
  bool    isTopFrame = true;
  uint8_t balls     = 0;
  uint8_t strikes   = 0;
  uint8_t outs      = 0;
  bool    invert    = false;
};

class ScoreboardBLE {
public:
  explicit ScoreboardBLE(ScoreboardState &state);

  void begin(const char *deviceName = "ESP32-Scoreboard");
  void syncCharacteristics();
  bool consumeDirty();

private:
  ScoreboardState &state_;
  volatile bool dirty_ = true;

  BLEServer *server_ = nullptr;
  BLEService *service_ = nullptr;

  BLECharacteristic *scoreHomeChar_ = nullptr;
  BLECharacteristic *scoreAwayChar_ = nullptr;
  BLECharacteristic *inningChar_ = nullptr;
  BLECharacteristic *isTopFrameChar_ = nullptr;
  BLECharacteristic *ballsChar_ = nullptr;
  BLECharacteristic *strikesChar_ = nullptr;
  BLECharacteristic *outsChar_ = nullptr;
  BLECharacteristic *invertChar_ = nullptr;

  static uint8_t clampU8(int value, uint8_t minV, uint8_t maxV);
  static int parseCharacteristicValue(BLECharacteristic *characteristic);

  class CharacteristicCallbacks : public BLECharacteristicCallbacks {
  public:
    explicit CharacteristicCallbacks(ScoreboardBLE *parent) : parent_(parent) {}
    void onWrite(BLECharacteristic *characteristic) override;
  private:
    ScoreboardBLE *parent_;
  };
};

#endif