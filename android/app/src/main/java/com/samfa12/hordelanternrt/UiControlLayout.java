package com.samfa12.hordelanternrt;
import android.content.Context;
import android.widget.Button;
import android.widget.FrameLayout;
import android.view.Gravity;

/** Reserved action/context slots. Hidden context buttons never move the combat buttons. */
final class UiControlLayout {
    static void apply(Context c,InterfacePreferences.Values v,Button swing,Button parry,
                      Button interact,Button light,int safeRight,int safeBottom,int availableWidth) {
        apply(c,v,swing,parry,null,interact,light,safeRight,safeBottom,availableWidth);
    }
    static void apply(Context c,InterfacePreferences.Values v,Button swing,Button parry,Button dodge,
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
        int above=bottom+Math.max(minimum,primaryHeight)+gap;
        if(dodge!=null) {
            place(dodge,primaryWidth,Math.max(minimum,primaryHeight),edge,above);
            above+=Math.max(minimum,primaryHeight)+gap;
        }
        place(interact,contextWidth,Math.max(minimum,contextHeight),edge,above);
        place(light,contextWidth,Math.max(minimum,contextHeight),edge+contextWidth+gap,above);
    }
    static void applyStance(Context c,InterfacePreferences.Values v,Button run,Button crouch,int safeLeft,int safeBottom,
                            int availableWidth,Button... combatControls) {
        if(crouch==null)return;
        int width=HordeUiTokens.dp(c,Math.round((v.compact?88:104)*v.scale/100f));
        int height=Math.max(HordeUiTokens.dp(c,48),HordeUiTokens.dp(c,Math.round((v.compact?64:72)*v.scale/100f)));
        int bottom=HordeUiTokens.dp(c,32)+safeBottom;
        FrameLayout.LayoutParams p=new FrameLayout.LayoutParams(width,height);
        p.gravity=Gravity.BOTTOM|Gravity.START;p.setMarginStart(HordeUiTokens.dp(c,24)+safeLeft);
        int runHeight=run!=null && run.getLayoutParams()!=null && run.getLayoutParams().height>0
                ?run.getLayoutParams().height:HordeUiTokens.dp(c,72);
        int gap=HordeUiTokens.dp(c,12);
        p.bottomMargin=bottom+runHeight+gap;
        // Keep the existing action/context slots fixed. In narrow compact layouts
        // their leftmost context slot can reach this column; move only Crouch above it.
        for(Button control:combatControls) {
            if(control==null || !(control.getLayoutParams() instanceof FrameLayout.LayoutParams))continue;
            FrameLayout.LayoutParams obstacle=(FrameLayout.LayoutParams)control.getLayoutParams();
            int right=availableWidth-obstacle.getMarginEnd(),left=right-obstacle.width;
            if(p.getMarginStart()<right && p.getMarginStart()+width>left &&
                    p.bottomMargin<obstacle.bottomMargin+obstacle.height &&
                    p.bottomMargin+height>obstacle.bottomMargin)
                p.bottomMargin=obstacle.bottomMargin+obstacle.height+gap;
        }
        crouch.setLayoutParams(p);
    }
    private static void place(Button b,int width,int height,int right,int bottom) {
        FrameLayout.LayoutParams p=new FrameLayout.LayoutParams(width,height);
        p.gravity=Gravity.BOTTOM|Gravity.END; p.setMarginEnd(right); p.bottomMargin=bottom;
        b.setLayoutParams(p);
    }
    private UiControlLayout() { }
}
