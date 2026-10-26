# Entry menu: equal secondary geometry and safe-area anchoring

More and Settings now use the same width, based on the larger existing label
measurement and bounded by the available safe span. Their physical insets are
applied on both sides so START/END remain safe in RTL. Play is centred inside
that same safe span rather than shifted by FrameLayout's full margin difference.
Typography, colours, artwork, labels, focusability, readiness and action
callbacks are unchanged. This is a bounded layout correction, not the requested
complete title/button style redesign.

The actual existing Android widget methods reproduce two original failures:
More112 versus Settings127 logical pixels at411x801, and an off-centre Play at
asymmetric physical insets. The original-source control retained both failures;
the exact candidate source was restored afterwards. After correction, the full
47-test campaign/menu class passes with no skipped tests, and Android Debug
lint passes without issues in the changed menu region. No assertion or timeout
was relaxed. The first density fixture mixed a1080-pixel root and a different
app display width; that failed receipt remains, and the final fixture explicitly
keeps both dimensions coherent.

At411x801/1x font, the secondary widths are now127/127 and both outer gaps16.
Portrait/landscape1x/2x font cases retain48dp-or-larger touch targets and separate
Play/secondary rectangles. At1080x2340, density2.625 and font1.3, asymmetric
physical insets40/100 yield the safe span61..959; Play centres at510 in both
LTR and RTL, with the secondary actions mirrored inside the same span.

[Receipt and geometry](receipt.json) retain the exact source hashes and checks.

## Geometry-only host renders

These are native Android **host widget fixtures**, with synthetic insets and no
RT world. This host drawing path does not render button glyphs reliably; the
images demonstrate plate geometry only, not text readability/contrast or the
real menu composition. They are not substitutes for the owner's screenshot.
The supplied Library reference resolved but its pixels were not available here;
no blocked download/helper retry or desktop/phone capture was performed.

[Before portrait](before-portrait-host-widgets.png) ·
[After portrait](after-portrait-host-widgets.png) ·
[After landscape](after-landscape-host-widgets.png)

Full title/style review, real RT-background composition, physical thumb comfort
and phone orientation/lifecycle acceptance remain. The owner's installed app,
saves/settings, running desktop candidate and Reef's phone allocation were not
touched. No renderer budget or normal gameplay rule changed.
