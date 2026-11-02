package com.samfa12.hordelanternrt;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/** Presentation only: native timings and measurement ownership are untouched. */
final class BenchmarkProgressText {
    private static final Pattern COURSE=Pattern.compile(
            "BENCHMARK\\s+(\\d+/\\d+)\\s*\\|\\s*WAYPOINT\\s+(\\d+/\\d+)(?:\\s*\\|.*)?");
    static String compact(String progress) {
        if(progress==null || progress.trim().isEmpty())return "";
        String[] lines=progress.trim().split("\\r?\\n");
        String course=lines[lines.length-1].trim();
        Matcher match=COURSE.matcher(course);
        return match.matches()?"Lap "+match.group(1)+" / Waypoint "+match.group(2):course;
    }
    private BenchmarkProgressText() {}
}
