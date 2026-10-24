package com.samfa12.hordelanternrt;

/** Coherent native ownership publication, separate from the course's result. */
final class OpeningBenchmarkState {
    static final int IDLE=0, QUEUED=1, STARTING=2, RUNNING=3, RETURNING=4, COMPLETE=5, BLOCKED=6;
    static final int NO_RESULT=0, SUCCEEDED=2, CANCELLED_OR_FAILED=3;
    final long request, surfaceGeneration, requestSurfaceGeneration;
    final int phase, result;
    final boolean entryPresented;

    private OpeningBenchmarkState(long[] v) {
        request=v[0]; surfaceGeneration=v[1]; phase=(int)v[2]; result=(int)v[3];
        entryPresented=v[4]==1; requestSurfaceGeneration=v[5];
    }
    static OpeningBenchmarkState decode(long[] v) {
        if(v==null || v.length!=6 || v[0]<0 || v[1]<0 || v[2]<IDLE || v[2]>BLOCKED ||
                (v[3]!=NO_RESULT && v[3]!=SUCCEEDED && v[3]!=CANCELLED_OR_FAILED) ||
                (v[4]!=0 && v[4]!=1) || v[5]<0 ||
                (v[0]>0 && v[5]==0) || (v[0]==0 && (v[2]!=IDLE || v[3]!=NO_RESULT || v[5]!=0))) return null;
        return new OpeningBenchmarkState(v);
    }
    boolean matches(long expectedRequest,long expectedOrigin) {
        return expectedRequest>0 && expectedOrigin>0 && request==expectedRequest && requestSurfaceGeneration==expectedOrigin;
    }
    boolean returnedToEntry(long expectedRequest,long expectedOrigin,long currentGeneration) {
        return matches(expectedRequest,expectedOrigin) && currentGeneration>0 && surfaceGeneration==currentGeneration &&
                phase==COMPLETE && entryPresented && (result==SUCCEEDED || result==CANCELLED_OR_FAILED);
    }
    private OpeningBenchmarkState(){throw new AssertionError();}
}
