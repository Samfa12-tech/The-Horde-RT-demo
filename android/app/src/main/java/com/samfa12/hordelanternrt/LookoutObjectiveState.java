package com.samfa12.hordelanternrt;

/** Immutable menu eligibility only. Never completes a quest or writes a save. */
final class LookoutObjectiveState {
    final long surfaceGeneration, worldGeneration;
    final int objective;
    final boolean available;
    private LookoutObjectiveState(long[] values) {
        surfaceGeneration=values[1];worldGeneration=values[2];objective=(int)values[3];
        available=objective!=0;
    }
    static LookoutObjectiveState decode(long[] values,long currentSurface) {
        if(values==null || values.length!=4 || values[0]!=2 || currentSurface<=0 ||
                values[1]!=currentSurface || values[2]<=0 || values[3]<0 || values[3]>9)return null;
        return new LookoutObjectiveState(values);
    }
    boolean isLookout() { return objective==1; }
    boolean matches(LookoutObjectiveState other) {
        return available && other!=null && other.available &&
                surfaceGeneration==other.surfaceGeneration && worldGeneration==other.worldGeneration &&
                objective==other.objective;
    }
}
