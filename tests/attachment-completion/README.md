# Attachment completion regression

This test covers the cursor-completion guard already published in `v0.1.0-alpha.1`.
It adds test code and CI integration. It does not change production code.

The accepted test contains 24 authored cases.
Its exact SHA-256 is `fa6dd03bf2669db4101d46a5547da746f437569b54d9ccea35006a4416461777`.
The accepted review record has SHA-256 `964c37f680ceaab9cd77cff55de2f13d037efc7ab90da63feeb81b01bf3baeb0`.
JA2 Owner selected this test for the public PR on November 15, 2026.

`prepare.py` checks the reconstructed source against its manifest and the accepted source hashes.
It removes only the recorded notice insertion from its extraction input.
It preserves the reconstructed files and their notices.
It binds 11 fragments to the actual `DoAttachment`, completion statements, and `OBJECTTYPE` lifetime methods.
The compiler uses the real engine headers through the existing Clang include-case overlay.
The original absolute include names remain mapped to these generated source files.

For the failing control, `P710_ATTACHMENT_UNGUARDED=ON` selects the historical unguarded completion fragment.
This option does not alter the reconstructed engine.
The normal test configuration keeps this option off.

## Coverage limits

`AttachObject` outcomes, UI effects, and map cursor clearing use controlled test doubles.
The test checks cursor completion and object preservation under those outcomes.
It does not test actual attachment rules, full callback routing, real UI behavior, or campaign behavior.
It does not establish save/reload correctness or shutdown leak resolution.
Source changes affecting the frozen bindings require a reviewed binding update.

The generated engine fragments retain their source terms.
Read the [component terms](../../docs/PUBLICATION-LIMITS.md) and [notices](../../notices/) before redistribution.
