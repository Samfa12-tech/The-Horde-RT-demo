package com.samfa12.hordelanternrt;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.widget.*;
import android.graphics.Typeface;
import java.text.SimpleDateFormat;
import java.util.*;
import java.util.concurrent.Executor;
import java.util.function.LongSupplier;

/** Local review/export survives renderer failure; no live-send capability. */
final class CompatibilityReportDialog {
    static final int SAVE_REQUEST = 7104;
    private final Activity activity;
    private final Executor worker;
    private final LongSupplier revision;
    private final CompatibilityReportReview.Preparer prepare;
    private final Handler ui = new Handler(Looper.getMainLooper());
    private CompatibilityReportReview review;
    private AlertDialog dialog;
    private TextView status, json;
    private Button prepareButton, copy, save;
    private boolean closed, pickerOutstanding;
    private long pickerToken;

    CompatibilityReportDialog(Activity activity, Executor worker, LongSupplier revision,
            CompatibilityReportReview.Preparer prepare) {
        this.activity=activity; this.worker=worker; this.revision=revision; this.prepare=prepare;
    }
    boolean pickerOutstanding() { return pickerOutstanding; }
    private int dp(int value) { return Math.round(activity.getResources().getDisplayMetrics().density*value); }
    private TextView body(LinearLayout panel,String text) {
        TextView view=new TextView(activity); view.setText(text); view.setTextColor(HordeUiTokens.PARCHMENT);
        view.setTextSize(14); view.setPadding(0,dp(8),0,dp(8)); panel.addView(view); return view;
    }
    private Button button(LinearLayout panel,String text,Runnable action) {
        Button button=new Button(activity); button.setText(text); button.setMinHeight(dp(48));
        button.setOnClickListener(v -> {if(!closed)action.run();}); panel.addView(button); return button;
    }
    void show() {
        long current;
        try {current=revision.getAsLong();} catch(RuntimeException | LinkageError unavailable) {current=0;}
        if(closed)return;
        final long expected=current;
        if(expected<=0) {
            dialog=new AlertDialog.Builder(activity).setTitle("Report compatibility to developer")
                .setMessage("Device details are unavailable because the reporting component could not start. Nothing has been sent.")
                .setNegativeButton("Close",(d,w)->close()).create();
            dialog.setOnDismissListener(d->close());dialog.show();return;
        }
        review=new CompatibilityReportReview(expected);
        LinearLayout panel=new LinearLayout(activity);panel.setOrientation(LinearLayout.VERTICAL);
        panel.setPadding(dp(20),dp(8),dp(20),dp(8));panel.setBackgroundColor(0xFF181512);
        body(panel,"Review device compatibility");
        body(panel,"Prepare a private report with the app version, device/OS/GPU information, capability checks and startup result. "
                +"No serial numbers, saves, screenshots or raw logs. Nothing is sent automatically or added to a public board.");
        CheckBox consent=new CheckBox(activity);consent.setText("Allow these details in a local report for review");
        consent.setTextColor(HordeUiTokens.PARCHMENT);consent.setMinHeight(dp(48));panel.addView(consent);
        status=body(panel,"Sending is not available yet. You can copy or save the reviewed report.");
        prepareButton=button(panel,"Prepare report",() -> {
            SimpleDateFormat utc=new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss'Z'",Locale.ROOT);utc.setTimeZone(TimeZone.getTimeZone("UTC"));
            final boolean ready;
            try {ready=review.prepare(revision.getAsLong(),consent.isChecked(),UUID.randomUUID().toString(),utc.format(new Date()),prepare);}
            catch(RuntimeException | LinkageError unavailable) {status.setText("Report preparation is unavailable. You can close this screen.");return;}
            if(!ready) {status.setText("Details changed or could not be prepared. Close and reopen this report to try again.");return;}
            consent.setEnabled(false);prepareButton.setEnabled(false);json.setText(review.json());
            copy.setEnabled(true);save.setEnabled(true);status.setText("Review these exact details before copying or saving. Nothing has been sent.");
        });prepareButton.setEnabled(false);
        consent.setOnCheckedChangeListener((v,checked)->{if(!closed)prepareButton.setEnabled(checked&&!review.isPrepared());});
        json=body(panel,"");json.setTypeface(Typeface.MONOSPACE);json.setTextIsSelectable(true);
        copy=button(panel,"Copy report",() -> {
            if(!review.isPrepared())return;
            ClipboardManager clipboard=(ClipboardManager)activity.getSystemService(Context.CLIPBOARD_SERVICE);
            if(clipboard==null) {status.setText("Could not copy. Your report is still here.");return;}
            clipboard.setPrimaryClip(ClipData.newPlainText("Horde compatibility report",review.json()));status.setText("Report copied. Nothing has been sent.");
        });copy.setEnabled(false);
        save=button(panel,"Save report",this::chooseDestination);save.setEnabled(false);
        ScrollView scroll=new ScrollView(activity);scroll.addView(panel);
        dialog=new AlertDialog.Builder(activity).setTitle("Report compatibility to developer").setView(scroll)
                .setNegativeButton("Close",(d,w)->close()).create();
        dialog.setOnDismissListener(d->close());dialog.show();
    }
    private void chooseDestination() {
        if(closed || pickerOutstanding || !review.beginSave())return;
        pickerOutstanding=true;pickerToken=review.exportOwner().token();save.setEnabled(false);
        Intent intent=new Intent(Intent.ACTION_CREATE_DOCUMENT).addCategory(Intent.CATEGORY_OPENABLE).setType("application/json")
                .putExtra(Intent.EXTRA_TITLE,"HordeLanternRT-compatibility.json");
        try {activity.startActivityForResult(intent,SAVE_REQUEST);}
        catch(RuntimeException unavailable) {pickerOutstanding=false;review.exportOwner().pickerCancelled(pickerToken);saveFailed();}
    }
    private void saveFailed() {
        if(closed)return;status.setText("Not saved. Your report is still here; choose Save report to retry.");save.setEnabled(true);
    }
    void finishPicker(int result,Intent data) {
        if(!pickerOutstanding)return;pickerOutstanding=false;
        if(closed || !review.isPrepared())return;
        PlaytestReportExport owner=review.exportOwner();long attempt=pickerToken;
        if(result!=Activity.RESULT_OK || data==null || data.getData()==null) {owner.pickerCancelled(attempt);saveFailed();return;}
        byte[] bytes=owner.startWrite(attempt);if(bytes==null)return;
        Uri uri=data.getData();android.content.ContentResolver resolver=activity.getApplicationContext().getContentResolver();
        status.setText("Saving report…");
        try {worker.execute(()->{
            boolean saved=PlaytestReportExport.writeApproved(owner,attempt,bytes,()->resolver.openOutputStream(uri,"wt"));
            ui.post(()->{if(closed || !owner.complete(attempt,saved))return;
                if(saved)status.setText("Report saved. Nothing has been sent.");else saveFailed();});
        });}catch(RuntimeException unavailable) {owner.complete(attempt,false);saveFailed();}
    }
    void close() {
        if(closed)return;closed=true;if(review!=null)review.close();
        if(dialog!=null)dialog.dismiss();
    }
}
