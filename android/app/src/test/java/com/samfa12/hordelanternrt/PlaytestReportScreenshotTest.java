package com.samfa12.hordelanternrt;

import org.junit.Test;
import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Random;
import java.util.zip.CRC32;
import java.util.zip.Inflater;
import static org.junit.Assert.*;

public final class PlaytestReportScreenshotTest {
    private static final byte[] SIGNATURE = {(byte)137, 80, 78, 71, 13, 10, 26, 10};

    private static int readInt(byte[] bytes, int offset) {
        return ((bytes[offset] & 0xff) << 24) | ((bytes[offset + 1] & 0xff) << 16) |
                ((bytes[offset + 2] & 0xff) << 8) | (bytes[offset + 3] & 0xff);
    }

    private static List<String> verifyChunks(byte[] png) {
        assertNotNull(png);
        assertTrue(png.length <= PlaytestReportScreenshot.MAX_PNG_BYTES);
        assertTrue(png.length >= 57);
        assertArrayEquals(SIGNATURE, Arrays.copyOf(png, SIGNATURE.length));
        List<String> types = new ArrayList<>();
        int at = SIGNATURE.length;
        while (at < png.length) {
            assertTrue("truncated PNG chunk header", png.length - at >= 12);
            int length = readInt(png, at);
            assertTrue(length >= 0 && (long) at + length + 12 <= png.length);
            String type = new String(png, at + 4, 4, StandardCharsets.US_ASCII);
            CRC32 crc = new CRC32();
            crc.update(png, at + 4, length + 4);
            assertEquals("CRC mismatch in " + type, crc.getValue(), readInt(png, at + 8 + length) & 0xffffffffL);
            types.add(type);
            if ("IHDR".equals(type)) {
                assertEquals(13, length);
                assertEquals(8, png[at + 16] & 0xff);
                assertEquals(2, png[at + 17] & 0xff);
                assertEquals(0, png[at + 18] & 0xff);
                assertEquals(0, png[at + 19] & 0xff);
                assertEquals(0, png[at + 20] & 0xff);
            }
            at += length + 12;
            if ("IEND".equals(type)) assertEquals("IEND must end the file", png.length, at);
        }
        assertEquals(png.length, at);
        assertEquals(Arrays.asList("IHDR", "IDAT", "IEND"), types);
        return types;
    }

    private static byte[] decodeRgbRows(byte[] png, int width, int height) throws Exception {
        ByteArrayOutputStream compressed = new ByteArrayOutputStream();
        int at = SIGNATURE.length;
        while (at < png.length) {
            int length = readInt(png, at);
            String type = new String(png, at + 4, 4, StandardCharsets.US_ASCII);
            if ("IDAT".equals(type)) compressed.write(png, at + 8, length);
            at += length + 12;
        }
        byte[] packed = new byte[height * (1 + width * 3)];
        Inflater inflater = new Inflater();
        try {
            inflater.setInput(compressed.toByteArray());
            int written = 0;
            while (!inflater.finished() && written < packed.length) {
                int count = inflater.inflate(packed, written, packed.length - written);
                if (count == 0 && (inflater.needsInput() || inflater.needsDictionary())) break;
                written += count;
            }
            assertEquals("inflated byte count", packed.length, written);
            assertTrue("zlib stream must finish", inflater.finished());
            assertEquals("no compressed trailing bytes", 0, inflater.getRemaining());
        } finally { inflater.end(); }
        byte[] rgb = new byte[height * width * 3];
        for (int y = 0; y < height; ++y) {
            int row = y * (width * 3 + 1);
            assertEquals("lossless Sub filter row " + y, 1, packed[row] & 0xff);
            for (int x = 0; x < width * 3; ++x) {
                int decoded = y * width * 3 + x;
                rgb[decoded] = (byte) (packed[row + 1 + x] + (x >= 3 ? rgb[decoded - 3] : 0));
            }
        }
        return rgb;
    }

    private static byte[] rgba(int width, int height) {
        byte[] pixels = new byte[width * height * 4];
        for (int i = 0; i < pixels.length; i += 4) {
            int pixel = i / 4;
            pixels[i] = (byte) (pixel * 37 + 3);
            pixels[i + 1] = (byte) (pixel * 71 + 11);
            pixels[i + 2] = (byte) (pixel * 19 + 29);
            pixels[i + 3] = (byte) (pixel * 53);
        }
        return pixels;
    }

    @Test public void roundTripsRgbPixelsAndIgnoresAlpha() throws Exception {
        int width = 5, height = 3;
        byte[] pixels = rgba(width, height);
        PlaytestReportScreenshot.Result result = PlaytestReportScreenshot.encodeRgba8(pixels, width, height);
        assertEquals(PlaytestReportScreenshot.Status.ENCODED, result.status);
        assertEquals(width, result.width);
        assertEquals(height, result.height);
        verifyChunks(result.png);
        assertEquals(width, readInt(result.png, 16));
        assertEquals(height, readInt(result.png, 20));
        byte[] decoded = decodeRgbRows(result.png, width, height);
        for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
            int offset = (y * width + x) * 4;
            int decodedOffset = (y * width + x) * 3;
            assertEquals(pixels[offset], decoded[decodedOffset]);
            assertEquals(pixels[offset + 1], decoded[decodedOffset + 1]);
            assertEquals(pixels[offset + 2], decoded[decodedOffset + 2]);
        }
        byte[] differentAlpha = pixels.clone();
        for (int i = 3; i < differentAlpha.length; i += 4) differentAlpha[i] ^= (byte) 0xff;
        assertArrayEquals(result.png, PlaytestReportScreenshot.encodeRgba8(differentAlpha, width, height).png);
    }

    @Test public void validPortraitAndLandscapeLimitsKeepExactDimensions() throws Exception {
        int[][] dimensions = {{720, 1280}, {1280, 720}};
        for (int[] size : dimensions) {
            PlaytestReportScreenshot.Result result = PlaytestReportScreenshot.encodeRgba8(rgba(size[0], size[1]), size[0], size[1]);
            assertTrue(result.status.toString(), result.isEncoded());
            verifyChunks(result.png);
            assertEquals(size[0], readInt(result.png, 16));
            assertEquals(size[1], readInt(result.png, 20));
            assertEquals(size[0] * size[1] * 3, decodeRgbRows(result.png, size[0], size[1]).length);
        }
    }

    @Test public void rejectsInvalidDimensionsAndMalformedPixelLengths() {
        assertEquals(PlaytestReportScreenshot.Status.INVALID_DIMENSIONS,
                PlaytestReportScreenshot.encodeRgba8(new byte[0], 0, 1).status);
        assertEquals(PlaytestReportScreenshot.Status.INVALID_DIMENSIONS,
                PlaytestReportScreenshot.encodeRgba8(new byte[0], -1, 4).status);
        assertEquals(PlaytestReportScreenshot.Status.INVALID_DIMENSIONS,
                PlaytestReportScreenshot.encodeRgba8(new byte[0], 1281, 1).status);
        assertEquals(PlaytestReportScreenshot.Status.INVALID_DIMENSIONS,
                PlaytestReportScreenshot.encodeRgba8(new byte[0], 721, 1280).status);
        assertEquals(PlaytestReportScreenshot.Status.INVALID_PIXELS,
                PlaytestReportScreenshot.encodeRgba8(null, 2, 2).status);
        assertEquals(PlaytestReportScreenshot.Status.INVALID_PIXELS,
                PlaytestReportScreenshot.encodeRgba8(new byte[15], 2, 2).status);
        assertEquals(PlaytestReportScreenshot.Status.INVALID_PIXELS,
                PlaytestReportScreenshot.encodeRgba8(new byte[17], 2, 2).status);
    }

    @Test public void noisyImageAbovePngCapFailsWithoutChangingDimensions() {
        int width = 1280, height = 720;
        byte[] noisy = new byte[width * height * 4];
        new Random(91).nextBytes(noisy);
        PlaytestReportScreenshot.Result result = PlaytestReportScreenshot.encodeRgba8(noisy, width, height);
        assertEquals(PlaytestReportScreenshot.Status.TOO_LARGE, result.status);
        assertNull(result.png);
        assertEquals(width, result.width);
        assertEquals(height, result.height);
    }

    @Test public void emittedChunksHaveNoAncillaryMetadataOrTrailingBytes() {
        PlaytestReportScreenshot.Result result = PlaytestReportScreenshot.encodeRgba8(rgba(4, 2), 4, 2);
        assertTrue(result.isEncoded());
        assertEquals(Arrays.asList("IHDR", "IDAT", "IEND"), verifyChunks(result.png));
    }
}
