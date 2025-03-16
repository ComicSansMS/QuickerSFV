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

Installation
------------
Download a binary from the [Releases](https://github.com/ComicSansMS/QuickerSFV/releases) page.

All binaries labeled `self-contained` should run on any Windows system without any additional dependencies.

If you get an error on startup saying that VCRUNTIME140_1.dll was not found, you need to install the Microsoft Visual C++ Redistributable on your system. It can be [downloaded here](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170#latest-microsoft-visual-c-redistributable-version).

Why update from QuickSFV?
-------------------------
QuickSFV is a great and well-established software, but after 15 years without any updates, it is starting to show its age.
The checksum algorithms it uses are not making use of the latest CPU features and [their performance is no longer on-par](#benchmarks) with newer libraries.
While this was not an issue in the days of slow HDDs, its impact becomes apparent when checking files from fast SSDs, where the disk I/O is no longer the sole bottleneck of the verification time.
QuickerSFV achieves significantly higher bandwidth during verification, cutting verification times to a fraction of what QuickSFV is able to achieve.

QuickerSFV also has proper support for filenames commonly encountered on modern Windows systems, including filenames with special characters and emojis in filenames as well as proper support for paths that are longer than 255 characters.

Compatibility
-------------
The goal is feature-parity with QuickSFV, so that QuickerSFV can act as a drop-in replacement. That means you should be able to literally replace the `QuickSFV.exe` in your local installation with QuickerSFV and everything just works faster.

There are a few restrictions currently:
   
   - The "Previously Verified Database" feature of QuickSFV is not supported.
   - Currently not the following file formats that QuickSFV supported are not supported in QuickerSFV:
       - `.txt` This file extension is too ambiguous to meaningfully associate with a specific format.
       - `.ckz` I could not find a file specification for this format. If someone can supply me with an example file, I would consider adding support. It seems to be a legacy file format that is no longer in use.
       - `.csv` Same as `.txt`, it is unclear which format to associate with this extension.
       - `.par` Par seems to be a legacy format that is no longer in use. Support for `.par2` might be added with a plugin in the future.
       - Backwards compatibility with WIN-SFV32 for `.sfv` files. WIN-SFV32 seems to be no longer in use.
   - The QuickSFV .ini configuration file is not supported. Placing .ini files in the working directory of the .exe file is no longer considered good practice for Windows applications. QuickerSFV allows saving its configuration in the Registry instead.

If you find one of these restrictions unacceptable or you are missing feature or workflow from QuickSFV, feel free to [open an issue](https://github.com/ComicSansMS/QuickerSFV/issues).

Benchmarks
----------
On HDDs QuickSFV and QuickerSFV both perform about the same, in the order of the speed of the drive. The checksum calculation is purely I/O-bound.

On faster SSDs and for files which are hot in Windows' disk cache, QuickerSFV is able to significantly outperform QuickSFV. A benchmark on a Samsung SSD 990 Pro 2TB, running on a Ryzen 9 7950X3D CPU shows QuickerSFV being roughly 9x faster for CRC and roughly 3x faster for MD5 calculations. This comparison is between QuickSFV v2.36 and QuickerSFV v0.1 on Windows 11.

<img src="https://github.com/ComicSansMS/QuickerSFV/blob/main/.github/images/plot_bar.png" alt="Benchmark Bar Plot" width="640" height="480" />


Development Status
------------------

QuickerSFV is currently in Beta.

The following features are planned for the 1.0 release:
   
   - Missing features from QuickSFV.
   - Shell Extension for creating checksum files from Windows Explorer context menu.
   - Plugin API support for self-contained files (like `.zip` or `.rar` archives).

With Version 1.0 the Plugin API will be fixed and is guaranteed to remain stable between major releases.

The goal is to allow expanding the featureset of the application mainly via Plugins at this point, and only update the core application for user interface improvements and critical bug fixes.

There are currently no plans to provide a graphical user interface for non-Windows platforms. A portable command line interface for Windows and non-Windows platforms may be provided at some point.


  [1]: https://github.com/ComicSansMS/QuickerSFV/blob/main/.github/images/quicker_sfv_logo.png
