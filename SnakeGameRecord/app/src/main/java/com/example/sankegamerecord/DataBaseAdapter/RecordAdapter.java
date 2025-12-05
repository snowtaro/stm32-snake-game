package com.example.sankegamerecord.DataBaseAdapter;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.SQLException;

import com.example.sankegamerecord.Adapter.GameRecord;

import java.util.ArrayList;
import java.util.List;

public class RecordAdapter extends AbstractDataBaseAdapter {

    public static final String TABLE_NAME = "RECORD";
    public static final int MAX_SIZE = 20;
    public static final String CREATE_TABLE_SQL =
            "CREATE TABLE " + TABLE_NAME + "("
                    + COLUMN_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, "
                    + COLUMN_RECORD + " TEXT NOT NULL);";

    public RecordAdapter(Context context) {
        super(context);
        try {
            open(); // database = dbHelper.getWritableDatabase() 호출
        } catch (SQLException e) {
            e.printStackTrace();
        }
    }

    // -----------------------------
    // 외부용 public
    // -----------------------------
    public long addRecord(GameRecord record) {
        try {
            String recordValue = gameRecordToJson(record);

            // -----------------------------
            // 1. 중복 확인 (playdate 기준)
            // -----------------------------
            try (Cursor cursor = database.query(
                    TABLE_NAME,
                    new String[]{COLUMN_RECORD},
                    null, null, null, null,
                    COLUMN_ID + " ASC")) {

                while (cursor.moveToNext()) {
                    GameRecord existing = parseGameRecord(cursor.getString(cursor.getColumnIndexOrThrow(COLUMN_RECORD)));
                    if (existing != null && existing.Playdate().equals(record.Playdate())) {
                        // 중복 발견 → 삽입하지 않고 -1 반환
                        return -1;
                    }
                }
            }

            // -----------------------------
            // 2. FIFO 최대 20개 유지
            // -----------------------------
            int count = 0;
            try (Cursor cursor = database.rawQuery("SELECT COUNT(*) FROM " + TABLE_NAME, null)) {
                if (cursor.moveToFirst()) count = cursor.getInt(0);
            }

            if (count >= MAX_SIZE) {
                // 가장 오래된 레코드 ID 찾기
                try (Cursor oldest = database.rawQuery(
                        "SELECT " + COLUMN_ID + " FROM " + TABLE_NAME + " ORDER BY " + COLUMN_ID + " ASC LIMIT 1",
                        null)) {
                    if (oldest.moveToFirst()) {
                        long oldestId = oldest.getLong(0);
                        database.delete(TABLE_NAME, COLUMN_ID + "=?", new String[]{String.valueOf(oldestId)});
                    }
                }
            }

            // -----------------------------
            // 3. 새로운 레코드 삽입
            // -----------------------------
            ContentValues values = new ContentValues();
            values.put(COLUMN_RECORD, recordValue);
            return database.insert(TABLE_NAME, null, values);

        } catch (Exception e) {
            e.printStackTrace();
            return -1;
        }
    }



    public List<String> getAllRecordsForDisplay() {
        List<String> list = new ArrayList<>();
        Cursor cursor = getAllRecords();
        if (cursor != null && cursor.moveToLast()) {  // 마지막 레코드부터 시작
            do {
                GameRecord gr = parseGameRecord(cursor.getString(cursor.getColumnIndexOrThrow(COLUMN_RECORD)));
                if (gr != null) {
                    String text = gr.Playdate() + " " + formatDuration(gr.Playtime())
                            + " " + (gr.Success() ? "성공" : "실패");
                    list.add(text);
                }
            } while (cursor.moveToPrevious()); // 이전 레코드로 이동
            cursor.close();
        }
        return list;
    }


    private Cursor getAllRecords() {
        return database.query(TABLE_NAME, null, null, null, null, null,
                COLUMN_ID + " ASC");
    }

}
