#include <MIDIUSB.h>
#include <AceButton.h>
using namespace ace_button;

const uint8_t BUTTON_PLAY_PIN = 4;
const uint8_t BUTTON_STOP_PIN = 3;
const uint8_t BUTTON_RECORD_PIN = 2;

// Button buttonPlay(BUTTON_PLAY_PIN);
// Button buttonStop(BUTTON_STOP_PIN);
// Button buttonRecord(BUTTON_RECORD_PIN);

AceButton buttonPlay(BUTTON_PLAY_PIN);
AceButton buttonStop(BUTTON_STOP_PIN);
AceButton buttonRecord(BUTTON_RECORD_PIN);

// MMC Messages for ID 127 / 0x7F (All Devices)
const uint8_t SYSEX_PLAY[6] = {0xF0, 0x7F, 0x7F, 0x06, 0x02, 0xF7}; // 02 - Play
const uint8_t SYSEX_STOP[6] = {0xF0, 0x7F, 0x7F, 0x06, 0x01, 0xF7}; // 01 - Stop
const uint8_t SYSEX_REC[6] = {0xF0, 0x7F, 0x7F, 0x06, 0x06, 0xF7};  // 06 - Record
const uint8_t MMC_SYSEX_SIZE = 6;

void handleButtons(AceButton *button, uint8_t eventType, uint8_t buttonState);
void handlePlayButton(AceButton *button, uint8_t eventType, uint8_t buttonState);
void handleStopButton(AceButton *button, uint8_t eventType, uint8_t buttonState);
void handleRecordButton(AceButton *button, uint8_t eventType, uint8_t buttonState);
void MidiUSB_controlChange(uint8_t channel, uint8_t control, uint8_t value);
void MidiUSB_sendSysEx(const uint8_t *data, size_t size);

void setup()
{
    pinMode(BUTTON_PLAY_PIN, INPUT_PULLUP);
    pinMode(BUTTON_STOP_PIN, INPUT_PULLUP);
    pinMode(BUTTON_RECORD_PIN, INPUT_PULLUP);

    ButtonConfig *buttonConfig = ButtonConfig::getSystemButtonConfig();
    buttonConfig->setEventHandler(handleButtons);
    buttonConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
    buttonConfig->setFeature(ButtonConfig::kFeatureSuppressClickBeforeDoubleClick);
    buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterClick);
    buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterDoubleClick);

    Serial.begin(115200);
}

void loop()
{
    buttonPlay.check();
    buttonStop.check();
    buttonRecord.check();
}

void handlePlayButton(AceButton *button, uint8_t eventType, uint8_t buttonState)
{
    switch (eventType)
    {
    case AceButton::kEventClicked:
    case AceButton::kEventReleased:
        MidiUSB_sendSysEx(SYSEX_PLAY, MMC_SYSEX_SIZE);
        MidiUSB.flush();
        break;
    }
}

void handleStopButton(AceButton *button, uint8_t eventType, uint8_t buttonState)
{
    switch (eventType)
    {
    case AceButton::kEventClicked:
    case AceButton::kEventReleased:
        MidiUSB_sendSysEx(SYSEX_STOP, MMC_SYSEX_SIZE);
        MidiUSB.flush();
        break;
    case AceButton::kEventDoubleClicked:
        MidiUSB_controlChange(1, 17, 127);
        MidiUSB.flush();
        break;
    }
}

void handleRecordButton(AceButton *button, uint8_t eventType, uint8_t buttonState)
{
    switch (eventType)
    {
    case AceButton::kEventClicked:
    case AceButton::kEventReleased:
        MidiUSB_sendSysEx(SYSEX_REC, MMC_SYSEX_SIZE);
        MidiUSB.flush();
        break;
    case AceButton::kEventDoubleClicked:
        MidiUSB_controlChange(1, 18, 127);
        MidiUSB.flush();
        break;
    }
}

void handleButtons(AceButton *button, uint8_t eventType, uint8_t buttonState)
{

    switch (button->getPin())
    {
    case BUTTON_PLAY_PIN:
        handlePlayButton(button, eventType, buttonState);
        break;
    case BUTTON_STOP_PIN:
        handleStopButton(button, eventType, buttonState);
        break;
    case BUTTON_RECORD_PIN:
        handleRecordButton(button, eventType, buttonState);
        break;
    }
}

void MidiUSB_controlChange(uint8_t channel, uint8_t control, uint8_t value)
{
    midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
    MidiUSB.sendMIDI(event);
}

void MidiUSB_sendSysEx(const uint8_t *data, size_t size)
{
    if (data == NULL || size == 0)
        return;

    size_t midiDataSize = (size + 2) / 3 * 4;
    uint8_t midiData[midiDataSize];
    const uint8_t *d = data;
    uint8_t *p = midiData;
    size_t uint8_tsRemaining = size;

    while (uint8_tsRemaining > 0)
    {
        switch (uint8_tsRemaining)
        {
        case 1:
            *p++ = 5; // SysEx ends with following single uint8_t
            *p++ = *d;
            *p++ = 0;
            *p = 0;
            uint8_tsRemaining = 0;
            break;
        case 2:
            *p++ = 6; // SysEx ends with following two uint8_ts
            *p++ = *d++;
            *p++ = *d;
            *p = 0;
            uint8_tsRemaining = 0;
            break;
        case 3:
            *p++ = 7; // SysEx ends with following three uint8_ts
            *p++ = *d++;
            *p++ = *d++;
            *p = *d;
            uint8_tsRemaining = 0;
            break;
        default:
            *p++ = 4; // SysEx starts or continues
            *p++ = *d++;
            *p++ = *d++;
            *p++ = *d++;
            uint8_tsRemaining -= 3;
            break;
        }
    }
    MidiUSB.write(midiData, midiDataSize);
}
