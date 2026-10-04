package com.samfa12.hordelanternrt;
import android.content.Context;
import android.widget.Button;
import android.widget.FrameLayout;
import android.view.Gravity;

/** Stable slots for the unchanged action routes. Apply only while input is gated. */
final class UiControlLayout {
    static void apply(Context c,InterfacePreferences.Values v,Button swing,Button parry,
                      Button interact,Button light,int safeRight,int safeBottom,int availableWidth) {
        float scale=v.scale/100f;
        int primaryWidth=HordeUiTokens.dp(c,Math.round((v.compact?88:104)*scale));
        int primaryHeight=HordeUiTokens.dp(c,Math.round((v.compact?64:72)*scale));
        int contextWidth=HordeUiTokens.dp(c,Math.round((v.compact?112:128)*scale));
        int contextHeight=HordeUiTokens.dp(c,Math.round((v.compact?56:60)*scale));
        int gap=HordeUiTokens.dp(c,12), edge=HordeUiTokens.dp(c,24)+safeRight;
        int minimum=HordeUiTokens.dp(c,48);
        int maximum=Math.max(minimum,(availableWidth-edge-HordeUiTokens.dp(c,16)-gap)/2);
        primaryWidth=Math.max(minimum,Math.min(primaryWidth,maximum));
        contextWidth=Math.max(minimum,Math.min(contextWidth,maximum));
        int bottom=HordeUiTokens.dp(c,32)+safeBottom;
        place(swing,primaryWidth,Math.max(minimum,primaryHeight),edge,bottom);
        place(parry,primaryWidth,Math.max(minimum,primaryHeight),edge+primaryWidth+gap,bottom);
        int above=bottom+primaryHeight+gap;
        place(interact,contextWidth,Math.max(minimum,contextHeight),edge,above);
        place(light,contextWidth,Math.max(minimum,contextHeight),edge+contextWidth+gap,above);
    }
    private static void place(Button b,int width,int height,int right,int bottom) {
        FrameLayout.LayoutParams p=new FrameLayout.LayoutParams(width,height);
        p.gravity=Gravity.BOTTOM|Gravity.END; p.setMarginEnd(right); p.bottomMargin=bottom;
        b.setLayoutParams(p);
    }
    private UiControlLayout() { }
}
