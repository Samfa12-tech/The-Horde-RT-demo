package com.samfa12.hordelanternrt;

/** Immutable owner publication; UI eligibility never performs healing or starts a timer. */
final class HealingFlaskUiState {
    final long generation;
    final boolean enabled, drinkEligible;
    final int restAnchor, charges, capacity, action;
    private HealingFlaskUiState(long[] v) {
        generation=v[1];enabled=v[2]==1;drinkEligible=v[3]==1;
        restAnchor=(int)v[4];charges=(int)v[5];capacity=(int)v[6];action=(int)v[7];
    }
    static HealingFlaskUiState decode(long[] v,long generation) {
        if(v==null || v.length!=8 || v[0]!=1 || generation<=0 || v[1]!=generation ||
           v[2]<0 || v[2]>1 || v[3]<0 || v[3]>1 || v[4]<0 || v[4]>3 ||
           v[5]<0 || v[5]>v[6] || v[6]<2 || v[6]>3 || v[7]<0 || v[7]>2)return null;
        return new HealingFlaskUiState(v);
    }
    boolean allows(boolean rest) {return enabled && action==0 && (rest?restAnchor!=0:drinkEligible);}
}
