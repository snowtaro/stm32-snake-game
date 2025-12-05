package com.example.sankegamerecord.DataBaseAdapter;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;

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
    private static final String COLUMN_RANK = "rank"; // 테이블에 추가된 순위 정보를 저장하는 컬럼 이름 (1위, 2위 등)

    /**
     * 랭킹 테이블을 생성하는 SQL 명령어입니다.
     * COLUMN_ID: 고유 ID (자동 증가), Primary Key
     * COLUMN_RECORD: 실제 게임 기록 데이터 (JSON 문자열 형태로 저장)
     * COLUMN_RANK: 해당 기록의 순위 (1, 2, 3, ...)
     */
    public static final String CREATE_TABLE_SQL =
            "CREATE TABLE " + TABLE_NAME + "("
                    + COLUMN_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, "
                    + COLUMN_RECORD + " TEXT NOT NULL, "
                    + COLUMN_RANK + " INTEGER NOT NULL);";

    /**
     * 생성자: 부모 클래스(AbstractDataBaseAdapter)를 호출하여 초기화합니다.
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
        try {
            List<GameRecord> records = new ArrayList<>();

            // 2. 현재 DB에 저장된 모든 랭킹 기록을 조회 (순위 순으로 정렬)
            Cursor cursor = db.query(TABLE_NAME, null, null, null, null, null,
                    COLUMN_RANK + " ASC");

            // 3. 조회된 모든 기록을 GameRecord 객체로 파싱하여 리스트에 추가
            while (cursor.moveToNext()) {
                // JSON 문자열을 GameRecord 객체로 변환 (부모 클래스 메서드 사용)
                GameRecord gr = parseGameRecord(cursor.getString(cursor.getColumnIndexOrThrow(COLUMN_RECORD)));
                // 파싱이 성공한 기록만 리스트에 추가
                if (gr != null && gr.Success()) records.add(gr);
            }
            cursor.close();

            // 4. 새 기록을 리스트에 추가하고 전체 리스트를 "플레이 시간(밀리초)" 기준으로 다시 정렬합니다.
            // 플레이 시간이 짧을수록 (toMillis() 값이 작을수록) 상위 랭크입니다.
            records.add(gameRecord);
            records.sort(Comparator.comparingLong(r -> r.Playtime().toMillis()));

            // 5. 상위 5개의 기록만 데이터베이스에 갱신하거나 삽입합니다.
            // Math.min(5, records.size()): 최대 5개 또는 리스트 크기만큼만 반복
            for (int i = 0; i < Math.min(5, records.size()); i++) {
                GameRecord gr = records.get(i);
                ContentValues values = new ContentValues();

                // 갱신될 데이터 준비: JSON 기록 및 새로운 순위 (i+1)
                values.put(COLUMN_RECORD, gameRecordToJson(gr)); // GameRecord를 JSON 문자열로 변환하여 저장
                values.put(COLUMN_RANK, i + 1); // 1위, 2위, ...

                // 현재 순위(i+1)에 이미 레코드가 있는지 확인
                Cursor check = db.query(TABLE_NAME, new String[]{COLUMN_ID}, COLUMN_RANK + "=?",
                        new String[]{String.valueOf(i + 1)}, null, null, null);

                // 해당 순위에 기존 레코드가 있다면 (UPDATE)
                if (check.moveToFirst()) {
                    long id = check.getLong(check.getColumnIndexOrThrow(COLUMN_ID));
                    // 기존 레코드 ID를 찾아 데이터 갱신
                    db.update(TABLE_NAME, values, COLUMN_ID + "=?", new String[]{String.valueOf(id)});
                } else {
                    // 기존 레코드가 없다면 (테이블이 비어있을 때 등) 새로운 레코드 삽입 (INSERT)
                    db.insert(TABLE_NAME, null, values);
                }
                check.close(); // 커서 해제
            }

            db.setTransactionSuccessful(); // 모든 작업 성공: 변경 사항을 최종적으로 데이터베이스에 반영
        } finally {
            db.endTransaction(); // 트랜잭션 종료 (성공 여부에 관계없이 반드시 호출되어야 함)
        }
    }

    /**
     * 현재 데이터베이스에 저장된 모든 랭킹 기록을 화면 표시에 적합한 문자열 리스트로 가져옵니다.
     * @return "순위 날짜 이름 플레이시간" 형식의 문자열 리스트
     */
    public List<String> getAllRanksForDisplay() {
        List<String> list = new ArrayList<>();
        Cursor cursor = getAllRanks(); // 랭킹을 순위 순으로 가져오는 내부 함수 호출

        if (cursor != null && cursor.moveToFirst()) {
            do {
                int rank = cursor.getInt(cursor.getColumnIndexOrThrow(COLUMN_RANK));
                // JSON 문자열에서 GameRecord 객체를 복원
                GameRecord gr = parseGameRecord(cursor.getString(cursor.getColumnIndexOrThrow(COLUMN_RECORD)));

                if (gr != null) {
                    // 표시할 문자열 포맷 생성: "1위 2025-11-13T17:11 이름 01:30.123"
                    String text = rank + ". " + gr.Playdate() + " "  + formatDuration(gr.Playtime());
                    list.add(text);
                }
            } while (cursor.moveToNext());
            cursor.close();
        }
        return list;
    }

    /**
     * 데이터베이스에서 랭킹 테이블의 모든 레코드를 순위(COLUMN_RANK) 오름차순으로 조회합니다.
     * 이 함수는 내부적으로 사용됩니다.
     * @return 조회된 레코드를 가리키는 Cursor 객체
     */
    private Cursor getAllRanks() {
        // database.query()를 사용하여 SELECT * FROM RANK ORDER BY rank ASC 쿼리 실행
        return database.query(TABLE_NAME, // 테이블 이름
                null,       // 모든 컬럼 조회 (null)
                null, null, // 조건 없음 (WHERE 절 없음)
                null, null, // GROUP BY, HAVING 절 없음
                COLUMN_RANK + " ASC"); // 순위(rank) 오름차순으로 정렬
    }

}