package com.samfa12.hordelanternrt;

/** Immutable, versioned save-menu publication. Never infers storage or renderer success. */
final class CampaignMenuState {
    static final int REFRESH=1, SAVE_MANUAL=2, LOAD=3, CANCEL=4, NEW_GAME=5;
    static final int MANUAL=0, AUTO=1;
    static final int UNAVAILABLE=0, READY=1, SAVING=2, READING=3, PREPARING=4,
            REBASE=5, PRESENTING=6, SUCCESS=7, FAILED=8;
    static final int UNKNOWN=0, CHECKING=1, EMPTY=2, VALID=3, INVALID=4, READ_ERROR=5;
    final long surfaceGeneration, revision, ackRequest, rebaseToken;
    final int phase, error, activeSlot, lastOperation, durability;
    final boolean canSaveLatest, presented, isolatedNamespace, confirmedRetry;
    final float yaw, pitch;
    private final int[] entries;
    private final long[] savedTimes;

    private CampaignMenuState(long[] v) {
        surfaceGeneration=v[1]; revision=v[2]; ackRequest=v[3]; phase=(int)v[4];
        error=(int)v[5]; activeSlot=(int)v[6]; canSaveLatest=v[7]==1; rebaseToken=v[8];
        yaw=Float.intBitsToFloat((int)v[9]); pitch=Float.intBitsToFloat((int)v[10]);
        presented=v[11]==1; lastOperation=(int)v[30]; durability=(int)v[31]; isolatedNamespace=v[32]==1;
        confirmedRetry=v.length>33 && v[33]==1;
        entries=new int[18];
        for(int i=0;i<18;i++) entries[i]=(int)v[12+i];
        savedTimes=new long[6];
        if(v[0]==2)for(int i=0;i<6;i++)savedTimes[i]=v[34+i];
    }
    static CampaignMenuState decode(long[] v) {
        if(v==null || v.length<33 || !((v[0]==1 && (v.length==33 || v.length==34)) || (v[0]==2 && v.length==40)) || v[1]<0 || v[2]<0 || v[3]<0 ||
                v[4]<0 || v[4]>FAILED || v[5]<0 || v[5]>Integer.MAX_VALUE ||
                v[6]<-1 || v[6]>2 || !bit(v[7]) || v[8]<0 || !bit(v[11]) ||
                v[30]<0 || v[30]>NEW_GAME || v[31]<0 || v[31]>2 || !bit(v[32])) return null;
        if(v.length>33 && !bit(v[33]))return null;
        if(v[0]==2)for(int i=34;i<40;i++)if(v[i]<0 || v[i]>253402300799L)return null;
        for(int i=0;i<6;i++) {
            int p=12+3*i;
            if(v[p]<UNKNOWN || v[p]>READ_ERROR || v[p+1]<0 || v[p+1]>3 || v[p+2]<0 || v[p+2]>3) return null;
            if(v[p]==VALID && (v[p+1]<1 || v[p+2]<1)) return null;
        }
        float yaw=Float.intBitsToFloat((int)v[9]), pitch=Float.intBitsToFloat((int)v[10]);
        if(!Float.isFinite(yaw) || !Float.isFinite(pitch) || pitch<-.32f || pitch>.28f ||
                (v[9]!=(long)(int)v[9] && v[9]!=Integer.toUnsignedLong((int)v[9])) ||
                (v[10]!=(long)(int)v[10] && v[10]!=Integer.toUnsignedLong((int)v[10]))) return null;
        if(v[4]==REBASE && (v[8]==0 || v[1]==0)) return null;
        return new CampaignMenuState(v);
    }
    private static boolean bit(long v) { return v==0 || v==1; }
    boolean current(long generation) { return generation>0 && generation==surfaceGeneration; }
    boolean pending() { return phase>=SAVING && phase<=PRESENTING; }
    boolean loadBarrier() { return phase>=READING && phase<=PRESENTING; }
    boolean completes(long request, int operation, long generation) {
        return request>0 && current(generation) && ackRequest==request && lastOperation==operation &&
                phase==SUCCESS && error==0 && (operation!=LOAD || presented);
    }
    int status(int slot,int kind) { return entries[(slot*2+kind)*3]; }
    long savedTime(int slot,int kind) {return savedTimes[slot*2+kind];}
    int selectedKind(int slot) {
        if(slot<0 || slot>=3 || status(slot,MANUAL)<=CHECKING || status(slot,AUTO)<=CHECKING)return -1;
        boolean m=status(slot,MANUAL)==VALID,a=status(slot,AUTO)==VALID;
        if(!m && !a)return -1;
        if(!m)return AUTO;
        if(!a)return MANUAL;
        long mt=savedTime(slot,MANUAL),at=savedTime(slot,AUTO);
        return mt>at || (mt!=0 && mt==at) ? MANUAL : AUTO;
    }
    static String formatSavedTime(long seconds,java.util.Locale locale,java.util.TimeZone zone) {
        if(seconds<=0 || seconds>253402300799L)return "Date unavailable";
        java.text.DateFormat format=java.text.DateFormat.getDateTimeInstance(java.text.DateFormat.MEDIUM,java.text.DateFormat.SHORT,locale);
        format.setTimeZone(zone);
        return format.format(new java.util.Date(seconds*1000L));
    }
    String slotDescription(int slot) {
        int kind=selectedKind(slot);
        if(kind<0) {
            if(status(slot,MANUAL)<=CHECKING || status(slot,AUTO)<=CHECKING)return "Checking saves...";
            if(status(slot,MANUAL)==EMPTY && status(slot,AUTO)==EMPTY)return "Empty";
            return "Save unavailable. Your files have been kept.";
        }
        int p=(slot*2+kind)*3;
        return (entries[p+1]==1?"Lower chamber":entries[p+1]==2?"Safe exit":"Forest lookout")+
                " | "+entries[p+2]+" / 3 hearts\n"+formatSavedTime(savedTime(slot,kind),java.util.Locale.getDefault(),java.util.TimeZone.getDefault());
    }
    boolean occupied(int slot) { return status(slot,MANUAL)!=EMPTY || status(slot,AUTO)!=EMPTY; }
    boolean hasUnreadable() {
        for(int slot=0;slot<3;slot++)for(int kind=0;kind<2;kind++)
            if(status(slot,kind)==INVALID || status(slot,kind)==READ_ERROR)return true;
        return false;
    }
    String description(int slot,int kind) {
        int p=(slot*2+kind)*3;
        String prefix=kind==MANUAL ? "Manual save: " : "Autosave: ";
        switch(entries[p]) {
            case EMPTY:return prefix+"Empty";
            case CHECKING:return prefix+"Checking…";
            case VALID:return prefix+(entries[p+1]==1 ? "Lower chamber" :
                    entries[p+1]==2 ? "Safe exit" : "Forest lookout")+" · "+entries[p+2]+" / 3 hearts";
            case INVALID:return prefix+"Cannot be loaded · kept";
            case READ_ERROR:return prefix+"Could not be opened · kept";
            default:return prefix+"Not checked yet";
        }
    }
    String message() {
        if(error!=0) {
            switch(error) {
                case 1:return "Your saves are unavailable.";
                case 2:return "Saving or loading is still in progress. Please wait.";
                case 3:return "Reach a safe checkpoint before saving.";
                case 4:return "That save is empty.";
                case 5:return "This save cannot be loaded. It has been kept.";
                case 6:return "Could not save or load. Check your saves before trying again.";
                case 7:return "The area could not load. Your game is still paused.";
                case 8:return "Cancelled or interrupted. Check your saves before trying again.";
                case 9:return "This save could not be restored. Your game is still paused.";
                case 10:return "We could not confirm the save. Check your saves before saving again.";
                case 11:return "Your saves could not be opened.";
                default:return "Could not finish. Check your saves before trying again.";
            }
        }
        switch(phase) {
            case SAVING:return "Saving…";
            case READING:return "Loading…";
            case PREPARING:return "Loading the area…";
            case REBASE:return "Getting ready…";
            case PRESENTING:return "Getting ready…";
            case SUCCESS:
                if(lastOperation==0)return "Saved.";
                if(lastOperation==LOAD)return presented ? "Ready." : "Getting ready…";
                if(lastOperation==SAVE_MANUAL)return "Saved.";
                if(lastOperation==NEW_GAME)return "Ready to begin.";
                return "Saves checked.";
            case FAILED:return "Could not finish. Check your saves before trying again.";
            case UNAVAILABLE:return "Your saves are unavailable.";
            default:return "Choose a slot.";
        }
    }
    static String testNamespace(boolean debug,String value) {
        return debug && value!=null && value.matches("[a-zA-Z0-9_-]{1,48}") ? value : null;
    }
}
