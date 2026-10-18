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
    final boolean canSaveLatest, presented, isolatedNamespace;
    final float yaw, pitch;
    private final int[] entries;

    private CampaignMenuState(long[] v) {
        surfaceGeneration=v[1]; revision=v[2]; ackRequest=v[3]; phase=(int)v[4];
        error=(int)v[5]; activeSlot=(int)v[6]; canSaveLatest=v[7]==1; rebaseToken=v[8];
        yaw=Float.intBitsToFloat((int)v[9]); pitch=Float.intBitsToFloat((int)v[10]);
        presented=v[11]==1; lastOperation=(int)v[30]; durability=(int)v[31]; isolatedNamespace=v[32]==1;
        entries=new int[18];
        for(int i=0;i<18;i++) entries[i]=(int)v[12+i];
    }
    static CampaignMenuState decode(long[] v) {
        if(v==null || v.length<33 || v[0]!=1 || v[1]<0 || v[2]<0 || v[3]<0 ||
                v[4]<0 || v[4]>FAILED || v[5]<0 || v[5]>Integer.MAX_VALUE ||
                v[6]<-1 || v[6]>2 || !bit(v[7]) || v[8]<0 || !bit(v[11]) ||
                v[30]<0 || v[30]>NEW_GAME || v[31]<0 || v[31]>2 || !bit(v[32])) return null;
        for(int i=0;i<6;i++) {
            int p=12+3*i;
            if(v[p]<UNKNOWN || v[p]>READ_ERROR || v[p+1]<0 || v[p+1]>2 || v[p+2]<0 || v[p+2]>3) return null;
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
    boolean occupied(int slot) { return status(slot,MANUAL)!=EMPTY || status(slot,AUTO)!=EMPTY; }
    String description(int slot,int kind) {
        int p=(slot*2+kind)*3;
        String prefix=kind==MANUAL ? "Manual: " : "Latest autosave: ";
        switch(entries[p]) {
            case EMPTY:return prefix+"Empty";
            case CHECKING:return prefix+"Checking…";
            case VALID:return prefix+(entries[p+1]==1 ? "Lower chamber" : "Safe exit")+" · "+entries[p+2]+" / 3 hearts";
            case INVALID:return prefix+"Unavailable or unsupported record";
            case READ_ERROR:return prefix+"Could not read record";
            default:return prefix+"Not checked";
        }
    }
    String message() {
        if(error!=0) {
            switch(error) {
                case 1:return "Save storage is unavailable.";
                case 2:return "Another request is pending. Try again when it finishes.";
                case 3:return "No safe checkpoint is available yet.";
                case 4:return "That record is empty.";
                case 5:return "That record is invalid or unsupported. It has been kept.";
                case 6:return "Storage could not complete the request. Refresh before retrying.";
                case 7:return "The destination RT scene is not ready. The game remains paused.";
                case 8:return "Request cancelled or interrupted. Refresh to check the records.";
                case 9:return "The checkpoint could not be restored. The game remains paused.";
                case 10:return "Write outcome is uncertain. Refresh the records before saving again.";
                case 11:return "The private save directory could not be prepared.";
                default:return "Could not complete the request. Refresh to check the previous records.";
            }
        }
        switch(phase) {
            case SAVING:return "Saving safe checkpoint…";
            case READING:return "Reading checkpoint…";
            case PREPARING:return "Preparing destination RT resources…";
            case REBASE:return "Restoring camera and releasing controls…";
            case PRESENTING:return "Waiting for the restored RT scene…";
            case SUCCESS:
                if(lastOperation==0)return "Latest autosave saved.";
                if(lastOperation==LOAD)return presented ? "Loaded. The game is paused until you choose Resume." : "Waiting for the restored RT scene…";
                if(lastOperation==SAVE_MANUAL)return "Manual checkpoint saved.";
                if(lastOperation==NEW_GAME)return "Slot selected. Ready to begin.";
                return "Records checked.";
            case FAILED:return "Could not complete the request. Refresh before retrying.";
            case UNAVAILABLE:return "Save storage is unavailable.";
            default:return "Manual saves and the latest autosave are kept separately.";
        }
    }
    static String testNamespace(boolean debug,String value) {
        return debug && value!=null && value.matches("[a-zA-Z0-9_-]{1,48}") ? value : null;
    }
}
