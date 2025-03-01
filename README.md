![Logo][1] QuickerSFV - A quicker checksum verifier
===================================================

QuickerSFV is an open-source rewrite of [QuickSFV](https://www.quicksfv.org/) for modern Windows systems.

Major Features
--------------
   - 📈 Significantly faster checking times than QuickSFV, especially on SSDs
   - ㊗️ Support for unicode characters in filenames
   - 🦒 Support for long paths (up to 32,767 characters) on Windows 10 and 11
   - 🔌 Plugin system for easy extensibility
   - 🐜 Tiny binary size
   - 🧳 Portable self-contained executables available
   - 📖 Fully open-sourced implementation licensed under GPL v3

Why update from QuickSFV?
-------------------------
QuickSFV is a great and well-established software, but after 15 years without any updates, it is starting to show its age.
The checksum algorithms it uses are not making use of the latest CPU features and their performance is no longer on-par with newer libraries.
While this was not an issue in the days of slow HDDs, its impact becomes apparent when checking files from fast SSDs, where the disk I/O is no longer the sole bottleneck of the verification time.
QuickerSFV achieves significantly higher bandwidth during verification, cutting verification times to a fraction of what QuickSFV is able to achieve.

QuickerSFV also has proper support for filenames commonly encountered on modern Windows systems, including filenames with special characters and emojis in filenames as well as proper support for paths that are longer than 255 characters.

Compatibility
-------------
The goal is feature-parity with QuickSFV, so that QuickerSFV can act as a drop-in replacement.
If you find a feature or workflow from QuickSFV that is missing, feel free to [open an issue](https://github.com/ComicSansMS/QuickerSFV/issues).

Benchmarks
----------


Development Status
------------------



  [1]: https://github.com/user-attachments/assets/84de2664-d676-4ae1-bddf-23d464d870d4
