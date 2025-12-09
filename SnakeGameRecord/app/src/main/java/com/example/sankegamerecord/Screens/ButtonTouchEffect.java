package com.example.sankegamerecord.Screens;

import android.content.Context;
import android.view.MotionEvent;
import android.view.View;
import android.view.animation.AnimationUtils;

import com.example.sankegamerecord.R;

/**
 * 터치 시 애니메이션 효과와 '눌림(Pressed)' 상태 변경만 관리하고,
 * 클릭 이벤트는 v.performClick()을 통해 OnClickListener가 안정적으로 처리하도록 위임합니다.
 * 이전의 지연(Handler) 로직은 제거되어 클릭 취소 문제를 해결합니다.
 */
public class ButtonTouchEffect implements View.OnTouchListener {

    private final Context context;

    public ButtonTouchEffect(Context context) {
        this.context = context;
    }

    @Override
    public boolean onTouch(View v, MotionEvent event) {
        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                // 1. 눌림 애니메이션 시작
                v.startAnimation(AnimationUtils.loadAnimation(context, R.anim.button_press));

                // 2. '눌림' 상태를 true로 설정하여 Selector의 눌린 상태 색상을 적용
                v.setPressed(true);

                // 지연 복구 로직이 제거되어, 손을 떼는 순간까지 눌림 상태가 유지됩니다.
                break;

            case MotionEvent.ACTION_UP:
                // 1. 떼는 애니메이션 시작
                v.startAnimation(AnimationUtils.loadAnimation(context, R.anim.button_release));

                // 2. OnClickListener를 강제로 실행합니다. (짧은 클릭을 보장)
                v.performClick();

                // 3. 눌림 상태를 즉시 false로 설정하여 상태를 복구
                v.setPressed(false);

                break;

            case MotionEvent.ACTION_CANCEL:
                // 터치가 취소된 경우, 눌림 상태 복구
                v.setPressed(false);
                v.startAnimation(AnimationUtils.loadAnimation(context, R.anim.button_release));
                break;
        }

        // onTouch 리스너가 이벤트를 소비(true)하고 performClick()으로 클릭 이벤트를 명시적으로 트리거합니다.
        return true;
    }
}