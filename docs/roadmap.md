# Roadmap

## V1.1.0 - Documentation

### Sprint 1: Documentation
nepcompress.h needs doxygen documentation too.
Need to build and deploy github pages documentation, using doxygen.
Need a CI action (user-run) which updates the gh-pages docs from source.
Need this action to be run automatically after every release.
Need a doxygen mainpage which describes nep-compress and has links to the provided compression filters, as well as a discussion which compares all available filters, including those built in to netcdf-c, zstd, zlib, and szlib.
Make sure all functions include @author tag, and all test code and some documentation include the author: Edward Hartnett, Intelligent Data Design, Inc.
The README should reference my two books: https://www.amazon.com/NetCDF-Developers-Handbook-Authoritative-High-Performance/dp/B0GYP4R5ZZ/ref=sr_1_2?dib=eyJ2IjoiMSJ9.Id1w-Fwu7QyFJvhJL0kdBrp07ACXa_DmUZaljsc5TflwIyQ_Zm9xmiRX4pwZ25ib.3U2eqqQeE5a5gX9QcbHcWEerID3L85jIX_I72uBvAYw&dib_tag=se&keywords=netcdf&qid=1792931211&sr=8-2 and https://www.amazon.com/Earth-Observation-Practice-Mission-Mission/dp/B0HJRZ9F7L/ref=sr_1_1?crid=4TKSYJ9Q604C&dib=eyJ2IjoiMSJ9.oxHqFCY61RhO47h_lKJ5XnDkmiGivjctgiLV7OTBjmD0qKdjqMMi0MM5L4OPH54bibGUud8QwV9GdLozmPn2C_gk1hZnhXOXE-1j8FqFJiyKVGFm1k9TBpgvgay2-9ic-k_IBlhQMXm5LfEj-TQ5C2IVcJwaOwUL7uhDgJrjjC8HN8-VxO44BPjXEAc4jRgLWzoN0SM_1pzEJfsVpnsr1katGmhwX_Zb8Vb5rXU6l6s.xKzsXo5C3IZ0gdGkmYGv6s58XrkmcTMxJlwbrvlUB2c&dib_tag=se&keywords=%22earth+observation+in+practice%22+hartnett&qid=1792931437&sprefix=earth+observation+in+practice+hartnett%2Caps%2C624&sr=8-1
The covers of both are stored in the docs directory as jpgs, make sure to use them in the README, at an size so they can appear side-by-side.

### Sprint 2: Fix Issues
Fix issues 3, 4, and 5.

### Sprint 3: Compression Performance Example
Add subdirectory performance.
Everything in performance subdir is written and run as a repo test, i.e. run with "make test"
Need a test which tries all available filters and compares.
We need some real data to try this on. Take a sizable (10 MB) sample from this data file: 
Style similar to this program from my book: https://github.com/captainkirk99/NetCDF_Developers_Handbook/blob/main/examples/performance/zstandard.c
Full doxygen documentation.
