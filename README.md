# Sanke Game Record System

## Overview
This project is an Android application that receives game play data from an STM32-based embedded system via Bluetooth SPP communication.

The received data includes:
- Game play date
- Play duration
- Score

Upon receiving the data, the application displays a notification, prompts the player to enter their name, and stores the game record locally.  
If no name is entered, a default or previously used name is applied automatically.

The system maintains two separate database tables: one for recent game records and another for ranking management.

---

## System Flow
1. STM32 sends game data via Bluetooth SPP
2. Android app(service) receives and parses the data
3. Heads Up Notification is displayed immediately
4. Game record is temporarily stored in a stack
5. Player name is requested and timeout is 20 seconds
   - If input is provided → use input
   - If not → use default or last-used name
6. Stack is flushed into database after processing
7. Records are stored and managed based on table policy

---

## Tech Stack
### Hardware
- STM32
- Bluetooth Module (SPP)

### Software
- Android (Java)
- SQLite (Local Database)

---

## Project Structure //only important code
### Project Structure

com.example.sankegamerecord <br />
├── Adapter <br />
│   ├── BluetoothConnectionAdapter # Bluetooth SPP connection handling <br />
│   ├── GameRecord # Game record data model <br />
│   └── RecordListAdapter # RecyclerView adapter <br />
├── Background <br />
│   ├── GetRecordService # Background data receiving service <br />
│   └── ProtocolInterpreter # Incoming data parsing logic <br />
├── DataBaseAdapter <br />
│   ├── AbstractDataBaseAdapter # Database base class <br />
│   ├── RecordAdapter # FIFO record table (max 20) <br />
│   └── RankAdapter # Ranking table (sorted insertion) <br />
├── Screens <br />
│   ├── MainActivity # Main screen <br />
│   ├── RankingActivity # Ranking screen <br />
│   └── ButtonTouchEffect # UI button interaction effect <br />
└── res <br />
    ├── anim <br />
    │   ├── button_press.xml <br />
    │   └── button_release.xml <br />
    ├── drawable <br />
    │   ├── button_selector.xml <br />
    │   ├── tab_left_selector.xml <br />
    │   ├── tab_right_selector.xml <br />
    │   └── mainlobby.png <br />
    └── layout <br />
        ├── activity_main.xml <br />
        ├── activity_ranking.xml <br />
        ├── item_bubble_card.xml <br />
        └── item_text_row.xml <br />

---

## Communication Protocol
The embedded system sends structured game data via Bluetooth SPP.
 - HEARTBEAT  Trash packet to maintain the connection
 - RPL  Valid packet that contains data
 - Broken packethead  Ignore
 
### Field	Description
 - dateTime	Game play date
 - duration	Play duration
 - score	Final score

The Android application interprets incoming packets using a protocol interpreter before processing.

---

## Database Design
Two separate tables are used:

### Record Table (History)
 - Stores recent game records
 - FIFO policy
 - Maximum of 20 records
 - Oldest records are removed automatically

### Rank Table (Leaderboard)
 - Stores records with high score.
 - Insert → sort → eliminate strategy
 - Maximum of 5 records
 - Maintains ordered leaderboard based on score

---

## Android Application Features
 - Bluetooth SPP data reception
 - Real-time notification on data arrival
 - Player name input handling
 - Automatic default name assignment
 - Record history view
 - Ranking view

---

## How to Run
### Android
1. Clone the repository
2. Open the project in Android Studio
3. Enable Bluetooth on the device
4. Build and run the application

### Embedded
1. Flash STM32 firmware with Bluetooth SPP enabled
2. Ensure correct data format is transmitted



Bug report is welcome! Tell me where the app crashes!
