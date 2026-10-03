# OpenSigns

Open-source DIY pitch caller for baseball. A belt-clipped M5StickC Plus2 sends a recorded pitch call over Bluetooth to the catcher's earpiece.

Status: v0.2. It works, but it has only been tested at my desk, outside at about 30 feet, and in the bullpen. It has never been used in a game, and it has real problems, listed below.

OpenSigns is not affiliated with, endorsed by, or connected to PitchCom.

- `firmware/` PlatformIO project for the M5StickC Plus2
- `audio/source/` original recorded pitch calls (fastball, curveball, slider, changeup)
- `tools/wav_to_header.py` turns a WAV recording into the audio file the firmware plays

## How it works

The device on your belt holds the calls. I recorded my own voice saying each pitch, and those four clips live in the M5StickC's memory.

It connects to the earbud over regular Bluetooth audio. That's the same way your phone plays music to your earbuds. Press A to pick the pitch. Press B and it plays that clip down the stream.

The earbud doesn't know anything about pitches. It doesn't store any of the calls. It just plays whatever audio shows up. So any cheap Bluetooth earbud can work, as long as you put its name in `TARGET_NAME`.

Between calls the stream sends silence. That keeps the connection open so the call plays the second you hit send. It's also a big part of why the battery drains (see known issue 3).

After you send a call, the screen only shows "PITCH SENT". The other dugout has all game to stare at your belt, so the screen never shows what you called. Press A to start the next one. It always resets to fastball.

## Parts

About $20 to $40 depending on what you already own.

| Part | What it does | Notes |
|---|---|---|
| M5StickC Plus2 | The sender. Screen, two buttons, Bluetooth, battery. | Sold by M5Stack and on Amazon. Link coming. |
| Bluetooth earbud (V80013) | The catcher hears the call here. | The firmware looks for this model by name. Other earbuds work if you change `TARGET_NAME`. Link coming. |
| Visor clip from an old garage door opener | Holds the device on your belt. | Any small clip works. |
| Small USB battery pack | Recharges the sender between innings. | See known issue 3. Link coming. |

You also need a computer and a USB-C cable that carries data, not just charge.

A note if you want to build your own sender from scratch. That's where I started, with a breadboard, a processor, a battery, and buttons. The board I picked was an ESP32-S3, and it had the wrong type of Bluetooth to connect to earbuds. Earbuds need Bluetooth Classic audio (A2DP). The S3 only has Bluetooth Low Energy, and so do a lot of newer chips like the ESP32-C3. The M5StickC Plus2 runs the original ESP32, which can. That's how I ended up on the M5.

## Build guide

### 1. Install the tools

- [VS Code](https://code.visualstudio.com/)
- The PlatformIO extension, from the Extensions tab inside VS Code
- The CH9102 USB driver from [M5Stack's M5StickC Plus2 page](https://docs.m5stack.com/en/core/M5StickC%20PLUS2). Without it your computer may not see the device when you plug it in.
- [Python](https://www.python.org/) and [ffmpeg](https://ffmpeg.org/), only if you're recording your own calls

### 2. Get the code

Download this repo as a ZIP from the green Code button and unzip it, or clone it with git.

### 3. Record your calls (optional)

The repo comes with my four calls in `audio/source/`. You can use those or record your own.

I recorded my own voice on my computer, in Audacity. Say each pitch clearly and close to the mic. Keep each one short, about a second. Every clip takes up space on the device. Export each one as a WAV and name them `fb.wav`, `cb.wav`, `sl.wav`, and `ch.wav` so they replace mine.

Then from the repo folder, run:

```
python tools/wav_to_header.py audio/source/fb.wav audio/source/cb.wav audio/source/sl.wav audio/source/ch.wav
```

That turns each recording into the file the firmware plays. Record loud. Quiet calls are known issue 2.

### 4. Set your earbud's name

Open `firmware/src/main.cpp` and find this line near the top:

```cpp
static const char* TARGET_NAME = "V80013";
```

Change `V80013` to your earbud's Bluetooth name. That's the name that shows up in your phone's Bluetooth list when you pair it. Part of the name is enough, as long as nothing else nearby has the same text in its name.

### 5. Flash the device

1. In VS Code, open the `firmware` folder. PlatformIO picks up the project from `platformio.ini`.
2. Plug the M5StickC Plus2 into your computer.
3. Click the arrow (Upload) in the PlatformIO toolbar at the bottom of VS Code. The first build downloads everything it needs, so give it a few minutes.
4. When it finishes, the screen shows "OpenSigns v0.2" and then starts looking for the earbud.

### 6. Connect the earbud

1. Turn off Bluetooth on your phone and every other device the earbud has paired with (known issue 4).
2. Put the earbud in pairing mode.
3. Turn on the M5StickC by holding the power button for about 2 seconds. M5Stack calls it Button C. It's not the B button you send with. The top of the screen shows "BT: SCANNING..." until it finds the earbud, then "BT: CONNECTED" and "AUDIO OK".

You can only send a call once it says AUDIO OK.

### 7. Use it

- **A (front button):** cycle through the pitches.
- **B (side button):** send the call. The screen switches to "PITCH SENT".
- **A again:** back to picking. It always starts on fastball.

Before you use it in a game, test it at full distance from the mound to the plate with the device where you'll actually wear it. Mine was hit or miss in the bullpen.

To add a pitch, record it, run it through `tools/wav_to_header.py`, then add a line for it to `PITCHES[]` in `firmware/src/main.cpp` and include its header at the top of the file.

## Known issues in v0.2

These come from testing it outside of games. Each one lists what happens, why, and what you can do about it today.

### 1. Range gets shaky at full mound distance

**What happens:** at my desk it was fine. Outside at about 30 feet the call seemed delayed. In the bullpen at the full 60 feet 6 inches it was hit or miss.

**Why:** the firmware never sets the Bluetooth transmit power, so the chip runs at its default level. The device sits on a belt, so the wearer's body is between it and the catcher. Bluetooth audio also holds a little in a buffer before it plays, and a weak signal probably makes that worse.

**For now:** wear the device on the front of your belt, facing the plate. Test range before the game at full distance.

**Next version:** set transmit power to maximum in firmware and field-test the result.

### 2. Volume is low

**What happens:** the call is hard to hear in the earpiece, especially with crowd and field noise.

**Why:** the recordings are quiet. The loudest point in each clip is around -19 to -21 dB, about a tenth of full volume. They were also recorded as 8-bit, 16 kHz mono, which adds hiss.

**For now:** re-record your calls louder and closer to the mic, then run them through `tools/wav_to_header.py`.

**Next version:** record in 16-bit and have the script normalize every clip to full volume automatically.

### 3. Battery won't last a full game

**What happens:** the sending device needs a battery pack or outlet between innings.

**Why:** the screen stays on at full brightness the whole game, and the Bluetooth audio stream runs nonstop, sending silence between calls.

**For now:** bring a small USB battery pack and charge between innings.

**Next version:** dim or turn off the screen between calls, and only wake the audio stream when sending.

### 4. Other devices steal the earpiece

**What happens:** if a phone or other device the earpiece has paired with has Bluetooth on, the earpiece may connect to it instead.

**Why:** the firmware finds the earpiece by its name, which is hardcoded to one model (`V80013`). Earpieces connect to one device at a time and reconnect to whatever they last paired with.

**For now:** turn off Bluetooth on every other device the earpiece has paired with before the game. To use a different earpiece, change `TARGET_NAME` in `firmware/src/main.cpp`.

**Next version:** a pairing mode on the device, so it pairs with any earpiece and remembers it.

### Also missing

- Only four pitches.
- The Bluetooth link is not encrypted.
- Nothing warns you when the connection drops.

## Is it legal?

It depends on your league. One-way communication is the key point: the device only talks to the catcher, never back. Check your league's rules before using it in a game.

## License

Apache 2.0. See `LICENSE`.
