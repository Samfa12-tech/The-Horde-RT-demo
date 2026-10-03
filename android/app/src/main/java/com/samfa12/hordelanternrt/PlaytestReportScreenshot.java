package com.samfa12.hordelanternrt;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.zip.CRC32;
import java.util.zip.Deflater;

/** Bounded metadata-free PNG encoder for consented game-target RGBA pixels. */
final class PlaytestReportScreenshot {
    static final int MAX_PNG_BYTES = 512 * 1024;
    static final int MAX_LONG_EDGE = 1280;
    static final int MAX_SHORT_EDGE = 720;
    private static final int PNG_FIXED_BYTES = 8 + 25 + 12 + 12;
    private static final int MAX_IDAT_BYTES = MAX_PNG_BYTES - PNG_FIXED_BYTES;
    private static final byte[] SIGNATURE = {(byte) 137, 80, 78, 71, 13, 10, 26, 10};
    private static final int ROW_BUFFER_BYTES = MAX_LONG_EDGE * 3 + 1;
    private static final int COMPRESS_BUFFER_BYTES = 8192;

    enum Status { ENCODED, INVALID_DIMENSIONS, INVALID_PIXELS, TOO_LARGE, ENCODER_FAILURE }

    static final class Result {
        final Status status;
        final byte[] png;
        final int width;
        final int height;
        private Result(Status status, byte[] png, int width, int height) {
            this.status = status; this.png = png; this.width = width; this.height = height;
        }
        boolean isEncoded() { return status == Status.ENCODED; }
    }

    private PlaytestReportScreenshot() { }

    /**
     * Encodes the supplied owned RGBA8 buffer as RGB8 PNG. Alpha is intentionally ignored.
     * Dimensions are validated as-is; this method never resizes or changes image quality.
     */
    static Result encodeRgba8(byte[] rgba, int width, int height) {
        if (width < 1 || height < 1 || Math.max(width, height) > MAX_LONG_EDGE ||
                Math.min(width, height) > MAX_SHORT_EDGE) {
            return failure(Status.INVALID_DIMENSIONS, width, height);
        }
        long expectedBytes = (long) width * height * 4L;
        if (rgba == null || expectedBytes != rgba.length) {
            return failure(Status.INVALID_PIXELS, width, height);
        }

        Deflater deflater = new Deflater(Deflater.DEFAULT_COMPRESSION, false);
        BoundedBytes compressed = new BoundedBytes(MAX_IDAT_BYTES);
        byte[] row = new byte[width * 3 + 1];
        byte[] compressedChunk = new byte[COMPRESS_BUFFER_BYTES];
        try {
            for (int y = 0; y < height; ++y) {
                row[0] = 1; // PNG Sub predictor, lossless RGB8; no pixel/quality change.
                int source = y * width * 4;
                int target = 1;
                for (int x = 0; x < width; ++x) {
                    row[target++] = rgba[source++];
                    row[target++] = rgba[source++];
                    row[target++] = rgba[source++];
                    ++source; // Drop alpha; no transparency or metadata is emitted.
                }
                // Reverse order preserves the original left pixel while forming
                // modulo-256 channel deltas, reducing ordinary scene texture bytes.
                for (int i = row.length - 1; i >= 4; --i) row[i] = (byte) (row[i] - row[i - 3]);
                deflater.setInput(row);
                while (!deflater.needsInput()) {
                    int count = deflater.deflate(compressedChunk);
                    if (count > 0) compressed.write(compressedChunk, 0, count);
                    else if (!deflater.needsInput()) return failure(Status.ENCODER_FAILURE, width, height);
                }
            }
            deflater.finish();
            while (!deflater.finished()) {
                int count = deflater.deflate(compressedChunk);
                if (count > 0) compressed.write(compressedChunk, 0, count);
                else if (!deflater.finished()) return failure(Status.ENCODER_FAILURE, width, height);
            }

            ByteArrayOutputStream png = new ByteArrayOutputStream(MAX_PNG_BYTES);
            png.write(SIGNATURE);
            byte[] ihdr = new byte[13];
            putInt(ihdr, 0, width);
            putInt(ihdr, 4, height);
            ihdr[8] = 8; // bit depth
            ihdr[9] = 2; // truecolour RGB, no alpha
            ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
            writeChunk(png, "IHDR", ihdr);
            writeChunk(png, "IDAT", compressed.toByteArray());
            writeChunk(png, "IEND", new byte[0]);
            byte[] result = png.toByteArray();
            if (result.length > MAX_PNG_BYTES) return failure(Status.TOO_LARGE, width, height);
            return new Result(Status.ENCODED, result, width, height);
        } catch (TooLargeException overBudget) {
            return failure(Status.TOO_LARGE, width, height);
        } catch (IOException | RuntimeException failed) {
            return failure(Status.ENCODER_FAILURE, width, height);
        } finally {
            deflater.end();
            Arrays.fill(row, (byte) 0);
            Arrays.fill(compressedChunk, (byte) 0);
            compressed.clear();
        }
    }

    private static Result failure(Status status, int width, int height) {
        return new Result(status, null, width, height);
    }

    private static void writeChunk(ByteArrayOutputStream output, String type, byte[] data) throws IOException {
        if (output.size() + data.length + 12L > MAX_PNG_BYTES) throw new TooLargeException();
        byte[] typeBytes = type.getBytes(java.nio.charset.StandardCharsets.US_ASCII);
        writeInt(output, data.length);
        output.write(typeBytes);
        output.write(data);
        CRC32 crc = new CRC32();
        crc.update(typeBytes);
        crc.update(data);
        writeInt(output, (int) crc.getValue());
    }

    private static void putInt(byte[] bytes, int offset, int value) {
        bytes[offset] = (byte) (value >>> 24);
        bytes[offset + 1] = (byte) (value >>> 16);
        bytes[offset + 2] = (byte) (value >>> 8);
        bytes[offset + 3] = (byte) value;
    }

    private static void writeInt(ByteArrayOutputStream output, int value) {
        output.write((value >>> 24) & 0xff);
        output.write((value >>> 16) & 0xff);
        output.write((value >>> 8) & 0xff);
        output.write(value & 0xff);
    }

    private static final class BoundedBytes extends ByteArrayOutputStream {
        private final int maximum;
        BoundedBytes(int maximum) { super(Math.min(maximum, 8192)); this.maximum = maximum; }
        @Override public synchronized void write(int value) {
            if (count + 1 > maximum) throw new TooLargeException();
            super.write(value);
        }
        @Override public synchronized void write(byte[] bytes, int offset, int length) {
            if (length < 0 || count + (long) length > maximum) throw new TooLargeException();
            super.write(bytes, offset, length);
        }
        void clear() { Arrays.fill(buf, (byte) 0); reset(); }
    }

    private static final class TooLargeException extends RuntimeException { }
}
