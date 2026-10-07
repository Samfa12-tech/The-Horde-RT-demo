package com.samfa12.hordelanternrt;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;

import org.junit.Test;

public class EquipmentAudioFeedbackTest {
    @Test public void attachmentTransportSelectsDistinctCues() {
        assertEquals("sword_draw", EquipmentAudioFeedback.soundKey(22, 1, true));
        assertEquals("sword_sheath", EquipmentAudioFeedback.soundKey(22, 2, true));
    }

    @Test public void drawStartCannotDuplicateAttachmentAudio() {
        assertNull(EquipmentAudioFeedback.soundKey(21, 1, true));
        assertNull(EquipmentAudioFeedback.soundKey(1, 1, true));
    }

    @Test public void inactiveAndUnknownEventsAreDiscarded() {
        assertNull(EquipmentAudioFeedback.soundKey(22, 1, false));
        assertNull(EquipmentAudioFeedback.soundKey(22, 2, false));
        assertNull(EquipmentAudioFeedback.soundKey(22, 0, true));
        assertNull(EquipmentAudioFeedback.soundKey(22, 3, true));
    }
}
