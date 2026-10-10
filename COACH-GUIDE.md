# OpenSigns for Coaches: Build One Without Knowing Code

This one is for the coach who has never written a line of code and has never heard of GitHub, but wants a pitch caller of their own. If you can start a chat with an AI, you can do this. That's how I did it.

You won't write any code. You'll download my files, change one line, and send it to the device. The AI chat is your assistant coach for anything that confuses you.

## What You're Building

A little stick on your belt holds your pitch calls. You pick a pitch, press a button, and your catcher hears it in a cheap Bluetooth earbud. It only goes one way, from you to the catcher.

Before you buy anything, check your league's rules. Some leagues allow one-way devices like this and some don't.

## What to Buy

- **An [M5StickC Plus2](https://www.amazon.com/s?k=M5StickC+Plus2).** About $20 to $30, depending on where you find one. It has to be the Plus2. The newer M5StickS3 looks similar but can't send audio to an earbud. M5Stack stopped making the Plus2, so you may have to look around for one.
- **A cheap Bluetooth earbud.** A single one is fine, [like this one](https://a.co/d/09H72HIC).
- **A USB-C cable that carries data.** Some cables only charge. If your computer doesn't see the device later, try a different cable first.
- **A small USB battery pack** to charge it between innings.
- **A clip** to hold it on your belt. I used the visor clip off my garage door opener.

You also need a computer, Windows or Mac.

## Set Up Your AI Helper

Open whatever AI chat you like and paste this in to start:

> I'm a baseball coach building OpenSigns, a DIY pitch caller. The files and build guide are at github.com/Morain-Labs/opensigns. I have never coded and I don't know what GitHub is. I'm on [Windows or Mac]. Walk me through the build guide one step at a time. Wait for me to say "done" before the next step. Don't change the plan or suggest different parts.

That last line matters. When I built mine, the AI drifted off my plan and played made-up beeps instead of my recordings. If it starts suggesting new parts or a different way, tell it to go back to the build guide.

Anytime you get stuck, take a screenshot of what's on your screen and give it to the chat. Same with any error message. Copy it and paste it in.

## The Steps

These follow the full build guide on GitHub. GitHub is just a website where the files live. You don't need an account.

### 1. Install three programs

- **VS Code.** A free program from Microsoft for opening code files. Download it from code.visualstudio.com.
- **PlatformIO.** An add-on inside VS Code that sends the code to the device. In VS Code, open the Extensions tab, search for PlatformIO, and install it.
- **The USB driver** from M5Stack's M5StickC Plus2 page. Without it, your computer may not see the device when you plug it in.

### 2. Download the files

Go to github.com/Morain-Labs/opensigns. Click the green Code button, then Download ZIP. Unzip it somewhere easy to find, like your desktop.

### 3. Use my calls (or not)

The download comes with my voice saying fastball, curveball, slider, and changeup. Use those to start. Get the device working first. Recording your own calls is a later project, and the build guide covers it when you're ready.

### 4. Change the earbud name

This is the one line you change. The device looks for your earbud by its Bluetooth name.

To find the name, pair the earbud with your phone once and look at what it's called in your Bluetooth list. Then in VS Code, open the `firmware` folder, then `src`, then `main.cpp`. Near the top is a line with `TARGET_NAME` and the name of my earbud in quotes. Replace what's inside the quotes with your earbud's name. Keep the quotes. Save the file.

If you're nervous about it, paste the line into your AI chat and ask it to show you exactly what it should look like with your earbud's name.

### 5. Send it to the device

Plug the stick into your computer. In VS Code, open the `firmware` folder. At the bottom of the window there's a small arrow for Upload. Click it. The first time takes a few minutes while it downloads what it needs. When it's done, the stick's screen says "OpenSigns v0.2."

### 6. Connect the earbud

Turn off Bluetooth on your phone first, or the earbud will grab your phone instead. Put the earbud in pairing mode. Turn on the stick by holding its power button for about 2 seconds. Wait until the screen says "AUDIO OK."

### 7. Call a pitch

- The front button cycles through the pitches.
- The side button sends the call. The screen just says "PITCH SENT."
- Press the front button again for the next pitch. It always starts back at fastball.

## Before Game Day

Test it at the full 60 feet 6 inches, with the stick where you'll actually wear it. Mine is hit or miss at full distance. Wear it on the front of your belt facing the plate, record your calls loud, and bring the battery pack. The other known problems are listed on GitHub.

## One More Thing

PitchCom holds patents on pitch-calling systems. I read them before sharing this, and I don't believe OpenSigns uses what they cover. That's my read, not legal advice. If you change it or build something different, check the patents first, and don't build, use, or sell anything that infringes them. OpenSigns isn't affiliated with PitchCom.

Figure it out. That's the whole point.
