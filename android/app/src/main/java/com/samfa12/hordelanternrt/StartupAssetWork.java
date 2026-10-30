package com.samfa12.hordelanternrt;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** One process-wide cache writer; Activity replacement never joins it on UI. */
final class StartupAssetWork {
    private static final ExecutorService SERIAL=Executors.newSingleThreadExecutor(work -> {
        Thread thread=new Thread(work,"HordeStartupAssets");thread.setDaemon(true);return thread;
    });
    static void execute(Runnable work) {SERIAL.execute(work);}
    private StartupAssetWork() { }
}
