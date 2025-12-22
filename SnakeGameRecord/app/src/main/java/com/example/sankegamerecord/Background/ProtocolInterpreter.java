package com.example.sankegamerecord.Background;

import android.os.Build;

import com.example.sankegamerecord.Adapter.GameRecord;

import java.time.Duration; // 시간 간격 (플레이 시간)을 나타내는 클래스
import java.time.LocalDateTime; // 날짜와 시간 정보를 나타내는 클래스
import java.time.format.DateTimeFormatter; // 날짜/시간 문자열 형식을 지정하는 클래스
import java.time.format.DateTimeParseException; // 날짜/시간 파싱 실패 시 발생하는 예외
import java.util.Objects; // 객체의 null 여부 등을 확인하는 유틸리티

/**
 * 블루투스를 통해 주고받는 통신 프레임(문자열)을 해석(파싱)하거나 생성하는 유틸리티 클래스입니다.
 * 외부 장치(게임 기기 등)와의 데이터 통신 규약(프로토콜)을 처리합니다.
 */
public class ProtocolInterpreter {

    // 생성자를 private으로 선언하여 외부에서 객체를 생성할 수 없도록 합니다.
    // 이 클래스는 모든 메서드가 static처럼 사용되는 유틸리티 클래스임을 의미합니다.
    public ProtocolInterpreter() {
    }
    private static final DateTimeFormatter FORMATTER =
            DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss");
    /**
     * 수신된 게임 기록 프레임 문자열을 GameRecord 객체로 변환(파싱)합니다.
     * 프로토콜 형식: "[날짜 시간] [플레이시간] [성공여부]"
     * 예: "2025-11-13 17:00:00 01:30:500 true"
     *
     * @param Frame 파싱할 기록 프레임 문자열
     * @return GameRecord 객체, 파싱 실패 시 null
     */
    public GameRecord makeRecord(String Frame, String name) {

        // LocalDateTime 파싱을 위한 형식 지정

        try {
            // 프레임을 공백(' ') 기준으로 분리합니다.
            String[] data = Frame.split("\\|");
            // 예상되는 data 배열의 요소:
            // data[0]: [구분자], data[1]: "yyyy-MM-dd HH:mm:ss", data[2]: [플레이시간], data[3]: [점수]
            if (data.length == 4 && data[0].equals("RPL")) {
                LocalDateTime dateTime;
                // 1. 날짜 및 시간 파싱
                try {
                    dateTime = LocalDateTime.parse(data[1], FORMATTER);
                }catch (DateTimeParseException e) {
                    // 날짜/시간 또는 Duration 형식 파싱 실패 시
                    dateTime = LocalDateTime.now().withNano(0);
                }
                // 2. 플레이 시간(Duration) 파싱 (위의 parseDuration 메서드 사용)
                int duration = Integer.parseInt(data[2]);
                int score = Integer.parseInt(data[3]);
                // 3. GameRecord 객체 생성
                // dataTime, success: data[3] (boolean)
                return new GameRecord(name, dateTime, duration, score);
            }

        } catch (ArrayIndexOutOfBoundsException e) {
            // split 결과 배열의 크기가 예상과 다를 때 (프레임 형식이 잘못되었을 때)
            System.err.println("프레임 요소 개수 오류: 예상치 못한 형식입니다.");
            return null;
        }
        return null;
    }

}
