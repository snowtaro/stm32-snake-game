package com.example.sankegamerecord.Adapter;

import java.time.Duration;
import java.time.LocalDateTime;

public record GameRecord(
        LocalDateTime Playdate,
        Duration Playtime,
        Boolean Success
){}
