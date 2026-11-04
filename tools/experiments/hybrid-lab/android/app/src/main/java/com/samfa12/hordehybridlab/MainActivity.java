package com.samfa12.hordehybridlab;

import android.app.Activity;
import android.os.Bundle;
import android.graphics.Color;
import android.content.Intent;
import android.view.View;
import android.webkit.WebView;
import android.widget.*;
import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.io.FileInputStream;
import java.io.OutputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

public class MainActivity extends Activity {
    static { System.loadLibrary("horde_hybrid_lab"); }
    private native String runNative(String output,int width,int height);
    private volatile boolean running;
    private TextView status;
    private Button run;
    private WebView report;
    private Button export;
    private File completed;
    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        LinearLayout root=new LinearLayout(this);root.setOrientation(LinearLayout.VERTICAL);root.setPadding(16,16,16,16);root.setBackgroundColor(Color.rgb(18,24,27));
        root.setOnApplyWindowInsetsListener((v,insets)->{v.setPadding(16+insets.getSystemWindowInsetLeft(),16+insets.getSystemWindowInsetTop(),16+insets.getSystemWindowInsetRight(),16+insets.getSystemWindowInsetBottom());return insets;});
        TextView title=new TextView(this);title.setText("The Horde / Hybrid Lab");title.setTextSize(24);title.setTextColor(Color.WHITE);root.addView(title);
        status=new TextView(this);status.setText("Experimental offscreen Vulkan test. Full hardware RT stays available. This is a tomb slice, not the full game. Keep the phone cool, use the same power state, and repeat runs. Reports remain on this device.");status.setTextColor(Color.WHITE);status.setTextSize(16);root.addView(status);
        Spinner resolution=new Spinner(this);resolution.setAdapter(new ArrayAdapter<String>(this,android.R.layout.simple_spinner_dropdown_item,new String[]{"640 × 360", "1280 × 720"}));root.addView(resolution);
        run=new Button(this);run.setText("Run full RT / hybrid comparison");root.addView(run);
        export=new Button(this);export.setText("Save report ZIP");export.setEnabled(false);root.addView(export);
        report=new WebView(this);report.getSettings().setAllowFileAccess(true);report.getSettings().setJavaScriptEnabled(false);root.addView(report,new LinearLayout.LayoutParams(-1,0,1));
        setContentView(root);
        export.setOnClickListener(v->{
            Intent intent=new Intent(Intent.ACTION_CREATE_DOCUMENT);intent.addCategory(Intent.CATEGORY_OPENABLE);intent.setType("application/zip");intent.putExtra(Intent.EXTRA_TITLE,"Horde-Hybrid-"+completed.getName()+".zip");startActivityForResult(intent,10);
        });
        run.setOnClickListener(v->{
            if(running)return;running=true;run.setEnabled(false);resolution.setEnabled(false);
            final int width=resolution.getSelectedItemPosition()==0?640:1280;final int height=width*9/16;
            File base=getExternalFilesDir(null);if(base==null)base=getFilesDir();
            final File output=new File(base,"run-"+new SimpleDateFormat("yyyyMMdd-HHmmss-SSS",Locale.US).format(new Date()));
            status.setText("Running eight interleaved blocks per scene. Please keep the app visible. Readback and report generation are excluded from GPU timing.");
            new Thread(()->{
                final String result=runNative(output.getAbsolutePath(),width,height);
                runOnUiThread(()->{
                    if(isFinishing()||isDestroyed())return;
                    running=false;run.setEnabled(true);resolution.setEnabled(true);
                    if(result.startsWith("FAILED:")){status.setText(result);}
                    else {status.setText("Completed. Compare the images and measured timings below. Save the report ZIP to share the results.");completed=output;export.setEnabled(true);report.loadUrl("file://"+new File(output,"index.html").getAbsolutePath());}
                });
            },"hybrid-vulkan-lab").start();
        });
    }
    @Override protected void onActivityResult(int request,int result,Intent data) {
        super.onActivityResult(request,result,data);
        if(request!=10||result!=RESULT_OK||data==null||data.getData()==null||completed==null)return;
        final File source=completed;final android.net.Uri destination=data.getData();export.setEnabled(false);
        new Thread(()->{
            String message;
            try(OutputStream stream=getContentResolver().openOutputStream(destination);
                ZipOutputStream zip=new ZipOutputStream(stream)) {
                File[] files=source.listFiles();if(files==null)throw new java.io.IOException("Report directory unavailable");
                byte[] buffer=new byte[65536];
                for(File file:files)if(file.isFile()) {
                    zip.putNextEntry(new ZipEntry(file.getName()));
                    try(FileInputStream input=new FileInputStream(file)){int n;while((n=input.read(buffer))!=-1)zip.write(buffer,0,n);}
                    zip.closeEntry();
                }
                message="Report ZIP saved.";
            } catch(Exception error){message="Report export failed: "+error.getMessage();}
            final String messageFinal=message;runOnUiThread(()->{if(!isFinishing()&&!isDestroyed()){status.setText(messageFinal);export.setEnabled(true);}});
        },"hybrid-report-export").start();
    }
}
