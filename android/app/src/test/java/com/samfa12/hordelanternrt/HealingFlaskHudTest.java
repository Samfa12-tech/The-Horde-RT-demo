package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.content.res.Configuration;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import org.junit.Test;
import org.junit.Before;
import com.samfa12.hordelanternrt.AndroidControllerIntegrationTest.Bridge;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

/** Real Activity wiring with the existing immutable native-publication shadow. */
@RunWith(RobolectricTestRunner.class)
@Config(sdk=34,shadows=AndroidControllerIntegrationTest.Bridge.class)
public final class HealingFlaskHudTest {
    private AndroidControllerIntegrationTest fixture;
    @Before public void setup()throws Exception {fixture=new AndroidControllerIntegrationTest();fixture.setup();}
    private static Field field(String name)throws Exception {
        Field f=MainActivity.class.getDeclaredField(name);f.setAccessible(true);return f;
    }
    private MainActivity activity()throws Exception {
        Field f=AndroidControllerIntegrationTest.class.getDeclaredField("a");f.setAccessible(true);return (MainActivity)f.get(fixture);
    }
    private void setHud(String name,Object value)throws Exception {field(name).set(activity(),value);}
    private Object invoke(String name,Class<?>[] types,Object...args)throws Exception {
        Method m=MainActivity.class.getDeclaredMethod(name,types);m.setAccessible(true);return m.invoke(activity(),args);
    }
    private void update()throws Exception {invoke("updateContextualControls",new Class<?>[]{boolean.class},true);}
    @Test public void chargesRemainVisibleWhileDisabledAndNearbyDoesNotGrantRest()throws Exception {
        MainActivity a=activity();Bridge.flask=new long[]{2,31,1,0,0,2,2,0,0,1};update();
        View panel=a.findViewById(R.id.flask_panel);TextView status=a.findViewById(R.id.flask_status);
        Button drink=a.findViewById(R.id.drink_flask_button),rest=a.findViewById(R.id.rest_button);
        assertEquals(View.VISIBLE,panel.getVisibility());assertTrue(status.getText().toString().contains("full"));
        assertFalse(drink.isEnabled());drink.performClick();assertEquals(0,Bridge.drinks);
        assertTrue(((TextView)a.findViewById(R.id.flask_hint)).getText().toString().contains("one heart"));
        Bridge.flask=new long[]{2,31,1,0,0,0,2,0,2,2};update();
        assertTrue(status.getText().toString().contains("empty"));assertEquals(View.VISIBLE,rest.getVisibility());
        assertFalse(rest.isEnabled());rest.performClick();assertEquals(0,Bridge.rests);
        Bridge.flask[4]=2;update();assertTrue(rest.isEnabled());rest.performClick();assertEquals(1,Bridge.rests);
        Bridge.flask[4]=0;Bridge.flask[8]=0;Bridge.flask[5]=1;Bridge.flask[3]=1;Bridge.flask[9]=0;update();
        drink.performClick();assertEquals(1,Bridge.drinks);
        invoke("updateContextualControls",new Class<?>[]{boolean.class},false);assertEquals(View.GONE,panel.getVisibility());
        Bridge.flask[1]=30;update();assertEquals(View.GONE,panel.getVisibility());
    }
    @Test public void activeInputGuidanceAndSessionLessonSurviveSurfaceRecreation()throws Exception {
        MainActivity a=activity();Bridge.flask=new long[]{2,31,1,1,2,1,2,0,2,0};update();
        TextView status=a.findViewById(R.id.flask_status);assertTrue(status.getText().toString().contains("tap Drink"));
        setHud("controllerMode",true);update();assertTrue(status.getText().toString().contains("D-pad Up"));
        assertEquals(View.GONE,a.findViewById(R.id.flask_touch_actions).getVisibility());
        setHud("controllerMode",false);setHud("latestSubtitleInputMode",MainActivity.SUBTITLE_INPUT_KEYBOARD_MOUSE_CONTROLLER);update();
        assertTrue(status.getText().toString().contains("H: Drink"));
        assertTrue(((TextView)a.findViewById(R.id.flask_hint)).getText().toString().contains("J: Rest"));
        setHud("flaskLessonUntil",1L);setHud("surfaceRequestGeneration",32L);Bridge.flask[1]=32;update();
        assertEquals(1L,field("flaskLessonUntil").getLong(a));
        setHud("controllerWindowFocused",false);invoke("requestHealingFlaskAction",new Class<?>[]{boolean.class},false);
        assertEquals(0,Bridge.drinks);
    }
    @Test public void lessonWaitsForTeachingAndInterruptionTakesPriority()throws Exception {
        MainActivity a=activity();TextView teaching=a.findViewById(R.id.combat_teaching_prompt);setHud("combatTeachingPrompt",teaching);
        teaching.setVisibility(View.VISIBLE);Bridge.flask=new long[]{2,31,1,1,0,1,2,0,0,0};update();
        assertFalse(field("flaskFirstLessonShown").getBoolean(a));
        teaching.setVisibility(View.GONE);update();assertTrue(field("flaskFirstLessonShown").getBoolean(a));
        Bridge.flask[9]=5;update();
        assertTrue(((TextView)a.findViewById(R.id.flask_hint)).getText().toString().contains("No charge spent"));
        assertEquals(0,Bridge.drinks);assertEquals(0,Bridge.rests);
    }
    private void largeFont(int width,int height)throws Exception {
        MainActivity a=activity();Configuration config=new Configuration(a.getResources().getConfiguration());config.fontScale=2f;
        a.getResources().updateConfiguration(config,a.getResources().getDisplayMetrics());
        View root=a.findViewById(R.id.root);root.layout(0,0,width,height);
        Bridge.flask=new long[]{2,31,1,1,2,1,2,0,2,0};update();
        root.measure(View.MeasureSpec.makeMeasureSpec(width,View.MeasureSpec.EXACTLY),View.MeasureSpec.makeMeasureSpec(height,View.MeasureSpec.EXACTLY));
        root.layout(0,0,width,height);
        LinearLayout actions=a.findViewById(R.id.flask_touch_actions);assertEquals(LinearLayout.VERTICAL,actions.getOrientation());
        View plate=a.findViewById(R.id.flask_panel),drink=a.findViewById(R.id.drink_flask_button),rest=a.findViewById(R.id.rest_button);
        assertTrue(drink.getHeight()>=48);assertTrue(rest.getHeight()>=48);assertTrue(plate.getBottom()<=height);
        assertTrue(rest.getTop()>=drink.getBottom());
        update();ViewGroup.LayoutParams before=plate.getLayoutParams();update();
        assertSame("Unchanged publication keeps layout parameters",before,plate.getLayoutParams());
    }
    @Test public void largeFontPortraitKeepsWholeTargets()throws Exception {largeFont(480,900);}
    @Test @Config(qualifiers="land") public void largeFontLandscapeKeepsWholeTargets()throws Exception {largeFont(900,480);}
}
