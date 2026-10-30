package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.app.*;
import android.content.*;
import android.net.Uri;
import android.view.*;
import android.widget.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.*;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowAlertDialog;

@RunWith(RobolectricTestRunner.class)
@Config(sdk=34)
public final class CompatibilityReportDialogTest {
    static <T extends View> T find(View view,Class<T> type,String text) {
        if(type.isInstance(view) && view instanceof TextView && text.contentEquals(((TextView)view).getText()))return type.cast(view);
        if(view instanceof ViewGroup)for(int i=0;i<((ViewGroup)view).getChildCount();i++) {
            T found=find(((ViewGroup)view).getChildAt(i),type,text);if(found!=null)return found;
        }
        return null;
    }
    private void localReviewAndClose() {
        Activity activity=Robolectric.buildActivity(Activity.class).setup().get();
        List<Runnable> writes=new ArrayList<>();int[] calls={0};
        CompatibilityReportDialog owner=new CompatibilityReportDialog(activity,writes::add,()->7L,
            (attempt,id,utc,consent)->{
                calls[0]++;assertEquals(7L,attempt);assertTrue(consent);
                String text=new String(DeveloperReportFlowTest.prepared("Example model"),StandardCharsets.UTF_8)
                    .replace(DeveloperReportFlowTest.ID,id).replace(DeveloperReportFlowTest.UTC,utc);
                return text.getBytes(StandardCharsets.UTF_8);
            });
        owner.show();AlertDialog dialog=ShadowAlertDialog.getLatestAlertDialog();assertTrue(dialog.isShowing());
        View root=dialog.getWindow().getDecorView();
        Button prepare=find(root,Button.class,"Prepare report"), save=find(root,Button.class,"Save report");
        assertNotNull(prepare);assertNotNull(save);assertFalse(prepare.isEnabled());assertFalse(save.isEnabled());assertEquals(0,calls[0]);
        CheckBox consent=find(root,CheckBox.class,"Allow these details in a local report for review");
        consent.setChecked(true);assertTrue(prepare.isEnabled());prepare.performClick();
        assertEquals(1,calls[0]);assertTrue(save.isEnabled());assertFalse(prepare.isEnabled());
        assertTrue(save.getMinHeight()>=48*activity.getResources().getDisplayMetrics().density);
        find(root,Button.class,"Copy report").performClick();
        ClipboardManager clipboard=(ClipboardManager)activity.getSystemService(Context.CLIPBOARD_SERVICE);
        String copied=clipboard.getPrimaryClip().getItemAt(0).getText().toString();
        assertTrue(copied.contains("\"rtPresented\":false"));assertFalse(copied.contains("consentToSubmit"));
        save.performClick();assertTrue(owner.pickerOutstanding());
        owner.close();assertFalse(dialog.isShowing());
        owner.finishPicker(Activity.RESULT_OK,new Intent().setData(Uri.parse("content://local-mock/report")));
        assertFalse(owner.pickerOutstanding());assertTrue(writes.isEmpty());assertEquals(1,calls[0]);
        activity.finish();
    }
    @Test @Config(qualifiers="port") public void portraitLocalConsentCopyAndClosedPickerOwnership() {localReviewAndClose();}
    @Test @Config(qualifiers="land") public void landscapeLocalConsentCopyAndClosedPickerOwnership() {localReviewAndClose();}
    @Test public void unavailableNativeComponentStillOffersResponsiveClose() {
        Activity activity=Robolectric.buildActivity(Activity.class).setup().get();
        CompatibilityReportDialog owner=new CompatibilityReportDialog(activity,Runnable::run,()->{throw new UnsatisfiedLinkError();},
            (a,b,c,d)->{fail("no preparation");return null;});
        owner.show();AlertDialog dialog=ShadowAlertDialog.getLatestAlertDialog();assertTrue(dialog.isShowing());
        dialog.getButton(AlertDialog.BUTTON_NEGATIVE).performClick();
        // AlertDialog posts its platform button callback to the main looper.
        Shadows.shadowOf(android.os.Looper.getMainLooper()).idle();
        assertFalse(dialog.isShowing());activity.finish();
    }
}
