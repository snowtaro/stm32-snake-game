package com.example.sankegamerecord.DataBaseAdapter;

import android.content.Context;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

import com.example.sankegamerecord.Adapter.GameRecord;

import java.time.Duration; // 시간 간격을 표현하는 자바 클래스 (예: 1분 30초)

import org.json.JSONObject; // JSON 데이터를 다루기 위한 클래스 (데이터 직렬화/역직렬화에 사용)

/**
 * SQLite 데이터베이스 작업을 위한 모든 Adapter 클래스의 공통 기본 클래스 (추상 클래스).
 * 데이터베이스 연결/해제, 테이블 공통 컬럼 정의, 데이터 직렬화/역직렬화(JSON) 등
 * 모든 하위 Adapter가 공통으로 사용할 기능을 정의합니다.
 */
public abstract class AbstractDataBaseAdapter {

    // 데이터베이스 테이블의 공통 컬럼 이름 정의
    protected static final String COLUMN_ID = "id"; // 각 레코드(행)를 구분하는 고유 ID
    protected static final String COLUMN_RECORD = "record"; // 실제 게임 기록 데이터를 JSON 문자열 형태로 저장할 컬럼

    protected final Context context; // 안드로이드 앱의 현재 상태와 환경 정보를 담는 객체
    protected DatabaseHelper dbHelper; // 데이터베이스 생성, 버전 관리, 연결을 도와주는 헬퍼 클래스
    protected SQLiteDatabase database; // 실제 데이터베이스에 명령을 실행하는 데 사용되는 객체

    /**
     * 생성자: DatabaseHelper 객체를 초기화합니다.
     * @param context 앱 컨텍스트
     */
    public AbstractDataBaseAdapter(Context context) {
        this.context = context;
        dbHelper = new DatabaseHelper(context);
    }
    /**
     * 각 구체 DB Adapter 클래스가 자신의 테이블 생성 SQL을 구현하는 메소드입니다.
     *
     * @param db SQLite DB 인스턴스
     */
    protected abstract void onCreateTable(SQLiteDatabase db);


    /**
     * 데이터베이스를 열고 쓰기 가능한 연결을 설정합니다.
     * @return 현재 어댑터 인스턴스
     * @throws SQLException 데이터베이스를 열지 못했을 때 발생
     */
    public AbstractDataBaseAdapter open() throws SQLException {
        // 데이터베이스를 열거나, 없으면 생성하고, 쓰기 가능한 SQLiteDatabase 객체를 가져옵니다.
        database = dbHelper.getWritableDatabase();
        return this;
    }

    /**
     * 데이터베이스 연결을 닫고 리소스를 해제합니다.
     */
    public void close() {
        if (dbHelper != null) dbHelper.close();
    }

    // -----------------------------
    // 공통 함수 (데이터 직렬화/역직렬화 및 포맷팅)
    // -----------------------------

    /**
     * GameRecord 객체를 데이터베이스 저장을 위해 JSON 문자열로 변환합니다. (직렬화)
     * @param gr 변환할 GameRecord 객체 (외부에서 정의된 것으로 추정)
     * @return JSON 형식의 문자열
     */
    protected String gameRecordToJson(GameRecord gr) {
        try {
            JSONObject json = new JSONObject();
            json.put("player",gr.Player());
            json.put("playdate", gr.Playdate().toString()); // 게임 플레이 날짜 및 시간
            // Duration을 밀리초(long)로 변환하여 저장 (데이터베이스에 저장하기 쉬운 형태로)
            json.put("playtime", gr.Playtime().toMillis());
            json.put("success", gr.Success()); // 성공 여부
            return json.toString();
        } catch (Exception e) {
            e.printStackTrace();
            return "{}"; // 오류 발생 시 빈 JSON 반환
        }
    }

    /**
     * 데이터베이스에서 읽어온 JSON 문자열을 GameRecord 객체로 복원합니다. (역직렬화)
     * @param jsonStr 복원할 JSON 문자열
     * @return 복원된 GameRecord 객체, 오류 시 null
     */
    protected GameRecord parseGameRecord(String jsonStr) {
        try {
            JSONObject json = new JSONObject(jsonStr);
            String name=json.getString("player");
            // 저장된 문자열을 LocalDateTime 객체로 다시 파싱
            java.time.LocalDateTime playdate = java.time.LocalDateTime.parse(json.getString("playdate"));
            // 저장된 밀리초 값을 Duration 객체로 다시 변환
            Duration playtime = Duration.ofMillis(json.getLong("playtime"));
            Boolean success = json.getBoolean("success");
            return new GameRecord(name, playdate, playtime, success); // 새 GameRecord 객체 생성 (외부 정의된 생성자 사용)
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    /**
     * Duration(시간 간격) 객체를 "분:초.밀리초" 형태의 보기 좋은 문자열로 포맷팅합니다.
     * @param duration 포맷팅할 Duration 객체
     * @return 포맷팅된 시간 문자열 (예: "01:30.123")
     */
    protected String formatDuration(Duration duration){
        long minutes = duration.toMinutes(); // 전체 분 계산
        long seconds = duration.minusMinutes(minutes).getSeconds(); // 분을 제외한 나머지 초 계산
        long millis = duration.minusMinutes(minutes).minusSeconds(seconds).toMillis(); // 분, 초를 제외한 나머지 밀리초 계산
        return String.format("%02d:%02d.%03d", minutes, seconds, millis); // 포맷 적용 (두 자리 수, 세 자리 수)
    }

    /**
     * 특정 테이블의 모든 레코드(행)를 조회합니다.
     * @param tableName 조회할 테이블 이름
     * @return 조회된 레코드를 가리키는 Cursor 객체
     */
    protected Cursor getAllRecords(String tableName) {
        // database.query()를 사용하여 SELECT * FROM tableName ORDER BY id 쿼리를 실행합니다.
        // 정렬 기준은 COLUMN_ID (id)입니다.
        return database.query(tableName, null, null, null, null, null, COLUMN_ID);
    }

    // -----------------------------
    // DBHelper (데이터베이스 생성 및 버전 관리)
    // -----------------------------

    /**
     * SQLiteOpenHelper를 상속받아 데이터베이스의 생성과 업그레이드를 관리하는 내부 헬퍼 클래스입니다.
     */
    protected class DatabaseHelper extends SQLiteOpenHelper {
        private static final String DATABASE_NAME = "myapp.db"; // 데이터베이스 파일 이름
        private static final int DATABASE_VERSION = 5; // 데이터베이스 버전 번호 (스키마 변경 시 증가)

        /**
         * 헬퍼 생성자. 데이터베이스 파일 이름과 버전을 설정합니다.
         */
        public DatabaseHelper(Context context) {
            super(context, DATABASE_NAME, null, DATABASE_VERSION);
        }

        /**
         * 데이터베이스 파일이 처음 생성될 때 딱 한 번 호출됩니다.
         * 테이블을 생성하는 SQL 문을 실행합니다.
         * @param db 테이블 생성을 수행할 SQLiteDatabase 객체
         */
        @Override
        public void onCreate(SQLiteDatabase db) {
            new RecordAdapter(context).onCreateTable(db);
            new RankAdapter(context).onCreateTable(db);
        }

        /**
         * 데이터베이스 버전이 변경될 때 (newVersion > oldVersion) 호출됩니다.
         * 기존 테이블을 제거하고 (DROP TABLE IF EXISTS), onCreate()를 다시 호출하여
         * 새 스키마로 테이블을 다시 생성합니다. (간단한 버전 업그레이드 전략)
         * @param db 업그레이드를 수행할 SQLiteDatabase 객체
         * @param oldVersion 이전 버전 번호
         * @param newVersion 새로운 버전 번호
         */
        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            // 기존 테이블들을 삭제
            db.execSQL("DROP TABLE IF EXISTS " + RecordAdapter.TABLE_NAME);
            db.execSQL("DROP TABLE IF EXISTS " + RankAdapter.TABLE_NAME);
            // 새 버전으로 테이블 다시 생성
            onCreate(db);
        }
    }
}