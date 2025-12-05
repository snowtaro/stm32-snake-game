package com.example.sankegamerecord.Adapter;

import android.view.*;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.example.sankegamerecord.R; // 리소스(레이아웃, ID 등) 접근 클래스

import java.util.*;

/**
 * RecyclerView를 위한 어댑터 클래스입니다.
 * 리스트 형태의 문자열 데이터(랭킹 또는 기록)를 받아 화면의 각 행(row)에 표시하는 역할을 합니다.
 * RecyclerView를 사용하려면 반드시 이 Adapter, ViewHolder, LayoutManager 세 가지 구성요소가 필요합니다.
 */
public class RecordListAdapter extends RecyclerView.Adapter<RecordListAdapter.ViewHolder> {

    // 화면에 표시할 데이터를 담는 리스트 (문자열 형태, 예: "1위 2025-11-13 이름 01:30.123")
    private List<String> dataList;

    /**
     * 생성자: 초기 데이터를 받아 dataList를 초기화합니다.
     * @param dataList 초기 문자열 데이터 리스트
     */
    public RecordListAdapter(List<String> dataList) {
        // 불변성을 위해 외부 리스트를 복사하여 내부 리스트를 초기화합니다.
        this.dataList = new ArrayList<>(dataList);
    }

    /**
     * 어댑터가 관리하는 데이터 리스트를 완전히 새로운 데이터로 갱신합니다.
     * @param newList 새로 표시할 문자열 데이터 리스트
     */
    public void updateData(List<String> newList) {
        this.dataList.clear(); // 기존 데이터 삭제
        this.dataList.addAll(newList); // 새 데이터 추가
        // 데이터가 변경되었음을 RecyclerView에 알려 화면을 새로 그리도록 요청합니다.
        notifyDataSetChanged();
    }

    /**
     * 화면에 보여줄 아이템 뷰(행)를 생성하고 해당 뷰를 관리하는 ViewHolder 객체를 만듭니다.
     * 이 메서드는 필요한 뷰가 없을 때(스크롤 등으로 새로운 뷰가 필요할 때) 호출됩니다.
     * @param parent 뷰가 추가될 부모 뷰 그룹 (RecyclerView)
     * @param viewType 뷰 타입 (여러 종류의 뷰를 사용할 경우 구분)
     * @return 새로 생성된 ViewHolder 객체
     */
    @NonNull
    @Override
    public RecordListAdapter.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        // item_text_row.xml 레이아웃을 인플레이트(메모리에 로드)하여 View 객체를 생성합니다.
        View v = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.item_bubble_card, parent, false);
        return new ViewHolder(v); // 생성된 뷰로 ViewHolder를 만들어 반환
    }

    /**
     * ViewHolder의 뷰에 실제 데이터를 연결(바인딩)합니다.
     * 이 메서드는 뷰가 재사용되거나 새로 생성될 때마다 호출되어 화면에 표시할 내용을 업데이트합니다.
     * @param holder 데이터를 연결할 ViewHolder 객체
     * @param position 데이터 리스트 내의 인덱스 (순서)
     */
    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        // dataList에서 position에 해당하는 문자열을 가져와 TextView에 설정합니다.
        holder.textItem.setText(dataList.get(position));
    }

    /**
     * 어댑터가 관리하는 전체 아이템(행)의 개수를 반환합니다.
     * RecyclerView는 이 값을 바탕으로 화면에 몇 개의 행을 표시해야 할지 결정합니다.
     * @return 데이터 리스트의 크기
     */
    @Override
    public int getItemCount() {
        return dataList.size();
    }

    /**
     * ====================================================================
     * ViewHolder: 화면의 한 행(아이템)에 해당하는 뷰들을 관리하는 객체
     * ====================================================================
     * 뷰를 재사용할 때마다 findViewById()를 다시 호출하는 비효율을 막고, 뷰 객체를 캐시(저장)해 둡니다.
     */
    public static class ViewHolder extends RecyclerView.ViewHolder {
        // item_text_row.xml 레이아웃에 정의된 TextView
        TextView textItem;

        /**
         * ViewHolder 생성자: 뷰를 초기화하고 findViewById()를 통해 컴포넌트를 연결합니다.
         * @param itemView 현재 행(row)의 전체 뷰 객체
         */
        public ViewHolder(View itemView) {
            super(itemView);
            // 뷰 내부에서 ID가 textItem인 TextView를 찾아 연결합니다.
            textItem = itemView.findViewById(R.id.textItem);
        }
    }
}