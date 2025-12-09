package com.example.sankegamerecord.Screens;

import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.widget.Button;

import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.swiperefreshlayout.widget.SwipeRefreshLayout;

import com.example.sankegamerecord.DataBaseAdapter.RankAdapter;
import com.example.sankegamerecord.DataBaseAdapter.RecordAdapter;
import com.example.sankegamerecord.Adapter.RecordListAdapter;
import com.example.sankegamerecord.R; // 리소스(레이아웃, 문자열 등) 접근 클래스
import com.google.android.material.floatingactionbutton.FloatingActionButton; // 화면 오른쪽 아래 등에 뜨는 동그란 버튼

/**
 * 게임의 랭킹(Rank)과 전체 기록(Record)을 사용자에게 보여주는 안드로이드 화면(Activity)입니다.
 * 두 가지 탭(버튼) 전환과 화면을 아래로 당겨 새로고침(SwipeRefresh) 기능을 구현합니다.
 */
public class RankingActivity extends AppCompatActivity {

    // UI 컴포넌트 변수
    private RecyclerView recyclerView; // 리스트 형태로 데이터를 보여주는 뷰
    private RecordListAdapter adapter; // RecyclerView에 데이터를 연결해주는 어댑터
    private Button btnRank, btnRecord; // "랭킹 보기"와 "기록 보기" 버튼
    private SwipeRefreshLayout swipeRefreshLayout; // 화면 당겨서 새로고침 기능 제공 뷰
    private FloatingActionButton fabBack; // 뒤로 가기 버튼

    // 데이터베이스 어댑터 변수
    private RankAdapter rankAdapter; // 랭킹 테이블 데이터 관리 객체
    private RecordAdapter recordAdapter; // 전체 기록 테이블 데이터 관리 객체

    // 상태 관리 변수
    private String buttonState; // 현재 어떤 탭(btnRank 또는 btnRecord)이 선택되었는지 저장하는 상태

    /**
     * Activity가 처음 생성될 때 호출됩니다. (화면 초기화)
     */
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // 화면에 activity_ranking.xml 레이아웃 파일을 설정합니다.
        setContentView(R.layout.activity_ranking);

        // 1. UI 컴포넌트 초기화 (XML 레이아웃에서 뷰 객체를 찾아 연결)
        recyclerView = findViewById(R.id.recyclerView);
        btnRank = findViewById(R.id.btnRank);
        btnRecord = findViewById(R.id.btnRecord);
        swipeRefreshLayout = findViewById(R.id.swipeRefreshLayout);


        // 2. 데이터베이스 어댑터 초기화 및 연결 열기
        // *주의: Activity가 살아있는 동안 데이터베이스 연결을 유지합니다.
        rankAdapter = new RankAdapter(this);
        rankAdapter.open(); // 데이터베이스 연결 시작

        recordAdapter = new RecordAdapter(this);
        recordAdapter.open(); // 데이터베이스 연결 시작

        // 3. RecyclerView 설정
        // 리스트를 세로 방향으로 배치하도록 LayoutManager 설정
        recyclerView.setLayoutManager(new LinearLayoutManager(this));

        // 초기 데이터: 랭크 어댑터에서 랭킹 데이터를 가져와 리스트 어댑터에 설정
        adapter = new RecordListAdapter(rankAdapter.getAllRanksForDisplay());
        recyclerView.setAdapter(adapter);

        // 4. 초기 상태 설정
        selectTab(btnRank); // 랭크 버튼을 선택된 상태로 표시
        buttonState="btnRank"; // 현재 상태를 랭크로 설정

        // 5. 버튼 클릭 리스너 설정

        // [랭크 버튼] 클릭 시
        btnRank.setOnClickListener(v -> {
            // 랭킹 데이터를 새로 가져와 어댑터 데이터 갱신
            adapter.updateData(rankAdapter.getAllRanksForDisplay());
            buttonState="btnRank";
            selectTab(btnRank); // 탭 선택 상태 업데이트
        });

        // [기록 버튼] 클릭 시
        btnRecord.setOnClickListener(v -> {
            // 전체 기록 데이터를 새로 가져와 어댑터 데이터 갱신 (RecordAdapter의 getAllRecordsForDisplay() 필요)
            adapter.updateData(recordAdapter.getAllRecordsForDisplay());
            buttonState="btnRecord";
            selectTab(btnRecord); // 탭 선택 상태 업데이트
        });

        // [당겨서 새로고침] 리스너 설정
        swipeRefreshLayout.setOnRefreshListener(new SwipeRefreshLayout.OnRefreshListener() {
            @Override
            public void onRefresh() {
                fetchNewData(); // 새로고침 작업 실행
            }
        });
    }

    /**
     * 두 버튼 중 하나를 선택된 상태로 시각적으로 변경하는 헬퍼 메서드입니다.
     * @param selectedButton 선택된 버튼 (btnRank 또는 btnRecord)
     */
    private void selectTab(Button selectedButton) {
        // 모든 버튼의 선택 상태를 초기화(false)
        btnRank.setSelected(false);
        btnRecord.setSelected(false);
        // 선택된 버튼만 선택 상태로 설정(true) (이 상태에 따라 XML의 Selector 스타일이 적용됨)
        selectedButton.setSelected(true);
    }

    /**
     * SwipeRefreshLayout에 의해 호출되며, 현재 선택된 탭에 따라 데이터를 새로 로드합니다.
     * 1초의 지연(Handler().postDelayed)은 사용자에게 새로고침이 진행 중임을 느끼게 하기 위함입니다.
     */
    private void fetchNewData() {
        new Handler().postDelayed(
                new Runnable() {
                    public void run() {
                        // 현재 상태(buttonState)에 따라 랭킹 또는 기록 데이터를 새로 가져옵니다.
                        switch (buttonState){
                            case "btnRank":
                                // 데이터베이스 연결이 닫혀있을 경우를 대비해 다시 open 시도
                                rankAdapter.open();
                                adapter.updateData(rankAdapter.getAllRanksForDisplay());
                                break;
                            case "btnRecord":
                                // 데이터베이스 연결이 닫혀있을 경우를 대비해 다시 open 시도
                                recordAdapter.open();
                                adapter.updateData(recordAdapter.getAllRecordsForDisplay());
                                break;
                            default:
                                // 예상치 못한 오류 상태 로깅
                                Log.e("ERROR","MissFire: Unknown buttonState " + buttonState);
                        }

                        // RecyclerView에 데이터가 변경되었음을 알려 화면을 갱신합니다.
                        adapter.notifyDataSetChanged();

                        // 새로고침 애니메이션을 종료합니다.
                        if (swipeRefreshLayout != null) {
                            swipeRefreshLayout.setRefreshing(false);
                        }
                    }
                },
                1000); // 1000ms (1초) 지연 후 실행
    }

    /**
     * Activity가 소멸될 때 호출됩니다. (화면 종료 시)
     * 데이터베이스 연결과 같은 리소스를 반드시 해제해야 합니다.
     */
    @Override
    protected void onDestroy() {
        super.onDestroy();
        // 데이터베이스 연결을 닫아 메모리 누수를 방지합니다.
        if (rankAdapter != null) rankAdapter.close();
        if (recordAdapter != null) recordAdapter.close();
    }
}