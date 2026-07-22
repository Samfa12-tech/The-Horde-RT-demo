# Future Feature Note: "Share with the dev"

Status: note only - not implemented.

If a future build cannot load the game and only reaches the diagnostic text screen, consider adding a clearly labelled **Share with the dev** button. Its purpose would be to make it easy for people to contribute compatibility evidence, especially for unsupported Vulkan RT devices.

The flow should be consent-first and transparent:

- Explain exactly what will be shared before anything leaves the device.
- Require an explicit user action and confirmation; never send diagnostics automatically.
- Show a reviewable summary containing the app/build version, Android version, device model, GPU name, Vulkan API/driver version, required extension results, selected RT mode, and presentation status.
- Offer a generated screenshot of the diagnostic screen showing the device/GPU result, with a preview before sharing.
- Prefer a controlled website or itch submission form for structured collection, with a fallback link or email option such as `samsmall1267@gmail.com` if appropriate.
- Do not include the user's name, email address, files, location, account identifiers, or other personal data unless the user separately chooses to provide it.
- Make the destination, attachment, and privacy implications clear for email, website, and itch alternatives.
- Record the resulting report in `docs/ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md` as `user-reported` or `user-reported + screenshot evidence`, not as local testing.

Potential share payload fields are the same fields already used by the diagnostics and compatibility evidence template. The feature must preserve the project's RT-or-nothing behaviour: it gathers evidence after an honest unsupported-device result and does not add a fallback renderer.
