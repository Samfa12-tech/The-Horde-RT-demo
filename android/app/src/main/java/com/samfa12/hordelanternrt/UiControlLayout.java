package com.samfa12.hordelanternrt;
import android.content.Context;
import android.widget.Button;
import android.widget.FrameLayout;
import android.view.Gravity;
import android.graphics.Paint;
import android.util.TypedValue;

/** Reserved action/context slots. Hidden context buttons never move the combat buttons. */
final class UiControlLayout {
    static void apply(Context c,InterfacePreferences.Values v,Button swing,Button parry,
                      Button interact,Button light,int safeRight,int safeBottom,int availableWidth) {
        apply(c,v,swing,parry,null,interact,light,safeRight,safeBottom,availableWidth);
    }
    static void apply(Context c,InterfacePreferences.Values v,Button swing,Button parry,Button dodge,
                      Button interact,Button light,int safeRight,int safeBottom,int availableWidth) {
        apply(c,v,swing,parry,dodge,interact,light,safeRight,safeBottom,availableWidth,
                c.getResources().getDisplayMetrics().heightPixels);
    }
    static void apply(Context c,InterfacePreferences.Values v,Button swing,Button parry,Button dodge,
                      Button interact,Button light,int safeRight,int safeBottom,int availableWidth,int availableHeight) {
        if(dodge!=null) {
            applyCombatArc(c,v,swing,parry,dodge,interact,light,safeRight,safeBottom,availableWidth,
                    availableHeight);
            return;
        }
        // Retain the explicit four-control legacy/fixture layout.
        float scale=v.scale/100f;
        int primaryWidth=HordeUiTokens.dp(c,Math.round((v.compact?72:80)*scale));
        int primaryHeight=HordeUiTokens.dp(c,Math.round((v.compact?52:56)*scale));
        int contextWidth=HordeUiTokens.dp(c,Math.round((v.compact?112:128)*scale));
        int contextHeight=HordeUiTokens.dp(c,Math.round((v.compact?56:60)*scale));
        int gap=HordeUiTokens.dp(c,8), edge=HordeUiTokens.dp(c,12)+safeRight;
        int minimum=HordeUiTokens.dp(c,48);
        // Keep deliberate actions in the right half. The left movement half
        // and the open look area above the cluster are interaction space.
        int maximum=Math.max(minimum,(availableWidth/2-edge-gap)/2);
        int labelMinimum=minimum;
        for(Button action:new Button[]{swing,parry,dodge})if(action!=null) {
            Paint label=new Paint(action.getPaint());
            label.setTextSize(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_SP,10,c.getResources().getDisplayMetrics()));
            labelMinimum=Math.max(labelMinimum,(int)Math.ceil(label.measureText(action.getText().toString()))+HordeUiTokens.dp(c,12)+2);
        }
        // At large system fonts in a narrow window, retain complete labels and
        // the movement half by stacking primary actions within the same dock.
        boolean stacked=dodge!=null && labelMinimum>maximum;
        primaryWidth=Math.max(minimum,Math.min(Math.max(primaryWidth,labelMinimum),maximum));
        if(stacked)primaryWidth=Math.min(availableWidth/2-edge,Math.max(primaryWidth,labelMinimum));
        contextWidth=Math.max(minimum,Math.min(contextWidth,availableWidth/2-edge));
        if(dodge==null)contextWidth=Math.min(contextWidth,primaryWidth);
        int bottom=HordeUiTokens.dp(c,16)+safeBottom;
        place(swing,primaryWidth,Math.max(minimum,primaryHeight),edge,bottom);
        place(parry,primaryWidth,Math.max(minimum,primaryHeight),stacked?edge:edge+primaryWidth+gap,
                stacked?bottom+Math.max(minimum,primaryHeight)+gap:bottom);
        int above=bottom+Math.max(minimum,primaryHeight)+gap;
        if(stacked)above+=Math.max(minimum,primaryHeight)+gap;
        int lightSize=HordeUiTokens.dp(c,56);
        int rowHeight=Math.max(Math.max(minimum,primaryHeight),lightSize);
        if(dodge!=null) {
            place(dodge,primaryWidth,Math.max(minimum,primaryHeight),edge,above+(rowHeight-Math.max(minimum,primaryHeight))/2);
        }
        place(light,lightSize,lightSize,edge+primaryWidth+gap+(stacked?0:(primaryWidth-lightSize)/2),above+(rowHeight-lightSize)/2);
        if(dodge!=null) above+=rowHeight+gap;
        place(interact,contextWidth,Math.max(minimum,contextHeight),edge,above);
    }
    private static void applyCombatArc(Context c,InterfacePreferences.Values v,Button swing,Button parry,
                                       Button dodge,Button interact,Button light,int safeRight,int safeBottom,
                                       int width,int height) {
        boolean landscape=width>height;
        float scale=v.scale/100f;
        int gap=HordeUiTokens.dp(c,8),utility=HordeUiTokens.dp(c,56);
        int edge=HordeUiTokens.dp(c,8)+safeRight,bottom=HordeUiTokens.dp(c,16)+safeBottom;
        int half=width/2-edge;
        int sword=Math.max(HordeUiTokens.dp(c,72),HordeUiTokens.dp(c,Math.round((v.compact?72:80)*scale)));
        int shield=Math.max(HordeUiTokens.dp(c,72),HordeUiTokens.dp(c,Math.round((v.compact?72:76)*scale)));
        boolean narrow=sword+shield+gap>half;
        if(narrow) {
            // A narrow portrait keeps both combat targets in one right column.
            // The lantern stays next to the bottom anchor, never in movement.
            sword=Math.min(sword,Math.max(HordeUiTokens.dp(c,48),half-utility-gap));
            shield=Math.min(shield,sword);
        }
        int dodgeWidth=Math.max(HordeUiTokens.dp(c,72),minimumLabelWidth(c,dodge));
        dodgeWidth=Math.min(dodgeWidth,half);
        int dodgeHeight=HordeUiTokens.dp(c,52);
        int contextWidth=Math.max(HordeUiTokens.dp(c,48),Math.min(half,HordeUiTokens.dp(c,Math.round((v.compact?112:128)*scale))));
        int contextHeight=Math.max(HordeUiTokens.dp(c,48),HordeUiTokens.dp(c,Math.round((v.compact?56:60)*scale)));
        if(narrow && bottom+sword+shield+dodgeHeight+contextHeight+3*gap>height) {
            // Short resizable windows fit the full reserved context row first.
            // Bound physical targets rather than letting Interact clip above the
            // parent; saved scale is unchanged, and normal phone arcs stay exact.
            int excess=bottom+sword+shield+dodgeHeight+contextHeight+3*gap-height;
            contextHeight=Math.min(contextHeight,Math.max(HordeUiTokens.dp(c,56),contextHeight-excess));
            int pairBudget=height-bottom-dodgeHeight-contextHeight-3*gap;
            if(sword+shield>pairBudget) {
                int fitted=Math.max(HordeUiTokens.dp(c,48),pairBudget/2);
                sword=Math.min(sword,fitted);shield=Math.min(shield,fitted);
            }
        }
        place(swing,sword,sword,edge,bottom);
        int shieldBottom=narrow?bottom+sword+gap:landscape?bottom+HordeUiTokens.dp(c,12):bottom+(sword-utility)/2+utility+gap;
        int shieldRight=narrow?edge:edge+sword+gap;
        place(parry,shield,shield,shieldRight,shieldBottom);
        int lightRight=narrow?edge+sword+gap:shieldRight+(shield-utility)/2;
        int lightBottom=landscape && !narrow?shieldBottom+shield+gap:bottom+(sword-utility)/2;
        place(light,utility,utility,lightRight,lightBottom);
        int dodgeBottom=narrow?shieldBottom+shield+gap:bottom+sword+gap;
        place(dodge,dodgeWidth,dodgeHeight,edge,dodgeBottom);
        int above=Math.max(dodgeBottom+dodgeHeight,Math.max(lightBottom+utility,shieldBottom+shield))+gap;
        place(interact,contextWidth,contextHeight,edge,above);
    }
    private static int minimumLabelWidth(Context c,Button action) {
        Paint label=new Paint(action.getPaint());
        label.setTextSize(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_SP,10,c.getResources().getDisplayMetrics()));
        return (int)Math.ceil(label.measureText(action.getText().toString()))+HordeUiTokens.dp(c,12)+2;
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
