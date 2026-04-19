#include "ScoreboardBLE.h"

//
// Placeholder UUIDs
//
static const char *SERVICE_UUID      = "00000000-0000-0000-0000-000000000100";
static const char *SCORE_HOME_UUID   = "00000000-0000-0000-0000-000000000101";
static const char *SCORE_AWAY_UUID   = "00000000-0000-0000-0000-000000000102";
static const char *INNING_UUID       = "00000000-0000-0000-0000-000000000103";
static const char *TOP_FRAME_UUID    = "00000000-0000-0000-0000-000000000104";
static const char *BALLS_UUID        = "00000000-0000-0000-0000-000000000105";
static const char *STRIKES_UUID      = "00000000-0000-0000-0000-000000000106";
static const char *OUTS_UUID         = "00000000-0000-0000-0000-000000000107";
static const char *INVERT_UUID       = "00000000-0000-0000-0000-000000000108";

ScoreboardBLE::ScoreboardBLE(ScoreboardState &state)
  : state_(state) {}

uint8_t ScoreboardBLE::clampU8(int value, uint8_t minV, uint8_t maxV) {
  if (value < minV) return minV;
  if (value > maxV) return maxV;
  return static_cast<uint8_t>(value);
}

int ScoreboardBLE::parseCharacteristicValue(BLECharacteristic *characteristic) {
  String value = characteristic->getValue().c_str();

  if (value.isEmpty()) return 0;

  if (value.length() == 1 && !isDigit(value[0])) {
    return static_cast<uint8_t>(value[0]);
  }

  return value.toInt();
}

void ScoreboardBLE::CharacteristicCallbacks::onWrite(BLECharacteristic *characteristic) {
  int raw = parseCharacteristicValue(characteristic);

  if (characteristic->getUUID().toString() == SCORE_HOME_UUID) {
    parent_->state_.scoreHome = clampU8(raw, 0, 99);
  } else if (characteristic->getUUID().toString() == SCORE_AWAY_UUID) {
    parent_->state_.scoreAway = clampU8(raw, 0, 99);
  } else if (characteristic->getUUID().toString() == INNING_UUID) {
    parent_->state_.inning = clampU8(raw, 0, 99);
  } else if (characteristic->getUUID().toString() == TOP_FRAME_UUID) {
    parent_->state_.isTopFrame = (raw != 0);
  } else if (characteristic->getUUID().toString() == BALLS_UUID) {
    parent_->state_.balls = clampU8(raw, 0, 3);
  } else if (characteristic->getUUID().toString() == STRIKES_UUID) {
    parent_->state_.strikes = clampU8(raw, 0, 2);
  } else if (characteristic->getUUID().toString() == OUTS_UUID) {
    parent_->state_.outs = clampU8(raw, 0, 2);
  } else if (characteristic->getUUID().toString() == INVERT_UUID) {
    parent_->state_.invert = (raw != 0);
  }

  parent_->dirty_ = true;
  parent_->syncCharacteristics();
}

void ScoreboardBLE::begin(const char *deviceName) {
  BLEDevice::init(deviceName);

  server_ = BLEDevice::createServer();
  service_ = server_->createService(SERVICE_UUID);

  const uint32_t props = BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE;

  scoreHomeChar_ = service_->createCharacteristic(SCORE_HOME_UUID, props);
  scoreAwayChar_ = service_->createCharacteristic(SCORE_AWAY_UUID, props);
  inningChar_ = service_->createCharacteristic(INNING_UUID, props);
  isTopFrameChar_ = service_->createCharacteristic(TOP_FRAME_UUID, props);
  ballsChar_ = service_->createCharacteristic(BALLS_UUID, props);
  strikesChar_ = service_->createCharacteristic(STRIKES_UUID, props);
  outsChar_ = service_->createCharacteristic(OUTS_UUID, props);
  invertChar_ = service_->createCharacteristic(INVERT_UUID, props);

  auto *cb = new CharacteristicCallbacks(this);

  scoreHomeChar_->setCallbacks(cb);
  scoreAwayChar_->setCallbacks(cb);
  inningChar_->setCallbacks(cb);
  isTopFrameChar_->setCallbacks(cb);
  ballsChar_->setCallbacks(cb);
  strikesChar_->setCallbacks(cb);
  outsChar_->setCallbacks(cb);
  invertChar_->setCallbacks(cb);

  syncCharacteristics();

  service_->start();
  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->start();
}

void ScoreboardBLE::syncCharacteristics() {
  scoreHomeChar_->setValue(String(state_.scoreHome).c_str());
  scoreAwayChar_->setValue(String(state_.scoreAway).c_str());
  inningChar_->setValue(String(state_.inning).c_str());
  isTopFrameChar_->setValue(state_.isTopFrame ? "1" : "0");
  ballsChar_->setValue(String(state_.balls).c_str());
  strikesChar_->setValue(String(state_.strikes).c_str());
  outsChar_->setValue(String(state_.outs).c_str());
  invertChar_->setValue(state_.invert ? "1" : "0");
}

bool ScoreboardBLE::consumeDirty() {
  bool wasDirty = dirty_;
  dirty_ = false;
  return wasDirty;
}
