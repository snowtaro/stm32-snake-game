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
com.example.sankegamerecord
├── Adapter
│   ├── BluetoothConnectionAdapter   // Bluetooth SPP connection handling
│   ├── GameRecord                   // Game record data model
│   └── RecordListAdapter            // RecyclerView adapter
│
├── Background
│   ├── GetRecordService             // Background data receiving service
│   └── ProtocolInterpreter          // Incoming data parsing logic
│
├── DataBaseAdapter
│   ├── AbstractDataBaseAdapter      // Database base class
│   ├── RankAdapter                  // Ranking table management
│   └── RecordAdapter                // Record table management
│
├── Screens
│   ├── ButtonTouchEffect            // UI button interaction effect
│   ├── MainActivity                 // Main screen
│   └── RankingActivity              // Ranking screen
│
res
├── anim
│   ├── button_press.xml
│   └── button_release.xml
│
├── drawable
│   ├── button_selector.xml        
│   ├── tab_left_selector.xml        // Changes button color
│   ├── tab_right_selector.xml
│   └── mainlobby.png
│
├── layout
│   ├── activity_main.xml      
│   ├── activity_ranking.xml
│   ├── item_bubble_card.xml        // Card Form for List 
│   └── item_text_row.xml           // Arrange the List

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
