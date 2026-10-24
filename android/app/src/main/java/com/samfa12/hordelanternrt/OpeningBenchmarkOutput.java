package com.samfa12.hordelanternrt;

import android.view.Gravity;
import android.view.SurfaceView;
import android.view.ViewGroup;
import android.widget.FrameLayout;

/** One request's SurfaceView-only sizing transaction. Saved graphics and HUD siblings are never changed. */
final class OpeningBenchmarkOutput {
    static final int APPLY_FIXED=1, RESTORE_LAYOUT=2, WIDTH=1280, HEIGHT=720;
    static final class Command {
        final long request,origin,generation,sequence;
        final int operation,width,height;
        private Command(long[] v) {
            request=v[0];origin=v[1];generation=v[2];sequence=v[3];operation=(int)v[4];width=(int)v[5];height=(int)v[6];
        }
        static Command decode(long[] v) {
            if(v==null || v.length!=8 || v[0]<=0 || v[1]<=0 || v[2]<=0 || v[3]<=0 ||
                    (v[4]!=APPLY_FIXED && v[4]!=RESTORE_LAYOUT) || v[5]<0 || v[5]>Integer.MAX_VALUE ||
                    v[6]<0 || v[6]>Integer.MAX_VALUE || v[7]<0 || v[7]>Integer.MAX_VALUE) return null;
            return new Command(v);
        }
        boolean same(Command other) {
            return other!=null && request==other.request && origin==other.origin && generation==other.generation &&
                sequence==other.sequence && operation==other.operation && width==other.width && height==other.height;
        }
    }
    static final class Ack {
        final Command command;
        final boolean success,layoutRestoredOrAspectPreserved;
        final int width,height;
        Ack(Command c,boolean pass,int w,int h,boolean geometry) {
            command=c;success=pass;width=w;height=h;layoutRestoredOrAspectPreserved=geometry;
        }
    }
    private final long request,origin;
    private final FrameLayout.LayoutParams original;
    private Command current;
    private long newestSequence;
    private boolean applied,failed,acknowledged,touched,restored,observationBoundary;

    static FrameLayout.LayoutParams snapshot(SurfaceView view) {
        return view!=null && view.getLayoutParams() instanceof FrameLayout.LayoutParams ?
            new FrameLayout.LayoutParams((FrameLayout.LayoutParams)view.getLayoutParams()) : null;
    }
    OpeningBenchmarkOutput(long request,long origin,FrameLayout.LayoutParams snapshot) {
        this.request=request;this.origin=origin;original=new FrameLayout.LayoutParams(snapshot);
    }
    OpeningBenchmarkOutput(long request,long origin,FrameLayout.LayoutParams snapshot,boolean recovery) {
        this(request,origin,snapshot);touched=recovery; // A recreated client must observe an explicit restore ACK.
    }
    FrameLayout.LayoutParams originalLayout(){return new FrameLayout.LayoutParams(original);}
    Command current(){return current;}
    boolean canRelease(){return !touched || restored;}
    boolean acceptsOwner(Command c,long generation) {
        return c!=null && c.request==request && c.origin==origin && generation>0 && c.generation==generation;
    }
    boolean begin(Command c,long generation,boolean cancelled,SurfaceView view) {
        if(!acceptsOwner(c,generation) || c.sequence<=newestSequence) return false;
        current=c;newestSequence=c.sequence;applied=failed=acknowledged=observationBoundary=false;
        if(c.operation==APPLY_FIXED && (cancelled || c.width!=WIDTH || c.height!=HEIGHT)) failed=true;
        else apply(view);
        return true;
    }
    void observedLayoutOrPost(Command c) {
        if(current!=null && current.same(c))observationBoundary=true;
    }
    private void apply(SurfaceView view) {
        if(failed || applied || current==null) return;
        try {
            if(current.operation==RESTORE_LAYOUT) {
                if(!sameLayout(view.getLayoutParams(),original))view.setLayoutParams(new FrameLayout.LayoutParams(original));
                view.getHolder().setSizeFromLayout();
            } else {
                if(!(view.getParent() instanceof ViewGroup)) { failed=true;return; }
                ViewGroup parent=(ViewGroup)view.getParent();
                final int availableWidth=parent.getWidth()-parent.getPaddingLeft()-parent.getPaddingRight(),
                    availableHeight=parent.getHeight()-parent.getPaddingTop()-parent.getPaddingBottom();
                int[] size=fit(availableWidth,availableHeight);
                if(size==null)return; // Wait for real parent layout, not display-size guesses.
                touched=true;restored=false;
                FrameLayout.LayoutParams fixed=new FrameLayout.LayoutParams(size[0],size[1],Gravity.CENTER);
                if(!sameLayout(view.getLayoutParams(),fixed))view.setLayoutParams(fixed);
                view.getHolder().setFixedSize(WIDTH,HEIGHT);
            }
            applied=true;
        } catch(RuntimeException unavailable) { failed=true; }
    }
    // Inputs are measured from this holder by the Activity, never copied from the command.
    Ack observe(Command published,long generation,boolean interrupted,SurfaceView view,
                boolean surfaceValid,int actualWidth,int actualHeight) {
        if(acknowledged || !acceptsOwner(published,generation) || current==null || !current.same(published))return null;
        if(interrupted && current.operation==APPLY_FIXED)failed=true;
        if(failed)return new Ack(current,false,actualWidth,actualHeight,false);
        apply(view);
        if(failed)return new Ack(current,false,actualWidth,actualHeight,false);
        if(!applied || !observationBoundary || !surfaceValid || !view.isLaidOut() || view.isLayoutRequested() || actualWidth<=0 || actualHeight<=0)return null;
        boolean geometry=current.operation==RESTORE_LAYOUT ?
            sameLayout(view.getLayoutParams(),original) && view.getWidth()==actualWidth && view.getHeight()==actualHeight :
            aspectPreserved(view) && actualWidth==WIDTH && actualHeight==HEIGHT;
        return geometry?new Ack(current,true,actualWidth,actualHeight,true):null;
    }
    void acknowledged(Ack ack) {
        if(current!=null && current.same(ack.command)) {
            acknowledged=true;
            if(ack.success && current.operation==RESTORE_LAYOUT)restored=true;
        }
    }
    static int[] fit(int width,int height) {
        if(width<=0 || height<=0)return null;
        if((long)width*HEIGHT<=(long)height*WIDTH)
            return new int[]{width,Math.max(1,Math.min(height,Math.round(width*(float)HEIGHT/WIDTH)))};
        return new int[]{Math.max(1,Math.min(width,Math.round(height*(float)WIDTH/HEIGHT))),height};
    }
    static boolean aspectPreserved(SurfaceView view) {
        if(!(view.getParent() instanceof ViewGroup))return false;
        ViewGroup parent=(ViewGroup)view.getParent();
        int availableWidth=parent.getWidth()-parent.getPaddingLeft()-parent.getPaddingRight(),
            availableHeight=parent.getHeight()-parent.getPaddingTop()-parent.getPaddingBottom();
        int[] expected=fit(availableWidth,availableHeight);
        if(expected==null || view.getWidth()!=expected[0] || view.getHeight()!=expected[1] ||
                Math.abs((long)view.getWidth()*HEIGHT-(long)view.getHeight()*WIDTH)>WIDTH)return false;
        int left=parent.getPaddingLeft()+(availableWidth-view.getWidth())/2,
            top=parent.getPaddingTop()+(availableHeight-view.getHeight())/2;
        return Math.abs(view.getLeft()-left)<=1 && Math.abs(view.getTop()-top)<=1;
    }
    static boolean sameLayout(ViewGroup.LayoutParams value,FrameLayout.LayoutParams expected) {
        if(!(value instanceof FrameLayout.LayoutParams))return false;
        FrameLayout.LayoutParams p=(FrameLayout.LayoutParams)value;
        return p.width==expected.width && p.height==expected.height && p.gravity==expected.gravity &&
            p.leftMargin==expected.leftMargin && p.topMargin==expected.topMargin &&
            p.rightMargin==expected.rightMargin && p.bottomMargin==expected.bottomMargin &&
            p.getMarginStart()==expected.getMarginStart() && p.getMarginEnd()==expected.getMarginEnd() &&
            p.getLayoutDirection()==expected.getLayoutDirection();
    }
}
