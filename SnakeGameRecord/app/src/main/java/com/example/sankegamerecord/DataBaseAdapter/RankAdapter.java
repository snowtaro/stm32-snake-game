package com.example.sankegamerecord.DataBaseAdapter;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.util.Log;

import com.example.sankegamerecord.Adapter.GameRecord;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

/**
 * 게임의 랭킹(순위) 정보를 데이터베이스에 저장하고 관리하는 어댑터 클래스입니다.
 * AbstractDataBaseAdapter를 상속받아 데이터베이스 연결 및 JSON 처리를 공유합니다.
 * 이 클래스는 성공 기록 중 상위 5개의 최고 기록(가장 짧은 플레이 시간)만 관리하는 로직을 포함합니다.
 */
public class RankAdapter extends AbstractDataBaseAdapter {

    public static final String TABLE_NAME = "RANK"; // 이 어댑터가 관리하는 데이터베이스 테이블 이름
    private static final String COLUMN_NAME = "PlayerName"; //플레이어 이름
    private static final String COLUMN_PLAYDATE = "PlayDate";
    private static final String COLUMN_PLAYTIME = "PlayTime";

    /**
     * 랭킹 테이블을 생성하는 SQL 명령어입니다.
     * COLUMN_ID: 고유 ID (자동 증가), Primary Key
     * COLUMN_RECORD: 실제 게임 기록 데이터 (JSON 문자열 형태로 저장)
     * COLUMN_RANK: 해당 기록의 순위 (1, 2, 3, ...)
     */
    public static final String CREATE_TABLE_SQL =
            "CREATE TABLE " + TABLE_NAME + "("
                    + COLUMN_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, "
                    + COLUMN_NAME + " TEXT NOT NULL, "
                    + COLUMN_PLAYDATE + " TEXT NOT NULL, "
                    + COLUMN_PLAYTIME + " INTEGER NOT NULL);";

    /**
     * 생성자: 부모 클래스(AbstractDataBaseAdapter)를 호출하여 초기화합니다.
     *
     * @param context 앱 컨텍스트
     */
    public RankAdapter(Context context) {
        super(context);
        try {
            open(); // database = dbHelper.getWritableDatabase() 호출
        } catch (SQLException e) {
            e.printStackTrace();
        }
    }

    /**
     * 랭킹 테이블을 생성하는 SQL 명령어입니다.
     * <p>
     * - COLUMN_NAME   : 플레이어 이름 (TEXT)
     * - COLUMN_SCORE  : 게임 점수 (INTEGER)
     * - COLUMN_MILLIS : 클리어 시간 (ms 단위, INTEGER)
     * <p>
     * 사용 목적:
     * 랭킹 화면에 표시할 데이터를 저장하며,
     * 점수 기준 정렬 및 시간 기반 비교가 가능하도록 설계되었습니다.
     */
    @Override
    protected void onCreateTable(SQLiteDatabase db) {
        db.execSQL(CREATE_TABLE_SQL);
    }


    // -----------------------------
    // 외부용 public (데이터 추가 및 조회)
    // -----------------------------

    /**
     * 새로운 게임 기록(Score)을 랭킹에 추가하고, 랭킹 목록(상위 5개)을 갱신합니다.
     * 이 작업은 데이터 일관성을 위해 트랜잭션(Transaction) 내에서 처리됩니다.
     *
     * @param gameRecord 새로 추가할 게임 기록 객체
     */
    public void addScore(GameRecord gameRecord) {
        // 1. 실패 기록 필터링: 게임을 성공하지 못한 기록은 랭킹에 반영하지 않고 즉시 종료합니다.
        if (!gameRecord.Success()) return;

        SQLiteDatabase db = this.database;
        db.beginTransaction(); // 데이터베이스 트랜잭션 시작 (작업 단위 묶기)
        long playtime = gameRecord.Playtime().toMillis();
        try {
            Log.i("PARSE", "ER"+ playtime);
        } catch (NumberFormatException e) {
            Log.e("DB_ERROR", "Invalid time format after cleaning: " + playtime);
            return;
        }
        ContentValues values = new ContentValues();
        values.put(COLUMN_NAME, gameRecord.Player());
        values.put(COLUMN_PLAYDATE, gameRecord.Playdate().toString());
        values.put(COLUMN_PLAYTIME, playtime);
        try {
            db.insert(TABLE_NAME, null, values);

            // 3. 초과 데이터 삭제 (Delete/Trim) 쿼리
            // 100번째로 빠른 기록의 PlayTime 값보다 큰(느린) 모든 기록을 삭제합니다.
            String deleteSql = "DELETE FROM " + TABLE_NAME +
                    " WHERE " + COLUMN_PLAYTIME + " > (" +
                    "    SELECT " + COLUMN_PLAYTIME +
                    "    FROM " + TABLE_NAME +
                    "    ORDER BY " + COLUMN_PLAYTIME + " ASC " + // 숫자가 낮은(빠른) 순서로 정렬
                    "    LIMIT 1 OFFSET 4" +
                    ");";

            db.execSQL(deleteSql);
            db.setTransactionSuccessful(); // 모든 작업 성공: 변경 사항을 최종적으로 데이터베이스에 반영
        } finally {
            db.endTransaction(); // 트랜잭션 종료 (성공 여부에 관계없이 반드시 호출되어야 함)
        }
    }

    public static String formatMillis(long millis) {
        long minutes = (millis / 60000);
        long seconds = (millis % 60000) / 1000;
        long ms = millis % 1000;

        return String.format("%02d:%02d:%03d", minutes, seconds, ms);
    }


    /**
     * 현재 데이터베이스에 저장된 모든 랭킹 기록을 화면 표시에 적합한 문자열 리스트로 가져옵니다.
     *
     * @return "순위 날짜 이름 플레이시간" 형식의 문자열 리스트
     */
    public List<String> getAllRanksForDisplay() {
        List<String> list = new ArrayList<>();
        Cursor cursor = getAllRanks(); // 랭킹을 순위 순으로 가져오는 내부 함수 호출

        if (cursor != null && cursor.moveToFirst()) {
            do {
                int rank = 1;

                String playerName = cursor.getString(1);
                String playDate = cursor.getString(2);
                long playtime = cursor.getLong(3);
                // 표시할 문자열 포맷 생성: "1위 (이름) 2025-11-13T17:11 01:30.123"
                String text = rank + ". " + playerName + " " + playDate + " " + formatMillis(playtime);
                list.add(text);
                ++rank;
            } while (cursor.moveToNext());
            cursor.close();
        }
        return list;
    }

    /**
     * 데이터베이스에서 랭킹 테이블의 모든 레코드를 순위(COLUMN_RANK) 오름차순으로 조회합니다.
     * 이 함수는 내부적으로 사용됩니다.
     *
     * @return 조회된 레코드를 가리키는 Cursor 객체
     */
    private Cursor getAllRanks() {
        // database.query()를 사용하여 SELECT * FROM RANK ORDER BY rank ASC 쿼리 실행
        return database.query(TABLE_NAME, // 테이블 이름
                null,       // 모든 컬럼 조회 (null)
                null, null, // 조건 없음 (WHERE 절 없음)
                null, null, // GROUP BY, HAVING 절 없음
                COLUMN_PLAYTIME + " ASC"); // 순위(rank) 오름차순으로 정렬
    }

}